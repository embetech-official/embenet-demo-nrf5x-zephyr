set(crc_VERSION 2.0.5)


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was crc-config_template.cmake                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include("${CMAKE_CURRENT_LIST_DIR}/architecture_selection.cmake")

if (NOT DEFINED crc_ARCHITECTURE)
  detect_architecture_triplet(crc_ARCHITECTURE)
  if (NOT crc_ARCHITECTURE)
    message(FATAL_ERROR "Unable to auto-detect the target architecture for crc. "
                        "Set crc_ARCHITECTURE manually before calling find_package(crc)."
    )
  endif ()
endif ()

message(DEBUG "crc_ARCHITECTURE=${crc_ARCHITECTURE}")

find_architecture_targets_file(crc "${crc_ARCHITECTURE}" "${CMAKE_CURRENT_LIST_DIR}" targets_file)
include("${targets_file}")
unset(targets_file)

check_required_components("crc")
