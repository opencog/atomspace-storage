
# Generic Comma-Seperated Value (CSV) decoding.
# Also TSV (tab-seperated)
ADD_LIBRARY (csv SHARED
	table_read.cc
)

TARGET_LINK_LIBRARIES(csv
	atomspace
	atombase
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS csv EXPORT AtomSpaceStorageTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	table_read.h
	DESTINATION "include/opencog/persist/csv"
)

# -------------------------------

ADD_LIBRARY (csv-table SHARED
   TableSCM.cc
)

TARGET_LINK_LIBRARIES(csv-table
	csv
	atomspace
	smob
)

ADD_GUILE_EXTENSION(SCM_CONFIG csv-table "opencog-ext-path-csv-table")

INSTALL (TARGETS csv-table EXPORT AtomSpaceStorageTargets
   DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

# -------------------------------

ADD_GUILE_MODULE (FILES
   opencog/csv-table.scm
#  COMPILE
)

# -------------------------------
