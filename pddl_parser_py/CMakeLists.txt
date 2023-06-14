cmake_minimum_required(VERSION 3.8)
project(pddl_parser_py)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

find_package(ament_cmake REQUIRED)
ament_python_install_package(pddl_parser)

find_package(pddl_parser REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11_vendor REQUIRED)
find_package(pybind11 REQUIRED)


include_directories(cpp/include)

pybind11_add_module(parser
    cpp/src/pddl_parser_py.cpp
    )

###HACK : ament_target_dependencies cannot declare its deps as private, which is needed for pybind11.. so copypaste
#function(ament_target_dependencies_private target)
#  if(NOT TARGET ${target})
#    message(FATAL_ERROR "ament_target_dependencies() the first argument must be a valid target name")
#  endif()
#  if(${ARGC} GREATER 0)
#    set(definitions "")
#    set(include_dirs "")
#    set(libraries "")
#    set(link_flags "")
#    foreach(package_name ${ARGN})
#      if(NOT ${${package_name}_FOUND})
#        message(FATAL_ERROR "ament_target_dependencies() the passed package name '${package_name}' was not found before")
#      endif()
#      list_append_unique(definitions ${${package_name}_DEFINITIONS})
#      list_append_unique(include_dirs ${${package_name}_INCLUDE_DIRS})
#      list(APPEND libraries ${${package_name}_LIBRARIES})
#      list_append_unique(link_flags ${${package_name}_LINK_FLAGS})
#    endforeach()
#    target_compile_definitions(${target}
#        PUBLIC ${definitions})
#    ament_include_directories_order(ordered_include_dirs ${include_dirs})
#    target_include_directories(${target}
#        PUBLIC ${ordered_include_dirs})
#    ament_libraries_deduplicate(unique_libraries ${libraries})
#    target_link_libraries(${target} PRIVATE
#        ${unique_libraries})
#    message("SGRWESGWEGWEEEGE  :" ${unique_libraries})
#    foreach(link_flag IN LISTS link_flags)
#      message("FLAG:::: " ${link_flag})
#      set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${link_flag} ")
#    endforeach()
#  endif()
#endfunction()

#ament_target_dependencies_private(
#    parser
#    pddl_parser
#)


target_link_libraries(parser PRIVATE
    pddl_parser::pddl_parser
    fmt::fmt
    )
target_include_directories(parser PUBLIC ${pddl_parser_INCLUDE_DIRS})

install(TARGETS parser
    DESTINATION "${PYTHON_INSTALL_DIR}/pddl_parser"
    )

ament_package()

## debug
#get_cmake_property(_variableNames VARIABLES)
#list(SORT _variableNames)
#
#foreach(_variableName ${_variableNames})
#  message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()