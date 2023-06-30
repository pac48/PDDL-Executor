#include <cstddef>
#include <unordered_set>
#include <array>
#include <cstring>
#include <stdexcept>

#pragma once

namespace pddl_lib {

    struct KBState {
        unsigned int depth = 0;
        unsigned int action = 0;
        unsigned int associated_state = 0;
        unsigned int children_begin = 0;
        unsigned int children_end = 0;
        unsigned int parent;
        char reached_goal = 0;
        unsigned char data[{{size_kb_data}}];

        bool operator==(const KBState & other) const{
            return std::memcmp(data, other.data, {{size_kb_data}}) == 0;
        }
    };

namespace indexers {
{% for val in indexers %}
    unsigned char & {{val}}(KBState & state){
        return state.data[{{loop.index -1 }}];
    }
    int {{val}}_index(){
        return {{loop.index -1 }};
    }
{%- endfor %}

std::vector<std::string> get_action_string(unsigned int index){

{% for action in actions %}
if (index == {{loop.index -1}}){
    return {"{{action.base_name}}"{% for param in action.params %}, "{{param}}"{%- endfor %}};
}
{%- endfor %}

{% for action in observe_actions %}
if (index == {{actions|length + loop.index -1}}){
    return {"{{action.base_name}}"{% for param in action.params %}, "{{param}}"{%- endfor %}};
}
{%- endfor %}
    throw std::runtime_error("index not in action set");
}

} // indexers
} // pddl_lib

namespace std {
    template<>
    struct hash<pddl_lib::KBState> {
        std::size_t operator()(const pddl_lib::KBState &obj) const {
            std::size_t hashValue = 0;
            std::size_t* data = (std::size_t*) obj.data;
            constexpr std::size_t prime = 1099511628211;  // A large prime number

            for (size_t i = 0; i < {{size_kb_data}}/64; ++i) {
                hashValue = (hashValue * prime) ^ data[i];
            }
            for (size_t i = 64*({{size_kb_data}}/64); i < {{size_kb_data}}; ++i) {
                hashValue = (hashValue * prime) ^ obj.data[i];
            }

            return hashValue;
        }
    };
}

namespace pddl_lib {

    void apply_observe_debug(){
        int o = 0;
    }

{% for action in actions %}
namespace {{action.name}} {
    bool check_preconditions(const KBState & state) {
        return {{action.pre}}
    }
    void apply_effect(KBState & state) {
        {{action.effect}}
    }
}
{% endfor %}

{% for action in observe_actions %}
namespace {{action.name}} {
bool check_preconditions(const KBState & state) {
    return {{action.pre}}
}
void apply_effect(KBState & state) {
    {{action.effect}}
}
void apply_observe(KBState & state1, KBState & state2) {
    {{action.observe}}
    apply_observe_debug();
}
}
{% endfor %}

void expand(const KBState& cur_state, std::array<KBState, {{actions|length + 2*observe_actions|length}}> & new_states, std::array<int, {{actions|length + 2*observe_actions|length}}> & valid){
{% for action in actions %}
if ({{action.name}}::check_preconditions(cur_state)){
        new_states[{{ loop.index - 1}}] = cur_state;
        new_states[{{ loop.index - 1}}].action = {{ loop.index - 1}};
        new_states[{{ loop.index - 1}}].associated_state = 0;
        {{action.name}}::apply_effect(new_states[{{ loop.index - 1}}]);
        valid[{{ loop.index - 1}}] = 1;
}
{% endfor %}

{% for action in observe_actions %}
if ({{action.name}}::check_preconditions(cur_state)){
    new_states[{{actions|length + 2*loop.index - 2}}] = cur_state;
    new_states[{{actions|length + 2*loop.index - 2 + 1}}] = cur_state;
    new_states[{{actions|length + 2*loop.index - 2}}].action = {{actions|length + loop.index - 1}};
    new_states[{{actions|length + 2*loop.index - 2 + 1}}].action = {{actions|length + loop.index - 1}};
    {{action.name}}::apply_effect(new_states[{{actions|length + 2*loop.index - 2}}]);
    {{action.name}}::apply_effect(new_states[{{actions|length + 2*loop.index - 2 + 1}}]);
    {{action.name}}::apply_observe(new_states[{{actions|length + 2*loop.index - 2}}], new_states[{{actions|length + 2*loop.index - 2 + 1}}]);
    valid[{{actions|length + 2*loop.index - 2}}] = 1;
    valid[{{actions|length + 2*loop.index - 2 + 1}}] = 1;
    new_states[{{actions|length + 2*loop.index - 2}}].associated_state = {{actions|length + 2*loop.index - 2 + 1}};
    new_states[{{actions|length + 2*loop.index - 2+1}}].associated_state = {{actions|length + 2*loop.index - 2}};
}
{% endfor %}
}

} // pddl_lib