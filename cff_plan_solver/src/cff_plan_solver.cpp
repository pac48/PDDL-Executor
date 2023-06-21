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

namespace pddl_lib {

    std::optional<Plan> runPlanner(int argc, const char *argv[], const std::string& plan_file_path) {
        std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("cff_plan_solver");
        auto exe_path = pkg_dir / "FF";

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


    std::optional<std::string> getPlan(const std::string &domain_str, const std::string &problem_str) {
        auto domain = parse_domain(domain_str);
        std::string domain_file_path = "/tmp/cff_plan_solver/domain_" + domain->name + ".pddl";
        std::string problem_file_path = "/tmp/cff_plan_solver/problem_" + domain->name + ".pddl";
        std::string plan_file_path = "/tmp/cff_plan_solver/pddlplan_" + domain->name + ".txt";
        if (domain.has_value()) {
            std::ofstream domainFile(domain_file_path);
            domainFile << domain_str;
            std::ofstream problemFile(problem_file_path);
            problemFile << problem_str;

        } else {
            return {};
        }

        const char *argv[] = {"prog_name", "-a", "0", "-o", domain_file_path.c_str(), "-f",
                              problem_file_path.c_str()};
        int argc = 7;

        if (auto plan = runPlanner(argc, argv, plan_file_path)) {
            return plan.value().convert_to_bt(domain.value());

        }

        return {};
    }

} // pddl_lib
