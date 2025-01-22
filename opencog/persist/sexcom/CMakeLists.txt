
# S-expression command dispatcher
ADD_LIBRARY (sexcom SHARED
	Commands.cc
	Dispatcher.cc
	SexprEval.cc
)

ADD_DEPENDENCIES(sexcom storage_types)

TARGET_LINK_LIBRARIES(sexcom
	storage-types
	persist-proxy
	persist
	atomspace
	execution
	atombase
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS sexcom EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	Commands.h
	Dispatcher.h
	SexprEval.h
	DESTINATION "include/opencog/persist/sexcom"
)

# -------------------------------
