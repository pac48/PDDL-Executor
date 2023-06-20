#include "cff_plan_solver/cff_parser.hpp"

namespace pddl_lib {
    unsigned int split_string(const std::string &text, std::vector<std::string> &tokens, char separator) {
        size_t pos = text.find(separator);
        unsigned int initialPos = 0;
        tokens.clear();

        while (pos != std::string::npos && pos < text.length()) {
            if (text.substr(initialPos, pos - initialPos + 1) != " ") {
                std::string s = text.substr(initialPos, pos - initialPos + 1);
                s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
                        s.end());
                tokens.push_back(s);
            }
            initialPos = pos + 1;
            pos = text.find(separator, initialPos);
        }

        tokens.push_back(text.substr(initialPos, text.size() - initialPos));
        return tokens.size();
    }

    void extract_elements_from_line(const std::vector<std::string> &tokens, std::string &action_id,
                                    std::string &operator_name,
                                    std::vector<std::string> &operator_params, std::string &true_action_id,
                                    std::string &false_action_id) {

        // Example of actions of Contigent-FF:
        //   17||0 --- FIND_OBJECT C1 ITEM_0 --- SON: 18||0
        //   18||0 --- PICKUP_OBJECT C1 ITEM_0 --- SON: 19||0

        action_id = tokens[0];
        operator_name = tokens[2];

        int idx = 3;
        while (idx < tokens.size() && tokens[idx] != "---") {
            operator_params.push_back(tokens[idx]);
            idx++;
        }
        if (tokens.size() - operator_params.size() == 9) {
            true_action_id = tokens[5 + operator_params.size()];
            false_action_id = tokens[8 + operator_params.size()];
        } else {
            true_action_id = tokens[5 + operator_params.size()];
            false_action_id = true_action_id;
        }
    }

    void createAction(const float &level, std::string operator_name, std::vector<std::string> operator_params,
                      CFFAction &action) {
        transform(operator_name.begin(), operator_name.end(), operator_name.begin(), ::tolower);
        for (auto &val: operator_params) {
            transform(val.begin(), val.end(), val.begin(), ::tolower);
        }

//    std::string operator_parameters;
//    for (auto it = operator_params.begin(); it != operator_params.end(); ++it) {
//        if (std::next(it) != operator_params.end()) {
//            operator_parameters = operator_parameters + (*it) + " ";
//        } else {
//            operator_parameters = operator_parameters + (*it);
//        }
//    }
//    std::stringstream ss;
//    transform(operator_name.begin(), operator_name.end(), operator_name.begin(), ::tolower);
////        ss << level << ": (" << operator_name << " " << operator_parameters << ")" << [" << 1 + level - 0.001 << "]\n";
//    ss << "(" << operator_name << " " << operator_parameters << ")";
        action.name = operator_name;
        action.params = operator_params;

    }


    std::optional<Plan>
    parsePlanOutput(const std::basic_string<char, std::char_traits<char>, std::allocator<char>> &plan_file_path) {
        std::string line;
        std::ifstream plan_file(plan_file_path);
        bool solution = false;
        bool is_parse_completed = false;
        float time = 0;
        std::unordered_map<std::string, std::shared_ptr<PlanNode>> tree_map;

        if (plan_file.is_open()) {

            while (std::getline(plan_file, line)) {
                if (line.compare("ff: found plan as follows") == 0) {
                    break;
                }
            }
            if (!line.empty()) {
                solution = true;

                while (!plan_file.eof() && !is_parse_completed) {
                    std::getline(plan_file, line);

                    if (!line.empty()) {
                        if (line.compare("-------------------------------------------------") != 0) {

                            std::vector<std::string> tokens;
                            split_string(line, tokens, ' ');

                            std::string action_id;
                            std::string operator_name;
                            std::vector<std::string> operator_params;
                            std::string true_action_id;
                            std::string false_action_id;
                            extract_elements_from_line(tokens, action_id, operator_name, operator_params,
                                                       true_action_id, false_action_id);

                            PlanItem item;
                            auto level = std::stoi(action_id.substr(0, action_id.find("||")));
                            item.time = level;
                            item.duration = 1.0;
                            createAction(level, operator_name, operator_params, item.action);

                            if (tree_map.find(action_id) == tree_map.end()) {
                                tree_map[action_id] = std::make_shared<PlanNode>();
                            }
                            tree_map[action_id]->item = item;

                            auto tmp = true_action_id.substr(true_action_id.find("||") + 2,
                                                             true_action_id.length() - 1);
                            if (std::stoi(tmp) != -1) {
                                if (tree_map.find(true_action_id) == tree_map.end()) {
                                    tree_map[true_action_id] = std::make_shared<PlanNode>();
                                }
                                tree_map[action_id]->true_node = tree_map[true_action_id];
                            }

                            tmp = false_action_id.substr(false_action_id.find("||") + 2, false_action_id.length() - 1);
                            if (std::stoi(tmp) != -1) {
                                if (tree_map.find(false_action_id) == tree_map.end()) {
                                    tree_map[false_action_id] = std::make_shared<PlanNode>();
                                }
                                if (false_action_id != true_action_id) {
                                    tree_map[action_id]->false_node = tree_map[false_action_id];
                                } else {
                                    tree_map[action_id]->false_node = nullptr;
                                }
                            }

                        }
                    } else {
                        is_parse_completed = true;
                    }
                }

            }
        }


        if (!solution) {
            return {};
        } else {
            auto root_node = tree_map["0||0"];
            return root_node;
        }
    }
} // pddl_lib