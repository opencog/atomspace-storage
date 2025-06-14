
# Generic JSON decoding.
ADD_LIBRARY (json SHARED
	DecodeJson.cc
	EncodeJson.cc
	JSCommands.cc
	JsonEval.cc
	McpPlugAtomSpace.cc
)

TARGET_LINK_LIBRARIES(json
	atomspace
	execution
	atombase
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS json EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	JSCommands.h
	JsonEval.h
	Json.h
	McpPlugin.h
	McpPlugAtomSpace.h
	DESTINATION "include/opencog/persist/json"
)

# -------------------------------
