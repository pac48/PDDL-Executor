#pragma once

#include "string"
#include "memory"
#include "optional"

struct PlanItem {
    int duration;
    double time;
    std::string action;
};
struct PlanNode {
    PlanItem item;
    std::shared_ptr<PlanNode> true_node;
    std::shared_ptr<PlanNode> false_node;
};