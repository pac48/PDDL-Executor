cmake_minimum_required(VERSION 3.8)
project(plan_solver)

set(CMAKE_INSTALL_PREFIX /tmp/plan_solver)

# find dependencies
find_package(pddl2cpp REQUIRED)

if (NOT DEFINED PDDL_PROBLEM)
  set(PDDL_PROBLEM "pddl/problem.pddl")
endif ()
if (NOT DEFINED PDDL_DOMAIN)
  set(PDDL_DOMAIN "pddl/domain.pddl")
endif ()

generate_search_header(pddl_problem
    ${PDDL_DOMAIN}
    ${PDDL_PROBLEM}
    )


add_executable(plan_solver src/plan_solver.cpp)

target_link_libraries(plan_solver pddl_problem)
target_include_directories(plan_solver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(plan_solver PUBLIC cxx_std_17)

install(TARGETS plan_solver
    DESTINATION lib/${PROJECT_NAME})
