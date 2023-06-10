cmake_minimum_required(VERSION 3.8)
project(jinja2cpp_ros)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

# find dependencies
find_package(ament_cmake REQUIRED)

include(ExternalProject)
ExternalProject_Add(
    inja
    GIT_REPOSITORY https://github.com/hughperkins/Jinja2CppLight.git
    GIT_TAG master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)

ament_export_include_directories(${CMAKE_INSTALL_PREFIX}/include/Jinja2CppLight)
ament_export_libraries(Jinja2CppLight)

ament_package()