#include <cstdio>
#include <filesystem>
#include <fstream>

#include "bt_medicine_domain.hpp"
#include "cff_plan_solver/cff_plan_solver.hpp"
#include "pddl_bt/actions.hpp"
#include <ament_index_cpp/get_package_share_directory.hpp>

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    BT::BehaviorTreeFactory factory = create_tree_factory();

    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_bt");
    std::filesystem::path test_dir = pkg_dir / "pddl";
    std::filesystem::path domain_file = test_dir / "medicine_domain.pddl";
    std::filesystem::path problem_file = test_dir / "medicine_problem.pddl";

    std::ifstream domain_file_stream(domain_file.string().c_str());
    std::stringstream ss;
    ss << domain_file_stream.rdbuf();
    std::string domain_str = ss.str();

    auto domain = parse_domain(domain_str);
    std::stringstream ss_domain;
    ss_domain << domain.value();


    InstantiatedParameter kitchen = {"kitchen", "landmark"};
    InstantiatedParameter couch = {"couch", "landmark"};
    InstantiatedParameter home = {"home", "landmark"};
    InstantiatedParameter pioneer = {"pioneer", "robot"};
    InstantiatedParameter nathan = {"nathan", "person"};

    auto &kb = KnowledgeBase::getInstance();
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

    auto problem_str = kb.convert_to_problem(domain.value());
    auto config = getPlan(domain_str, problem_str);
    // TODO get  config from planner!!
    auto tree = factory.createTreeFromText(config);

    while (tree.tickRoot() == BT::NodeStatus::RUNNING) {
        printf("wait\n");
    }

    return 0;
}
