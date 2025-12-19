
ADD_LIBRARY (tlb SHARED
	TLB.cc
)

TARGET_LINK_LIBRARIES(tlb
	atombase
	atomspace
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS tlb EXPORT AtomSpaceStorageTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	TLB.h
	DESTINATION "include/opencog/persist/tlb"
)

# --------------------------------

ADD_LIBRARY (guile-uuid UuidSCM.cc)

TARGET_LINK_LIBRARIES(guile-uuid SHARED
	tlb
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARY}
)
ADD_GUILE_EXTENSION(SCM_CONFIG guile-uuid "opencog-ext-path-uuid")

ADD_GUILE_MODULE (FILES
	opencog/uuid.scm
	DEPENDS guile-uuid
#  COMPILE
)

INSTALL (TARGETS guile-uuid
   EXPORT AtomSpaceStorageTargets
   DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

# --------------------------------
