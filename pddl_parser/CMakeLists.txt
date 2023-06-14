cmake_minimum_required(VERSION 3.8)
project(pddl_parser)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


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

install(DIRECTORY include/
    DESTINATION include/
    )

install(
    TARGETS pddl_parser
    EXPORT pddl_parserTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

ament_export_targets(pddl_parserTargets HAS_LIBRARY_TARGET)
ament_export_dependencies(FMT)
ament_export_include_directories(include)
ament_package()
