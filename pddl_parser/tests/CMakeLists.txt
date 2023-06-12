list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(gtest)
find_package(ament_index_cpp REQUIRED)

add_executable(test_pdd_parser
    test.cpp
    )

find_package(GTest REQUIRED)
target_link_libraries(test_pdd_parser ${GTEST_LIBRARIES} pddl_parser)
ament_target_dependencies(test_pdd_parser
    ament_index_cpp
    )
