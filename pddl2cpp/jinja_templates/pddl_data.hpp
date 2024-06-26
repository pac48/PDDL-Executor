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
        unsigned char data[{{size_kb_data}}];
        unsigned int depth = 0;
        unsigned int action = 0;
        int goal_dist = -1;
        char valid = 0;
        KBState* associated_state = {};
        std::vector<KBState*> children = {};
        std::vector<KBState*> parents = {};
//        std::vector<std::string> action_name_; //TODO remove after done debugging
//        unsigned int subgraph = 0;

        bool operator==(const KBState & other) const{
            return std::memcmp(data, other.data, {{size_kb_data}}) == 0; // && subgraph == other.subgraph;
        }
//        bool operator==(const KBState* & other) const{
//            return std::memcmp(data, other->data, {{size_kb_data}}) == 0; // && subgraph == other.subgraph;
//        }
    };

    std::function<std::pair<bool,bool>(KBState &)> create_constraint(const Constraint& constraint, std::unordered_map<std::string, unsigned int> func_map){
        if (constraint.constraint == CONSTRAINTS::ONEOF){
            std::vector<unsigned int> inds;
            for (const auto& c : constraint.condition.conditions){
                if (auto pred = std::get_if<InstantiatedPredicate>(&c)){
                    std::stringstream ss;
                    ss << *pred;
                    inds.push_back(func_map[ss.str()]);
                } else {
                    throw std::runtime_error("ONEOF constraint must only contain predicates");
                }
            }
            return [inds](KBState & state){
                unsigned int num_unknowns = 0;
                unsigned int num_true = 0;
                for (const auto& ind : inds){
                    num_true += state.data[ind]==1;
                    num_unknowns += state.data[ind]==2;
                }
                if (num_true > 1 || (num_true == 0 && num_unknowns == 0)) {
                    return std::make_pair(false, false);
                }

                bool modified = false;
                if (num_true==1 && num_unknowns > 0){
                    for (const auto& ind : inds){
                        state.data[ind] = state.data[ind]*(state.data[ind]==1);
                    }
                    modified = true;
                }
                if (num_true==0 && num_unknowns==1){
                    for (const auto& ind : inds){
                        state.data[ind] = state.data[ind]==1 || state.data[ind]==2;
                    }
                    modified = true;
                }
                return std::make_pair(true, modified);
            };

        } else if (constraint.constraint == CONSTRAINTS::OR){
            assert(constraint.condition.conditions.size() == 2);
            std::vector<std::pair<unsigned int, unsigned char>> conds;
            for (const auto & c  : constraint.condition.conditions) {
                if (auto pred = std::get_if<InstantiatedPredicate>(&c)){
                    std::stringstream ss;
                    ss << *pred;
                    conds.push_back({func_map[ss.str()], 1});
                } else {
                    auto cond = std::get<InstantiatedCondition>(c);
                    assert(cond.op == OPERATION::NOT);
                    assert(cond.conditions.size()==1);
                    std::stringstream ss;
                    ss << std::get<InstantiatedPredicate>(cond.conditions[0]);
                    conds.push_back({func_map[ss.str()], 0});
                }
            }
            assert(conds.size()==2);

            return [conds](KBState & state){
                const auto & cond_1 = conds[0];
                const auto & cond_2 = conds[1];
                if (state.data[cond_1.first] != cond_1.second && state.data[cond_1.first] != 2
                    && state.data[cond_2.first] != cond_2.second && state.data[cond_2.first] != 2){
                    assert(0);
//                    return std::make_pair(false,false);
                }
//                if (state.data[cond_1.first] == cond_1.second && state.data[cond_1.first] != 2
//                    && state.data[cond_2.first] == cond_2.second && state.data[cond_2.first] != 2){
//                    return std::make_pair(false,false);
//                }
                bool modified = false;
                if (state.data[cond_1.first] == 2 && state.data[cond_2.first] != 2 && state.data[cond_2.first] != cond_2.second){
                    state.data[cond_1.first] = cond_1.second;
                    modified = true;
                } else if (state.data[cond_2.first] == 2 && state.data[cond_1.first] != 2 && state.data[cond_1.first] != cond_1.second){
                    state.data[cond_2.first] = cond_2.second;
                    modified = true;
                }

                if (state.data[cond_1.first] == 2 && state.data[cond_2.first] != 2 && state.data[cond_2.first] == cond_2.second){
                    state.data[cond_1.first] = 1-cond_1.second;
                    modified = true;
                } else if (state.data[cond_2.first] == 2 && state.data[cond_1.first] != 2 && state.data[cond_1.first] == cond_1.second){
                    state.data[cond_2.first] = 1-cond_2.second;
                    modified = true;
                }

                return std::make_pair(true, modified);
            };
        } else{
            throw std::runtime_error("Constraint not supported");
        }
    }

    std::function<bool(const KBState &)> create_goal(const InstantiatedCondition& goal, std::unordered_map<std::string, unsigned int> func_map){
        std::vector<std::pair<unsigned int, unsigned char>> conds;
        for (const auto & c  : goal.conditions){
            if (auto pred = std::get_if<InstantiatedPredicate>(&c)){
                std::stringstream ss;
                ss << *pred;
                conds.push_back({func_map[ss.str()], 1});
            } else{
                auto cond = std::get<InstantiatedCondition>(c);
                assert(cond.op == OPERATION::NOT);
                assert(cond.conditions.size()==1);
                std::stringstream ss;
                ss << std::get<InstantiatedPredicate>(cond.conditions[0]);
                conds.push_back({func_map[ss.str()], 0});
            }

        }

        return [conds](const KBState & state){
            bool val = true;
            for (const auto& c : conds ){
                val &= state.data[c.first]==c.second;
            }
            return val;
        };

    }


    std::tuple<KBState, std::array<KBState, {{actions|length + 2*observe_actions|length}}>,
    std::vector<std::function<std::pair<bool,bool>(KBState &)>>, std::function<bool (const KBState&)>> initialize_problem(const std::string& problem_str){
        pddl_lib::KBState state{};
        std::array<KBState, {{actions|length + 2*observe_actions|length}}> new_states{};
        memset(new_states.data(), 0, sizeof(new_states));
        auto problem = pddl_lib::parse_problem(problem_str).value();
        std::unordered_map<std::string, unsigned int> func_map;
        std::function<bool (const KBState&)> check_goal;
        std::vector<std::function<std::pair<bool,bool> (KBState &)>> constraints{};
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

        check_goal = create_goal(problem.goal, func_map);
        return {state, new_states, constraints, check_goal};
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

std::string get_state_string(const KBState & state){
    std::stringstream ss;
{% for val in indexers %}
if (state.data[{{loop.index -1 }}]==1){
    ss << "{{val}}\n";
}
if (state.data[{{loop.index -1 }}]==2){
    ss << "unknown({{val}})\n";
}
{%- endfor %}

return ss.str();
}

} // indexers
} // pddl_lib

