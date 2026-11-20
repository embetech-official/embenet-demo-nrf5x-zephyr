#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "embetech::crc" for configuration "Release"
set_property(TARGET embetech::crc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(embetech::crc PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/thumbv7em-none-eabihf/libcrc-2.0.5.a"
  )

list(APPEND _cmake_import_check_targets embetech::crc )
list(APPEND _cmake_import_check_files_for_embetech::crc "${_IMPORT_PREFIX}/lib/thumbv7em-none-eabihf/libcrc-2.0.5.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
