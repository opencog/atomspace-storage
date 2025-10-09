/*
 * ProxyNode.cc
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

#include <opencog/persist/proxy/ProxyNode.h>
#include <opencog/persist/storage/storage_types.h>

using namespace opencog;

ProxyNode::ProxyNode(const std::string&& name)
	: StorageNode(PROXY_NODE, std::move(name))
{
	init();
}

ProxyNode::ProxyNode(Type t, const std::string&& name)
	: StorageNode(t, std::move(name))
{
	init();
}

ProxyNode::~ProxyNode()
{
}

void ProxyNode::init(void)
{
	have_getAtom = false;
	have_fetchIncomingSet = false;
	have_fetchIncomingByType = false;
	have_storeAtom = false;
	have_removeAtom = false;
	have_storeValue = false;
	have_updateValue = false;
	have_loadValue = false;
	have_loadType = false;
	have_loadAtomSpace = false;
	have_storeAtomSpace = false;
}

void ProxyNode::proxy_open(void)
{
	throw RuntimeException(TRACE_INFO,
		"If you want to open the proxy, just say `cog-open`");
}

void ProxyNode::proxy_close(void)
{
	throw RuntimeException(TRACE_INFO,
		"If you want to close the proxy, just say `cog-close`");
}

void ProxyNode::set_proxy(const Handle&)
{
	throw RuntimeException(TRACE_INFO,
		"Error: `cog-set-proxy!` is not appropriate, here.");
}

std::string ProxyNode::monitor(void)
{
	std::string rpt;
	rpt += "Monitoring not implemented for ";
	rpt += to_short_string().substr(1);
	rpt.pop_back();
	rpt += "\n";
	return rpt;
}

void ProxyNode::setValue(const Handle& key, const ValuePtr& value)
{
	// It is safe to store the parts of a proxy node. Always.
	StorageNode::setValue(key, value);

	// If we don't understand the message, just ignore it.
	if (PREDICATE_NODE != key->get_type() or
	    0 != key->get_name().compare("*-proxy-parts-*"))
	{
		return;
	}

	// If there is no ListLink, then just grab that.
	if (value->is_type(STORAGE_NODE))
	{
		_parts.emplace_back(StorageNodeCast(HandleCast(value)));
		return;
	}

	// Expect the parts to be wrapped in a ListLink
	if (not value->is_type(LIST_LINK))
		throw SyntaxException(TRACE_INFO,
			"Expecting parameters in a ListLink! Got\n%s\n",
			value->to_short_string().c_str());

	for (const Handle& h : HandleCast(value)->getOutgoingSet())
	{
		StorageNodePtr stnp = StorageNodeCast(h);
		if (nullptr == stnp)
		{
			// If its a StorageNode but the cast failed, that
			// means the type definition was not loaded. Print a
			// user-friendly error message for this case.
			if (nameserver().isA(h->get_type(), STORAGE_NODE))
			{
				throw SyntaxException(TRACE_INFO,
					"There is no definition for %s.\n"
					"Did you forget to load the module that deffines this?\n"
					"For example: `(use-modules (opencog persist-rocks))`\n"
					"Config was %s\n",
					h->to_short_string().c_str(),
					value->to_short_string().c_str());
			}
			throw SyntaxException(TRACE_INFO,
				"Expecting a list of Storage or ProxyNodes! Got\n%s\n",
				value->to_short_string().c_str());
		}

		_parts.emplace_back(stnp);
	}
}

void ProxyNode::destroy(void) {}
void ProxyNode::erase(void) {}

HandleSeq ProxyNode::loadFrameDAG(void)
{
	// XXX FIXME;
	return HandleSeq();
}

Handle ProxyNode::getLink(Type t, const HandleSeq& hseq)
{
	// Ugh Copy
	HandleSeq hsc(hseq);
	return _atom_space->get_link(t, std::move(hsc));
}

void opencog_persist_proxy_init(void)
{
   // Force shared lib ctors to run
};

/* ===================== END OF FILE ===================== */
