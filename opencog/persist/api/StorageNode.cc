/*
 * opencog/persist/api/StorageNode.cc
 *
 * Copyright (c) 2008-2010 OpenCog Foundation
 * Copyright (c) 2009,2013,2020,2022,2025 Linas Vepstas
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

#include <string>

#include <opencog/atoms/core/TypeNode.h>
#include <opencog/atoms/value/LinkValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atomspace/AtomSpace.h>
#include <opencog/persist/storage/storage_types.h>
#include "StorageNode.h"
#include "DispatchHash.h"

using namespace opencog;

// ====================================================================

StorageNode::StorageNode(Type t, std::string uri) :
	Node(t, uri)
{
	if (not nameserver().isA(t, STORAGE_NODE))
		throw RuntimeException(TRACE_INFO, "Bad inheritance!");
}

StorageNode::~StorageNode()
{
}

void StorageNode::setValue(const Handle& key, const ValuePtr& value)
{
	// The value must be store only if it is not one of the values
	// that causes an action to be taken. Action messages must not be
	// recorded, as otherwise, restore from disk/net will cause the
	// action to be triggered!
	if (PREDICATE_NODE != key->get_type())
	{
		Atom::setValue(key, value);
		return;
	}

	// Create a fast dispatch table by using case-statement
	// branching, instead of string compare.
	static constexpr uint32_t p_load_atomspace =
		dispatch_hash("*-load-atomspace-*");
	static constexpr uint32_t p_store_atomspace =
		dispatch_hash("*-store-atomspace-*");
	static constexpr uint32_t p_load_atoms_of_type =
		dispatch_hash("*-load-atoms-of-type-*");
	static constexpr uint32_t p_store_value =
		dispatch_hash("*-store-value-*");
	static constexpr uint32_t p_update_value =
		dispatch_hash("*-update-value-*");
	static constexpr uint32_t p_delete = dispatch_hash("*-delete-*");
	static constexpr uint32_t p_delete_recursive =
		dispatch_hash("*-delete-recursive-*");
	static constexpr uint32_t p_barrier = dispatch_hash("*-barrier-*");

	// *-load-frames-* is in getValue
	static constexpr uint32_t p_store_frames = dispatch_hash("*-store-frames-*");
	static constexpr uint32_t p_delete_frame = dispatch_hash("*-delete-frame-*");
	static constexpr uint32_t p_erase = dispatch_hash("*-erase-*");

	static constexpr uint32_t p_proxy_open = dispatch_hash("*-proxy-open-*");
	static constexpr uint32_t p_proxy_close = dispatch_hash("*-proxy-close-*");
	static constexpr uint32_t p_set_proxy = dispatch_hash("*-set-proxy-*");

// There's almost no chance at all that any user will use some key
// that is a PredicateNode that has a string name that collides with
// one of the above. That's because there's really no reason to set
// static values on a StorageNode. That I can think of. Still there's
// some chance of a hash collision. In this case, define
// COLLISION_PROOF, and recompile. Sorry in advance for the awful
// debug session you had that caused you to discover this comment!
//
// #define COLLISION_PROOF
#ifdef COLLISION_PROOF
	#define COLL(STR) if (0 == pred.compare(STR)) break;
#else
	#define COLL(STR)
#endif

	const std::string& pred = key->get_name();
	switch (dispatch_hash(pred.c_str()))
	{
		case p_load_atomspace:
			COLL("*-load-atomspace-*");
			load_atomspace(AtomSpaceCast(value).get());
			return;
		case p_store_atomspace:
			COLL("*-store-atomspace-*");
			store_atomspace(AtomSpaceCast(value).get());
			return;
		case p_load_atoms_of_type: {
			COLL("*-load-atoms-of-type-*");
			if (not value->is_type(TYPE_NODE)) return;
			Type t = TypeNodeCast(HandleCast(value))->get_type();
			fetch_all_atoms_of_type(t, getAtomSpace());
			return;
		}
		case p_store_value: {
			COLL("*-store-value-*");
			if (not value->is_type(LINK_VALUE)) return;
			const ValueSeq& vsq(LinkValueCast(value)->value());
			if (2 > vsq.size()) return;
			store_value(HandleCast(vsq[0]), HandleCast(vsq[1]));
			return;
		}
		case p_update_value: {
			COLL("*-update-value-*");
			if (not value->is_type(LINK_VALUE)) return;
			const ValueSeq& vsq(LinkValueCast(value)->value());
			if (3 > vsq.size()) return;
			update_value(HandleCast(vsq[0]), HandleCast(vsq[1]), vsq[2]);
			return;
		}
		case p_delete:
			COLL("*-delete-*");
			remove_msg(key, value, false);
			return;
		case p_delete_recursive:
			COLL("*-delete-recursive-*");
			remove_msg(key, value, true);
			return;
		case p_barrier:
			COLL("*-barrier-*");
			barrier(AtomSpaceCast(value).get());
			return;
		case p_store_frames:
			COLL("*-store-frames-*");
			store_frames(HandleCast(value));
			return;
		case p_delete_frame:
			COLL("*-delete-frame-*");
			delete_frame(HandleCast(value));
			return;
		case p_erase:
			COLL("*-erase-*");
			erase();
			return;
		case p_proxy_open:
			COLL("*-proxy-open-*");
			proxy_open();
			return;
		case p_proxy_close:
			COLL("*-proxy-close-*");
			proxy_close();
			return;
		case p_set_proxy:
			COLL("*-set-proxy-*");
			set_proxy(HandleCast(value));
			return;
		default:
			break;
	}

	// Some other predicate. Store it.
	Atom::setValue(key, value);
}

ValuePtr StorageNode::getValue(const Handle& key) const
{
	if (PREDICATE_NODE != key->get_type())
		return Atom::getValue(key);

	const std::string& pred(key->get_name());

	if (0 == pred.compare("*-load-frames-*"))
		return createLinkValue(const_cast<StorageNode*>(this)->load_frames());

	if (0 == pred.compare("*-monitor-*"))
		return createStringValue(const_cast<StorageNode*>(this)->monitor());

	return Atom::getValue(key);
}

void StorageNode::proxy_open(void)
{
	throw RuntimeException(TRACE_INFO,
		"This StorageNode does not implement proxying!");
}

void StorageNode::proxy_close(void)
{
	throw RuntimeException(TRACE_INFO,
		"This StorageNode does not implement proxying!");
}

void StorageNode::set_proxy(const Handle&)
{
	throw RuntimeException(TRACE_INFO,
		"This StorageNode does not implement proxying!");
}

std::string StorageNode::monitor(void)
{
	return "This StorageNode does not implement a monitor.\n";
}

// ====================================================================

void StorageNode::barrier(AtomSpace* as)
{
	if (nullptr == as) as = getAtomSpace();
	as->barrier();
}

void StorageNode::store_atom(const Handle& h)
{
	if (_atom_space->get_read_only())
		throw RuntimeException(TRACE_INFO, "Read-only AtomSpace!");

	storeAtom(h);
}

void StorageNode::store_value(const Handle& h, const Handle& key)
{
	if (_atom_space->get_read_only())
		throw RuntimeException(TRACE_INFO, "Read-only AtomSpace!");

	storeValue(h, key);
}

void StorageNode::update_value(const Handle& h, const Handle& key,
                               const ValuePtr& delta)
{
	if (_atom_space->get_read_only())
		throw RuntimeException(TRACE_INFO, "Read-only AtomSpace!");

	updateValue(h, key, delta);
}

bool StorageNode::remove_atom(AtomSpace* as, Handle h, bool recursive)
{
	// Removal is done with a two-step process. First, we tell storage
	// about the Atom that is going away. It's still in the AtomSpace at
	// this point, so storage can grab whatever data it needs from the
	// AtomSpace (e.g. grab the IncomingSet). Next, we extract from the
	// AtomSpace, and then finally, we tell storage that we're done.
	//
	// The postRemove call gets the return code from the AtomSpace
	// extract. This is because the AtomSpace extract logic is complex,
	// with success & failure depending on subtle interactions between
	// read-only, framing, hiding and other concerns. It is too hard to
	// ask each kind of storage to try to replicate this logic. The
	// solution used here is minimalist: if the AtomSpace extract worked,
	// let storage know. If the AtomSpace extract worked, storage should
	// finish the remove. Otherwise, it should keep the atom.

	if (not recursive and not h->isIncomingSetEmpty()) return false;

	// Removal of atoms from read-only storage is not allowed. However,
	// it is OK to remove atoms from a read-only AtomSpace, because
	// it is acting as a cache for the database, and removal is used
	// used to free up RAM storage.
	if (_atom_space->get_read_only())
		return as->extract_atom(h, recursive);

	// Warn storage about upcoming extraction; do the extraction, then
	// tell storage how it all worked out.
	preRemoveAtom(as, h, recursive);
	bool exok = as->extract_atom(h, recursive);
	postRemoveAtom(as, h, recursive, exok);

	return exok;
}

//. Same as above, but using teh message format.
void StorageNode::remove_msg(Handle h, ValuePtr value, bool recursive)
{
	// Expect either delete of a single atom, or
	// a LinkValue giving the AtomSpace and the Atom to delete.
	if (value->is_type(ATOM))
	{
		Handle atm(HandleCast(value));
		remove_atom(atm->getAtomSpace(), atm, recursive);
		return;
	}

	// Assume a LinkValue of some kind.
	const LinkValuePtr& lvp(LinkValueCast(value));
	AtomSpacePtr asp;
	for (const ValuePtr& vp: lvp->value())
	{
		if (vp->is_type(ATOM_SPACE))
		{
			asp = AtomSpaceCast(vp);
			continue;
		}
		if (asp)
			remove_atom(asp.get(), HandleCast(vp), recursive);
		else
		{
			Handle atm(HandleCast(vp));
			remove_atom(atm->getAtomSpace(), atm, recursive);
		}
	}
}

Handle StorageNode::fetch_atom(const Handle& h, AtomSpace* as)
{
	if (nullptr == h) return Handle::UNDEFINED;
	if (nullptr == as) as = getAtomSpace();

	// Now, get the latest values from the backing store.
	// The operation here is to CLOBBER the values, NOT to merge them!
	// The goal of an explicit fetch is to explicitly fetch the values,
	// and not to play monkey-shines with them.  If you want something
	// else, then save the old TV, fetch the new TV, and combine them
	// with your favorite algo.
	Handle ah = as->add_atom(h);
	if (nullptr == ah) return ah; // if read-only, then cannot update.
	getAtom(ah);
	return ah;
}

Handle StorageNode::fetch_value(const Handle& h, const Handle& key,
                                AtomSpace* as)
{
	if (nullptr == as) as = getAtomSpace();
	Handle lkey = as->add_atom(key);
	Handle lh = as->add_atom(h);
	loadValue(lh, lkey);
	return lh;
}

Handle StorageNode::fetch_incoming_set(const Handle& h, bool recursive,
                                       AtomSpace* as)
{
	if (nullptr == as) as = getAtomSpace();
	Handle lh = as->get_atom(h);
	if (nullptr == lh) return lh;

	// Get everything from the backing store.
	fetchIncomingSet(as, lh);

	if (not recursive) return lh;

	IncomingSet vh(h->getIncomingSet());
	for (const Handle& lp : vh)
		fetch_incoming_set(lp, true, as);

	return lh;
}

Handle StorageNode::fetch_incoming_by_type(const Handle& h, Type t,
                                           AtomSpace* as)
{
	if (nullptr == as) as = getAtomSpace();
	Handle lh = as->get_atom(h);
	if (nullptr == lh) return lh;

	// Get everything from the backing store.
	fetchIncomingByType(as, lh, t);

	return lh;
}

Handle StorageNode::fetch_query(const Handle& query, const Handle& key,
                                const Handle& metadata, bool fresh,
                                AtomSpace* as)
{
	// Queries can be anything executable or evaluatable.
	if (not query->is_executable() and not query->is_evaluatable())
		throw RuntimeException(TRACE_INFO, "Not executable!");

	if (nullptr == as) as = getAtomSpace();
	Handle lkey = as->add_atom(key);
	Handle lq = as->add_atom(query);
	Handle lmeta = metadata;
	if (Handle::UNDEFINED != lmeta) lmeta = as->add_atom(lmeta);

	runQuery(lq, lkey, lmeta, fresh);
	return lq;
}

void StorageNode::load_atomspace(AtomSpace* as)
{
	if (nullptr == as) as = getAtomSpace();
	loadAtomSpace(as);
}

/**
 * Use the backing store to store entire AtomSpace.
 */
void StorageNode::store_atomspace(AtomSpace* as)
{
	if (nullptr == as) as = getAtomSpace();
	storeAtomSpace(as);
}

void StorageNode::fetch_all_atoms_of_type(Type t, AtomSpace* as)
{
	if (nullptr == as) as = getAtomSpace();
	loadType(as, t);
}

HandleSeq StorageNode::load_frames(void)
{
	return loadFrameDAG();
}

void StorageNode::store_frames(const Handle& has)
{
	return storeFrameDAG((AtomSpace*)has.get());
}

void StorageNode::delete_frame(const Handle& has)
{
	return deleteFrame((AtomSpace*)has.get());
}

// ====================================================================
