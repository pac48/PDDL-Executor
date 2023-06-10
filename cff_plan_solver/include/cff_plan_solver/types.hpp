#pragma once

#include <vector>
#include "string"
#include "memory"
#include "optional"

struct Action {
    std::string name;
    std::vector<std::string> params;
};
struct PlanItem {
    int duration;
    double time;
    Action action;
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

    friend std::ostream &operator<<(std::ostream &os, const Plan &obj);

private:
    void add_observe_action_sequence(bool observe_result, const PlanItem &item, std::stringstream &tree) const;

    void add_action_sequence(const PlanItem &item, std::stringstream &tree) const;

    void get_sub_tree(const std::shared_ptr<PlanNode> &root, std::stringstream &tree) const;

    std::shared_ptr<PlanNode> root_;
    std::unordered_map<std::string, std::string> template_map_;

};

