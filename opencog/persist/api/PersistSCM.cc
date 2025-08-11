/*
 * opencog/persist/api/PersistSCM.cc
 *
 * Copyright (c) 2008 by OpenCog Foundation
 * Copyright (c) 2008, 2009, 2013, 2015, 2022 Linas Vepstas <linasvepstas@gmail.com>
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
#include <opencog/guile/SchemeSmob.h>
#include "PersistSCM.h"

using namespace opencog;

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
	define_scheme_primitive("cog-connected?",
	             &PersistSCM::connected, this, "persist");
	define_scheme_primitive("cog-storage-node",
	             &PersistSCM::current_storage, this, "persist");

	// define_scheme_primitive(..., false); means that these functions
	// will NOT be `define-public` and just plain `define`. Thus,
	// accessible within the module, but not outside of it.
	define_scheme_primitive("sn-fetch-query-2args",
	             &PersistSCM::sn_fetch_query2, "persist", false);
	define_scheme_primitive("sn-fetch-query-4args",
	             &PersistSCM::sn_fetch_query4, "persist", false);
	define_scheme_primitive("sn-store-atom",
	             &PersistSCM::sn_store_atom, "persist", false);

	define_scheme_primitive("sn-setvalue",
	             &PersistSCM::sn_setvalue, "persist", false);
	define_scheme_primitive("sn-getvalue",
	             &PersistSCM::sn_getvalue, "persist", false);
}

// =====================================================================

// South Texas Nuclear Project
#define GET_STNP \
	if (not nameserver().isA(hsn->get_type(), STORAGE_NODE)) { \
		throw RuntimeException(TRACE_INFO, \
			"Expecting StorageNode, got %s", hsn->to_short_string().c_str()); \
	} \
 \
	StorageNodePtr stnp = StorageNodeCast(hsn); \
 \
	/* The cast will fail, if the dynamic library that defines the type */ \
	/* isn't loaded. This is the user's job. They can do it by saying */ \
	/* (use-modules (opencog persist-foo)) */ \
	if (nullptr == stnp) { \
		if (hsn->get_type() == STORAGE_NODE) { \
			throw RuntimeException(TRACE_INFO, \
				"A StorageNode cannot be used directly; " \
				"only it's sub-types provide the needed implementation!"); \
		} \
		throw RuntimeException(TRACE_INFO, \
			"Not opened; please load module that defines %s\n" \
			"Like so: (use-modules (opencog persist-foo))\n" \
			"where `foo` is the module providing the node.", \
			nameserver().getTypeName(hsn->get_type()).c_str()); \
	}

StorageNodePtr PersistSCM::_sn;

void PersistSCM::open(Handle hsn)
{
	GET_STNP;
	if (stnp->connected())
		throw RuntimeException(TRACE_INFO,
			"StorageNode %s is already open!",
			hsn->to_short_string().c_str());

	// It can happen that, due to user error, stnp looks to be closed,
	// but the StorageNode dtor has not run, and so it still seems to
	// be open. One solution is to force all use counts on the smart
	// pointers to go to zero (done further below). Another is to just
	// double-check the URL, and force the close. Seems that we need to
	// do both: belt and suspenders.
	if (_sn and _sn->get_name() == stnp->get_name())
		close(Handle(_sn));

	// Clobber the smart pointer so use count goes to zero, and the
	// StorageNode dtor runs (which then closes the connection.)
	if (_sn and nullptr == _sn->getAtomSpace()) _sn = nullptr;

	// Like above, but the same StorageNode in a different AtomSpace!?
	// if (_sn and *_sn == *stnp) _sn = nullptr;

	stnp->open();

	if (nullptr == _sn) _sn = stnp;
}

void PersistSCM::close(Handle hsn)
{
	GET_STNP;
	if (not stnp->connected())
		throw RuntimeException(TRACE_INFO,
			"StorageNode %s is not open!",
			hsn->to_short_string().c_str());

	stnp->close();

	if (stnp == _sn) _sn = nullptr;
}

bool PersistSCM::connected(Handle hsn)
{
	StorageNodePtr stnp = StorageNodeCast(hsn);
	if (nullptr == stnp) return false;
	return stnp->connected();
}

// =====================================================================

/**
 * Store the single atom to the backing store hanging off the
 * atom-space.
 */
Handle PersistSCM::sn_store_atom(Handle h, Handle hsn)
{
	GET_STNP;
	stnp->store_atom(h);
	return h;
}

Handle PersistSCM::sn_fetch_query2(Handle query, Handle key, Handle hsn)
{
	GET_STNP;
	const AtomSpacePtr& asp = SchemeSmob::ss_get_env_as("fetch-query");
	return stnp->fetch_query(query, key, Handle::UNDEFINED, false, asp.get());
}

Handle PersistSCM::sn_fetch_query4(Handle query, Handle key,
                                Handle meta, bool fresh, Handle hsn)
{
	GET_STNP;
	const AtomSpacePtr& asp = SchemeSmob::ss_get_env_as("fetch-query");
	return stnp->fetch_query(query, key, meta, fresh, asp.get());
}

void PersistSCM::sn_setvalue(Handle hsn, Handle key, ValuePtr val)
{
	GET_STNP;
	stnp->setValue(key, val);
}

ValuePtr PersistSCM::sn_getvalue(Handle hsn, Handle key)
{
	GET_STNP;
	return stnp->getValue(key);
}

// =====================================================================

Handle PersistSCM::current_storage(void)
{
	if (_sn and not _sn->connected()) _sn = nullptr;
	return Handle(_sn);
}

void opencog_persist_init(void)
{
	static PersistSCM patty;
}

// =================== END OF FILE ====================
