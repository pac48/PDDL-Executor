#include <memory>
#include <filesystem>
#include <fstream>

#include "rclcpp/rclcpp.hpp"
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "bt_blocks_actions.hpp"

using namespace pddl_lib;


class UpdatePredicatesImpl : public UpdatePredicates {
public:

    TRUTH_VALUE clear(TRUTH_VALUE val, None b) const override {
        return val;
    }

    TRUTH_VALUE on(TRUTH_VALUE val, None b1, None b2) const override {
        return val;
    }

    TRUTH_VALUE on_table(TRUTH_VALUE val, None b) const override {
        return val;
    }

};

class BlockWorldActions : public pddl_lib::ActionInterface {
public:
    BT::NodeStatus blocksworld_senseON(const InstantiatedAction &action) override {
        return (rand() % 2) == 0 ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }

    BT::NodeStatus blocksworld_senseCLEAR(const InstantiatedAction &action) override {
        return (rand() % 2) == 0 ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }

    BT::NodeStatus blocksworld_senseONTABLE(const InstantiatedAction &action) override {
        return (rand() % 2) == 0 ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }

    BT::NodeStatus blocksworld_move_b_to_b(const InstantiatedAction &action) override {
        return BT::NodeStatus::SUCCESS;
    }

    BT::NodeStatus blocksworld_move_to_t(const InstantiatedAction &action) override {
        return BT::NodeStatus::SUCCESS;
    }

    BT::NodeStatus blocksworld_move_t_to_b(const InstantiatedAction &action) override {
        return BT::NodeStatus::SUCCESS;
    }
};

std::optional<std::string> getPlan(const std::string &domain, const std::string &problem) {
    {
        auto tmp_path = std::filesystem::path("/tmp/plan_solver/");
        std::filesystem::create_directory(tmp_path);
        std::ofstream domainFile(tmp_path / "domain.pddl");
        domainFile << domain;
        std::ofstream problemFile(tmp_path / "problem.pddl");
        problemFile << problem;
    }
    std::string cmd = "ros2 run plan_solver_py plan_solver -o /tmp/plan_solver/domain.pddl -f /tmp/plan_solver/problem.pddl > /dev/null";
    std::system(cmd.c_str());

    std::ifstream file("/tmp/plan_solver/bt.xml");
    if (!file) {
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::string get_file_content(const std::string &file_name) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("pddl_executor_example");
    auto file_path = pkg_dir / "pddl" / file_name;
    std::ifstream f(file_path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);

    auto &kb = KnowledgeBase::getInstance();

    auto domain_content = get_file_content("domain_blocks.pddl");
    auto problem_content = get_file_content("problem_blocks.pddl");
    pddl_lib::Domain domain = parse_domain(domain_content).value();
    pddl_lib::Problem prob = parse_problem(problem_content).value();

    auto factory = create_tree_factory<BlockWorldActions>();
    UpdatePredicatesImpl updater;

    while (rclcpp::ok()) {
        kb.clear();
        kb.load_kb(prob);
        updater.update();
        std::optional<std::string> config = getPlan(domain.str(), kb.convert_to_problem(domain));
        auto tree = factory.createTreeFromText(config.value());
        tree.tickRoot();
        std::cout << "\n\n";
        rclcpp::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}



