#pragma once
#include <vector>
#include <pddl_parser/pddl_parser.hpp>
#include "string"
#include "memory"
#include "optional"
#include "unordered_map"

struct CFFAction {
    std::string name;
    std::vector<std::string> params;
};

struct PlanItem {
    int duration;
    double time;
    CFFAction action;
};
struct PlanNode {
    PlanItem item;
    std::shared_ptr<PlanNode> true_node;
    std::shared_ptr<PlanNode> false_node;
};

class Plan {
public:
    Plan() = default;

    Plan(std::shared_ptr<PlanNode> root);

    std::string convert_to_bt(const Domain & domain) const;

    friend std::ostream &operator<<(std::ostream &os, const Plan &obj);
private:

    void add_observe_action_sequence(bool observe_result, const PlanItem &item, std::stringstream &tree, std::unordered_map<std::string, Action> & action_map) const;

    void add_action_sequence(const PlanItem &item, std::stringstream &tree, std::unordered_map<std::string, Action> & action_map) const;

    void get_sub_tree(const std::shared_ptr<PlanNode> &root, std::stringstream &tree, std::unordered_map<std::string, Action>& action_map) const;

    std::shared_ptr<PlanNode> root_;
    std::unordered_map<std::string, std::string> template_map_;

};

