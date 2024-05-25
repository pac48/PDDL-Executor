#include "array"
#include <fstream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <list>
#include <algorithm>

#include "pddl_problem.hpp"


constexpr int CHUNK_SIZE = 128 * 128 * 16;
struct Chunk {
    std::array<pddl_lib::KBState, CHUNK_SIZE> memory;
};

class OpenList {
private:
    size_t size_ = 0;
    std::list<Chunk> list_;

    Chunk &get_chunk(size_t index) {
        if (index >= list_.size() * CHUNK_SIZE) {
            throw std::runtime_error("index out of bounds");
        }
        auto it = list_.begin();
        while (index >= CHUNK_SIZE && it != list_.end()) {
            index -= CHUNK_SIZE;
            it++;
        }
        return *it;
    }

    [[nodiscard]] const Chunk &get_chunk(size_t index) const {
        if (index >= list_.size() * CHUNK_SIZE) {
            throw std::runtime_error("index out of bounds");
        }
        auto it = list_.begin();
        while (index >= CHUNK_SIZE && it != list_.end()) {
            index -= CHUNK_SIZE;
            it++;
        }
        return *it;
    }

public:

    const pddl_lib::KBState &operator[](size_t index) const {
        return get_chunk(index).memory[index % CHUNK_SIZE];
    }

    pddl_lib::KBState &operator[](size_t index) {
        return get_chunk(index).memory[index % CHUNK_SIZE];
    }

    void push_back(pddl_lib::KBState state) {
        if (size_ == list_.size() * CHUNK_SIZE) {    // need new chunk
            list_.emplace_back();
        }
//        std::cout << size_ <<"\n";
        Chunk &chunk = list_.back();
        chunk.memory[size_ % CHUNK_SIZE] = state;
        size_++;
    }

    pddl_lib::KBState &back() {
        Chunk &chunk = list_.back();
        return chunk.memory[(size_ - 1) % CHUNK_SIZE];
    }

    size_t size() {
        return size_;
    }

};

std::vector<pddl_lib::KBState *> get_best_child(const pddl_lib::KBState *state) {
    if (state->children.empty()) {
        return {};
    }
    int best = 99999;
    int best_ind = -1;
    for (auto i = 0; i < state->children.size(); i++) {
        auto &child = state->children[i];
        if (child->goal_dist != -1 && child->goal_dist < best && child->associated_state == nullptr) {
            best_ind = i;
            best = child->goal_dist;
        }
        if (child->goal_dist != -1 && child->associated_state != nullptr && child->associated_state->goal_dist != -1 &&
            std::max(child->goal_dist, child->associated_state->goal_dist) < best) {
            best = std::max(child->goal_dist, child->associated_state->goal_dist);
            best_ind = i;
        }
    }
    if (state->children[best_ind]->associated_state == nullptr) {
        return {state->children[best_ind]};
    } else {
        return {state->children[best_ind], state->children[best_ind]->associated_state};
    }
}

void print_plan(const OpenList &open_list) {
    assert(open_list[0].goal_dist != -1);

    std::unordered_map<unsigned int, std::vector<pddl_lib::KBState *> > map;
    std::unordered_map<int, int> depth_map; // tack the number of values inserted at each depth
    std::queue<std::pair<unsigned int, pddl_lib::KBState *>> q;
    std::unordered_set<pddl_lib::KBState *> close_list;
    std::vector<pddl_lib::KBState *> best_children = get_best_child(&open_list[0]);
    for (const auto &child: best_children) {
        assert(child->goal_dist != -1);
        q.emplace(1, child);
    }
    while (!q.empty()) {
        auto state = q.front().second;
        auto depth = q.front().first;
        q.pop();
//        if (close_list.find(state) != close_list.end()) {
//            continue;
//        }
//        close_list.insert(state);

        best_children = get_best_child(state);
        for (const auto &child: best_children) {
            assert(child->goal_dist != -1);
            q.emplace(depth + 1, child);
        }

        map[depth].push_back(state);
    }
    std::stringstream ss;

    ss << "-------------------------------------------------\n";
    for (int depth = 1; depth <= map.size(); depth++) {
        const auto &state_set = map[depth];
        int ind = 0;
        int next_ind = 0;
        for (const auto &state: state_set) {
            auto name_vec = pddl_lib::indexers::get_action_string(state->action);
            std::stringstream ss_local;
            for (const auto &val: name_vec) {
                ss_local << val << " ";
            }
            auto action_name = ss_local.str();
            if (state->children.size() == 0) {
                ss << depth - 1 << "||" << ind << " --- " << action_name << "--- SON: " << depth << "||" << -1
                   //                          << " DEBUG: " << state->goal_dist //TODO remove
                   << "\n";
            } else {
                if (state->associated_state == nullptr) {
                    ss << depth - 1 << "||" << ind << " --- " << action_name << "--- SON: " << depth << "||"
                       << next_ind
                       //                              << " DEBUG: " << state->goal_dist //TODO remove
                       << "\n";
                    next_ind++;
                } else if (state->associated_state > state->associated_state->associated_state) {
                    ss << depth - 1 << "||" << ind << " --- " << action_name << "--- TRUESON: " << depth << "||"
                       << next_ind << " --- FALSESON: " << depth << "||"
                       << next_ind + 1
                       //                              << " DEBUG: " << state->goal_dist << " "<<state->associated_state->goal_dist //TODO remove
                       << "\n";
                    next_ind += 2;
                } else {
                    ind--;
                    assert(state->associated_state != nullptr);
//                    assert(1 == abs(((int) (state.associated_state)) - ((int) (open_list[state.associated_state].associated_state))));
                }
//          ss << pddl_lib::indexers::get_state_string(state);

            }
            ind++;
        }
        ss << "-------------------------------------------------\n";
    }

    std::cout << ss.str();

    std::ofstream outputFile("/tmp/plan_solver/plan.txt");
    outputFile << ss.rdbuf();

}

