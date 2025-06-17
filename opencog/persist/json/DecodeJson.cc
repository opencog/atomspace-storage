/*
 * DecodeJson.cc
 * Decode JSON describing Atoms and Values.
 *
 * Copyright (c) 2019, 2022 Linas Vepstas <linas@linas.org>
 * SPDX-License-Identifier: AGPL-3.0-or-later
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
#include <iomanip>

#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/atoms/value/LinkValue.h>
#include <opencog/atoms/value/StringValue.h>
#include <opencog/atoms/value/ValueFactory.h>
#include <opencog/atomspace/AtomSpace.h>

#include "Json.h"

using namespace opencog;

// This does NOT use any external JSON decoder libraries because those
// libs don't really make anything simpler, and also we don't need most
// of the features that they offer, and also I don't want more
// dependencies in the AtomSpace.

/* ================================================================== */
/**
 * Look for a type name of the form "ConceptNode" (with quotes)
 * starting at location `pos` in `tna`.
 * Return the type and update `pos` to point after the typename.
 */
Type Json::decode_type(const std::string& tna, size_t& pos)
{
	// Advance past whitespace.
	pos = tna.find_first_not_of(" \n\t", pos);
	if (std::string::npos == pos)
		throw SyntaxException(TRACE_INFO, "Bad Type >>%s<<",
			tna.substr(pos).c_str());

	// Check if we have a quoted string
	if ('"' == tna[pos]) {
		pos++; // skip opening quote
		size_t endquote = tna.find('"', pos);
		if (std::string::npos == endquote)
			throw SyntaxException(TRACE_INFO, "Unterminated quoted type >>%s<<",
				tna.substr(pos).c_str());

		Type t = nameserver().getType(tna.substr(pos, endquote-pos));
		if (NOTYPE == t)
			throw SyntaxException(TRACE_INFO, "Unknown Type >>%s<<",
				tna.substr(pos, endquote-pos).c_str());

		pos = endquote + 1; // skip closing quote
		return t;
	}

	// Unquoted string - advance to next whitespace
	size_t nos = tna.find_first_of(",) \n\t", pos);
	if (std::string::npos == nos)
		nos = tna.size();

	Type t = nameserver().getType(tna.substr(pos, nos-pos));
	if (NOTYPE == t)
		throw SyntaxException(TRACE_INFO, "Unknown Type >>%s<<",
			tna.substr(pos, nos-pos).c_str());

	pos = nos;
	return t;
}

/* ================================================================== */

/**
 * Decode a type argument that can be either a simple string like
 * "ConceptNode" or a JSON object like {"type": "ConceptNode"}.
 * It might also be a field in a JSON object:
 *   {"other": "stuff", "type": "ConceptNode", "more": "stuff"}
 */
Type Json::decode_type_arg(const std::string& tna, size_t& pos)
{
	// Advance past whitespace.
	pos = tna.find_first_not_of(" \n\t", pos);
	if (std::string::npos == pos)
		throw SyntaxException(TRACE_INFO, "Bad Type - empty string");

	// Check if this is a JSON object format
	if ('{' == tna[pos])
	{
		// Save the start position of the JSON object
		size_t obj_start = pos;

		// Look for "type": field
		size_t tpos = tna.find("\"type\":", pos);
		if (std::string::npos == tpos)
			throw SyntaxException(TRACE_INFO, "Missing type field in JSON object");

		tpos += 7; // skip past "type":

		// Call the original decode_type to parse the actual type name
		Type t = decode_type(tna, tpos);

		// For MCP format, we're looking at a params object that might have
		// other fields before "type". We need to find the closing brace
		// of the entire params object, not just the first closing brace.
		// We'll count braces but also handle string literals properly.
		size_t scan_pos = obj_start;
		int brace_count = 0;
		size_t close = std::string::npos;
		bool in_string = false;
		bool escape_next = false;

		while (scan_pos < tna.size()) {
			char ch = tna[scan_pos];

			if (escape_next) {
				escape_next = false;
			} else if (ch == '\\' && in_string) {
				escape_next = true;
			} else if (ch == '"') {
				in_string = !in_string;
			} else if (!in_string) {
				if (ch == '{') {
					brace_count++;
				} else if (ch == '}') {
					brace_count--;
					if (brace_count == 0) {
						close = scan_pos;
						break;
					}
				}
			}
			scan_pos++;
		}

		if (std::string::npos == close)
			throw SyntaxException(TRACE_INFO, "Missing closing brace >>%s<<",
				tna.substr(obj_start).c_str());

		// Update pos to point after the closing brace.
		pos = close + 1;

		return t;
	}
	else
	{
		// Simple string format - just call decode_type directly
		return decode_type(tna, pos);
	}
}