namespace std {
    template<>
    struct equal_to<pddl_lib::KBState*>{
        bool operator()(const pddl_lib::KBState * state1, const pddl_lib::KBState *state2) const {
            if (state1->associated_state == nullptr && state2->associated_state == nullptr){
                return  state1->action == state2->action && std::memcmp(state1->data, state2->data, {{size_kb_data}}) == 0;
            }
            else if ((state1->associated_state == nullptr && state2->associated_state != nullptr)
                       || (state1->associated_state != nullptr && state2->associated_state == nullptr) ){
                return false;
            } else{
                return state1->action == state2->action && (std::memcmp(state1->data, state2->data, {{size_kb_data}}) == 0)
                       && (std::memcmp(state1->associated_state->data, state2->associated_state->data, {{size_kb_data}}) == 0);
            }
        }
    };

    template<>
    struct hash<pddl_lib::KBState*> {
        std::size_t operator()(const pddl_lib::KBState* obj) const {
            std::size_t* data = (std::size_t*) obj->data;
//            std::size_t hashValue = 0;
//            std::hash<std::size_t> intHash;
//            for (size_t i = 0; i < {{size_kb_data}}/8; ++i) {
//                auto val = intHash(data[i]);
//                hashValue = hashValue ^ val;
//            }
            constexpr std::size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
            constexpr std::size_t FNV_PRIME = 1099511628211ULL;

            std::size_t hashValue = FNV_OFFSET_BASIS;
            for (std::size_t i = 0; i < {{size_kb_data}}/8; ++i) {
                hashValue ^= data[i];
                hashValue *= FNV_PRIME;
            }

            return hashValue;
        }
    };
}

