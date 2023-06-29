cmake_minimum_required(VERSION 3.8)
project(plan_solver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(pddl2cpp REQUIRED)

generate_search_header(pddl_problem
    pddl/domain.pddl
    pddl/problem.pddl
    )

add_executable(plan_solver src/plan_solver.cpp)
target_link_libraries(plan_solver pddl_problem)
target_include_directories(plan_solver PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(plan_solver PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17


install(TARGETS plan_solver
  DESTINATION lib/${PROJECT_NAME})



ament_package()
