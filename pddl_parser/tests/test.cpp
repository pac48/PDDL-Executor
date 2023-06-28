#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "pddl_parser/pddl_parser.hpp"

using namespace pddl_lib;

TEST(DOMAIN, parse) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_parser");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path pddl_file = test_dir / "medicine_domain.pddl";

    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string content = ss.str();

    if (auto domain = parse_domain(content)){
    } else{
        std::cout << domain.error() <<std::endl;
        ASSERT_TRUE(false);
    }

//    std::cout << domain.value() << std::endl;

}

TEST(DOMAIN, parse_high_level) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_parser");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path pddl_file = test_dir / "high_level_domain.pddl";

    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string content = ss.str();

    if (auto domain = parse_domain(content)){
    } else{
        std::cout << domain.error() <<std::endl;
        ASSERT_TRUE(false);
    }

//    std::cout << domain.value() << std::endl;

}

TEST(KB, convert_To_problem) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_parser");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path pddl_file = test_dir / "medicine_domain.pddl";

    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string content = ss.str();

    auto domain_opt = parse_domain(content);
    ASSERT_TRUE(domain_opt.has_value());
    auto domain = domain_opt.value();

    InstantiatedParameter kitchen = {"kitchen", "landmark"};
    InstantiatedParameter couch = {"couch", "landmark"};
    InstantiatedParameter home = {"home", "landmark"};
    InstantiatedParameter pioneer = {"pioneer", "robot"};
    InstantiatedParameter nathan = {"nathan", "person"};

    auto &kb = KnowledgeBase::getInstance();
    kb.objects.concurrent_insert(kitchen);
    kb.objects.concurrent_insert(couch);
    kb.objects.concurrent_insert(home);
    kb.objects.concurrent_insert(pioneer);
    kb.objects.concurrent_insert(nathan);


    kb.knownPredicates.insert({"robot_at", {pioneer, home}});
    kb.knownPredicates.insert({"medicine_location", {}});

    kb.unknownPredicates.insert({"person_at", {nathan, couch}});
    kb.unknownPredicates.insert({"person_at", {nathan, kitchen}});
    kb.unknownPredicates.insert({"person_at", {nathan, home}});

    kb.unknownPredicates.insert({"guide_to_succeeded_attempt_1", {}});
    kb.unknownPredicates.insert({"guide_to_succeeded_attempt_2", {}});
    kb.unknownPredicates.insert({"notify_automated_succeeded", {}});
    kb.unknownPredicates.insert({"notify_recorded_succeeded", {}});

    kb.unknownPredicates.constraints.push_back({CONSTRAINTS::ONEOF,
                                                {{"person_at", {nathan, couch}},
                                                 {"person_at", {nathan, kitchen}},
                                                 {"person_at", {nathan, home}}
                                                }
                                               });

//    std::cout << kb.convert_to_problem(domain) << std::endl;

}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}