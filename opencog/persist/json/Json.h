/*
 * Json.h
 * Encoding and Decoding of Atomese in JSON format.
 *
 * Copyright (C) 2020, 2021 Linas Vepstas
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

#ifndef _JSON_ECODE_H
#define _JSON_ECODE_H

#include <string>
#include <opencog/atoms/base/Handle.h>

namespace opencog
{
/** \addtogroup grp_persist
 *  @{
 */

class Json
{
public:
	/// Decode the JSON containing an atom, starting at
	/// location `pos`. Return the Atom, and update `pos` to point
	/// just past the end of the trailing brace.
	static Handle decode_atom(const std::string& s, size_t& pos)
	{
		size_t start = pos;
		size_t end = s.length();
		return decode_atom(s, start, end);
	}

	static Handle decode_atom(const std::string& s) {
		size_t junk = 0;
		return decode_atom(s, junk);
	}

	static Handle decode_atom(const std::string& s,
                             size_t& l, size_t& r);

	static std::string get_node_name(const std::string&, size_t& l, size_t& r);

#if NOT_IMPLEMENTED_YET
	static ValuePtr decode_value(const std::string&, size_t&);
#endif // NOT_IMPLEMENTED_YET
	static Type decode_type(const std::string& s, size_t& pos);

#if NOT_IMPLEMENTED_YET
	static void decode_slist(const Handle&, const std::string&, size_t&);
	static void decode_alist(const Handle&, const std::string&, size_t&);
	static void decode_alist(const Handle& h, const std::string& s) {
		size_t junk = 0;
		decode_alist(h, s, junk);
	}
#endif // NOT_IMPLEMENTED_YET

	// -------------------------------------------
	// Encoding functions
	static std::string encode_atom(const Handle&, const std::string& = "");
	static std::string encode_value(const ValuePtr&, const std::string& = "");
	static std::string encode_atom_values(const Handle&);

	static std::string dump_atom(const Handle&);
	static std::string dump_vatom(const Handle&, const Handle&);
};

/** @}*/
} // namespace opencog

#endif // _JSON_ECODE_H
