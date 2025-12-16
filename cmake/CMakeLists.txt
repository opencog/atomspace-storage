
# CMake boilerplate
#    find_package(AtomspaceStorage REQUIRED 4.3.0)
# and have it work.

include(CMakePackageConfigHelpers)

set(ConfigPackageLocation lib/cmake/AtomSpaceStorage)

install(EXPORT AtomSpaceStorageTargets
	FILE AtomSpaceStorageTargets.cmake
	DESTINATION ${ConfigPackageLocation}
)

configure_package_config_file(AtomSpaceStorageConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/AtomSpaceStorageConfig.cmake
	INSTALL_DESTINATION ${ConfigPackageLocation}
	PATH_VARS CMAKE_INSTALL_PREFIX
)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/AtomSpaceStorageConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

INSTALL(FILES
	${CMAKE_CURRENT_BINARY_DIR}/AtomSpaceStorageConfigVersion.cmake
	${CMAKE_CURRENT_BINARY_DIR}/AtomSpaceStorageConfig.cmake
	DESTINATION ${ConfigPackageLocation}
)

# -----------------------------------------------------------
