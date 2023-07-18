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
    using std::string::string;
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

      for (const auto object : kb.get_objects()){
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
              if (kb.find_predicate(pred)){
                  old_val = TRUTH_VALUE::TRUE;
              } else if(kb.find_unknown_predicate(pred)){
                  old_val = TRUTH_VALUE::UNKNOWN;
              } else{
                  old_val = TRUTH_VALUE::FALSE;
              }
            auto new_val = {{pred.name}}(old_val{% for param in pred.parameters %}, {{param.type}}({{param.type}}_instance.name){%- endfor %});
            if (new_val==TRUTH_VALUE::TRUE){
                kb.insert_predicate(pred);
                kb.erase_unknown_predicate(pred);
            } else if(new_val==TRUTH_VALUE::UNKNOWN){
                kb.erase_predicate(pred);
                kb.insert_unknown_predicate(pred);
            } else {
                kb.erase_predicate(pred);
                kb.erase_unknown_predicate(pred);
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

class ActionInterface {
public:
    {%- for name in action_names %}
    virtual BT::NodeStatus {{name.underscore}}(const InstantiatedAction & action){return BT::NodeStatus::SUCCESS;} // do nothing default
    {%- endfor %}
};

{{ action_classes }}

template<typename T>
BT::BehaviorTreeFactory create_tree_factory(){
    static_assert(
            std::is_base_of<ActionInterface, T>::value,
            "template is not derived from ActionInterface"
    );

    BT::BehaviorTreeFactory factory;
    {% for name in action_names %}
    factory.registerNodeType<{{name.qualified}}<T>>("{{name.underscore}}");
    {%- endfor %}

    return factory;
}


} // pddl_lib