/* ================================================================== */

/// Extracts Node name-string. Given the string `s`, this updates
/// the `l` and `r` values such that `l` points at the first
/// non-whitespace character of the name, and `r` points at the last.
/// The string is considered to start *after* the first quote, and ends
/// just before the last quote. In this case, escaped quotes \" are
/// ignored (are considered to be part of the string).
///
/// This returns the unescaped node name.
///
std::string Json::get_node_name(const std::string& s,
                                size_t& l, size_t& r)
{
	// Advance past whitespace.
	while (l < r and (s[l] == ' ' or s[l] == '\t' or s[l] == '\n')) l++;

	l++;
	size_t p = l;
	for (; p < r and (s[p] != '"' or ((0 < p) and (s[p - 1] == '\\'))); p++);
	r = p;

	// We use std::quoted() to unescape embedded quotes.
	// Unescaping works ONLY if the leading character is a quote!
	// So readjust left and right to pick those up.
	if ('"' == s[l-1]) l--; // grab leading quote, for std::quoted().
	if ('"' == s[r]) r++;   // step past trailing quote.
	std::stringstream ss;
	std::string name;
	ss << s.substr(l, r-l);
	ss >> std::quoted(name);
	return name;
}

/* ================================================================== */

/**
 * Get a node name argument that can be either a direct string
 * like "foo" or from a JSON object like {"name": "foo"}.
 */
std::string Json::get_node_name_arg(const std::string& s, size_t& pos, size_t& r)
{
	// Advance past whitespace.
	pos = s.find_first_not_of(" \n\t", pos);
	if (std::string::npos == pos)
		throw SyntaxException(TRACE_INFO, "Bad node name - empty string");

	// Check if this is a JSON object format
	if ('{' == s[pos])
	{
		// Look for "name": field
		size_t npos = s.find("\"name\":", pos);
		if (std::string::npos == npos)
			throw SyntaxException(TRACE_INFO, "Missing name field in JSON object");

		npos += 7; // skip past "name":

		// Find the closing brace to use as boundary
		size_t close = s.find('}', npos);
		if (std::string::npos == close)
			throw SyntaxException(TRACE_INFO, "Missing closing brace >>%s<<",
				s.substr(npos).c_str());

		// Call get_node_name to extract the actual name
		std::string name = get_node_name(s, npos, close);

		// Update pos to point after the closing brace
		pos = close + 1;

		return name;
	}
	else
	{
		// Simple string format - just call get_node_name directly
		return get_node_name(s, pos, r);
	}
}

/* ================================================================== */

/// Helper function to find the closing delimiter of a JSON object or array
/// For objects (starting with '{'), returns the position after the closing '}'
/// For arrays (starting with '['), returns the position after the closing ']'
/// Returns string::npos if not found or if start doesn't begin with '{' or '['
static size_t find_json_end(const std::string& s, size_t start)
{
	if (start >= s.size()) return std::string::npos;

	char open_char = s[start];

	if (open_char != '{' && open_char != '[') {
		return std::string::npos;
	}

	int depth = 1;
	size_t pos = start + 1;
	bool in_string = false;
	bool escape_next = false;

	while (pos < s.size() && depth > 0) {
		char ch = s[pos];
		if (escape_next) {
			escape_next = false;
		} else if (ch == '\\' && in_string) {
			escape_next = true;
		} else if (ch == '"') {
			in_string = !in_string;
		} else if (!in_string) {
			if (ch == open_char) {
				// Only count the same type of delimiter we're looking for
				depth++;
			} else if ((open_char == '{' && ch == '}') ||
			           (open_char == '[' && ch == ']')) {
				depth--;
				if (depth == 0) {
					// Found the matching closing delimiter
					return pos + 1; // Return position after the delimiter
				}
			}
		}
		pos++;
	}

	return (depth == 0) ? pos : std::string::npos;
}

