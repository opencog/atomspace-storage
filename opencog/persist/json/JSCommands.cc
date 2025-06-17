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
#include <opencog/atomspace/version.h>

#include "JSCommands.h"
#include "Json.h"

using namespace opencog;

// Helper function to parse boolean parameters from commands
static bool parse_bool_param(const std::string& cmd, size_t& pos, size_t epos, bool js_mode)
{
	bool recursive = false;
	if (js_mode) {
		pos = cmd.find_first_not_of(",) \n\t", pos);
		if (std::string::npos != pos) {
			recursive = true;
			if (0 == cmd.compare(pos, 1, "0") or
			    0 == cmd.compare(pos, 5, "false") or
			    0 == cmd.compare(pos, 5, "False"))
				recursive = false;
		}
	} else {
		// In MCP mode, look for "subclass" or "recursive" field
		// Search backwards from epos to find the boolean
		size_t bool_pos = cmd.rfind("\"subclass\"", epos);
		if (std::string::npos == bool_pos)
			bool_pos = cmd.rfind("\"recursive\"", epos);
		if (std::string::npos != bool_pos) {
			bool_pos = cmd.find(':', bool_pos);
			if (std::string::npos != bool_pos && bool_pos < epos) {
				bool_pos = cmd.find_first_not_of(": \n\t", bool_pos);
				if (std::string::npos != bool_pos && bool_pos < epos) {
					if (0 == cmd.compare(bool_pos, 4, "true"))
						recursive = true;
				}
			}
		}
	}
	return recursive;
}

static std::string reterr(const std::string& cmd)
{
	return "{\"success\": false, \"error\": { \"code\": -32600, "
		"\"message\": \"Invalid Request\", \"data\": { "
		"\"details\": \"" + cmd + "\"}}}\n";
}

static std::string retmsgerr(const std::string& errmsg)
{
	return "{\"success\": false, \"error\": { \"code\": -32602, "
		"\"message\": \"Invalid params\", \"data\": { "
		"\"details\": \"" + errmsg + "\"}}}\n";
}

// Because MCP is not really JSON, (see below) we cannot return booleans
// as booleans; they must be strings. So add some extra quotes to them.
#define RETURN(RV) { \
	if (js_mode) return RV "\n"; \
	return "{\"content\": [{\"type\":\"text\", \"text\": \"" RV "\"}]}\n"; }

// Sigh. So MCP is not really JSON, it's pseudo-json. It does use
// "valid" JSON to create, send and receive messages, but all tool-use
// messages must always be text strings (and not JSON). But our core
// API returns ... actual JSON. So .. fake it. Escape all quotes in
// in the JSON, surround the whole thing with quotes, and bingo: its
// now a text string. Hurrah. Well, for now, it seems that at least
// Claude Code seems to grok this at some level, so we're good for now.
// I personally think it's ugly and annoying but whatever.
#define RETURNSTR(RV) { \
	if (js_mode) return RV + "\n"; \
	/* return "{\"content\": [{\"type\":\"text\", \"text\": " + RV + "}]}\n"; } */ \
	std::string srv(RV); \
	std::replace(srv.begin(), srv.end(), '\n', ' '); \
	std::stringstream ss; \
	ss << std::quoted(srv); \
	std::string rs = "{\"content\": [{\"type\":\"text\", \"text\": "; \
	rs += ss.str(); \
	rs += "}]}\n"; \
	return rs; \
	}

// Common boilerplate
#define CHK_FOR_JSON_ARG \
	size_t brace = cmd.find_first_not_of(" \n\t", pos); \
	bool is_json_object = (brace != std::string::npos and cmd[brace] == '{');

#define GET_TYPE \
	Type t = NOTYPE; \
	try { \
		t = Json::decode_type_arg(cmd, pos); \
	} catch(...) { \
		return retmsgerr("Unknown type: " + cmd.substr(pos)); \
	}

#define GET_BOOL \
	bool recursive = parse_bool_param(cmd, pos, epos, js_mode);

#define GET_ATOM(rv) \
	Handle h = Json::decode_atom(cmd, pos, epos); \
	if (nullptr == h) RETURN(rv); \
	h = as->get_atom(h); \
	if (nullptr == h) RETURN(rv);

#define ADD_ATOM \
	Handle h = Json::decode_atom(cmd, pos, epos); \
	if (nullptr == h) RETURN("false"); \
	h = as->add_atom(h); \
	if (nullptr == h) RETURN("false");

#define GET_KEY \
	pos = cmd.find("\"key\":", epos); \
	if (std::string::npos == pos) RETURN("false"); \
	pos += 6; \
	epos = cmd.size(); \
	Handle k = Json::decode_atom(cmd, pos, epos); \
	if (nullptr == k) RETURN("false"); \
	k = as->add_atom(k); \
	pos = cmd.find(',', epos); \
	if (std::string::npos == pos) RETURN("false");

