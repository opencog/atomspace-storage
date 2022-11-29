/*
 * opencog/persist/proxy/PassThruyProxy.h
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

#ifndef _OPENCOG_PASS_THRU_PROXY_H
#define _OPENCOG_PASS_THRU_PROXY_H

#include <opencog/persist/api/StorageNode.h>

namespace opencog
{
/** \addtogroup grp_atomspace
 *  @{
 */
class PassThruProxy : public StorageNode
{
protected:
	std::vector<StorageNodePtr> _store_nodes;

public:
	PassThruProxy(Type, std::string);
	virtual ~PassThruProxy();

	// ----------------------------------------------------------------
	// StorageNode virtuals  are all no-ops.
	virtual void open(void) {}
	virtual void close(void) {}
	virtual bool connected(void) { return  true; }
	virtual void create(void) {}

	virtual void destroy(void);
	virtual void erase(void);

	virtual std::string monitor(void);

	// ----------------------------------------------------------------
	// BackingStore virtuals.

	virtual void getAtom(const Handle&);
	virtual void fetchIncomingSet(AtomSpace*, const Handle&);
	virtual void fetchIncomingByType(AtomSpace*, const Handle&, Type);
	virtual void storeAtom(const Handle&, bool synchronous = false);
	virtual void removeAtom(AtomSpace*, const Handle&, bool recursive);
	virtual void storeValue(const Handle& atom, const Handle& key);
	virtual void updateValue(const Handle& atom, const Handle& key,
	                         const ValuePtr& delta);
	virtual void loadValue(const Handle& atom, const Handle& key);

	virtual void loadType(AtomSpace*, Type);
	virtual void loadAtomSpace(AtomSpace*);
	virtual void storeAtomSpace(const AtomSpace*);

	virtual HandleSeq loadFrameDAG(void);
	virtual void storeFrameDAG(AtomSpace*);

	virtual void deleteFrame(AtomSpace*);
	virtual void barrier(AtomSpace* = nullptr);

protected:
	virtual Handle getLink(Type, const HandleSeq&);
};

typedef std::shared_ptr<PassThruProxy> PassThruProxyPtr;
static inline PassThruProxyPtr PassThruProxyCast(const Handle& h)
	{ return std::dynamic_pointer_cast<PassThruProxy>(h); }

/** @}*/
} // namespace opencog

#endif // _OPENCOG_PASS_THRU_PROXY_H
