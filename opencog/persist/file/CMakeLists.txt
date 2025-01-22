
# -------------------------------
# File-specific code.

ADD_LIBRARY (fast_load_scm SHARED
	fast_load.cc
)

TARGET_LINK_LIBRARIES(fast_load_scm
	sexcom
	sexpr
)

INSTALL (TARGETS fast_load_scm EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	fast_load.h
	DESTINATION "include/opencog/persist/file"
)

# -------------------------------

ADD_LIBRARY (persist-file SHARED
	FileStorage.cc
	PersistFileSCM.cc
)

TARGET_LINK_LIBRARIES(persist-file
	persist
	storage-types
	fast_load_scm
	sexcom
	sexpr
	atomspace
	smob
)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-file "opencog-ext-path-persist-file")

ADD_GUILE_MODULE (FILES
   opencog/persist-file.scm
   DEPENDS persist-file
#  COMPILE
)

INSTALL (TARGETS persist-file EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	FileStorage.h
	DESTINATION "include/opencog/persist/file"
)
