#include <utility>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include "ament_index_cpp/get_package_share_directory.hpp"

#include "cff_plan_solver/types.hpp"

#include "Jinja2CppLight.h"

Plan::Plan(std::shared_ptr<PlanNode> root) {
    root_ = std::move(root);
    std::filesystem::path pkg_dir = ament_index_cpp::get_package_share_directory("cff_plan_solver");
    std::filesystem::path template_dir = pkg_dir / "jinja_templates";

    for (const auto &entry: std::filesystem::directory_iterator(template_dir)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();

            std::ifstream inputFile(filePath);
            if (inputFile) {
                std::stringstream strStream;
                strStream << inputFile.rdbuf(); //read the file
                std::string str = strStream.str(); //str holds the content of the file
                template_map_[entry.path().filename().string()] = str;
            }
        }
    }

}

void Plan::add_observe_action_sequence(bool observe_result, const PlanItem &item, std::stringstream &tree,
                                       std::unordered_map<std::string, const Action *> &action_map) const {
    const std::string action_id = item.action.name + ":" + std::to_string(static_cast<int>(item.time * 1000));

    auto action = action_map[item.action.name];

    Jinja2CppLight::Template tree_template(template_map_.at("action.xml"));
    tree_template.setValue("action_name", action->name);
    std::stringstream ss;
    std::string sep;
    for (int i = 0; i < item.action.params.size(); i++) {
        auto params = action->parameters[i];
        std::string inst_name = item.action.params[i];
        int len = params.name.size();
//        ss << sep << params.name.substr(1, len - 1) << "_" << params.type << "=\"" << inst_name << "\"";
        ss << sep << params.name.substr(1, len - 1) << "=\"" << inst_name << "\"";
        sep = " ";
    }

    tree << "<Sequence name=\"" + action_id + "\">\n";
    tree_template.setValue("ports", ss.str());
    tree << tree_template.render();
    tree << "</Sequence>\n";
}

void Plan::add_action_sequence(const PlanItem &item, std::stringstream &tree,
                               std::unordered_map<std::string, const Action *> &action_map) const {
    const std::string action_id = item.action.name + ":" + std::to_string(static_cast<int>(item.time * 1000));
    auto action = action_map[item.action.name];

    Jinja2CppLight::Template tree_template(template_map_.at("action.xml"));
    tree_template.setValue("action_name", action->name);
    std::stringstream ss;
    std::string sep;
    for (int i = 0; i < item.action.params.size(); i++) {
        auto params = action->parameters[i];
        std::string inst_name = item.action.params[i];
        int len = params.name.size();
//        ss << sep << params.name.substr(1, len - 1) << "_" << params.type << "=\"" << inst_name << "\"";
        ss << sep << params.name.substr(1, len - 1) << "=\"" << inst_name << "\"";
        sep = " ";
    }

    tree << "<Sequence name=\"" + action_id + "\">\n";
    tree_template.setValue("ports", ss.str());
    tree << tree_template.render();
    tree << "</Sequence>\n";

}

void Plan::get_sub_tree(const std::shared_ptr<PlanNode> &root, std::stringstream &tree,
                        std::unordered_map<std::string, const Action *> &action_map) const {
    if (root == nullptr) {
        return;
    }
    if (root->false_node) {

        std::stringstream tree_local_1;
        std::stringstream tree_local_2;

        add_observe_action_sequence(true, root->item, tree_local_1, action_map);
        get_sub_tree(root->true_node, tree_local_1, action_map);

        add_observe_action_sequence(false, root->item, tree_local_2, action_map);
        get_sub_tree(root->false_node, tree_local_2, action_map);

        Jinja2CppLight::Template tree_template(template_map_.at("observe_sequence.xml"));
        tree_template.setValue("action_sequence_1", tree_local_1.str());
        tree_template.setValue("action_sequence_2", tree_local_2.str());

        tree << tree_template.render();

    } else {
        add_action_sequence(root->item, tree, action_map);
        get_sub_tree(root->true_node, tree, action_map);
    }
}

std::ostream &operator<<(std::ostream &os, const Plan &obj) {

    std::unordered_map<std::string, const Action *> action_map;
    Jinja2CppLight::Template tree_template(obj.template_map_.at("bt.xml"));
    std::stringstream tree;
    obj.get_sub_tree(obj.root_, tree, action_map);

    tree_template.setValue("tree", tree.str());

    os << tree_template.render();

    return os;

}

std::string Plan::convert_to_bt(const Domain &domain) const {
    std::unordered_map<std::string, const Action *> action_map;
    for (const auto &action: domain.actions) {
        std::string action_name = action.name;
        std::transform(action_name.begin(), action_name.end(), action_name.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        action_map[action_name] = &action;
    }

    Jinja2CppLight::Template tree_template(template_map_.at("bt.xml"));
    std::stringstream tree;
    get_sub_tree(root_, tree, action_map);

    tree_template.setValue("tree", tree.str());
    return tree_template.render();
}
