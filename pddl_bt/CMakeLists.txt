cmake_minimum_required(VERSION 3.8)
project(pddl_bt)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

find_package(ament_cmake REQUIRED)


if (BUILD_TESTING)
  find_package(behaviortree_cpp_v3 REQUIRED)
  find_package(pddl2cpp REQUIRED)
  find_package(cff_plan_solver REQUIRED)
  find_package(ament_index_cpp REQUIRED)

  find_package(GTest REQUIRED)

  generate_bt_header(bt_shr_actions
      tests/pddl/medicine_domain.pddl
      tests/pddl/high_level_domain.pddl
      )
  add_executable(test_bt tests/test.cpp)
  target_link_libraries(test_bt
      bt_shr_actions
      ${GTEST_LIBRARIES}
      )
  ament_target_dependencies(test_bt
      ament_index_cpp behaviortree_cpp_v3 cff_plan_solver
      )
  target_include_directories(test_bt PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>)
  install(DIRECTORY tests/pddl
      DESTINATION share/pddl_bt/tests/
      )
endif ()

ament_package()
