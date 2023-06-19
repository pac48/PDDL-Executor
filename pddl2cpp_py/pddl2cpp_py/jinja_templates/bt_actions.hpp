#pragma once

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/xml_parsing.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "behaviortree_cpp_v3/tree_node.h"
#include "pddl_parser/pddl_parser.hpp"

enum TRUTH_VALUE{
    FALSE, TRUE, UNKNOWN
};

{% for type in types %}typedef std::string {{type}};
{% endfor %}

class UpdatePredicates {
  UpdatePredicates(){
      // maybe we don't need to give default values
      {%- for pred in predicates %}
      set_{{pred.name}}([](TRUTH_VALUE val{% for param in pred.parameters %}, {{param.type}} {{param.name}}{% endfor %}){return val;});{% endfor %}
  }

  void update(){
      auto & kb = KnowledgeBase::getInstance();

      {%- for type in types %}
      std::vector<InstantiatedParameter> {{type}}s;
      {%- endfor %}

      for (const auto object : kb.objects){
          {%- for type in types %}
          if (object.type == "{{type}}"){
              {{type}}s.push_back(object);
          }
          {%- endfor %}
      }

      {% for args in func_signatures %}
      for (auto [key, func] : pred_update_map_{{loop.index}}_){
          {%- for arg in args %}
          for (auto {{arg}} : {{arg}}s ) {
          {%- endfor %}
              InstantiatedPredicate pred = {key, { {%- for arg in args %}{{arg}}{%- if loop.index < loop.length %}, {% endif -%}{%- endfor %} } };
              TRUTH_VALUE old_val;
              if (kb.knownPredicates.find(pred) != kb.knownPredicates.end()){
                  old_val = TRUTH_VALUE::TRUE;
              } else if(kb.unknownPredicates.find(pred) != kb.unknownPredicates.end()){
                  old_val = TRUTH_VALUE::UNKNOWN;
              } else{
                  old_val = TRUTH_VALUE::FALSE;
              }
              func(old_val{% for arg in args %}, {{arg}}.name{%- endfor %});
          {% for arg in args %} }
          {% endfor -%}
          }
      {% endfor %}
  }

  {% for pred in predicates %}void set_{{pred.name}}(std::function<TRUTH_VALUE(TRUTH_VALUE{% for param in pred.parameters %}, {{param.type}} {{param.name}}{% endfor %})> & func){
        set_pred_update_map("{{pred.name}}", func);
  }
  {% endfor -%}

  private:
    {% for args in func_signatures -%}
    void set_pred_update_map(const std::string name, const std::function<TRUTH_VALUE(TRUTH_VALUE{% for type in args %}, {{type}}{% endfor %})> & func){
        pred_update_map_{{loop.index}}_[name] = func;
    }
    {% endfor -%}
    {% for args in func_signatures -%}
  std::unordered_map<std::string, std::vector<std::function<TRUTH_VALUE(TRUTH_VALUE{% for type in args %}, {{type}}{% endfor %})>> > pred_update_map_{{loop.index}}_;
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
