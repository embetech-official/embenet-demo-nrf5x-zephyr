#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "embenet::node-aarch32-linux-gnu" for configuration "Release"
set_property(TARGET embenet::node-aarch32-linux-gnu APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(embenet::node-aarch32-linux-gnu PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/aarch32-linux-gnu/libembenet_node-1.1.1-evaluation.a"
  )

list(APPEND _cmake_import_check_targets embenet::node-aarch32-linux-gnu )
list(APPEND _cmake_import_check_files_for_embenet::node-aarch32-linux-gnu "${_IMPORT_PREFIX}/lib/aarch32-linux-gnu/libembenet_node-1.1.1-evaluation.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
