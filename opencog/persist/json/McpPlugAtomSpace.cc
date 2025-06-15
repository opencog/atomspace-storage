/*
 * McpPlugAtomSpace.cc
 *
 * MCP Plugin for AtomSpace operations
 * Copyright (c) 2025 Linas Vepstas
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

#include "McpPlugAtomSpace.h"
#include "JSCommands.h"

using namespace opencog;

// Helper to add a tool description
static void add_tool(std::string& json, const std::string& name, 
                     const std::string& description,
                     const std::string& schema,
                     bool last = false)
{
	json += "\t{\n";
	json += "\t\t\"name\": \"" + name + "\",\n";
	json += "\t\t\"description\": \"" + description + "\",\n";
	json += "\t\t\"inputSchema\": " + schema + "\n";
	json += "\t}";
	if (!last) json += ",";
	json += "\n";
}

// Tool descriptions for all AtomSpace operations
std::string McpPlugAtomSpace::get_tool_descriptions() const
{
	std::string json = "[\n";

	// version
	add_tool(json, "version", "Get the AtomSpace version string",
		"{\"type\": \"object\", \"properties\": {}, \"required\": []}");

	// getSubTypes
	add_tool(json, "getSubTypes", "Get all subtypes of a given atom type",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type to get subtypes of\"}, "
		"\"recursive\": {\"type\": \"boolean\", \"description\": \"Whether to get all subtypes recursively\"}}, "
		"\"required\": [\"type\"]}");

	// getSuperTypes
	add_tool(json, "getSuperTypes", "Get all supertypes of a given atom type",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type to get supertypes of\"}, "
		"\"recursive\": {\"type\": \"boolean\", \"description\": \"Whether to get all supertypes recursively\"}}, "
		"\"required\": [\"type\"]}");

	// getAtoms
	add_tool(json, "getAtoms", "Get all atoms of a specific type from the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type to retrieve\"}, "
		"\"subclass\": {\"type\": \"boolean\", \"description\": \"Whether to include atoms of subtypes\"}}, "
		"\"required\": [\"type\"]}");

	// haveNode
	add_tool(json, "haveNode", "Check if a node with the given type and name exists",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The node type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name of the node\"}}, "
		"\"required\": [\"type\", \"name\"]}");

	// haveLink
	add_tool(json, "haveLink", "Check if a link with the given type and outgoing set exists",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The link type\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"Array of atom specifications\"}}, "
		"\"required\": [\"type\", \"outgoing\"]}");

	// haveAtom
	add_tool(json, "haveAtom", "Check if an atom exists in the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}}, "
		"\"required\": [\"type\"]}");

	// makeAtom
	add_tool(json, "makeAtom", "Create an atom in the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type to create\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}}, "
		"\"required\": [\"type\"]}");

	// loadAtoms
	add_tool(json, "loadAtoms", "Create multiple atoms in the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"atoms\": {\"type\": \"array\", \"description\": \"Array of atom specifications to create\", "
		"\"items\": {\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\"}, \"name\": {\"type\": \"string\"}, "
		"\"outgoing\": {\"type\": \"array\"}}, \"required\": [\"type\"]}}}, "
		"\"required\": [\"atoms\"]}");

	// getIncoming
	add_tool(json, "getIncoming", "Get all links that contain a given atom in their outgoing set",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}}, "
		"\"required\": [\"type\"]}");

	// getTV
	add_tool(json, "getTV", "Get the truth value of an atom",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}}, "
		"\"required\": [\"type\"]}");

	// setTV
	add_tool(json, "setTV", "Set the truth value of an atom",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}, "
		"\"value\": {\"type\": \"object\", \"description\": \"The truth value to set\"}}, "
		"\"required\": [\"type\", \"value\"]}");

	// getValues
	add_tool(json, "getValues", "Get all values attached to an atom",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}}, "
		"\"required\": [\"type\"]}");

	// setValue
	add_tool(json, "setValue", "Set a value on an atom with a given key",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}, "
		"\"key\": {\"type\": \"object\", \"description\": \"The key atom\"}, "
		"\"value\": {\"type\": \"object\", \"description\": \"The value to set\"}}, "
		"\"required\": [\"type\", \"key\", \"value\"]}");

	// execute
	add_tool(json, "execute", "Execute an executable atom and get the result",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The executable atom type\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set\"}}, "
		"\"required\": [\"type\"]}");

	// extract
	add_tool(json, "extract", "Remove an atom from the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type\"}, "
		"\"name\": {\"type\": \"string\", \"description\": \"The name (for nodes)\"}, "
		"\"outgoing\": {\"type\": \"array\", \"description\": \"The outgoing set (for links)\"}, "
		"\"recursive\": {\"type\": \"boolean\", \"description\": \"Whether to recursively remove\"}}, "
		"\"required\": [\"type\"]}", true);  // last = true

	json += "]";
	return json;
}

std::string McpPlugAtomSpace::invoke_tool(const std::string& tool_name,
                                          const std::string& arguments) const
{
	// Construct the MCP command format that JSCommands expects
	std::string mcp_command = "{ \"tool\": \"" + tool_name + "\", \"params\": " + arguments + "}";

	// Use JSCommands to process the command
	return JSCommands::interpret_command(_as, mcp_command);
}
