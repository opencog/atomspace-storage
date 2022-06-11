/*
 * JSCommands.cc
 * Fast command interpreter for basic JSON AtomSpace commands.
 *
 * Copyright (C) 2020, 2021, 2022 Linas Vepstas
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

#include <time.h>

#include <functional>
#include <iomanip>
#include <string>

#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/truthvalue/TruthValue.h>
#include <opencog/atomspace/AtomSpace.h>

#include "JSCommands.h"
#include "Json.h"

using namespace opencog;

static std::string reterr(const std::string& cmd)
{
	return "JSON/JavaScript function not supported: >>" + cmd + "<<\n";
}

/// The cogserver provides a network API to send/receive Atoms, encoded
/// as JSON, over the internet. This is NOT as efficient as the
/// s-expression API, but is more convenient for web developers.
//
std::string JSCommands::interpret_command(AtomSpace* as,
                                          const std::string& cmd)
{
	// Fast dispatch. There should be zero hash collisions
	// here. If there are, we are in trouble. (Well, if there
	// are collisions, just prepend a dot?)
	static const size_t gtatm = std::hash<std::string>{}("getAtoms");
	static const size_t haven = std::hash<std::string>{}("haveNode");
	static const size_t havel = std::hash<std::string>{}("haveLink");
	static const size_t havea = std::hash<std::string>{}("haveAtom");
	static const size_t makea = std::hash<std::string>{}("makeAtom");
	static const size_t gtinc = std::hash<std::string>{}("getIncoming");
	static const size_t gettv = std::hash<std::string>{}("getTV");
	static const size_t settv = std::hash<std::string>{}("setTV");
	static const size_t gtval = std::hash<std::string>{}("getValues");
	static const size_t stval = std::hash<std::string>{}("setValue");

	// Ignore comments, blank lines
	if ('/' == cmd[0]) return "";
	if ('\n' == cmd[0]) return "";

	// Find the command and dispatch
	size_t cpos = cmd.find_first_of(".");
	if (std::string::npos == cpos) return reterr(cmd);

	size_t pos = cmd.find_first_not_of(". \n\t", cpos);
	if (std::string::npos == pos) return reterr(cmd);

	size_t epos = cmd.find_first_of("( \n\t", pos);
	if (std::string::npos == epos) return reterr(cmd);

	size_t act = std::hash<std::string>{}(cmd.substr(pos, epos-pos));

	// -----------------------------------------------
	// AtomSpace.getAtoms("Node", true)
	if (gtatm == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		Type t = NOTYPE;
		try {
			t = Json::decode_type(cmd, pos);
		}
		catch(...) {
			return "Unknown type: " + cmd.substr(pos);
		}

		pos = cmd.find_first_not_of(",) \n\t", pos);
		bool get_subtypes = true;
		if (std::string::npos != pos and (
				0 == cmd.compare(pos, 1, "0") or
				0 == cmd.compare(pos, 5, "false")))
			get_subtypes = false;

		std::string rv = "[\n";
		HandleSeq hset;
		as->get_handles_by_type(hset, t, get_subtypes);
		bool first = true;
		for (const Handle& h: hset)
		{
			if (not first) { rv += ",\n"; } else { first = false; }
			rv += Json::encode_atom(h, "  ");
		}
		rv += "]\n";
		return rv;
	}

	// -----------------------------------------------
	// AtomSpace.haveNode("Concept", "foo")
	if (haven == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		Type t = NOTYPE;
		try {
			t = Json::decode_type(cmd, pos);
		}
		catch(...) {
			return "Unknown type: " + cmd.substr(pos);
		}

		if (not nameserver().isA(t, NODE))
			return "Type is not a Node type: " + cmd.substr(epos);

		pos = cmd.find_first_not_of(",) \n\t", pos);
		epos = cmd.size();
		std::string name = Json::get_node_name(cmd, pos, epos);
		Handle h = as->get_node(t, std::move(name));

		if (nullptr == h) return "false\n";
		return "true\n";
	}

	// -----------------------------------------------
	// AtomSpace.haveLink("List", [{ "type": "ConceptNode", "name": "foo"}])
	if (havel == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		Type t = NOTYPE;
		try {
			t = Json::decode_type(cmd, pos);
		}
		catch(...) {
			return "Unknown type: " + cmd.substr(pos);
		}

		if (not nameserver().isA(t, LINK))
			return "Type is not a Link type: " + cmd.substr(epos);

		pos = cmd.find_first_not_of(", \n\t", pos);
		epos = cmd.size();

		HandleSeq hs;

		size_t l = pos;
		size_t r = epos;
		while (std::string::npos != r)
		{
			Handle ho = Json::decode_atom(cmd, l, r);
			if (nullptr == ho) return "false\n";
			hs.push_back(ho);

			// Look for the comma
			l = cmd.find(",", r);
			if (std::string::npos == l) break;
			l ++;
			r = epos;
		}
		Handle h = as->get_link(t, std::move(hs));

		if (nullptr == h) return "false\n";
		return "true\n";
	}

	// -----------------------------------------------
	// AtomSpace.haveAtom({ "type": "ConceptNode", "name": "foo"})
	if (havea == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		epos = cmd.size();

		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) return "false\n";

		h = as->get_atom(h);

		if (nullptr == h) return "false\n";
		return "true\n";
	}

	// -----------------------------------------------
	// AtomSpace.makeAtom({ "type": "ConceptNode", "name": "foo"})
	if (makea == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		epos = cmd.size();

		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) return "false\n";

		h = as->add_atom(h);

		if (nullptr == h) return "false\n";
		return "true\n";
	}

	// -----------------------------------------------
	// AtomSpace.getIncoming({ "type": "ConceptNode", "name": "foo"})
	if (gtinc == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		epos = cmd.size();

		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) return "[]\n";

		h = as->get_atom(h);

		if (nullptr == h) return "[]\n";

		Type t = NOTYPE;
		pos = cmd.find(",", epos);
		if (std::string::npos != pos)
		{
			pos++;
			try {
				t = Json::decode_type(cmd, pos);
			}
			catch(...) {
				return "Unknown type: " + cmd.substr(pos);
			}
		}

		IncomingSet is;
		if (NOTYPE != t)
			is = h->getIncomingSetByType(t);
		else
			is = h->getIncomingSet();

		bool first = true;
		std::string alist = "[";
		for (const Handle& hi : is)
		{
			if (not first) { alist += ",\n"; } else { first = false; }
			alist += Json::encode_atom(hi, "");
		}
		alist += "]\n";
		return alist;
	}

	// -----------------------------------------------
	// AtomSpace.getValues({ "type": "ConceptNode", "name": "foo"})
	if (gtval == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		epos = cmd.size();

		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) return "[]\n";

		h = as->get_atom(h);

		return Json::encode_atom_values(h);
	}

	// -----------------------------------------------
	// AtomSpace.setValue({ "type": "ConceptNode", "name": "foo",
	//     "key": { "type": "PredicateNode", "name": "keewee" },
	//     "value": { "type": "FloatValue", "value": [1, 2, 3] } } )
	if (stval == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		epos = cmd.size();

		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) return "false";

		h = as->get_atom(h);

		if (nullptr == h) return "false";

		// The key follows the name.
		pos = cmd.find("\"key\":", epos);
		if (std::string::npos == pos) return "false";
		pos += 6;
		epos = cmd.size();
		Handle k = Json::decode_atom(cmd, pos, epos);
		if (nullptr == k) return "false";

		k = as->add_atom(k);

		// Skip past a comma
		pos = cmd.find(',', epos);
		if (std::string::npos == pos) return "false";

		// The value comes next.
		pos = cmd.find("\"value\":", pos);
		if (std::string::npos == pos) return "false";
		pos += 8;
		epos = cmd.size();
		ValuePtr v = Json::decode_value(cmd, pos, epos);
		if (nullptr == v) return "false";

		as->set_value(h, k, v);
		return "true";
	}

	// -----------------------------------------------
	// AtomSpace.getTV({ "type": "ConceptNode", "name": "foo"})
	if (gettv == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		epos = cmd.size();

		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) return "[]\n";

		h = as->get_atom(h);

		if (nullptr == h) return "[]\n";

		std::string alist = "[{ \"value\": \n";
		alist += Json::encode_value(ValueCast(h->getTruthValue()));
		alist += "}]\n";
		return alist;
	}

	// -----------------------------------------------
	// AtomSpace.setTV({ "type": "ConceptNode", "name": "foo",
	//     "value": { "type": "SimpleTruthValue", "value": [0.2, 0.3] } } )
	if (settv == act)
	{
		pos = cmd.find_first_of("(", epos);
		if (std::string::npos == pos) return reterr(cmd);
		pos++;
		epos = cmd.size();

		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) return "false";

		h = as->get_atom(h);

		if (nullptr == h) return "false";

		// The value comes next.
		pos = cmd.find("\"value\":", pos);
		if (std::string::npos == pos) return "false";
		pos += 8;
		epos = cmd.size();
		ValuePtr v = Json::decode_value(cmd, pos, epos);
		if (nullptr == v) return "false";

printf("duuude tv=%s\n", v->to_string().c_str());
		as->set_truthvalue(h, TruthValueCast(v));
		return "true";
	}

	// -----------------------------------------------
	return reterr(cmd);
}
