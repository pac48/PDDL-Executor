cmake_minimum_required(VERSION 3.8)
project(pddl_executor_example)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif ()

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)

# generate bt action interface
find_package(pddl2cpp REQUIRED)
generate_bt_header(bt_blocks_actions
    pddl/domain_blocks.pddl
    )

set(dependencies
    ament_index_cpp
    rclcpp
    )

add_executable(planning_controller_node src/planning_controller_node.cpp)
ament_target_dependencies(planning_controller_node ${dependencies})

target_link_libraries(planning_controller_node
    bt_blocks_actions
    )

install(TARGETS
    planning_controller_node
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION lib/${PROJECT_NAME}
    )

install(DIRECTORY pddl
    DESTINATION share/pddl_executor_example/
    )


ament_package()