//bool goal_search(pddl_lib::KBState *state, std::unordered_set<pddl_lib::KBState *> &checked_states) {
//    if (checked_states.find(state) != checked_states.end()) {
////        assert(state->reached_goal == 0);
//        return state->reached_goal == 1;
//    }
//    if (state->reached_goal == 1) {
//        return true;
//    }
////    if (state->reached_goal == 1  && state->associated_state == nullptr) {
////        return true;
////    }
////    if (state->reached_goal == 1 && state->associated_state != nullptr && state->associated_state->reached_goal == 1) {
////        return true;
////    }
//    checked_states.insert(state);
//    for (const auto &child: state->children) {
//        bool reached = goal_search(child, checked_states);
//        if (reached && child->associated_state == nullptr) {
//            if (state->action_name_[0]=="Wait"){
//                int o = 0;
//            }
//            state->reached_goal = 1;
//            return true;
//        }
//        if (reached && child->associated_state != nullptr) {
////            checked_states.insert(child->associated_state);
//            reached = goal_search(child->associated_state, checked_states);
////            checked_states.erase(child->associated_state);
//            if (reached) {
//                state->reached_goal = 1;
//                return true;
//            }
//        }
//    }
//    checked_states.erase(state);
//    assert(state->reached_goal == false);
//    state->reached_goal = 0;
//
//    return false;
//}


//bool goal_propagate(pddl_lib::KBState *state, std::unordered_set<pddl_lib::KBState *> &checked_states) {
//    assert(state->goal_dist != -1);
//    if (state->depth == 0) {
//        return true;
//    }
//    if (state->associated_state != nullptr && state->associated_state->goal_dist == -1) {
//        return false;
//    }
////    for (auto parent: state->parents) {
////        if (parent->goal_dist != -1) {
////            int o = 0;
////        }
////    }
//    assert(!state->parents.empty());
//    int parent_dist;
//    if (state->associated_state != nullptr) {
//        parent_dist = std::max(state->goal_dist + 1, state->associated_state->goal_dist + 1);
//    } else {
//        parent_dist = state->goal_dist + 1;
//    }
////    if (parent_dist != state->goal_dist + 1) {
////        int o = 0;
////    }
//
//    bool reached = false;
//    for (auto parent: state->parents) {
//        if (parent->goal_dist == -1 || parent_dist < parent->goal_dist) {
//            parent->goal_dist = parent_dist;
//            reached |= goal_propagate(parent, checked_states);
////            if (reached) {
////                return true;
////            }
//        }
//    }
//    return reached;
//}
//
//void recurse(pddl_lib::KBState *const state, std::vector<pddl_lib::KBState *> &goals,
//             std::unordered_set<pddl_lib::KBState *> &checked) {
//    if (checked.find(state) != checked.end()) {
//        return;
//    }
//    checked.insert(state);
//    if (state->goal_dist != -1) {
//        goals.push_back(state);
//        return;
//    }
//    for (const auto &child: state->children) {
//        recurse(child, goals, checked);
//    }
//}


