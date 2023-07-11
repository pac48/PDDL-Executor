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

#include <ament_index_cpp/get_package_share_directory.hpp>
#include "pddl_problem.hpp"


constexpr int CHUNK_SIZE = 128 * 16;
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
        while (index > CHUNK_SIZE > 0 && it != list_.end()) {
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
        while (index > CHUNK_SIZE > 0 && it != list_.end()) {
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


//void disable_subtree(OpenList &open_list, pddl_lib::KBState &cur_state) {
//    cur_state.reached_goal = -1;
//    for (auto child = cur_state.children; child < cur_state.children_end; child++) {
//        if (child > 0) {
//            disable_subtree(open_list, open_list[child]);
//        }
//    }
//}

//bool goal_propagate(OpenList &open_list, unsigned int goal_ind) {
//    if (goal_ind == 0) {
//        return true;
//    }
//    pddl_lib::KBState &goal_state = open_list[goal_ind];
//    if (goal_state.associated_state != 0 && open_list[goal_state.associated_state].reached_goal != 1) {
//        return false;
//    }
//    auto &goal_parent = open_list[goal_state.parent];
//    goal_parent.reached_goal = 1;
//    for (auto child = goal_parent.children_begin; child < goal_parent.children_end; child++) {
//        if (child > 0 && child != goal_ind && child != goal_state.associated_state) {
//            disable_subtree(open_list, open_list[child]);
//        }
//    }
//    return goal_propagate(open_list, goal_state.parent);
//}

void print_plan(const OpenList &open_list, unsigned int goal_ind) {
    assert(open_list[goal_ind].reached_goal);

    std::unordered_map<unsigned int, std::vector<pddl_lib::KBState*> > map;
    std::unordered_map<int, int> depth_map; // tack the number of values inserted at each depth
    std::queue<pddl_lib::KBState*> q;
    for (const auto &child : open_list[0].children) {
        if (child->reached_goal == 1) {
            q.emplace(child);
        }
    }
    while (!q.empty()) {
        auto state = q.front();

        map[state->depth].push_back(q.front());

        q.pop();
        int counter = 0;
        for (const auto & child : state->children) {
            if (child->reached_goal == 1) {
                q.emplace(child);
                counter++;
            }
        }

//        assert(counter == 0 || counter == 1 || counter == 2); TODO enable after done debug print
    }

    std::cout << "-------------------------------------------------\n";
    for (int depth = 1; depth <= map.size(); depth++) {
        const auto &state_set = map[depth];
        int ind = 0;
        int next_ind = 0;
        for (const auto &state: state_set) {
            auto name_vec = pddl_lib::indexers::get_action_string(state->action);
            std::stringstream ss;
            for (const auto &val: name_vec) {
                ss << val << " ";
            }
            auto action_name = ss.str();
            if (state->children.size() == 0) {
                std::cout << depth - 1 << "||" << ind << " --- " << action_name << "--- SON: " << depth << "||" << -1
                          << "\n";
            } else {
                if (state->associated_state == nullptr) {
                    std::cout << depth - 1 << "||" << ind << " --- " << action_name << "--- SON: " << depth << "||"
                              << next_ind
                              << "\n";
                    next_ind++;
                } else if (state->associated_state > state->associated_state->associated_state) {
                    std::cout << depth - 1 << "||" << ind << " --- " << action_name << "--- TRUESON: " << depth << "||"
                              << next_ind << " --- FALSESON: " << depth << "||"
                              << next_ind + 1
                              << "\n";
                    next_ind += 2;
                } else {
                    ind--;
                    assert(state->associated_state != nullptr);
//                    assert(1 == abs(((int) (state.associated_state)) - ((int) (open_list[state.associated_state].associated_state))));
                }
//          std::cout << pddl_lib::indexers::get_state_string(state);

            }
            ind++;
        }
        std::cout << "-------------------------------------------------\n";
    }

}

bool goal_search(pddl_lib::KBState & state){

    return false;
}

int main(int argc, char **argv) {
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("plan_solver");
    std::filesystem::path test_dir = pkg_dir / "pddl";
    std::filesystem::path pddl_file = test_dir / "problem.pddl";
    std::ifstream f(pddl_file);
    std::ifstream pddl_file_stream(pddl_file.string().c_str());
    std::stringstream ss;
    ss << pddl_file_stream.rdbuf();
    std::string pddl_str = ss.str();
    auto [init_state, new_states, constraints, check_goal] = pddl_lib::initialize_problem(pddl_str);

    auto start = std::chrono::high_resolution_clock::now();
    unsigned int max_depth = 0;
    bool plan_found = false;

    std::unordered_set<pddl_lib::KBState *> close_list;
    OpenList open_list;
    open_list.push_back(init_state);
    unsigned int counter = 0;
    while (open_list.size() > counter) {
        pddl_lib::KBState & cur_state =  open_list[counter];
        if (cur_state.reached_goal == -1) {
            counter++;
            continue;
        }

        if (check_goal(cur_state)) {
            cur_state.reached_goal = 1;
//            std::cout << "new goal found" << std::endl;
//            if (goal_propagate(open_list, counter)) {
            if (goal_search(open_list[0])) {
                plan_found = true;
                std::cout << "fond plan" << std::endl;
                print_plan(open_list, counter);
                break;
            } else {
                counter++;
                continue;
            }
        }
        pddl_lib::expand(cur_state, new_states, constraints);

        if ((counter % 1000000) == 0) {
            std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
                      << open_list.size() << std::endl;
        }
        unsigned int num_added = 0;
        unsigned int open_list_base_size = open_list.size();
        for (auto & potential_new_state : new_states) {
            if (potential_new_state.valid) {
                auto it = close_list.find(&potential_new_state);
                if (it == close_list.end()) {
                    if (potential_new_state.associated_state != nullptr && !potential_new_state.associated_state->valid) {
                        assert(0); // I do not think this can occur
                        potential_new_state.valid = 0;
                        continue;
                    }
                    open_list.push_back(potential_new_state);
                    auto & new_state = open_list.back();
                    new_state.depth = cur_state.depth + 1;
//                    new_states[i].parent = counter;
                    close_list.insert(&new_state);
                    cur_state.children.push_back(&new_state);

                    if (new_state.associated_state != nullptr) {
                        assert(new_state.associated_state->associated_state != nullptr);
                        new_state.associated_state->associated_state = &new_state;

                        auto it2 = close_list.find(new_state.associated_state);
                        if (it2 != close_list.end()){
                            new_state.associated_state->valid = 0;
                            new_state.associated_state = *it2;
                            assert(new_state.associated_state != nullptr);
                        }
                    }

                    if (new_state.depth > max_depth) {
                        max_depth = new_state.depth;
                        std::cout << "max_depth: " << max_depth << std::endl;
                    }
                    num_added++;
                }
            } else {
                break;
            }
        }

        counter++;
    }

//    if (!plan_found) {
//        std::cout << "DEBUG PLAN FAILED!\n";
//        counter--;
//        for (auto &val: open_list) {
//            val.reached_goal = true;
//        }
//        print_plan(open_list, counter);
//    }

    std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
              << open_list.size() << std::endl;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken by function: "
              << duration.count() << " milliseconds" << std::endl;


    return 0;
}
