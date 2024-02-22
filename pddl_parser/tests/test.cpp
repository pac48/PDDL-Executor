#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "pddl_parser/pddl_parser.hpp"

using namespace pddl_lib;


TEST(PROBLEM, parse) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_parser");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path pddl_file = test_dir / "medicine_problem.pddl";

    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string content = ss.str();

    if (auto problem = parse_problem(content)) {
        std::cout << problem.value() << std::endl;
    } else {
        std::cout << problem.error() << std::endl;
        ASSERT_TRUE(false);
    }

}

TEST(DOMAIN, parse) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_parser");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path pddl_file = test_dir / "medicine_domain.pddl";

    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string content = ss.str();

    if (auto domain = parse_domain(content)) {
        std::cout << domain.value() << std::endl;
    } else {
        std::cout << domain.error() << std::endl;
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

    if (auto domain = parse_domain(content)) {
    } else {
        std::cout << domain.error() << std::endl;
        ASSERT_TRUE(false);
    }


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
    kb.insert_object(kitchen);
    kb.insert_object(couch);
    kb.insert_object(home);
    kb.insert_object(pioneer);
    kb.insert_object(nathan);


    kb.insert_predicate({"robot_at", {pioneer, home}});
    kb.insert_predicate({"medicine_location", {}});

    kb.insert_unknown_predicate({"person_at", {nathan, couch}});
    kb.insert_unknown_predicate({"person_at", {nathan, kitchen}});
    kb.insert_unknown_predicate({"person_at", {nathan, home}});

    kb.insert_unknown_predicate({"guide_to_succeeded_attempt_1", {}});
    kb.insert_unknown_predicate({"guide_to_succeeded_attempt_2", {}});
    kb.insert_unknown_predicate({"notify_automated_succeeded", {}});
    kb.insert_unknown_predicate({"notify_recorded_succeeded", {}});
    InstantiatedCondition cond = {{},
                                  {},
                                  {
                                          InstantiatedPredicate{"person_at", {nathan, couch}},
                                          InstantiatedPredicate{"person_at", {nathan, kitchen}},
                                          InstantiatedPredicate{"person_at", {nathan, home}}
                                  }};

    kb.insert_constraint({
                                 CONSTRAINTS::ONEOF,
                                 cond
                         });

//    std::cout << kb.convert_to_problem(domain) << std::endl;

}

TEST(KB, convert_To_problem_2) {
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
    kb.insert_object(kitchen);
    kb.insert_object(couch);
    kb.insert_object(home);
    kb.insert_object(pioneer);
    kb.insert_object(nathan);

    kb.insert_predicate({"robot_at", {pioneer, home}});
    kb.insert_predicate({"medicine_location", {}});

    kb.insert_predicate({"abort", {}});
    InstantiatedCondition cond = {{},
                                  {},
                                  {
                                          InstantiatedPredicate{"medicine_location", {}},
                                          InstantiatedPredicate{"robot_at", {pioneer, home}},
                                          InstantiatedCondition{pddl_lib::OPERATION::NOT, {} , {InstantiatedPredicate{"abort", {}}}},
                                  }};
    kb.check_conditions(cond);

}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}