cmake_minimum_required(VERSION 3.8)
project(pddl2cpp NONE)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

# find dependencies
find_package(ament_cmake REQUIRED)

install(
    DIRECTORY cmake jinja_templates
    DESTINATION share/pddl2cpp
)

ament_package(
    CONFIG_EXTRAS "pddl2cpp-extras.cmake"
)
