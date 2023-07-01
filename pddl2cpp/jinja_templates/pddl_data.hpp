#include <cstddef>
#include <unordered_set>
#include <array>
#include <tuple>
#include <cstring>
#include <stdexcept>
#include "pddl_parser/pddl_parser.hpp"

#pragma once

namespace pddl_lib {

    unsigned int subgraph_counter = 0;
    struct KBState {
        KBState(){
            memset(data, 0, sizeof(data));
        }
        unsigned int depth = 0;
        unsigned int action = 0;
        unsigned int associated_state = 0;
        unsigned int children_begin = 0;
        unsigned int children_end = 0;
        unsigned int parent;
        char reached_goal = 0;
        unsigned char data[{{size_kb_data}}];
        unsigned int subgraph = 0;

        bool operator==(const KBState & other) const{
            return std::memcmp(data, other.data, {{size_kb_data}}) == 0 && subgraph == other.subgraph;
        }
    };

    std::function<bool(KBState &)> create_constraint(const Constraint& constraint, std::unordered_map<std::string, unsigned int> func_map){
        if (constraint.constraint == CONSTRAINTS::ONEOF){
            assert(constraint.condition.conditions.empty());
            std::vector<unsigned int> inds;
            for (const auto& pred : constraint.condition.predicates){
                std::stringstream ss;
                ss << pred;
                inds.push_back(func_map[ss.str()]);
            }
            return [inds](KBState & state){
                unsigned int num_unknowns = 0;
                unsigned int num_true = 0;
                for (const auto& ind : inds){
                    num_true += state.data[ind]==1;
                    num_unknowns += state.data[ind]==2;
                }
//                assert(num_true < 2);
                if (num_true > 1 || (num_true == 0 && num_unknowns == 0)) {
                    return false;
                }

                if (num_true==1 && num_unknowns > 0){
                    for (const auto& ind : inds){
                        state.data[ind] = state.data[ind]*(state.data[ind]==1);
                    }
                }
                if (num_true==0 && num_unknowns==1){
                    for (const auto& ind : inds){
                        state.data[ind] = state.data[ind]==1 || state.data[ind]==2;
                    }
                }
                return true;
            };

        } else if (constraint.constraint == CONSTRAINTS::OR_CONSTRAINT){
            assert(constraint.condition.conditions.size() == 2);
            std::vector<std::pair<unsigned int, unsigned char>> conds;
            for (const auto & pred  : constraint.condition.predicates){
                std::stringstream ss;
                ss << pred;
                conds.push_back({func_map[ss.str()], 1});
            }
            for (const auto & cond  : constraint.condition.conditions){
                assert(cond.op == OPERATION::NOT);
                assert(cond.predicates.size()==1);
                std::stringstream ss;
                ss << cond.predicates[0];
                conds.push_back({func_map[ss.str()], 0});
            }
            assert(conds.size()==2);

            return [conds](KBState & state){
                auto cond_1 = conds[0];
                auto cond_2 = conds[1];
                if(state.data[cond_1.first] == 2 && state.data[cond_2.first]==0){
                    state.data[cond_1.first] = cond_1.second;
                } else if (state.data[cond_1.first] == 0 && state.data[cond_2.first]==2){
                    state.data[cond_2.first] = cond_2.second;
                } else if (state.data[cond_1.first] == 0 && state.data[cond_2.first]==0){
//                    assert(0); TODO is this needed?
                    return false;
                }
                return true;
            };


        } else{
            throw std::runtime_error("Constraint not supported");
        }
    }


    std::tuple<KBState, std::array<KBState, {{actions|length + 2*observe_actions|length}}>, std::array<int, {{actions|length + 2*observe_actions|length}}>,
    std::vector<std::function<bool(KBState &)>>, std::function<bool (const KBState&)>> initialize_problem(const std::string& problem_str){
        pddl_lib::KBState state{};
        std::array<KBState, {{actions|length + 2*observe_actions|length}}> new_states{};
        std::array<int, {{actions|length + 2*observe_actions|length}}> valid{};
        memset(valid.data(), 0, sizeof(valid));
        memset(new_states.data(), 0, sizeof(new_states));
        auto problem = pddl_lib::parse_problem(problem_str).value();
        std::unordered_map<std::string, unsigned int> func_map;
        std::function<bool (const KBState&)> check_goal;
        std::vector<std::function<bool (KBState &)>> constraints{};
        {{problem_initialization}}
        for (const auto& pred : problem.init){
            std::stringstream ss;
            ss << pred;
            state.data[func_map.at(ss.str())] = 1;
        }
        for (const auto& pred : problem.unknowns){
            std::stringstream ss;
            ss << pred;
            state.data[func_map.at(ss.str())] = 2;
        }
        for (const auto& constraint : problem.constraints){
            constraints.push_back(create_constraint(constraint, func_map));
        }
        check_goal = [](const KBState state){return {{goal_condition}}; };

        return {state, new_states, valid, constraints, check_goal};
    }

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

            return hashValue ^ obj.subgraph;
        }
    };
}

namespace pddl_lib {

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

void apply_observe_debug_1(){
    int o = 0;
}
void apply_observe_debug_2(){
    int o = 0;
}
{% for action in observe_actions %}
namespace {{action.name}} {
bool check_preconditions(const KBState & state) {
    return {{action.pre}}
}
void apply_effect(KBState & state) {
    {{action.effect}}
}
void apply_observe(KBState & state1, KBState & state2, int & valid1, int & valid2, const std::vector<std::function<bool(KBState &)>> & constraints) {
    {{action.observe}}
    apply_observe_debug_1();
    for (auto &constraint: constraints) {
        valid1 = constraint(state1)*constraint(state2);
        valid2 = valid1;
//        bool none_true = true;
//        unsigned int num_unknown = 0;
//        unsigned int unknown_index = 0;
//        for (unsigned int i = 0ul; i < constraint.size(); i++) {
//            none_true &= (state2.data[i] != 1 || constraint[i] == 0);
//            num_unknown += (state2.data[i] == 2 && constraint[i] == 1);
//            unknown_index = std::max(unknown_index, i*(state2.data[i]==2)*(constraint[i] == 1));
//        }
//        if (num_unknown == 1 && none_true){
//            apply_observe_debug_2();
//            state2.data[unknown_index] = 1;
//        }
    }
}
}
{% endfor %}

void expand(const KBState& cur_state, std::array<KBState, {{actions|length + 2*observe_actions|length}}> & new_states, std::array<int, {{actions|length + 2*observe_actions|length}}> & valid,
            const std::vector<std::function<bool(KBState &)>> & constraints={}){
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
    valid[{{actions|length + 2*loop.index - 2}}] = 1;
    valid[{{actions|length + 2*loop.index - 2 + 1}}] = 1;
    {{action.name}}::apply_observe(new_states[{{actions|length + 2*loop.index - 2}}], new_states[{{actions|length + 2*loop.index - 2 + 1}}],
                                   valid[{{actions|length + 2*loop.index - 2}}], valid[{{actions|length + 2*loop.index - 2 + 1}}], constraints);
    new_states[{{actions|length + 2*loop.index - 2}}].associated_state = {{actions|length + 2*loop.index - 2 + 1}};
    new_states[{{actions|length + 2*loop.index - 2+1}}].associated_state = {{actions|length + 2*loop.index - 2}};
    subgraph_counter++;
    new_states[{{actions|length + 2*loop.index - 2}}].subgraph = subgraph_counter;
    new_states[{{actions|length + 2*loop.index - 2+1}}].subgraph = subgraph_counter;
}
{% endfor %}
}

} // pddl_lib