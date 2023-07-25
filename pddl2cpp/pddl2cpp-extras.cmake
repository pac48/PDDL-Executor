find_package(pddl_parser REQUIRED)
find_package(behaviortree_cpp_v3 REQUIRED)
include("${pddl2cpp_DIR}/generate_bt_header.cmake")
include("${pddl2cpp_DIR}/generate_search_header.cmake")
