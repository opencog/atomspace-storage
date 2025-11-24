/*
 * AtomSexpr.cc
 * Decode Atomese written in s-expression format.
 *
 * Copyright (C) 2020 Alexey Potapov, Anatoly Belikov
 *
 * Authors: Alexey Potapov
 *          Anatoly Belikov
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
#include <stdexcept>
#include <string>

#include <opencog/util/exceptions.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/core/NumberNode.h>
#include <opencog/atomspace/AtomSpace.h>

#include "Sexpr.h"

using namespace opencog;

/// Extract s-expression. Given a string `s`, update the `l` and `r`
/// values so that `l` points at the next open-parenthesis (left paren)
/// and `r` points at the matching close-paren.  Returns parenthesis
/// count. If zero, the parens match. If non-zero, then `r` points
/// at the first non-valid character in the string (e.g. comment char).
int Sexpr::get_next_expr(const std::string& s, size_t& l, size_t& r,
                         size_t line_cnt)
{
	// Advance past whitespace.
	// Would be more efficient to say l = s.find_first_not_of(" \t\n", l);
	while (l < r and (s[l] == ' ' or s[l] == '\t' or s[l] == '\n')) l++;
	if (l == r) return 0;

	// Ignore comment lines.
	if (s[l] == ';') { l = r; return 1; }

	if (s[l] != '(')
		throw SyntaxException(TRACE_INFO,
			"Syntax error at line %lu Unexpected text: >>%s<<",
			line_cnt, s.substr(l).c_str());

	size_t p = l;
	int count = 1;
	bool quoted = false;
	do {
		p++;

		// Skip over any escapes
		if (s[p] == '\\') { p ++; continue; }

		if (s[p] == '"') quoted = !quoted;
		else if (quoted) continue;
		else if (s[p] == '(') count++;
		else if (s[p] == ')') count--;
		else if (s[p] == ';') break;      // comments!
	} while (p < r and count > 0);

	r = p;
	return count;
}

static NameServer& namer = nameserver();

/// Extracts link or node type. Given the string `s`, this updates
/// the `l` and `r` values such that `l` points at the first
/// non-whitespace character of the name, and `r` points at the last.
static Type get_typename(const std::string& s, size_t& l, size_t& r,
                         size_t line_cnt)
{
	// Advance past whitespace.
	l = s.find_first_not_of(" \t\n", l);

	if (s[l] != '(')
		throw SyntaxException(TRACE_INFO,
			"Error at line %lu unexpected content: >>%s<< in %s",
			line_cnt, s.substr(l, r-l+1).c_str(), s.c_str());

	// Advance until whitespace or closing paren.
	l++;
	r = s.find_first_of("() \t\n", l);

	const std::string stype = s.substr(l, r-l);
	Type atype = namer.getType(stype);
	if (atype == opencog::NOTYPE)
	{
		if (0 < line_cnt)
			throw SyntaxException(TRACE_INFO,
				"Error at line %lu unknown Atom type: %s",
				line_cnt, stype.c_str());
		throw SyntaxException(TRACE_INFO,
			"Unknown Atom type: %s in expression %s",
			stype.c_str(), s.c_str());
	}

	return atype;
}

/// Extracts Node name-string. Given the string `s`, this updates
/// the `l` and `r` values such that `l` points at the first
/// non-whitespace character of the name, and `r` points at the last.
/// The string is considered to start *after* the first quote, and ends
/// just before the last quote. In this case, escaped quotes \" are
/// ignored (are considered to be part of the string).
///
/// If the node is a Type node, then `l` points at the first
/// non-whitespace character of the type name and `r` points to the next
/// opening parenthesis.
///
/// This function was originally written to allow in-place extraction
/// of the node name. Unfortunately, node names containing escaped
/// quotes need to be unescaped, which prevents in-place extraction.
/// So, instead, this returns a copy of the name string.
std::string Sexpr::get_node_name(const std::string& s,
                                 size_t& l, size_t& r,
                                 Type atype, size_t line_cnt)
{
	// Advance past whitespace.
	while (l < r and (s[l] == ' ' or s[l] == '\t' or s[l] == '\n')) l++;

	// Check if we have content
	if (l >= r)
		throw SyntaxException(TRACE_INFO,
			"Error at line %zu: empty node name",
			line_cnt);

	bool typeNode = namer.isA(atype, TYPE_NODE);
	bool numberNode = namer.isA(atype, NUMBER_NODE);

	// Detect if this is a quoted value or not
	bool quoted_value = (s[l] == '"');
	bool scm_symbol = false;

	// Scheme strings start and end with double-quote.
	// Scheme symbols start with single-quote.
	// NumberNode allows unquoted numeric values.
	if (typeNode and s[l] == '\'')
		scm_symbol = true;
	else if (not typeNode and not numberNode and not quoted_value)
		throw SyntaxException(TRACE_INFO,
			"Syntax error at line %zu Unexpected content: >>%s<< in %s",
			line_cnt, s.substr(l, r-l+1).c_str(), s.c_str());

	// Skip opening quote or symbol marker
	if (quoted_value or scm_symbol)
		l++;

	size_t p = l;
	if (scm_symbol)
		for (; p < r and (s[p] != '(' or s[p] != ' ' or s[p] != '\t' or s[p] != '\n'); p++);
	else if (numberNode and not quoted_value)
		// For unquoted NumberNode: extract until whitespace or closing paren
		for (; p < r and s[p] != ')' and s[p] != ' ' and s[p] != '\t' and s[p] != '\n'; p++);
	else
		for (; p < r and (s[p] != '"' or ((0 < p) and (s[p - 1] == '\\'))); p++);
	r = p;

	// We use std::quoted() to unescape embedded quotes.
	// Unescaping works ONLY if the leading character is a quote!
	// So readjust left and right to pick those up.
	if (quoted_value) l--; // grab leading quote, for std::quoted().
	if (quoted_value and '"' == s[r]) r++;   // step past trailing quote.

	// Validate extraction bounds
	if (r < l)
		throw SyntaxException(TRACE_INFO,
			"Error at line %zu: invalid node name bounds",
			line_cnt);

	std::stringstream ss;
	std::string name;
	if (numberNode and not quoted_value)
	{
		// For unquoted NumberNode, directly extract the numeric string
		name = s.substr(l, r-l);
	}
	else
	{
		// For quoted strings, use std::quoted to unescape
		ss << s.substr(l, r-l);
		ss >> std::quoted(name);
	}
	return name;
}

/// Convert an Atomese S-expression into a C++ Atom.
/// For example: `(Concept "foobar")`  or
/// `(Evaluation (Predicate "blort") (List (Concept "foo") (Concept "bar")))`
/// will return the corresponding atoms.
///
/// The string to decode is `s`, beginning at location `l` and using `r`
/// as a hint for the end of the expression. The `line_count` is an
/// optional argument for printing file line-numbers, in case of error.
///
Handle Sexpr::decode_atom(const std::string& s,
                          size_t l, size_t r, size_t line_cnt,
                          std::unordered_map<std::string, Handle>& ascache)
{
	size_t l1 = l, r1 = r;
	Type atype = get_typename(s, l1, r1, line_cnt);

	l = r1;
	if (namer.isLink(atype))
	{
		AtomSpace* as = nullptr;
		HandleSeq outgoing;
		do {
			l1 = l;
			r1 = r;
			get_next_expr(s, l1, r1, line_cnt);
			if (l1 == r1) break;

			// Atom names never start with lower-case.
			if (islower(s[l1+1])) break;

			outgoing.push_back(decode_atom(s, l1, r1, line_cnt, ascache));

			l = r1 + 1;
		} while (l < r);

		Handle h(createLink(std::move(outgoing), atype));
		if (as) h = as->add_atom(h);

		// alist's occur at the end of the sexpr.
		if (l1 != r1 and l < r)
			decode_slist(h, s, l1);

		return h;
	}
	else
	if (namer.isNode(atype))
	{
		l1 = l;
		r1 = r;
		const std::string name = get_node_name(s, l1, r1, atype, line_cnt);

		Handle h(createNode(atype, std::move(name)));

		size_t l2 = r1;
		size_t r2 = r;
		get_next_expr(s, l2, r2, line_cnt);
		if (l2 < r2)
		{
			AtomSpace* as = nullptr;
			if (0 == s.compare(l2, 11, "(AtomSpace "))
			{
				Handle hasp(decode_frame(Handle::UNDEFINED, s, l2, ascache));
				as = (AtomSpace*) hasp.get();
				h = as->add_atom(h);
			}
			if (l2 < r2)
			{
				if (0 == s.compare(l2, 7, "(alist "))
					decode_slist(h, s, l2);
			}
		}

		return h;
	}
	if (namer.isA(atype, ATOM_SPACE))
	{
		// Get the AtomSpace name
		l1 = l;
		r1 = r;
		const std::string name = get_node_name(s, l1, r1, atype, line_cnt);

		AtomSpacePtr asp(createAtomSpace());
		asp->set_name(name);
		Handle h(asp);
		return h;
	}
	throw SyntaxException(TRACE_INFO,
		"Syntax error at line %zu unknown Atom type %d >>%s<< in %s",
		line_cnt, atype, s.substr(l1, r1-l1).c_str(), s.c_str());
}
