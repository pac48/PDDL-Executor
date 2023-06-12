cmake_minimum_required(VERSION 3.8)
project(pddl_parser)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()


include_directories(include)
find_package(ament_cmake REQUIRED)
find_package(FMT REQUIRED)
add_library(pddl_parser src/pddl_parser.cpp)
target_link_libraries(pddl_parser fmt::fmt)


if (BUILD_TESTING)
  add_subdirectory(tests)

  install(DIRECTORY tests/pddl
      DESTINATION share/pddl_parser/tests/
      )

endif ()

ament_package()
