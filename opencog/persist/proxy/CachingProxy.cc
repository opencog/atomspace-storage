/*
 * CachingProxy.cc
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

#include <opencog/persist/proxy/CachingProxy.h>

using namespace opencog;

CachingProxy::CachingProxy(const std::string&& name)
	: ProxyNode(CACHING_PROXY_NODE, std::move(name))
{
	init();
}

CachingProxy::CachingProxy(Type t, const std::string&& name)
	: ProxyNode(t, std::move(name))
{
	init();
}

CachingProxy::~CachingProxy()
{
}

void CachingProxy::init(void)
{
	have_loadType = true;
	have_fetchIncomingByType = true;
	have_fetchIncomingSet = true;
	have_getAtom = true;
	have_loadValue = true;
}

// Get our configuration from the ProxyParameterLink we live in.
// XXX TODO Add support for expriation times, limited AtomSpace
// size and whatever other whizzy caching ideas we might want.
void CachingProxy::open(void)
{
	// Can't use this if there are more parameters...
	StorageNodeSeq rdr = setup();
	if (1 != rdr.size())
		throw RuntimeException(TRACE_INFO,
			"Expecting exactly one StorageNode");

	_reader = rdr[0];
	_reader->open();
}

void CachingProxy::close(void)
{
	_reader->close();
	_reader = nullptr;
}

#define CHECK_OPEN if (nullptr == _reader) return;

void CachingProxy::getAtom(const Handle& h)
{
	CHECK_OPEN;

	// We want to do this:
	// const Handle& ch = _atom_space->get_atom(h);
	// if (ch) return;
	// but it won't work, of course, because by this point,
	// h has already been inserted into the AtomSpace.
	// (BTW, this causes issues in several places, not just here.
	// So XXX TODO Review if this was a good design choice. Someday.)
	// Instead, we look to see if it's decorated with any Values.
	// It won't have any, if its a fresh atom.
	if (h->haveValues()) return;

	_reader->fetch_atom(h);
	_reader->barrier();
}

void CachingProxy::fetchIncomingSet(AtomSpace* as, const Handle& h)
{
	CHECK_OPEN;
	if (0 < h->getIncomingSetSize(as)) return;

	_reader->fetch_incoming_set(h, false, as);
	_reader->barrier(as);
}

void CachingProxy::fetchIncomingByType(AtomSpace* as, const Handle& h, Type t)
{
	CHECK_OPEN;
	if (0 < h->getIncomingSetSizeByType(t, as)) return;

	_reader->fetch_incoming_by_type(h, t, as);
	_reader->barrier(as);
}

void CachingProxy::loadValue(const Handle& atom, const Handle& key)
{
	CHECK_OPEN;
	if (nullptr != atom->getValue(key)) return;

	_reader->fetch_value(atom, key);
	_reader->barrier();
}

// We're just going to be unconditional, here.
void CachingProxy::loadType(AtomSpace* as, Type t)
{
	CHECK_OPEN;
	_reader->fetch_all_atoms_of_type(t, as);
	_reader->barrier(as);
}

void CachingProxy::barrier(AtomSpace* as)
{
	_reader->barrier(as);
}

DEFINE_NODE_FACTORY(CachingProxy, CACHING_PROXY_NODE)
