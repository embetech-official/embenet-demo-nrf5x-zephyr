#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "embetech::ring_buffer" for configuration "Release"
set_property(TARGET embetech::ring_buffer APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(embetech::ring_buffer PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/thumbv6m-none-eabi/libring_buffer-1.0.5.a"
  )

list(APPEND _cmake_import_check_targets embetech::ring_buffer )
list(APPEND _cmake_import_check_files_for_embetech::ring_buffer "${_IMPORT_PREFIX}/lib/thumbv6m-none-eabi/libring_buffer-1.0.5.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
