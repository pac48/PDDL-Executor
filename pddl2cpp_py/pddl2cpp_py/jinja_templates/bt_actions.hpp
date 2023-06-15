#pragma once

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/xml_parsing.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "behaviortree_cpp_v3/tree_node.h"
#include "pddl_parser/pddl_parser.hpp"

{{action_classes}}

BT::BehaviorTreeFactory create_tree_factory(){
    BT::BehaviorTreeFactory factory;
    {% for name in action_names %}
    factory.registerNodeType<{{name}}>("{{name}}");
    {%- endfor %}

    return factory;
}