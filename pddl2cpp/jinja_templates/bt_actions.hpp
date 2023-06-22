#pragma once

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/xml_parsing.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "behaviortree_cpp_v3/tree_node.h"
#include "pddl_parser/pddl_parser.hpp"

namespace pddl_lib {

enum TRUTH_VALUE{
    FALSE, TRUE, UNKNOWN
};

{% for type in types %}class {{type}} {
public:
    std::string value;
    {{type}}(const std::string & value_in) :  value{value_in} {

    }
};
{% endfor %}

class UpdatePredicates {
public:
  void update() const {
      auto & kb = KnowledgeBase::getInstance();

      {%- for type in types %}
      std::vector<InstantiatedParameter> {{type}}_instances;
      {%- endfor %}

      for (const auto object : kb.objects){
          {%- for type in types %}
          if (object.type == "{{type}}"){
              {{type}}_instances.push_back(object);
          }
          {%- endfor %}
      }

      {% for args in func_signatures %}
      for (auto [key, func] : pred_update_map_{{loop.index}}_){
          {%- for arg in args %}
          for (auto {{arg}}_instance : {{arg}}_instances ) {
          {%- endfor %}
              InstantiatedPredicate pred = {key, { {%- for arg in args %}{{arg}}_instance{%- if loop.index < loop.length %}, {% endif -%}{%- endfor %} } };
              TRUTH_VALUE old_val;
              if (kb.knownPredicates.concurrent_find(pred)){
                  old_val = TRUTH_VALUE::TRUE;
              } else if(kb.unknownPredicates.concurrent_find(pred)){
                  old_val = TRUTH_VALUE::UNKNOWN;
              } else{
                  old_val = TRUTH_VALUE::FALSE;
              }
              auto new_val = func(old_val{% for arg in args %}, ({{arg}}) {{arg}}_instance.name{%- endfor %});
              if (new_val==TRUTH_VALUE::TRUE){
                  kb.knownPredicates.concurrent_insert(pred);
                  kb.unknownPredicates.concurrent_erase(pred);
              } else if(new_val==TRUTH_VALUE::UNKNOWN){
                  kb.knownPredicates.concurrent_erase(pred);
                  kb.unknownPredicates.concurrent_insert(pred);
              } else{
                  kb.knownPredicates.concurrent_erase(pred);
                  kb.unknownPredicates.concurrent_erase(pred);
              }
          {% for arg in args %} }
          {% endfor -%}
          }
      {% endfor %}
  }

  {% for pred in predicates %}void register_{{pred.name}}(std::function<TRUTH_VALUE(TRUTH_VALUE{% for param in pred.parameters %}, {{param.type}} {{param.name}}{% endfor %})>  func){
        register_pred_update_map("{{pred.name}}", func);
  }
  {% endfor -%}

  private:
    {% for args in func_signatures -%}
    void register_pred_update_map(const std::string name, const std::function<TRUTH_VALUE(TRUTH_VALUE{% for type in args %}, {{type}}{% endfor %})> & func){
        pred_update_map_{{loop.index}}_[name] = func;
    }
    {% endfor -%}
    {% for args in func_signatures -%}
  std::unordered_map<std::string, std::function<TRUTH_VALUE(TRUTH_VALUE{% for type in args %}, {{type}}{% endfor %})> > pred_update_map_{{loop.index}}_;
    {% endfor -%}

};

{{action_classes}}

BT::BehaviorTreeFactory create_tree_factory(){
    BT::BehaviorTreeFactory factory;
    {% for name in action_names %}
    factory.registerNodeType<pddl_lib::{{name}}>("pddl_lib::{{name}}");
    {%- endfor %}

    return factory;
}

} // pddl_lib