/// Helper function to find a top-level field in a JSON object
/// Returns the position after the colon, or string::npos if not found
static size_t find_top_level_field(const std::string& s,
                                   const std::string& field_name,
                                   size_t obj_start, size_t obj_end)
{
	std::string search_pattern = "\"" + field_name + "\":";
	size_t search_pos = obj_start + 1;
	int depth = 0;
	bool in_string = false;
	bool escape_next = false;

	while (search_pos < obj_end) {
		char ch = s[search_pos];
		if (escape_next) {
			escape_next = false;
		} else if (ch == '\\' && in_string) {
			escape_next = true;
		} else if (ch == '"') {
			in_string = !in_string;
			// When we exit a string at depth 0, check if it's our field
			if (!in_string && depth == 0) {
				// We just exited a string, check if it matches our pattern
				// Make sure we have enough characters before search_pos
				// We need at least field_name.length() + 2 chars (for quotes) before search_pos
				if (search_pos >= field_name.length() + 2) {
					size_t check_start = search_pos - field_name.length() - 1;
					// Additional bounds check
					if (check_start >= obj_start &&
					    check_start + search_pattern.length() <= s.length() &&
					    s.compare(check_start, search_pattern.length(), search_pattern) == 0) {
						return check_start + search_pattern.length();
					}
				}
			}
		} else if (!in_string) {
			if (ch == '{' || ch == '[') {
				depth++;
			} else if (ch == '}' || ch == ']') {
				depth--;
			}
		}
		search_pos++;
	}

	return std::string::npos;
}

/* ================================================================== */

/// Convert an Atomese JSON expression into a C++ Atom.
/// For example: `{ "type": "Concept", "name": "foo" }`
/// will return the corresponding atom.
/// Reversed arguments `{ "name": "foo", "type": "Concept" }`
/// should also work.
///
/// The string to decode is `s`, beginning at location `l` and using `r`
/// as a hint for the end of the expression.
///
Handle Json::decode_atom(const std::string& s,
                         size_t& l, size_t& r)
{
	// Find the start of the JSON object
	l = s.find_first_not_of(" \n\t", l);
	if (std::string::npos == l || s[l] != '{') {
		return Handle::UNDEFINED;
	}

	// Find the end of this JSON object
	size_t obj_start = l;
	size_t obj_end = find_json_end(s, obj_start);
	if (std::string::npos == obj_end) return Handle::UNDEFINED;

	// Find the "type" field at the top level
	size_t tpos = find_top_level_field(s, "type", obj_start, obj_end);
	if (std::string::npos == tpos) return Handle::UNDEFINED;

	Type t = NOTYPE;
	try {
		t = Json::decode_type(s, tpos);
	}
	catch(...) {
		return Handle::UNDEFINED;
	}

	if (nameserver().isA(t, NODE))
	{
		// Find the "name" field at the top level
		size_t apos = find_top_level_field(s, "name", obj_start, obj_end);
		if (std::string::npos == apos) return Handle::UNDEFINED;

		apos = s.find_first_not_of(" \n\t", apos);
		size_t name_end = obj_end;
		std::string name = Json::get_node_name(s, apos, name_end);

		// Update r to point after the closing brace of the JSON object
		r = obj_end;

		return createNode(t, std::move(name));
	}

	if (nameserver().isA(t, LINK))
	{
		// Find the "outgoing" field at the top level
		size_t opos = find_top_level_field(s, "outgoing", obj_start, obj_end);
		if (std::string::npos == opos) return Handle::UNDEFINED;

		// Find the opening bracket of the outgoing array
		l = s.find("[", opos);
		if (std::string::npos == l) return Handle::UNDEFINED;

		// Find the closing bracket using our helper function
		size_t array_end = find_json_end(s, l);
		if (std::string::npos == array_end) return Handle::UNDEFINED;

		l++; // Move past the '['
		size_t close_bracket = array_end - 1; // Position of the ']'

		HandleSeq hs;

		// Process each atom in the outgoing array
		while (l < close_bracket)
		{
			// Skip whitespace and commas
			l = s.find_first_not_of(" \n\t,", l);
			if (l >= close_bracket || std::string::npos == l) break;

			// Check if we've reached the end of the array
			if (s[l] == ']') break;

			size_t atom_end = close_bracket;
			Handle ho = Json::decode_atom(s, l, atom_end);
			if (nullptr == ho) return Handle::UNDEFINED;
			hs.push_back(ho);

			// Update position for next iteration
			l = atom_end;
		}

		// Update r to point after the closing brace of the JSON object
		r = obj_end;

		return createLink(std::move(hs), t);
	}

	return Handle::UNDEFINED;
}

