#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "pddl_parser/pddl_parser.hpp"



TEST(AnyTest, assign_construct) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_parser");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path pddl_file = test_dir / "medicine_domain.pddl";

    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string content = ss.str();


    parse_domain(content);

    ASSERT_TRUE(1 == 1);
}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}