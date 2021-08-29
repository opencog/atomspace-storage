/*
 * FUNCTION:
 * File-backed persistent storage.
 *
 * HISTORY:
 * Copyright (c) 2021 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
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

#ifndef _OPENCOG_FILE_STORAGE_H
#define _OPENCOG_FILE_STORAGE_H

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/LinkValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atoms/base/Valuation.h>

#include <opencog/persist/api/StorageNode.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class FileStorageNode : public StorageNode
{
	private:
		void connect(const char *);

	public:
		FileStorageNode(Type t, const std::string& uri);
		FileStorageNode(const std::string& uri);
		virtual ~FileStorageNode();

		void open(void);
		void close(void);
		void connect(void);
		bool connected(void); // connection to DB is alive

		void kill_data(void);       // destroy DB contents

#if 0
		void create(void) { create_database(); }
		void destroy(void) { kill_data(); /* TODO also delete the db */ }
		void erase(void) { kill_data(); }
#endif

		// AtomStorage interface
		Handle getNode(Type, const char *);
		Handle getLink(Type, const HandleSeq&);
		void getIncomingSet(AtomTable&, const Handle&);
		void getIncomingByType(AtomTable&, const Handle&, Type t);
		void storeAtom(const Handle&, bool synchronous = false);
		void removeAtom(const Handle&, bool recursive);
		void storeValue(const Handle&, const Handle&);
		void loadValue(const Handle&, const Handle&);
		void loadType(AtomTable&, Type);
		void barrier();
		void flushStoreQueue();

		// Large-scale loads and saves
		void loadAtomSpace(AtomTable &); // Load entire contents of DB
		void storeAtomSpace(const AtomTable &); // Store all of AtomTable

		static Handle factory(const Handle&);
};

typedef std::shared_ptr<FileStorageNode> FileStorageNodePtr;
static inline FileStorageNodePtr FileStorageNodeCast(const Handle& h)
   { return std::dynamic_pointer_cast<FileStorageNode>(h); }
static inline FileStorageNodePtr FileStorageNodeCast(AtomPtr a)
   { return std::dynamic_pointer_cast<FileStorageNode>(a); }

#define createFileStorageNode std::make_shared<FileStorageNode>


/** @}*/
} // namespace opencog

#endif // _OPENCOG_FILE_STORAGE_H
