
# The storage_types.h file is written to the build directory
INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR})

ADD_LIBRARY (persist-flow SHARED
	FetchValueOfLink.cc
	StoreValueOfLink.cc
)

# Without this, parallel make will race and crap up the generated files.
ADD_DEPENDENCIES(persist-flow storage_types)

TARGET_LINK_LIBRARIES(persist-flow
	persist
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARY}
)

# Arghhhh. I hate this stuff. This sets up a file-load path that the
# `(opencog persist)` module will use.  There is no `(opencog persist-flow)`
# module.
ADD_GUILE_EXTENSION(SCM_CONFIG persist-flow "opencog-ext-path-persist-flow")

INSTALL (TARGETS persist-flow EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	FetchValueOfLink.h
	StoreValueOfLink.h
	DESTINATION "include/opencog/persist/flow"
)
