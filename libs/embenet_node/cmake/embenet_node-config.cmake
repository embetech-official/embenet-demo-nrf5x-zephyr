set(embenet_node_VERSION 1.1.1)


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was embenet_node-config_template.cmake                            ########

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

include(CMakeFindDependencyMacro)
find_dependency(embenet_udp)
find_dependency(crc)
find_dependency(logger)
find_dependency(embeutils)

include("${CMAKE_CURRENT_LIST_DIR}/architecture_selection.cmake")

if (NOT DEFINED EMBENET_NODE_ARCHITECTURE)
  detect_architecture_triplet(EMBENET_NODE_ARCHITECTURE)
  if (NOT EMBENET_NODE_ARCHITECTURE)
    message(FATAL_ERROR "Unable to auto-detect the target architecture for embenet_node. "
                        "Set EMBENET_NODE_ARCHITECTURE manually before calling find_package(embenet_node)."
    )
  endif ()
endif ()

message(DEBUG "EMBENET_NODE_ARCHITECTURE=${EMBENET_NODE_ARCHITECTURE}")

find_architecture_targets_file(embenet_node "${EMBENET_NODE_ARCHITECTURE}" "${CMAKE_CURRENT_LIST_DIR}" targets_file)
include("${targets_file}")
unset(targets_file)

add_library(embenet::node ALIAS embenet::node-${EMBENET_NODE_ARCHITECTURE})

check_required_components("embenet_node")
