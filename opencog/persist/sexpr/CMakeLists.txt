
# Generic S-expression decoding.
ADD_LIBRARY (sexpr SHARED
	AtomSexpr.cc
	FrameSexpr.cc
	ValueSexpr.cc
)

TARGET_LINK_LIBRARIES(sexpr
	atomspace
	atombase
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS sexpr EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	Sexpr.h
	DESTINATION "include/opencog/persist/sexpr"
)

# -------------------------------
