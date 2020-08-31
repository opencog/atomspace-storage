/*
 * opencog/persist/api/PersistSCM.cc
 *
 * Copyright (c) 2008 by OpenCog Foundation
 * Copyright (c) 2008, 2009, 2013, 2015 Linas Vepstas <linasvepstas@gmail.com>
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

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/guile/SchemePrimitive.h>
#include "PersistSCM.h"

using namespace opencog;

StorageNodePtr PersistSCM::_sn;

PersistSCM::PersistSCM(void)
	: ModuleWrap("opencog persist")
{
	static bool is_init = false;
	if (is_init) return;
	is_init = true;
	module_init();
}

void PersistSCM::init(void)
{
	define_scheme_primitive("cog-open",
	             &PersistSCM::open, this, "persist");
	define_scheme_primitive("cog-close",
	             &PersistSCM::close, this, "persist");
	define_scheme_primitive("fetch-atom",
	             &PersistSCM::fetch_atom, this, "persist");
	define_scheme_primitive("fetch-value",
	             &PersistSCM::fetch_value, this, "persist");
	define_scheme_primitive("fetch-incoming-set",
	             &PersistSCM::fetch_incoming_set, this, "persist");
	define_scheme_primitive("fetch-incoming-by-type",
	             &PersistSCM::fetch_incoming_by_type, this, "persist");
	define_scheme_primitive("fetch-query-2args",
	             &PersistSCM::fetch_query2, this, "persist");
	define_scheme_primitive("fetch-query-4args",
	             &PersistSCM::fetch_query4, this, "persist");
	define_scheme_primitive("store-atom",
	             &PersistSCM::store_atom, this, "persist");
	define_scheme_primitive("store-value",
	             &PersistSCM::store_value, this, "persist");
	define_scheme_primitive("load-atoms-of-type",
	             &PersistSCM::load_type, this, "persist");
	define_scheme_primitive("load-atomspace",
	             &PersistSCM::load_atomspace, this, "persist");
	define_scheme_primitive("store-atomspace",
	             &PersistSCM::store_atomspace, this, "persist");
	define_scheme_primitive("barrier",
	             &PersistSCM::barrier, this, "persist");
}

// =====================================================================

void PersistSCM::open(Handle h)
{
	if (not nameserver().isA(h->get_type(), STORAGE_NODE))
		throw RuntimeException(TRACE_INFO,
			"Expecting StorageNode, got %s", h->to_short_string().c_str());

	StorageNodePtr stnp = StorageNodeCast(h);

	// The cast will fail, if the dynamic library that defines the type
	// isn't loaded. This is the user's job. They can do it by saying
	// (use-modules (opencog persist-foo))
	if (nullptr == stnp)
		throw RuntimeException(TRACE_INFO,
			"Not opened; please load module that defines %s\n"
			"Like so: (use-modules (persist-foo))",
			nameserver().getTypeName(h->get_type()).c_str());

	stnp->open();

	if (nullptr == _sn) _sn = stnp;
}

void PersistSCM::close(Handle h)
{
	if (not nameserver().isA(h->get_type(), STORAGE_NODE))
		throw RuntimeException(TRACE_INFO,
			"Expecting StorageNode, got %s", h->to_short_string().c_str());

	StorageNodePtr stnp = StorageNodeCast(h);
	stnp->close();

	if (stnp == _sn) _sn = nullptr;
}

#define CHECK \
	if (nullptr == _sn) \
		throw RuntimeException(TRACE_INFO, "No open connection to storage!");

Handle PersistSCM::fetch_atom(Handle h)
{
	CHECK;
	return _sn->fetch_atom(h);
}

Handle PersistSCM::fetch_value(Handle h, Handle key)
{
	CHECK;
	return _sn->fetch_value(h, key);
}

Handle PersistSCM::fetch_incoming_set(Handle h)
{
	CHECK;
	// The "false" flag here means that the fetch is NOT recursive.
	return _sn->fetch_incoming_set(h, false);
}

Handle PersistSCM::fetch_incoming_by_type(Handle h, Type t)
{
	CHECK;
	return _sn->fetch_incoming_by_type(h, t);
}

Handle PersistSCM::fetch_query2(Handle query, Handle key)
{
	CHECK;
	return _sn->fetch_query(query, key, Handle::UNDEFINED, false);
}

Handle PersistSCM::fetch_query4(Handle query, Handle key,
                                Handle meta, bool fresh)
{
	CHECK;
	return _sn->fetch_query(query, key, meta, fresh);
}

/**
 * Store the single atom to the backing store hanging off the
 * atom-space.
 */
Handle PersistSCM::store_atom(Handle h)
{
	CHECK;
	_sn->store_atom(h);
	return h;
}

void PersistSCM::store_value(Handle h, Handle key)
{
	CHECK;
	_sn->store_value(h, key);
}

void PersistSCM::load_type(Type t)
{
	CHECK;
	_sn->fetch_all_atoms_of_type(t);
}

void PersistSCM::load_atomspace(void)
{
	CHECK;
	_sn->load_atomspace();
}

void PersistSCM::store_atomspace(void)
{
	CHECK;
	_sn->store_atomspace();
}

void PersistSCM::barrier(void)
{
	CHECK;
	_sn->barrier();
}

void opencog_persist_init(void)
{
	static PersistSCM patty;
}