namespace pddl_lib {

{% for action in actions %}
namespace {{action.name}} {
    inline bool check_preconditions(const KBState & state) {
        return {{action.pre}}
    }
    inline void apply_effect(KBState & state) {
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
    void apply_observe(KBState & state1, KBState & state2, const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints) {
        {{action.observe}}
        apply_observe_debug_1();
        bool modified;
        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state1);
                modified |= modified_i;
                state1.valid = valid;
                if (!valid){
                    state2.valid = false;
                    return;
                }
            }
        } while (modified);

        do {
            modified = false;
            for (auto &constraint: constraints) {
                auto [valid, modified_i] = constraint(state2);
                modified |= modified_i;
                state2.valid = valid;
                if (!valid){
                    state1.valid = false;
                    return;
                }
            }
        } while (modified);
    }
}
{% endfor %}

void expand(const KBState& cur_state, std::array<KBState, {{actions|length + 2*observe_actions|length}}> & new_states,
            const std::vector<std::function<std::pair<bool,bool>(KBState &)>> & constraints={}){
    int num = 0;
    {% for action in observe_actions %}
    if ({{action.name}}::check_preconditions(cur_state)){
        std::memcpy(new_states[num].data, cur_state.data, {{size_kb_data}});
        new_states[num].depth = cur_state.depth;
        new_states[num].action = cur_state.action;
        new_states[num].goal_dist = cur_state.goal_dist;
        new_states[num].associated_state = cur_state.associated_state;
        std::memcpy(new_states[num+1].data, cur_state.data, {{size_kb_data}});
        new_states[num+1].depth = cur_state.depth;
        new_states[num+1].action = cur_state.action;
        new_states[num+1].goal_dist = cur_state.goal_dist;
        new_states[num+1].associated_state = cur_state.associated_state;

        new_states[num].children.clear();
        new_states[num].parents.clear();
        new_states[num+1].children.clear();
        new_states[num+1].parents.clear();
        new_states[num].action = {{actions|length + loop.index - 1}};
        new_states[num+1].action = {{actions|length + loop.index - 1}};
    //    new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
    //    new_states[num+1].action_name_ = pddl_lib::indexers::get_action_string(new_states[num+1].action);
        {{action.name}}::apply_effect(new_states[num]);
        {{action.name}}::apply_effect(new_states[num+1]);
        {{action.name}}::apply_observe(new_states[num], new_states[num+1], constraints);
        new_states[num].associated_state = &new_states[num+1];
        new_states[num+1].associated_state = &new_states[num];
        num += 2;
    }
    {% endfor %}

    {% for action in actions %}
    if ({{action.name}}::check_preconditions(cur_state)){
            std::memcpy(new_states[num].data, cur_state.data, {{size_kb_data}});
            new_states[num].depth = cur_state.depth;
            new_states[num].action = cur_state.action;
            new_states[num].goal_dist = cur_state.goal_dist;
            new_states[num].associated_state = cur_state.associated_state;
            new_states[num].action = {{ loop.index - 1}};
            new_states[num].children.clear();
            new_states[num].parents.clear();
    //        new_states[num].action_name_ = pddl_lib::indexers::get_action_string(new_states[num].action);
            new_states[num].associated_state = nullptr;
            {{action.name}}::apply_effect(new_states[num]);
            new_states[num].valid = 1;
            num++;
    }
    {% endfor %}

}

} // pddl_lib