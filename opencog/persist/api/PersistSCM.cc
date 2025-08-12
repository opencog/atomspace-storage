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
	// define_scheme_primitive(..., false); means that these functions
	// will NOT be `define-public` and just plain `define`. Thus,
	// accessible within the module, but not outside of it.
	define_scheme_primitive("direct-setvalue!",
	             &PersistSCM::direct_setvalue, "persist", false);
}

// =====================================================================

void PersistSCM::direct_setvalue(Handle hsn, Handle key, ValuePtr val)
{
	if (not nameserver().isA(hsn->get_type(), STORAGE_NODE)) {
		throw RuntimeException(TRACE_INFO,
			"Expecting StorageNode, got %s", hsn->to_short_string().c_str());
	}

	StorageNodePtr stnp = StorageNodeCast(hsn);

	/* The cast will fail, if the dynamic library that defines the type */
	/* isn't loaded. This is the user's job. They can do it by saying */
	/* (use-modules (opencog persist-foo)) */
	if (nullptr == stnp) {
		if (hsn->get_type() == STORAGE_NODE) {
			throw RuntimeException(TRACE_INFO,
				"A StorageNode cannot be used directly; "
				"only it's sub-types provide the needed implementation!");
		}
		throw RuntimeException(TRACE_INFO,
			"Not opened; please load module that defines %s\n"
			"Like so: (use-modules (opencog persist-foo))\n"
			"where `foo` is the module providing the node.",
			nameserver().getTypeName(hsn->get_type()).c_str());
	}

	stnp->setValue(key, val);
}

// =====================================================================

void opencog_persist_init(void)
{
	static PersistSCM patty;
}

// =================== END OF FILE ====================
