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

{% for type in types %}class {{type}} : public std::string {
public:
    // Inherit constructors from std::string
    using std::string::string;
    // Prevent implicit conversion to std::string
    explicit {{type}}(const std::string& str) : std::string(str) {}
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

      {% for pred in predicates %}
        {
          {%- for param in pred.parameters %}
            for (auto {{param.type}}_instance : {{param.type}}_instances ) {
          {%- endfor %}
              InstantiatedPredicate pred = {"{{pred.name}}", { {%- for param in pred.parameters %}{{param.type}}_instance{%- if loop.index < loop.length %}, {% endif -%}{%- endfor %} } };
              TRUTH_VALUE old_val;
              if (kb.knownPredicates.concurrent_find(pred)){
                  old_val = TRUTH_VALUE::TRUE;
              } else if(kb.unknownPredicates.concurrent_find(pred)){
                  old_val = TRUTH_VALUE::UNKNOWN;
              } else{
                  old_val = TRUTH_VALUE::FALSE;
              }
            auto new_val = {{pred.name}}(old_val{% for param in pred.parameters %}, {{param.type}}({{param.type}}_instance.name){%- endfor %});
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
          {%- for param in pred.parameters %}
             }
          {%- endfor %}
        }
      {%- endfor %}

  }

    {% for pred in predicates %}virtual TRUTH_VALUE {{pred.name}}(TRUTH_VALUE val{% for param in pred.parameters %}, {{param.type}} {{param.name}}{% endfor %}) const {
        return val;
    }
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