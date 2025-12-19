
# Python bindings
IF (HAVE_CYTHON)
	ADD_SUBDIRECTORY (cython)
ENDIF (HAVE_CYTHON)

ADD_LIBRARY (persist SHARED
	BackingQuery.cc
	BackingStore.cc
	StorageNode.cc
)

TARGET_LINK_LIBRARIES(persist
	storage-types
	atomspace
	smob
)

ADD_GUILE_EXTENSION(SCM_CONFIG persist "opencog-ext-path-persist")

# Need to recompile persist.scm whenever storage_types.scm changes
# However, no matter what I do, I can't get the rebuild to trigger.
# In other words, the persist module is badly broken.
ADD_GUILE_MODULE (FILES
	opencog/persist.scm
	DEPENDS storage_types
#	COMPILE
)

INSTALL (TARGETS persist EXPORT AtomSpaceStorageTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	BackingStore.h
	StorageNode.h
	DESTINATION "include/opencog/persist/api"
)