#define GET_VALUE \
	pos = cmd.find("\"value\":", pos); \
	if (std::string::npos != pos) \
		pos += 8; \
	else { \
		pos = cmd.find("\"values\":", pos); \
		if (std::string::npos == pos) RETURN("false"); \
		pos += 9; \
	} \
	epos = cmd.size(); \
	ValuePtr v = Json::decode_value(cmd, pos, epos); \
	if (nullptr == v) RETURN("false");

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
	static const size_t versn = std::hash<std::string>{}("version");
	static const size_t gtatm = std::hash<std::string>{}("getAtoms");
	static const size_t gtsub = std::hash<std::string>{}("getSubTypes");
	static const size_t gtsup = std::hash<std::string>{}("getSuperTypes");
	static const size_t haven = std::hash<std::string>{}("haveNode");
	static const size_t havel = std::hash<std::string>{}("haveLink");
	static const size_t havea = std::hash<std::string>{}("haveAtom");
	static const size_t makea = std::hash<std::string>{}("makeAtom");
	static const size_t loada = std::hash<std::string>{}("loadAtoms");
	static const size_t gtinc = std::hash<std::string>{}("getIncoming");
	static const size_t gettv = std::hash<std::string>{}("getTV");
	static const size_t settv = std::hash<std::string>{}("setTV");
	static const size_t gtval = std::hash<std::string>{}("getValues");
	static const size_t stval = std::hash<std::string>{}("setValue");
	static const size_t execu = std::hash<std::string>{}("execute");
	static const size_t extra = std::hash<std::string>{}("extract");

	// Ignore comments, blank lines
	size_t cpos = cmd.find_first_not_of(" \n\t");
	if (std::string::npos == cpos) return "";
	if ('/' == cmd[cpos]) return "";
	if ('#' == cmd[cpos]) return "";

	// In js-mode, the commands are all of the form
	//    AtomSpace.someCommand(args)
	// In MCP-mode, the commands are all of the form
	//    { "tool": "someToolName", "params": { args }}
	// So if we find an open brace at the start of the string,
	// then we are in MCP-mode. Else we are in JS-mode.

	bool js_mode = true;

	if ('{' == cmd[cpos])
		js_mode = false;
	else if ('A' != cmd[cpos])
		return reterr(cmd);

	size_t pos, epos;
	size_t act;

	if (js_mode)
	{
		cpos = cmd.find_first_of(".", cpos);
		if (std::string::npos == cpos) return reterr(cmd);

		cpos = cmd.find_first_not_of(". \n\t", cpos);
		if (std::string::npos == cpos) return reterr(cmd);

		epos = cmd.find_first_of("( \n\t", cpos);
		if (std::string::npos == epos) return reterr(cmd);
		pos = epos + 1;

		act = std::hash<std::string>{}(cmd.substr(cpos, epos-cpos));
		epos = cmd.size();
	}
	else
	{
		cpos = cmd.find("\"tool\":", cpos);
		if (std::string::npos == cpos) return reterr(cmd);
		cpos += 7; // 7 == strlen("\"tool\":");

		cpos = cmd.find_first_not_of("\" \n\t", cpos);
		if (std::string::npos == cpos) return reterr(cmd);

		size_t tool_end = cmd.find_first_of("\", \n\t", cpos);
		if (std::string::npos == tool_end) return reterr(cmd);

		// Extract the tool name
		std::string tool_name = cmd.substr(cpos, tool_end-cpos);
		act = std::hash<std::string>{}(tool_name);

		// OK, so now pos and tool_end bracket the tool name.
		// Advance cpos past the params.
		pos = cmd.find("\"params\":", tool_end);
		if (std::string::npos == pos) return reterr(cmd);
		pos += 9; // 9 == strlen("\"params\":");

		// For MCP mode, find the closing } of the params object
		// We need to find the matching } for the { that comes after "params":
		int brace_count = 0;
		size_t scan_pos = pos;
		epos = std::string::npos;

		while (scan_pos < cmd.size()) {
			if (cmd[scan_pos] == '{') {
				brace_count++;
			} else if (cmd[scan_pos] == '}') {
				brace_count--;
				if (brace_count == 0) {
					epos = scan_pos + 1; // Include the closing }
					break;
				}
			}
			scan_pos++;
		}

		if (std::string::npos == epos) return reterr(cmd);
	}

	// -----------------------------------------------
	// Get version
	// AtomSpace.version()
	// AtomSpace.version({})
	if (versn == act)
	{
		RETURN(ATOMSPACE_VERSION_STRING);
	}

	// -----------------------------------------------
	// Get subtypes of the named type.
	// AtomSpace.getSubTypes("Link")
	// AtomSpace.getSubTypes("Link", true)
	// AtomSpace.getSubTypes({ "type": "Link"})
	// AtomSpace.getSubTypes({ "type": "Link", "recursive": true})
	if (gtsub == act)
	{
		GET_TYPE;
		GET_BOOL;

		std::vector<Type> vect;
		if (recursive)
			nameserver().getChildrenRecursive(t, std::back_inserter(vect));
		else
			nameserver().getChildren(t, std::back_inserter(vect));
		RETURNSTR(Json::encode_type_list(vect));
	}

	// -----------------------------------------------
	// Get supertypes of the named type.
	// AtomSpace.getSuperTypes("ListLink")
	// AtomSpace.getSuperTypes("ListLink", true)
	// AtomSpace.getSuperTypes({ "type": "ListLink", "recursive": true})
	// AtomSpace.getSuperTypes({ "type": "ListLink"})
	if (gtsup == act)
	{
		GET_TYPE;
		GET_BOOL;

		std::vector<Type> vect;
		if (recursive)
			nameserver().getParentsRecursive(t, std::back_inserter(vect));
		else
			nameserver().getParents(t, std::back_inserter(vect));
		RETURNSTR(Json::encode_type_list(vect));
	}

	// -----------------------------------------------
	// AtomSpace.getAtoms("Node") // no subclassing
	// AtomSpace.getAtoms("Node", true)
	// AtomSpace.getAtoms({"type": "Node"}) // no sublassing
	// AtomSpace.getAtoms({"type": "Node", "subclass": true})
	if (gtatm == act)
	{
		GET_TYPE;
		GET_BOOL;

		std::string rv = "[\n";
		HandleSeq hset;
		as->get_handles_by_type(hset, t, recursive);
		bool first = true;
		for (const Handle& h: hset)
		{
			if (not first) { rv += ",\n"; } else { first = false; }
			rv += Json::encode_atom(h, "  ");
		}
		rv += "]";
		RETURNSTR(rv);
	}

	// -----------------------------------------------
	// AtomSpace.haveNode("Concept", "foo")
	// AtomSpace.haveNode({ "type": "Concept", "name": "foo"})
	if (haven == act)
	{
		CHK_FOR_JSON_ARG;
		if (is_json_object)
		{
			GET_ATOM("false")
			RETURN("true");
		}

		// Function argument format: type followed by name
		GET_TYPE;

		if (not nameserver().isA(t, NODE))
			return retmsgerr("Type is not a Node type: " + cmd.substr(epos));

		pos = cmd.find_first_not_of(",) \n\t", pos);
		std::string name = Json::get_node_name(cmd, pos, epos);
		Handle h = as->get_node(t, std::move(name));

		if (nullptr == h) RETURN("false");
		RETURN("true");
	}

	// -----------------------------------------------
	// AtomSpace.haveLink("List", [{ "type": "ConceptNode", "name": "foo"}])
	// AtomSpace.haveLink({ "type": "List", "outgoing": [{ "type": "ConceptNode", "name": "foo"}]})
	if (havel == act)
	{
		CHK_FOR_JSON_ARG;
		if (is_json_object)
		{
			GET_ATOM("false")
			RETURN("true");
		}

		// Function argument format: type followed by outgoing array
		GET_TYPE;

		if (not nameserver().isA(t, LINK))
			return retmsgerr("Type is not a Link type: " + cmd.substr(epos));

		pos = cmd.find_first_not_of(", \n\t", pos);
		// Skip the opening '[' if present
		if (pos < cmd.size() && cmd[pos] == '[') pos++;

		HandleSeq hs;

		size_t l = pos;
		size_t r = epos;
		while (std::string::npos != r)
		{
			l = cmd.find_first_not_of(" \n\t", l);
			if (l == std::string::npos || cmd[l] == ']') break;

			Handle ho = Json::decode_atom(cmd, l, r);
			if (nullptr == ho) RETURN("false");
			hs.push_back(ho);

			// Look for the comma
			l = cmd.find(",", r);
			if (std::string::npos == l) break;
			l ++;
			r = epos;
		}
		Handle h = as->get_link(t, std::move(hs));

		if (nullptr == h) RETURN("false");
		RETURN("true");
	}

	// -----------------------------------------------
	// AtomSpace.haveAtom({ "type": "ConceptNode", "name": "foo"})
	if (havea == act)
	{
		GET_ATOM("false");
		RETURN("true");
	}

	// -----------------------------------------------
	// AtomSpace.makeAtom({ "type": "ConceptNode", "name": "foo"})
	// AtomSpace.makeAtom(
	//    {"outgoing":
	//       [{"type":"Concept","name":"start"},
	//       {"type":"Concept","name":"finish"}],
	//    "type":"EdgeLink"})
	if (makea == act)
	{
		ADD_ATOM;
		RETURN("true");
	}

	// -----------------------------------------------
	// A list version of above.
	// AtomSpace.loadAtoms([{ "type": "ConceptNode", "name": "foo"},
	//                      { "type": "ConceptNode", "name": "oofdah"}])
	// AtomSpace.loadAtoms({"atoms":
	//                         [{ "type": "ConceptNode", "name": "foo"},
	//                         { "type": "ConceptNode", "name": "oofdah"}]})
	if (loada == act)
	{
		CHK_FOR_JSON_ARG;
		if (is_json_object)
		{
			pos = cmd.find("\"atoms\":", pos);
			if (std::string::npos == pos) RETURN("false");
			pos += 8; // 8 == strlen("\"atoms\":");
		}

		pos = cmd.find_first_not_of(" \n\t", pos);
		if ('[' != cmd[pos]) RETURN("false");
		pos++;
		while (epos != cmd.npos)
		{
			ADD_ATOM;
			pos = epos;

			// We expect a comma or a close-bracket.
			if  (cmd.npos == pos) RETURN("false");

			// Skip whitespace
			pos = cmd.find_first_not_of(" \n\t", pos);
			if  (cmd.npos == pos) RETURN("false");

			// If end of list, we are done.
			if (']' == cmd[pos]) break;

			// If not end of list, we expect a comma.
			if (',' != cmd[pos]) RETURN("false");
			pos++;
			epos = cmd.size();
		}
		RETURN("true");
	}

	// -----------------------------------------------
	// AtomSpace.getIncoming({ "type": "ConceptNode", "name": "foo"})
	if (gtinc == act)
	{
		GET_ATOM("[]");

		Type t = NOTYPE;
		pos = cmd.find(",", epos);
		if (std::string::npos != pos)
		{
			pos++;
			try {
				t = Json::decode_type(cmd, pos);
			}
			catch(...) {
				return retmsgerr("Unknown type: " + cmd.substr(pos));
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
		alist += "]";
		RETURNSTR(alist);
	}

	// -----------------------------------------------
	// AtomSpace.getValues({ "type": "ConceptNode", "name": "foo"})
	if (gtval == act)
	{
		GET_ATOM("[]");
		RETURNSTR(Json::encode_atom_values(h));
	}

	// -----------------------------------------------
	// AtomSpace.setValue({ "type": "ConceptNode", "name": "foo",
	//     "key": { "type": "PredicateNode", "name": "keewee" },
	//     "value": { "type": "FloatValue", "value": [1, 2, 3] } } )
	// If all is well, then the member fields can occur in arbitrary
	// order, so that the key can be given before the name, which can
	// be given before the type.
	if (stval == act)
	{
		size_t save_pos = pos;
		ADD_ATOM;
		epos = save_pos;
		GET_KEY;
		GET_VALUE;

		as->set_value(h, k, v);
		RETURN("true");
	}

	// -----------------------------------------------
	// AtomSpace.getTV({ "type": "ConceptNode", "name": "foo"})
	if (gettv == act)
	{
		GET_ATOM("[]");

		std::string alist = "[{ \"value\": \n";
		alist += Json::encode_value(ValueCast(h->getTruthValue()));
		alist += "}]";
		RETURNSTR(alist);
	}

	// -----------------------------------------------
	// AtomSpace.setTV({ "type": "ConceptNode", "name": "foo",
	//     "value": { "type": "SimpleTruthValue", "value": [0.2, 0.3] } } )
	if (settv == act)
	{
		ADD_ATOM;
		GET_VALUE;

		as->set_truthvalue(h, TruthValueCast(v));
		RETURN("true");
	}

	// -----------------------------------------------
	// AtomSpace.execute({ "type": "PlusLink", "outgoing":
	//     [{ "type": "NumberNode", "name": "2" },
	//      { "type": "NumberNode", "name": "2" }] })
	if (execu == act)
	{
		ADD_ATOM;

		ValuePtr vp = h->execute();
		RETURNSTR(Json::encode_value(vp));
	}

	// -----------------------------------------------
	// AtomSpace.extract({ "type": "Concept", "name": "foo"}, true)
	if (extra == act)
	{
		Handle h = Json::decode_atom(cmd, pos, epos);
		if (nullptr == h) RETURN("false");
		pos = epos;
		GET_BOOL;
		bool ok = as->extract_atom(h, recursive);
		if (ok) RETURN("true");
		RETURN("false");
	}

	// -----------------------------------------------
	return reterr(cmd);
}
