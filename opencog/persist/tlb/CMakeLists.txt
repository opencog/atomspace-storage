
ADD_LIBRARY (tlb
	TLB.cc
)

TARGET_LINK_LIBRARIES(tlb
	atombase
	atomspace
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS tlb EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	TLB.h
	DESTINATION "include/opencog/persist/tlb"
)
