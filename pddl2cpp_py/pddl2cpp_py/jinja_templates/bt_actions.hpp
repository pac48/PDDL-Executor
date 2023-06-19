#pragma once

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/xml_parsing.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "behaviortree_cpp_v3/tree_node.h"
#include "pddl_parser/pddl_parser.hpp"

// Hash function for strings
struct StringHash {
  std::size_t operator()(const std::string& str) const {
    // Use std::hash to hash the string
    return std::hash<std::string>{}(str);
  }
};

// Hash function for vector of strings
struct VectorHash {
  std::size_t operator()(const std::vector<std::string>& vec) const {
    std::size_t seed = vec.size();
    StringHash stringHash;

    for (const std::string& str : vec) {
      seed ^= stringHash(str) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
  }
};

class UpdatePredicates {
  void update(){

  }
  {% for pred in predicates %}void set_{{pred}}(std::vector<std::string> type, std::function<void(std::vector<std::string>)> & func){
    if (pred_name_map_.find("{{pred}}") == pred_name_map_.end()){
      pred_update_map_.insert(std::make_pair(type, &func));
      pred_name_map_["{{pred}}"] = pred_update_map_.at(type).size()-1;
    }
    pred_update_map_.at(type)[pred_name_map_["{{pred}}"]] = func;
  }
  {% endfor -%}

  private:
  std::unordered_map<std::vector<std::string>, std::vector<std::function<void(std::vector<std::string>)>> > pred_update_map_;
  std::unordered_map<std::string, int> pred_name_map_;
  {% for pred in predicates %}std::function<void(std::string)>{{pred}};
  {% endfor -%}

};

{{action_classes}}


BT::BehaviorTreeFactory create_tree_factory(){
    BT::BehaviorTreeFactory factory;
    {% for name in action_names %}
    factory.registerNodeType<{{name}}>("{{name}}");
    {%- endfor %}

    return factory;
}
