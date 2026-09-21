#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "embenet::node-x86_64-windows-gnu" for configuration "Release"
set_property(TARGET embenet::node-x86_64-windows-gnu APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(embenet::node-x86_64-windows-gnu PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/x86_64-windows-gnu/libembenet_node-1.1.1-evaluation.a"
  )

list(APPEND _cmake_import_check_targets embenet::node-x86_64-windows-gnu )
list(APPEND _cmake_import_check_files_for_embenet::node-x86_64-windows-gnu "${_IMPORT_PREFIX}/lib/x86_64-windows-gnu/libembenet_node-1.1.1-evaluation.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
