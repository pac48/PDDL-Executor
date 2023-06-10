#include <utility>
#include <filesystem>
#include <fstream>
#include <unordered_map>
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

void Plan::add_observe_action_sequence(bool observe_result, const PlanItem &item, std::stringstream &tree) const {
    const std::string action_id =
            item.action + ":" + std::to_string(static_cast<int>(item.time * 1000));
    std::string observe_expr = item.action;
    tree << "<Sequence name=\"" + action_id + "\">\n";
    if (observe_result) {
        tree << "<WaitAtStartReq action=\"" + action_id + "\"/>\n";
        tree << "<ReactiveSequence name=\"" + action_id + "\">\n";
        tree << "<CheckOverAllReq action=\"" + action_id + "\"/>\n";
        tree << "<ExecuteAction action=\"" + action_id + "\"/>\n";
        tree << "</ReactiveSequence>\n";

        tree << "<ApplyObservation observe=\"" + observe_expr +
                "\" value=\"true\"/>\n";
    } else {
        tree << "<ApplyObservation observe=\"" + observe_expr +
                "\" value=\"false\"/>\n";
    }
    tree << "</Sequence>\n";
}

void Plan::add_action_sequence(const PlanItem &item, std::stringstream &tree) const {
    const std::string action_id = item.action + ":" + std::to_string(static_cast<int>(item.time * 1000));
    tree << "<Sequence name=\"" + action_id + "\">\n";
    tree << "<WaitAtStartReq action=\"" + action_id + "\"/>\n";
    tree << "<ApplyAtStartEffect action=\"" + action_id + "\"/>\n";
    tree << "<ReactiveSequence name=\"" + action_id + "\">\n";
    tree << "<CheckOverAllReq action=\"" + action_id + "\"/>\n";
    tree << "<ExecuteAction action=\"" + action_id + "\"/>\n";
    tree << "</ReactiveSequence>\n";
    tree << "<CheckAtEndReq action=\"" + action_id + "\"/>\n";
    tree << "<ApplyAtEndEffect action=\"" + action_id + "\"/>\n";
    tree << "</Sequence>\n";
}

void Plan::get_sub_tree(const std::shared_ptr<PlanNode> &root, std::stringstream &tree) const {
    if (root == nullptr) {
        return;
    }
    if (root->false_node) {

        std::stringstream tree_local_1;
        std::stringstream tree_local_2;

        add_observe_action_sequence(true, root->item, tree_local_1);
        get_sub_tree(root->true_node, tree_local_1);

        add_observe_action_sequence(false, root->item, tree_local_2);
        get_sub_tree(root->false_node, tree_local_2);

        Jinja2CppLight::Template tree_template(template_map_.at("observe_sequence.xml"));
        tree_template.setValue("action_sequence_1", tree_local_1.str());
        tree_template.setValue("action_sequence_2", tree_local_2.str());

        tree << tree_template.render();

    } else {
        add_action_sequence(root->item, tree);
        get_sub_tree(root->true_node, tree);
    }
}

std::ostream &operator<<(std::ostream &os, const Plan &obj) {

//    std::string source = R"DELIM(
//        This is my {{avalue}} template.  It's {{secondvalue}} hooohooo Values list: {{valuesList}}
//    )DELIM";
//
    Jinja2CppLight::Template tree_template(obj.template_map_.at("bt.xml"));
    std::stringstream tree;
    obj.get_sub_tree(obj.root_, tree);

    tree_template.setValue("tree", tree.str());

    os << tree_template.render();
//    os << tree.str();
    return os;

}
