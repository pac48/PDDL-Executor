#pragma once

#include "string"
#include "algorithm"
#include "sstream"
#include "fstream"
#include "optional"
#include "cff_plan_solver/types.hpp"


unsigned int split_string(const std::string &text, std::vector<std::string> &tokens, char separator);

void extract_elements_from_line(const std::vector<std::string> &tokens, std::string &action_id,
                                std::string &operator_name,
                                std::vector<std::string> &operator_params, std::string &true_action_id,
                                std::string &false_action_id);

void createAction(const float &level, std::string operator_name, std::vector<std::string> operator_params,
                  std::string &action);

std::optional<Plan>
parsePlanOutput(const std::basic_string<char, std::char_traits<char>, std::allocator<char>> &plan_file_path);