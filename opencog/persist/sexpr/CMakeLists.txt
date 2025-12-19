
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
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	Sexpr.h
	DESTINATION "include/opencog/persist/sexpr"
)

# -------------------------------