/* ================================================================== */

#define GET_VALUE_POS \
	size_t opos = s.find("\"value\":", l); \
	if (std::string::npos != opos) \
		opos += 8;  /* skip past "value": */ \
	else { \
		opos = s.find("\"values\":", l); \
		if (std::string::npos == opos) return nullptr; \
		opos += 9;  /* skip past "values": */ \
	}

/// Convert an Atomese JSON expression into a C++ Value.
/// For example: `{ "type": "FloatValue", "value": [1, 2, 3] }`
/// will return the corresponding ValuePtr.
///
/// The string to decode is `s`, beginning at location `l` and using `r`
/// as a hint for the end of the expression.
///
ValuePtr Json::decode_value(const std::string& s,
                            size_t& lo, size_t& ro)
{
	size_t l = lo;
	l = s.find("{", l);
	if (std::string::npos == l) return nullptr;

	size_t tpos = s.find("\"type\":", l);
	if (std::string::npos == tpos) return nullptr;
	tpos += 7;  // skip past "type":

	Type t = NOTYPE;
	try {
		t = Json::decode_type(s, tpos);
	}
	catch(...) {
		return nullptr;
	}

	if (nameserver().isA(t, ATOM))
	{
		return decode_atom(s, lo, ro);
	}

	if (nameserver().isA(t, FLOAT_VALUE))
	{
		l = tpos;
		GET_VALUE_POS;
		l = s.find("[", opos);

		size_t r = ro;
		std::vector<double> vd;
		while (std::string::npos != l)
		{
			l++;
			r = s.find_first_of(",]", l);
			if (std::string::npos == r) break;
			std::stringstream ss;
			ss << s.substr(l, r-l);
			double d;
			ss >> d;
			vd.push_back(d);

			if (']' == s[r]) break;
			l = r;
		}

		r = s.find("}", r);
		ro = r;
		return valueserver().create(t, std::move(vd));
	}

	if (nameserver().isA(t, STRING_VALUE))
	{
		l = tpos;
		GET_VALUE_POS;
		l = s.find("[", opos);

		size_t r = ro;
		std::vector<std::string> vs;
		while (std::string::npos != l)
		{
			l++;
			std::stringstream ss;
			ss << s.substr(l);
			std::string uq;
			ss >> std::quoted(uq);
			vs.push_back(uq);

			// Step past the closing quote.
			r = l + uq.size() + 2;

			// Get to the next elt
			r = s.find_first_of(",]", r);
			if (std::string::npos == r) break;
			if (']' == s[r]) break;
			l = r;
		}

		r = s.find("}", r);
		ro = r;
		return valueserver().create(t, std::move(vs));
	}

	if (nameserver().isA(t, LINK_VALUE))
	{
		l = tpos;
		GET_VALUE_POS;
		l = s.find("[", opos);

		size_t r = ro;
		std::vector<ValuePtr> vv;
		while (std::string::npos != l)
		{
			l++;
			ValuePtr vp = decode_value(s, l, r);
			vv.push_back(vp);

			// Get to the next elt
			r = s.find_first_of(",]", r);
			if (std::string::npos == r) break;
			if (']' == s[r]) break;
			l = r;
		}

		r = s.find("}", r);
		ro = r;
		return valueserver().create(t, std::move(vv));
	}

	return nullptr;
}

/* ============================= END OF FILE ================= */
