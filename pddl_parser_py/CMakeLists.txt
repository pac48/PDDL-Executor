cmake_minimum_required(VERSION 3.8)
project(pddl_parser_py)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()


find_package(ament_cmake REQUIRED)
find_package(pddl_parser REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11_vendor REQUIRED)
find_package(pybind11 REQUIRED)

ament_python_install_package(pddl_parser)

include_directories(cpp/include)

pybind11_add_module(parser
    cpp/src/pddl_parser_py.cpp
    )
#target_link_libraries(parser_pddl_py PRIVATE ...)

ament_target_dependencies(parser
    pddl_parser
    )

#configure_build_install_location(parser_pddl_py)


#ament_export_targets(parser_pddl_pyTargets HAS_LIBRARY_TARGET)
ament_package()