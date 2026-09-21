#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "embenet::node-thumbv8m.base-none-eabi" for configuration "Release"
set_property(TARGET embenet::node-thumbv8m.base-none-eabi APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(embenet::node-thumbv8m.base-none-eabi PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/thumbv8m.base-none-eabi/libembenet_node-1.1.1-evaluation.a"
  )

list(APPEND _cmake_import_check_targets embenet::node-thumbv8m.base-none-eabi )
list(APPEND _cmake_import_check_files_for_embenet::node-thumbv8m.base-none-eabi "${_IMPORT_PREFIX}/lib/thumbv8m.base-none-eabi/libembenet_node-1.1.1-evaluation.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
