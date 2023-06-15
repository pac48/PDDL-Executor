cmake_minimum_required(VERSION 3.8)
project(pddl_bt)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

find_package(ament_cmake REQUIRED)
find_package(behaviortree_cpp_v3 REQUIRED)
find_package(pddl2cpp REQUIRED)

generate_bt_header(src/medicine_domain.pddl
    bt_medicine_domain
    )

add_executable(main src/main.cpp)
add_dependencies(main bt_medicine_domain)
target_include_directories(main PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(main PUBLIC c_std_99 cxx_std_17)  # Require C99 and C++17
ament_target_dependencies(main behaviortree_cpp_v3)
install(TARGETS main
    DESTINATION lib/${PROJECT_NAME})

ament_package()