bool goal_propagate(const std::vector<pddl_lib::KBState *> &goals) {
//    assert(goal->goal_dist == 0);
    std::queue<pddl_lib::KBState *> q;
    std::unordered_set<pddl_lib::KBState *> close_list;
    for (const auto &g: goals) {
        q.emplace(g);
    }
    bool goal_reached = false;
    while (!q.empty()) {
        auto state = q.front();
        q.pop();

        assert(state->goal_dist != -1);
        if (state->depth == 0) {
            goal_reached = true;
            continue;
        }
        if (state->associated_state != nullptr && state->associated_state->goal_dist == -1) {
            continue;
        }

        assert(!state->parents.empty());
        int parent_dist;
        if (state->associated_state != nullptr) {
            parent_dist = std::max(state->goal_dist + 1, state->associated_state->goal_dist + 1);
        } else {
            parent_dist = state->goal_dist + 1;
        }

        for (auto parent: state->parents) {
            auto it = close_list.find(parent);
            if (it == close_list.end() || (it != close_list.end() && (*it)->goal_dist > parent_dist)) {
                parent->goal_dist = parent_dist;
                q.emplace(parent);
                close_list.insert(parent);
            }
        }
    }
    return goal_reached;
}

bool goal_search(std::vector<pddl_lib::KBState *> &goals) {
    std::unordered_set<pddl_lib::KBState *> checked;
//    recurse(state, goals, checked);
    std::sort(goals.begin(), goals.end(), [](const pddl_lib::KBState *a, const pddl_lib::KBState *b) {
        return a->depth < b->depth;
    });
    std::unordered_set<pddl_lib::KBState *> checked_states;
    bool valid = false;
//    for (const auto &goal: goals) {
    if (goal_propagate(goals)) {
        valid = true; //TODO maybe should continue checking?
    }
//    }

    return valid;
}

std::string convert_to_bt(const pddl_lib::Domain &domain);

int main(int argc, char **argv) {
    auto pddl_file = argv[1];
    std::ifstream pddl_file_stream(pddl_file);
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string pddl_str = ss.str();
    auto [init_state, new_states, constraints, check_goal] = pddl_lib::initialize_problem(pddl_str);

    auto start = std::chrono::high_resolution_clock::now();
    unsigned int max_depth = 0;
    bool plan_found = false;
    int num_goals = 0;

    std::unordered_set<pddl_lib::KBState *> close_list;
    std::vector<pddl_lib::KBState *> goals;
    std::unordered_set<unsigned int> depth_check;
    OpenList open_list;
    open_list.push_back(init_state);
    close_list.insert(&open_list.back());
    unsigned int counter = 0;
    while (open_list.size() > counter) {
        if ((counter % 1000000) == 0) {
            std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
                      << open_list.size() << std::endl;
        }
        pddl_lib::KBState &cur_state = open_list[counter];

        if (check_goal(cur_state)) {
            cur_state.goal_dist = 0;
            goals.push_back(&cur_state);
            num_goals++;
        }

        bool should_check = (!goals.empty() && open_list.size() == counter + 1) ||
                            (depth_check.find(max_depth) == depth_check.end() && cur_state.goal_dist == 0);
        if (should_check) {
            depth_check.insert(max_depth);
            std::unordered_set<pddl_lib::KBState *> checked_states;
            std::cout << "checking goal" << std::endl;
            goal_search(goals);
        }

        if (open_list[0].goal_dist != -1 &&
            (open_list[0].goal_dist < cur_state.depth || open_list.size() == counter + 1)) {
            plan_found = true;
            std::cout << "found plan" << std::endl;
            print_plan(open_list);
            break;
        }

        if (cur_state.goal_dist == 0) {
            counter++;
            continue;
        }

        assert(cur_state.goal_dist == -1);
        pddl_lib::expand(cur_state, new_states, constraints);

        for (auto &potential_new_state: new_states) {
            if (potential_new_state.valid) {
                auto it = close_list.find(&potential_new_state);
                if (it == close_list.end()) {
                    open_list.push_back(potential_new_state);
                    potential_new_state.valid = 0;
                    auto &new_state = open_list.back();
                    new_state.depth = cur_state.depth + 1;
                    close_list.insert(&new_state);
                    cur_state.children.push_back(&new_state);
                    new_state.parents.push_back(&cur_state);

                    if (new_state.associated_state != nullptr) {
                        assert(new_state.associated_state->associated_state != nullptr);
                        new_state.associated_state->associated_state = &new_state;

                        auto it2 = close_list.find(new_state.associated_state);
                        if (it2 != close_list.end()) { //TODO I do not know if this is needed
                            new_state.associated_state = *it2;
                            assert(new_state.associated_state != nullptr);
                            assert((*it2)->associated_state == &new_state);
                        }
                    }

                    if (new_state.depth > max_depth) {
                        max_depth = new_state.depth;
                        std::cout << "max_depth: " << max_depth << std::endl;
                    }
                } else {
                    potential_new_state.valid = 0;
                    cur_state.children.push_back(*it);
                    (*it)->parents.push_back(&cur_state);
                }
            } else {
                break;
            }
        }

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

