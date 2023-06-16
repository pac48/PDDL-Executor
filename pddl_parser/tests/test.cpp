#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "pddl_parser/pddl_parser.hpp"


TEST(DOMAIN, parse) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_parser");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path pddl_file = test_dir / "medicine_domain.pddl";

    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string content = ss.str();

    auto domain = parse_domain(content);
    ASSERT_TRUE(domain.has_value());

    std::cout << domain.value() << std::endl;

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
    kb.objects = {kitchen, couch, home, pioneer, nathan};
    kb.knownKnowledgeBase.insert({"robot_at", {pioneer, home}});
    kb.knownKnowledgeBase.insert({"medicine_location", {}});

    kb.unknownKnowledgeBase.insert({"person_at", {nathan, couch}});
    kb.unknownKnowledgeBase.insert({"person_at", {nathan, kitchen}});
    kb.unknownKnowledgeBase.insert({"person_at", {nathan, home}});

    kb.unknownKnowledgeBase.insert({"guide_to_succeeded_attempt_1", {}});
    kb.unknownKnowledgeBase.insert({"guide_to_succeeded_attempt_2", {}});
    kb.unknownKnowledgeBase.insert({"notify_automated_succeeded", {}});
    kb.unknownKnowledgeBase.insert({"notify_recorded_succeeded", {}});

    kb.unknownKnowledgeBase.constraints.push_back({CONSTRAINTS::ONEOF,
                                                   {{"person_at", {nathan, couch}},
                                                    {"person_at", {nathan, kitchen}},
                                                    {"person_at", {nathan, home}}
                                                   }
                                                  });

    std::cout << kb.convert_to_problem(domain) << std::endl;

}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}