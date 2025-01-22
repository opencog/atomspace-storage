
# Generic Datalog/prolog decoding.
ADD_LIBRARY(datalog SHARED
	DecodeProlog.cc
	EncodeProlog.cc
)

TARGET_LINK_LIBRARIES(datalog
	atomspace
	execution
	atombase
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS datalog EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	Prolog.h
	DESTINATION "include/opencog/persist/prolog"
)

# -------------------------------
