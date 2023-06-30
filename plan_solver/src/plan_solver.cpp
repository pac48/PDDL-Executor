#include <unordered_map>
#include <queue>
#include <iostream>
#include <chrono>
#include <cassert>
#include <sstream>
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

bool goal_propagate(std::vector<pddl_lib::KBState> &open_list, unsigned int goal_ind) {
    if (goal_ind == 0) {
        return true;
    }
    pddl_lib::KBState &goal_state = open_list[goal_ind];
    if (goal_state.associated_state != 0 && open_list[goal_state.associated_state].reached_goal != 1) {
        return false;
    }
    auto &goal_parent = open_list[goal_state.parent];
    goal_parent.reached_goal = 1;
    for (auto child = goal_parent.children_begin; child < goal_parent.children_end; child++) {
        if (child > 0 && child != goal_ind && child != goal_state.associated_state) {
            disable_subtree(open_list, open_list[child]);
        }
    }
    return goal_propagate(open_list, goal_state.parent);
}

void print_plan(const std::vector<pddl_lib::KBState> &open_list, unsigned int goal_ind) {
    assert(open_list[goal_ind].reached_goal);

    std::unordered_map<int, std::vector<pddl_lib::KBState> > map;
    std::unordered_map<int, int> depth_map; // tack the number of values inserted at each depth
    std::queue<unsigned int> q;
    for (auto ind = open_list[0].children_begin; ind < open_list[0].children_end; ind++) {
        auto &child = open_list[ind];
        if (child.reached_goal == 1) {
            q.emplace(ind);
        }
    }
    while (!q.empty()) {
        auto &state = open_list[q.front()];
//        if (depth_map.find(state.depth+1) == depth_map.end()) {
//            depth_map[state.depth+1] = 0;
//        }
//        depth_map[state.depth+1]++;


        map[state.depth].push_back(open_list[q.front()]);

        q.pop();
        int counter = 0;
        for (auto ind = state.children_begin; ind < state.children_end; ind++) {
            auto &child = open_list[ind];
            if (child.reached_goal == 1) {
                q.emplace(ind);
                counter++;
            }
        }

        assert(counter == 0 || counter == 1 || counter == 2);
    }

    std::cout << "-------------------------------------------------\n";
    for (int depth = 1; depth <= map.size(); depth++) {
        const auto &state_set = map[depth];
        int ind = 0;
        int next_ind = 0;
        for (const auto &state: state_set) {
            auto name_vec = pddl_lib::indexers::get_action_string(state.action);
            std::stringstream ss;
            for (const auto &val: name_vec) {
                ss << val << " ";
            }
            auto action_name = ss.str();
            if (state.children_end - state.children_begin <= 0) {
                std::cout << depth - 1 << "||" << ind << " --- " << action_name << "--- SON: " << depth << "||" << -1
                          << "\n";
            } else {
                if (state.associated_state == 0) {
                    std::cout << depth - 1 << "||" << ind << " --- " << action_name << "--- SON: " << depth << "||"
                              << next_ind
                              << "\n";
                    next_ind++;
                } else if (state.associated_state < open_list[state.associated_state].associated_state) {
                    std::cout << depth - 1 << "||" << ind << " --- " << action_name << "--- TRUESON: " << depth << "||"
                              << next_ind << " --- FALSESON: " << depth << "||"
                              << next_ind + 1
                              << "\n";
                    next_ind += 2;
                }

            }
            ind++;
        }
        std::cout << "-------------------------------------------------\n";
    }

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


    unsigned int max_depth = 0;
    auto start = std::chrono::high_resolution_clock::now();

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
        if (open_list[counter].reached_goal == -1) {
            counter++;
            continue;
        }
        if (open_list[counter].reached_goal == 1) {
            assert(0);
            counter++;
            continue;
        }

        memset(valid.data(), 0, sizeof(valid));
        if (open_list[counter].data[success_index] == 1) {
            open_list[counter].reached_goal = 1;
            if (goal_propagate(open_list, counter)) {
                std::cout << "fond plan" << std::endl;
                print_plan(open_list, counter);
                break;
            } else {
                counter++;
                continue;
            }
        }
        close_list.insert(open_list[counter]); // TODO it seems like the goal state should not be in the closes list

        pddl_lib::expand(open_list[counter], new_states, valid);

        if ((counter % 1000000) == 0) {
            std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
                      << open_list.size() << std::endl;
        }
        open_list[counter].children_begin = open_list.size();
        auto i = 0ul;
        while (i < valid.size()) {
            if (valid[i] == 1) {
                if (close_list.find(new_states[i]) == close_list.end()) {
                    if (new_states[i].associated_state != 0) {
                        new_states[i + 1].associated_state = open_list.size();
                        new_states[i].associated_state = open_list.size() + 1;
                        new_states[i].depth = open_list[counter].depth + 1;
                        new_states[i].parent = counter;
                        open_list.push_back(new_states[i]);
                        i++;
                        new_states[i].depth = open_list[counter].depth + 1;
                        new_states[i].parent = counter;
                        open_list.push_back(new_states[i]);
                    } else {
                        new_states[i].depth = open_list[counter].depth + 1;
                        new_states[i].parent = counter;
                        open_list.push_back(new_states[i]);
                    }
                    if (new_states[i].depth > max_depth) {
                        max_depth = new_states[i].depth;
                        std::cout << "max_depth: " << max_depth << std::endl;
                    }
                } else {
                    if (new_states[i].data[success_index] == 1) {
                        assert(0);
                    }
                }
            }
            i++;
        }
        open_list[counter].children_end = open_list.size(); // exclusive
        counter++;
    }

    std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
              << open_list.size() << std::endl;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken by function: "
              << duration.count() << " milliseconds" << std::endl;


    return 0;
}
