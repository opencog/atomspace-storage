/*
 * WriteBufferProxy.cc
 *
 * Copyright (C) 2022 Linas Vepstas
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <chrono>
#include <math.h>

#include <opencog/atoms/core/NumberNode.h>
#include <opencog/persist/proxy/WriteBufferProxy.h>

using namespace opencog;

// One writeback queue should be enough.
#define NUM_WB_QUEUES 1

WriteBufferProxy::WriteBufferProxy(const std::string&& name) :
	WriteThruProxy(WRITE_BUFFER_PROXY_NODE, std::move(name))
{
	init();
}

WriteBufferProxy::WriteBufferProxy(Type t, const std::string&& name) :
	WriteThruProxy(t, std::move(name))
{
	init();
}

WriteBufferProxy::~WriteBufferProxy()
{
}

void WriteBufferProxy::init(void)
{
	// Default decay time of 30 seconds
	_decay = 30.0;
	_stop = false;
}

// Get configuration from the ProxyParametersLink we live in.
void WriteBufferProxy::open(void)
{
	// Let ProxyNode::setup() do the basic work.
	WriteThruProxy::open();

	// Now fish out the rate parameter, if it is there.
	IncomingSet dli(getIncomingSetByType(PROXY_PARAMETERS_LINK));
	const Handle& pxy = dli[0];
	if (2 <= pxy->size())
	{
		const Handle& hdecay = pxy->getOutgoingAtom(2);
		if (not hdecay->is_type(NUMBER_NODE))
			throw SyntaxException(TRACE_INFO,
				"Expecting decay time in a NumberNode, got %s",
				hdecay->to_short_string().c_str());

		NumberNodePtr nnp = NumberNodeCast(hdecay);
		_decay = nnp->get_value();
	}

	// Start the writer
	_stop = false;
	_write_thread = std::thread(&WriteBufferProxy::write_loop, this);
}

void WriteBufferProxy::close(void)
{
	// Stop writing
	_stop = true;
	_write_thread.join();

	// Drain the queues
	barrier();
	_atom_queue.close();
	_value_queue.close();

printf("you close\n");
	WriteThruProxy::close();
}

void WriteBufferProxy::storeAtom(const Handle& h, bool synchronous)
{
	if (synchronous)
	{
		WriteThruProxy::storeAtom(h, synchronous);
		return;
	}
printf("yo store atom %s\n", h->to_string().c_str());
	_atom_queue.insert(h);
}

// Two-step remove. Just pass the two steps down to the children.
void WriteBufferProxy::preRemoveAtom(AtomSpace* as, const Handle& h,
                                     bool recursive)
{
	WriteThruProxy::preRemoveAtom(as, h, recursive);
}

void WriteBufferProxy::postRemoveAtom(AtomSpace* as, const Handle& h,
                                    bool recursive, bool extracted_ok)
{
	WriteThruProxy::postRemoveAtom(as, h, recursive, extracted_ok);
}

void WriteBufferProxy::storeValue(const Handle& atom, const Handle& key)
{
printf("yo store value %s\n", atom->to_string().c_str());
	_value_queue.insert({atom, key});
}

void WriteBufferProxy::updateValue(const Handle& atom, const Handle& key,
                            const ValuePtr& delta)
{
	WriteThruProxy::updateValue(atom, key, delta);
}

void WriteBufferProxy::barrier(AtomSpace* as)
{
	// Drain both queues.
	std::pair<Handle, Handle> pr;
	while (_value_queue.try_get(pr))
		WriteThruProxy::storeValue(pr.first, pr.second);

	Handle atom;
	while (_atom_queue.try_get(atom))
		WriteThruProxy::storeAtom(atom, false);

	WriteThruProxy::barrier(as);
}

// ==============================================================

// This runs in it's own thread, and drains a fraction of the queue.
void WriteBufferProxy::write_loop(void)
{
	using namespace std::chrono;

	// Keep distinct clocks for atoms and values.
	// That's because the first writer delays the second writer
	steady_clock::time_point atostart = steady_clock::now();
	steady_clock::time_point valstart = atostart;

	// Keep a moving average queue size.
	double mavg_atoms = 0.0;
	double mavg_vals = 0.0;

#define POLLY 4.0
	// POLLY=4, minfrac = 1-exp(-0.25) = 1-0.7788 = 0.2212;
	static const double minfrac = 1.0 - exp(-1.0/POLLY);

	// After opening, sleep for the first fourth of the decay time.
	uint nappy = 1 + (uint) (1000.0 * _decay / POLLY);

	while(not _stop)
	{
		std::this_thread::sleep_for(milliseconds(nappy));

		steady_clock::time_point wake = steady_clock::now();
		if (not _atom_queue.is_empty())
		{
			// How long have we slept, in seconds?
			double waited = duration_cast<duration<double>>(wake-atostart).count();
			// What fraction of the decay time is that?
			double frac = waited / _decay;

			// How many Atoms awiting to be written?
			double qsz = (double) _atom_queue.size();
#define WEI 0.2
			mavg_atoms = (1.0-WEI) * mavg_atoms + WEI * qsz;

			// How many should we write?
			uint nwrite = ceil(frac * qsz);

			// Whats the min to write? The goal here is to not
			// dribble out the tail, but to push it out if its almot all
			// gone anyway.
			uint mwr = ceil(0.5 * minfrac * mavg_atoms);
			if (nwrite < mwr) nwrite = mwr;

			// Store that many
			for (uint i=0; i < nwrite; i++)
			{
				Handle atom;
				bool got = _atom_queue.try_get(atom);
				if (not got) break;
				WriteThruProxy::storeAtom(atom);
			}
printf("duude wait=%f qsz=%f nwrite=%u\n", waited, qsz, nwrite);
		}
		atostart = wake;

		// re-measure, because above may have taken a long time.
		wake = steady_clock::now();

		// cut-n-paste of above.
		if (not _value_queue.is_empty())
		{
			// How long have we slept, in seconds?
			double waited = duration_cast<duration<double>>(wake-valstart).count();
			// What fraction of the decay time is that?
			double frac = waited / _decay;

			double qsz = (double) _value_queue.size();
			mavg_vals = (1.0-WEI) * mavg_vals + WEI * qsz;

			// How many should we write?
			uint nwrite = ceil(frac * qsz);

			// Whats the min to write? The goal here is to not
			// dribble out the tail, but to push it out if its almot all
			// gone anyway.
			uint mwr = ceil(0.5 * minfrac * mavg_vals);
			if (nwrite < mwr) nwrite = mwr;

			// Store that many
			for (uint i=0; i < nwrite; i++)
			{
				std::pair<Handle, Handle> kvp;
				bool got = _value_queue.try_get(kvp);
				if (not got) break;
				WriteThruProxy::storeValue(kvp.first, kvp.second);
			}
printf("duude vait=%f qsz=%f nwrite=%u\n", waited, qsz, nwrite);
		}
		valstart = wake;
	}
}

DEFINE_NODE_FACTORY(WriteBufferProxy, WRITE_BUFFER_PROXY_NODE)
