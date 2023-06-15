#include "string"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <queue>
#include <optional>

#include "ament_index_cpp/get_package_share_directory.hpp"

#include "cff_plan_solver/cff_plan_solver.hpp"
#include <cff_plan_solver/cff_parser.hpp>
#include "cff_plan_solver/types.hpp"
#include "pddl_parser/pddl_parser.hpp"


std::optional<Plan> runPlanner(int argc, char *const *argv) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("cff_plan_solver");
    auto exe_path = pkg_dir / "FF";

    std::string plan_out_dir = "/tmp/cff_plan_solver/";
    if (!std::filesystem::exists(plan_out_dir)) {
        std::filesystem::create_directory(plan_out_dir);
    }
    auto plan_file_path = plan_out_dir + "pddlplan.txt";

    std::stringstream args_stream;
    for (int i = 1; i < argc; i++) {
        args_stream << " ";
        args_stream << argv[i];
    }
    args_stream << "> " << plan_file_path;
    // run planner
    std::system((exe_path.string() + args_stream.str()).c_str());

    return parsePlanOutput(plan_file_path);
}


std::string getPlan(const std::string &domain_str, const std::string &problem_str) {
    {
        std::ofstream domainFile("/tmp/cff_plan_solver/domain.pddl");
        domainFile << domain_str;
        std::ofstream problemFile("/tmp/cff_plan_solver/problem.pddl");
        problemFile << problem_str;
    }


    char *argv[] = {"prog_name", "-a", "0", "-o", "/tmp/cff_plan_solver/domain.pddl", "-f", "/tmp/cff_plan_solver/problem.pddl"};
    int argc = 7;
    auto plan = runPlanner(argc, argv);

    Plan plan_val = plan.value();
    // modify
    auto domain = parse_domain(domain_str);

    std::cout << plan_val.convert_to_bt(domain.value()) << std::endl;
    int o = 0;

    return "";

}


