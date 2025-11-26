
# The storage_types.h file is written to the build directory
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR})

# -------------------------------
# File-specific code.

ADD_LIBRARY (fast_load_scm SHARED
	fast_load.cc
)

# Without this, parallel make will race and crap up the generated files.
ADD_DEPENDENCIES(fast_load_scm storage_types)

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

# Without this, parallel make will race and crap up the generated files.
ADD_DEPENDENCIES(persist-file storage_types)

TARGET_LINK_LIBRARIES(persist-file
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
