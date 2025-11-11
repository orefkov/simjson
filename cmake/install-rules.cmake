if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/simjson-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package simjson)

install(
    DIRECTORY
    include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT simjson_Development
)

install(
    TARGETS simjson_simjson
    EXPORT simjsonTargets
    RUNTIME #
    COMPONENT simjson_Runtime
    LIBRARY #
    COMPONENT simjson_Runtime
    NAMELINK_COMPONENT simjson_Development
    ARCHIVE #
    COMPONENT simjson_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    simjson_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE simjson_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(simjson_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${simjson_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT simjson_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${simjson_INSTALL_CMAKEDIR}"
    COMPONENT simjson_Development
)

install(
    EXPORT simjsonTargets
    NAMESPACE simjson::
    DESTINATION "${simjson_INSTALL_CMAKEDIR}"
    COMPONENT simjson_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
