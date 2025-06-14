# JSON command tests

LINK_LIBRARIES(
	json
	persist
	atomspace
	execution
)

ADD_CXXTEST(JSCommandsUTest)
ADD_CXXTEST(MCPCommandsUTest)
