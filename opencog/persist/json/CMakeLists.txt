
# Generic JSON decoding.
ADD_LIBRARY (json SHARED
	DecodeJson.cc
	EncodeJson.cc
	JSCommands.cc
	JsonEval.cc
)

TARGET_LINK_LIBRARIES(json
	sexpr
	atomspace
	execution
	atombase
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS json EXPORT AtomSpaceStorageTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	JSCommands.h
	JsonEval.h
	Json.h
	DESTINATION "include/opencog/persist/json"
)

# -------------------------------
