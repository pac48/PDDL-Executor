project(cff_plan_solver)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(contingent-ff)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(jinja2cpp_ros REQUIRED)


include_directories(
    include
    contingent-ff
)

add_executable(cff_plan_solver
    src/cff_plan_solver.cpp src/cff_parser.cpp src/types.cpp
    )
#target_link_libraries(cff_plan_solver)


ament_target_dependencies(cff_plan_solver
    ament_index_cpp
    jinja2cpp_ros
)


install(TARGETS
    cff_plan_solver
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )

install(TARGETS
    FF
    RUNTIME DESTINATION share/cff_plan_solver
    )

install(DIRECTORY jinja_templates
    DESTINATION share/cff_plan_solver
    )

install(DIRECTORY include/
    DESTINATION include/
    )


ament_export_include_directories(include)
ament_export_libraries(cff_plan_solver)
#ament_export_dependencies(${dependencies})

ament_package()


# debug
get_cmake_property(_variableNames VARIABLES)
list(SORT _variableNames)

foreach(_variableName ${_variableNames})
  message(STATUS "${_variableName}=${${_variableName}}")
endforeach()