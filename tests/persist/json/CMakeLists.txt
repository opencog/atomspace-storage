# JSON command tests

LINK_LIBRARIES(
	json
	atomspace
	execution
)

ADD_CXXTEST(JSCommandsUTest)
ADD_CXXTEST(MCPCommandsUTest)
ADD_CXXTEST(SexTunnelUTest)
