#include <cstdio>
#include <unordered_map>
#include <queue>
#include <iostream>
#include "array"
#include "pddl_problem.hpp"


void disable_subtree(std::vector<pddl_lib::KBState> &open_list, pddl_lib::KBState &cur_state) {
    cur_state.reached_goal = -1;
    for (auto child = cur_state.children_begin; child < cur_state.children_end; child++) {
        if (child > 0) {
            disable_subtree(open_list, open_list[child]);
        }
    }
}

bool goal_propagate(std::vector<pddl_lib::KBState> &open_list, pddl_lib::KBState &goal_state, unsigned int goal_ind) {
    if (goal_ind == 0) {
        return true;
    }
    if (goal_state.associated_state != 0 && open_list[goal_state.associated_state].reached_goal == 0) {
        return false;
    }
    auto &goal_parent = open_list[goal_state.parent];
    goal_parent.reached_goal = 1;
    for (auto child = goal_parent.children_begin; child < goal_parent.children_end; child++) {
        if (child != goal_ind && child > 0) {
            disable_subtree(open_list, open_list[child]);
        }
    }
    return goal_propagate(open_list, goal_parent, goal_state.parent);
}


int main(int argc, char **argv) {
    pddl_lib::KBState state{};
    memset(state.data, 0, sizeof(state.data));

    pddl_lib::indexers::robot_atpioneerhome(state) = 1;
    pddl_lib::indexers::door_locationdoor(state) = 1;
    // unknown
    pddl_lib::indexers::person_atnathandoor(state) = 2;
    pddl_lib::indexers::person_decides_to_go_outside_1(state) = 2;
    pddl_lib::indexers::person_decides_to_go_outside_2(state) = 2;
    pddl_lib::indexers::person_decides_to_return_1(state) = 2;
    pddl_lib::indexers::person_decides_to_return_2(state) = 2;
    pddl_lib::indexers::person_decides_to_go_to_bed_1(state) = 2;
    pddl_lib::indexers::person_decides_to_go_to_bed_2(state) = 2;
    pddl_lib::indexers::person_goes_to_bed_after_return_1(state) = 2;
    pddl_lib::indexers::person_goes_to_bed_after_return_2(state) = 2;

    std::array<pddl_lib::KBState, 103> new_states{};
    std::array<int, 103> valid{};
    memset(valid.data(), 0, sizeof(valid));
    memset(new_states.data(), 0, sizeof(new_states));

    std::unordered_set<pddl_lib::KBState> close_list;


    int success_index = pddl_lib::indexers::success_index();
    std::vector<pddl_lib::KBState> open_list;
    open_list.push_back(state);
    auto counter = 0ul;
    while (open_list.size() > counter) {
        memset(valid.data(), 0, sizeof(valid));
        pddl_lib::KBState cur_state = open_list[counter];
        if (cur_state.data[success_index] == 1) {
            cur_state.reached_goal = 1;
            if (goal_propagate(open_list, cur_state, counter)) {
                std::cout << "fond plan" << std::endl;
                break;
            }
        }
        close_list.insert(cur_state); // TODO it seems like the goal state should not be in the closes list

        pddl_lib::expand(cur_state, new_states, valid);

        if ((counter % 1000) == 0) {
            std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
                      << open_list.size() << std::endl;
        }
        cur_state.children_begin = open_list.size();
        auto i = 0ul;
        while (i < valid.size()) {
            if (valid[i] == 1) {
                if (close_list.find(new_states[i]) == close_list.end()) {
                    if (new_states[i].associated_state != 0) {
                        new_states[i + 1].associated_state = open_list.size();
                        new_states[i].associated_state = open_list.size() + 1;
                        new_states[i].parent = counter;
                        open_list.push_back(new_states[i]);
                        i++;
                        new_states[i].parent = counter;
                        open_list.push_back(new_states[i]);
                    } else {
                        new_states[i].parent = counter;
                        open_list.push_back(new_states[i]);
                    }
                } else {
                    if (new_states[i].data[success_index] == 1) {
                        goal_propagate(open_list, new_states[i], counter);
                    }
                }
            }
            i++;
        }
        cur_state.children_end = open_list.size() - 1;
        open_list[counter] = cur_state;
        counter++;
    }

    std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
              << open_list.size() << std::endl;
    return 0;
}
