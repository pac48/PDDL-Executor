project(cff_plan_solver)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(contingent-ff)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ament_index_cpp REQUIRED)


include_directories(
    include
    contingent-ff
)

add_executable(cff_plan_solver
    src/cff_plan_solver.cpp
    )
target_link_libraries(cff_plan_solver FF)
#ament_target_dependencies(cff_plan_solver dependencies)



install(TARGETS
    cff_plan_solver
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )

#install(TARGETS
#    FF
#    ARCHIVE DESTINATION lib
#    LIBRARY DESTINATION lib
#    RUNTIME DESTINATION lib/${PROJECT_NAME}
#    )


install(DIRECTORY include/
    DESTINATION include/
    )


ament_export_include_directories(include)
ament_export_libraries(cff_plan_solver)
#ament_export_dependencies(${dependencies})

ament_package()
