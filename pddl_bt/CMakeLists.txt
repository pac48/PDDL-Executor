cmake_minimum_required(VERSION 3.8)
project(pddl_bt)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

find_package(ament_cmake REQUIRED)
find_package(behaviortree_cpp_v3 REQUIRED)
find_package(pddl2cpp REQUIRED)
find_package(cff_plan_solver REQUIRED)
find_package(ament_index_cpp REQUIRED)

generate_bt_header(pddl/medicine_domain.pddl
    bt_medicine_domain
    )

add_executable(main src/main.cpp)

target_include_directories(main PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(main PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17
ament_target_dependencies(main behaviortree_cpp_v3 cff_plan_solver ament_index_cpp)

target_link_libraries(main bt_medicine_domain)


install(TARGETS main
    DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY pddl
    DESTINATION share/pddl_bt/
    )

ament_package()
