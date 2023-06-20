#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "cff_plan_solver/cff_plan_solver.hpp"
#include "pddl_bt/actions.hpp"
#include <ament_index_cpp/get_package_share_directory.hpp>

using namespace pddl_lib;

Domain load_domain(std::string &domain_file) {
    std::string domain_str;
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_bt");
    std::filesystem::path domain_file_path = pkg_dir / "tests" / "pddl" / domain_file;

    std::ifstream domain_file_stream(domain_file_path.c_str());
    std::stringstream ss;
    ss << domain_file_stream.rdbuf();
    domain_str = ss.str();
    return parse_domain(domain_str).value();
}

TEST(DOMAIN, parse) {
    BT::BehaviorTreeFactory factory = create_tree_factory();
    active_domain = "high_level_domain.pddl";

    auto &kb = KnowledgeBase::getInstance();
    kb.knownPredicates.insert({"time_to_take_medicine", {}});
//    kb.knownPredicates.insert({"person_on_ground", {}});
    kb.knownPredicates.insert({"priority_1", {}});


    Domain domain = load_domain(active_domain);
    kb.knownPredicates.erase({"success", {}});
    auto problem_str = kb.convert_to_problem(domain);
    if (auto config = getPlan(domain.str(), problem_str)) {
        auto tree = factory.createTreeFromText(config.value());
        while (tree.tickRoot() == BT::NodeStatus::RUNNING) {
            printf("wait\n");
        }
    }


    InstantiatedParameter kitchen = {"kitchen", "landmark"};
    InstantiatedParameter couch = {"couch", "landmark"};
    InstantiatedParameter home = {"home", "landmark"};
    InstantiatedParameter pioneer = {"pioneer", "robot"};
    InstantiatedParameter nathan = {"nathan", "person"};

    kb.objects = {kitchen, couch, home, pioneer, nathan};
    kb.knownPredicates.insert({"robot_at", {pioneer, home}});
    kb.knownPredicates.insert({"medicine_location", {kitchen}});

    kb.unknownPredicates.insert({"guide_to_succeeded_attempt_1", {}});
    kb.unknownPredicates.insert({"guide_to_succeeded_attempt_2", {}});
    kb.unknownPredicates.insert({"notify_automated_succeeded", {}});
    kb.unknownPredicates.insert({"notify_recorded_succeeded", {}});

    kb.unknownPredicates.insert({"person_at", {nathan, couch}});
    kb.unknownPredicates.insert({"person_at", {nathan, kitchen}});
    kb.unknownPredicates.insert({"person_at", {nathan, home}});
    kb.unknownPredicates.constraints.push_back({CONSTRAINTS::ONEOF,
                                                {{"person_at", {nathan, couch}},
                                                 {"person_at", {nathan, kitchen}},
                                                 {"person_at", {nathan, home}}
                                                }
                                               });

    domain = load_domain(active_domain);
    kb.knownPredicates.erase({"success", {}});
    problem_str = kb.convert_to_problem(domain);
    if (auto config = getPlan(domain.str(), problem_str)) {
        auto tree = factory.createTreeFromText(config.value());
        while (tree.tickRoot() == BT::NodeStatus::RUNNING) {
            printf("wait\n");
        }
    }


    domain = load_domain(active_domain);
//    kb.knownPredicates.erase({"success", {}});
    problem_str = kb.convert_to_problem(domain);
    if (auto config = getPlan(domain.str(), problem_str)) {
        auto tree = factory.createTreeFromText(config.value());
        while (tree.tickRoot() == BT::NodeStatus::RUNNING) {
            printf("wait\n");
        }
    }

}


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}