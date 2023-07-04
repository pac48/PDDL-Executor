#include "array"
#include <fstream>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <queue>
#include <sstream>
#include <unordered_map>

#include <ament_index_cpp/get_package_share_directory.hpp>
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

//        assert(counter == 0 || counter == 1 || counter == 2); TODO enable after done debug print
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
        } else if (state.associated_state > open_list[state.associated_state].associated_state) {
          std::cout << depth - 1 << "||" << ind << " --- " << action_name << "--- TRUESON: " << depth << "||"
                    << next_ind << " --- FALSESON: " << depth << "||"
                    << next_ind + 1
                    << "\n";
          next_ind += 2;
        } else {
          ind--;
          assert(state.associated_state != 0);
          assert(1 == abs(((int) (state.associated_state)) -
                          ((int) (open_list[state.associated_state].associated_state))));
        }

      }
      ind++;
    }
    std::cout << "-------------------------------------------------\n";
  }

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
  auto [state, new_states, valid, constraints, check_goal] = pddl_lib::initialize_problem(pddl_str);
  auto valid_data = valid.data();
  const auto valid_size = valid.size();

  auto start = std::chrono::high_resolution_clock::now();
  unsigned int max_depth = 0;
  bool plan_found = false;

  std::unordered_map<pddl_lib::KBState, unsigned int> close_list;
  std::vector<pddl_lib::KBState> open_list;
  open_list.push_back(state);
  unsigned int counter = 0;
  while (open_list.size() > counter) {
    if (open_list[counter].reached_goal == -1) {
      counter++;
      continue;
    }

    memset(valid.data(), 0, sizeof(valid));
    if (check_goal(open_list[counter])) {
      open_list[counter].reached_goal = 1;
      if (goal_propagate(open_list, counter)) {
        plan_found = true;
        std::cout << "fond plan" << std::endl;
        print_plan(open_list, counter);
        break;
      } else {
        counter++;
        continue;
      }
    }

    // TODO I am not sure if this is safe to enable
//        if (close_list.find(open_list[counter]) != close_list.end()) {
//            counter++;
//            continue;
//        }
//        close_list.insert(open_list[counter]); // TODO it seems like the goal state should not be in the closes list

    pddl_lib::expand(open_list[counter], new_states, valid, constraints);

    if ((counter % 1000000) == 0) {
      std::cout << "counter: " << counter << ", close_list_size: " << close_list.size() << ", open_list_size: "
                << open_list.size() << std::endl;
    }
    open_list[counter].children_begin = open_list.size();
    unsigned int num_added = 0;
    unsigned int open_list_base_size = open_list.size();
    for (unsigned int i = 0; i < valid_size; i++) {
      if (*(valid_data + i) == 1) {
        auto it = close_list.find(new_states[i]);
        if (it == close_list.end()) {
          if (new_states[i].associated_state != 0) {
            unsigned int num_skipped = i - num_added;
            new_states[i].associated_state = open_list_base_size + (new_states[i].associated_state - num_skipped);
          }
          new_states[i].depth = open_list[counter].depth + 1;
          new_states[i].parent = counter;
          close_list[new_states[i]] = open_list.size();
          open_list.push_back(new_states[i]);
          if (new_states[i].depth > max_depth) {
            max_depth = new_states[i].depth;
            std::cout << "max_depth: " << max_depth << std::endl;
          }
          num_added++;
        }
      } else {
        break;
      }
    }
    open_list[counter].children_end = open_list.size(); // exclusive
    counter++;
  }

//    if (!plan_found){
//        counter--;
//        for (auto & val : open_list){
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
