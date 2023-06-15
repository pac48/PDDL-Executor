#include <iostream>
#include <cff_plan_solver/types.hpp>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <pddl_parser/pddl_parser.hpp>
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "cff_plan_solver/cff_plan_solver.hpp"

int main(){
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("cff_plan_solver");
    std::filesystem::path test_dir = pkg_dir / "tests" / "pddl";
    std::filesystem::path domain_file = test_dir / "medicine_domain.pddl";
    std::filesystem::path problem_file = test_dir / "medicine_problem.pddl";

    std::ifstream domain_file_stream(domain_file.string().c_str());
    std::stringstream ss;
    ss << domain_file_stream.rdbuf();
    std::string domain_str = ss.str();

    std::ifstream problem_file_stream(problem_file.string().c_str());
    std::stringstream ss_problem;
    ss_problem << problem_file_stream.rdbuf();
    std::string problem_str = ss_problem.str();

    auto domain = parse_domain(domain_str);
    std::stringstream ss_domain;
    ss_domain << domain.value();

    auto out = getPlan(ss_domain.str(), problem_str);

    return 0;
}