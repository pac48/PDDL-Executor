#include <cstdio>

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/xml_parsing.h"
#include "behaviortree_cpp_v3/loggers/bt_file_logger.h"
#include "behaviortree_cpp_v3/blackboard.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "behaviortree_cpp_v3/tree_node.h"

#include "bt_medicine_domain.hpp"


BT::NodeStatus SimpleAction(BT::TreeNode &n) {
    std::cout << "SimpleAction: " << "1" << std::endl;
    return BT::NodeStatus::SUCCESS;
}


std::string config = " <root BTCPP_format=\"4\" >\n"
                     "     <BehaviorTree ID=\"MainTree\">\n"
                     "        <Sequence name=\"root_sequence\">\n"
                     "            <initMoveToLandmark   r_robot=\"pioneer\" />\n"
                     "            <checkGuideToSucceeded1   loc_landmark=\"room\" />\n"
                     "        </Sequence>\n"
                     "     </BehaviorTree>\n"
                     " </root>";


BT::NodeStatus
detectPerson::tick_action(const std::string &r_robot, const std::string &p_person, const std::string &loc_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus initMoveToLandmark::tick_action(const std::string &r_robot) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus moveToLandmark::tick_action(const std::string &r_robot, const std::string &to_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus InitguidePersonToLandmarkAttempt::tick_action(const std::string &r_robot, const std::string &p_person,
                                                             const std::string &to_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus guidePersonToLandmarkAttempt1::tick_action(const std::string &r_robot, const std::string &p_person,
                                                          const std::string &to_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus guidePersonToLandmarkAttempt2::tick_action(const std::string &r_robot, const std::string &p_person,
                                                          const std::string &to_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus checkGuideToSucceeded1::tick_action(const std::string &loc_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus checkGuideToSucceeded2::tick_action(const std::string &loc_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdatePersonLoc1::tick_action(const std::string &p_person, const std::string &from_landmark,
                                             const std::string &to_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdatePersonLoc2::tick_action(const std::string &p_person, const std::string &from_landmark,
                                             const std::string &to_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdateSuccess1::tick_action() {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdateSuccess2::tick_action() {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdateSuccess3::tick_action(const std::string &p_person) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus notifyAutomatedMedicineAt::tick_action(const std::string &r_robot, const std::string &p_person,
                                                      const std::string &loc_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus notifyRecordedMedicineAt::tick_action(const std::string &r_robot, const std::string &p_person,
                                                     const std::string &loc_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus askCaregiverHelpMedicine1::tick_action(const std::string &r_robot, const std::string &p_person,
                                                      const std::string &loc_landmark) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus askCaregiverHelpMedicine2::tick_action(const std::string &r_robot, const std::string &p_person,
                                                      const std::string &loc_landmark) {
    return BT::NodeStatus::SUCCESS;
}

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    // We use the BehaviorTreeFactory to register our custom nodes
    BT::BehaviorTreeFactory factory = create_tree_factory();

    auto tree = factory.createTreeFromText(config);

    while (tree.tickRoot() == BT::NodeStatus::RUNNING) {
        printf("wait\n");
    }

    printf("hello world pddl_bt package\n");
    return 0;
}
