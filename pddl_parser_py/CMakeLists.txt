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

if (BUILD_TESTING)
  install(DIRECTORY tests/pddl
      DESTINATION share/pddl_parser_py/tests/
      )

endif ()

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