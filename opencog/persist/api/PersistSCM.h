/*
 * opencog/persist/api/PersistSCM.h
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

#ifndef _OPENCOG_PERSIST_SCM_H
#define _OPENCOG_PERSIST_SCM_H

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/base/Handle.h>
#include <opencog/guile/SchemeModule.h>
#include <opencog/persist/api/StorageNode.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class PersistSCM : public ModuleWrap
{
private:
	void init(void);

	// These take the storage-node to use as first argument.
	static void sn_setvalue(Handle, Handle, ValuePtr);
	static ValuePtr sn_getvalue(Handle, Handle);

public:
	PersistSCM(void);
}; // class

/** @}*/
}  // namespace

extern "C" {
void opencog_persist_init(void);
};

#endif // _OPENCOG_PERSIST_SCM_H
