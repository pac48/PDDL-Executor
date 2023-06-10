#include <cstdio>

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/xml_parsing.h"
#include "behaviortree_cpp_v3/loggers/bt_file_logger.h"
#include "behaviortree_cpp_v3/blackboard.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "behaviortree_cpp_v3/tree_node.h"


BT::NodeStatus SimpleAction(BT::TreeNode &n) {
    std::cout << "SimpleAction: " << "1" << std::endl;
    return BT::NodeStatus::SUCCESS;
}

class GiveReminder : public BT::SyncActionNode {
public:
    GiveReminder(const std::string &name, const BT::NodeConfiguration &config)
            : SyncActionNode(name, config) {}

    static BT::PortsList providedPorts() {
        return {BT::InputPort<std::string>("prompt"), BT::InputPort<std::string>("location")};
    }

    BT::NodeStatus tick() override {
        BT::Optional<std::string> prompt = getInput<std::string>("prompt");
        BT::Optional<std::string> location = getInput<std::string>("location");
        if (!prompt || !location) {
            throw BT::RuntimeError("missing required input");
        }

        return BT::NodeStatus::SUCCESS;
    }
};


std::string config = " <root BTCPP_format=\"4\" >\n"
                     "     <BehaviorTree ID=\"MainTree\">\n"
                     "        <Sequence name=\"root_sequence\">\n"
                     "            <SimpleAction   name=\"simple_action\" />\n"
                     "            <GiveReminder   name=\"give_reminder\" prompt=\"2222222\" location=\"1212\" />\n"
                     "            <GiveReminder   name=\"give_reminder\" prompt=\"my_prompt\" location=\"my_location\" />\n"
                     "        </Sequence>\n"
                     "     </BehaviorTree>\n"
                     " </root>";

// use jinja https://github.com/jinja2cpp/Jinja2Cpp
// contingent planner: https://www.upf.edu/web/ai-ml/clg-contingent-planner
//https://www.upf.edu/web/ai-ml/clg-contingent-planner1

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    // We use the BehaviorTreeFactory to register our custom nodes
    BT::BehaviorTreeFactory factory;
    factory.registerSimpleCondition("SimpleAction", SimpleAction);
    factory.registerNodeType<GiveReminder>("GiveReminder");

    auto tree = factory.createTreeFromText(config);

    while (tree.tickRoot() == BT::NodeStatus::RUNNING) {
        printf("wait\n");
    }

    printf("hello world pddl_bt package\n");
    return 0;
}
