#include <cstdio>

#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "behaviortree_cpp_v3/xml_parsing.h"
#include "behaviortree_cpp_v3/loggers/bt_file_logger.h"
#include "behaviortree_cpp_v3/blackboard.h"
#include "behaviortree_cpp_v3/basic_types.h"
#include "behaviortree_cpp_v3/tree_node.h"

#include "bt_medicine_domain.hpp"


std::string config = "<root BTCPP_format=\"4\">\n"
                     "    <BehaviorTree ID=\"MainTree\\\">\n"
                     "        <Sequence name=\"root_sequence\\\">\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"detectperson:0\">\n"
                     "<detectPerson   name=\"detectPerson\" r=\"pioneer\" p=\"nathan\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"initguidepersontolandmarkattempt:1000\">\n"
                     "<InitguidePersonToLandmarkAttempt   name=\"InitguidePersonToLandmarkAttempt\" r=\"pioneer\" p=\"nathan\" to=\"home\" /></Sequence>\n"
                     "<Sequence name=\"guidepersontolandmarkattempt1:2000\">\n"
                     "<guidePersonToLandmarkAttempt1   name=\"guidePersonToLandmarkAttempt1\" r=\"pioneer\" p=\"nathan\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"checkguidetosucceeded1:3000\">\n"
                     "<checkGuideToSucceeded1   name=\"checkGuideToSucceeded1\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"updatepersonloc1:4000\">\n"
                     "<UpdatePersonLoc1   name=\"UpdatePersonLoc1\" p=\"nathan\" from=\"home\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyautomatedmedicineat:5000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess1:6000\">\n"
                     "<UpdateSuccess1   name=\"UpdateSuccess1\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyautomatedmedicineat:5000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyrecordedmedicineat:6000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess2:7000\">\n"
                     "<UpdateSuccess2   name=\"UpdateSuccess2\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyrecordedmedicineat:6000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"askcaregiverhelpmedicine1:7000\">\n"
                     "<askCaregiverHelpMedicine1   name=\"askCaregiverHelpMedicine1\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess3:8000\">\n"
                     "<UpdateSuccess3   name=\"UpdateSuccess3\" p=\"nathan\" /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"checkguidetosucceeded1:3000\">\n"
                     "<checkGuideToSucceeded1   name=\"checkGuideToSucceeded1\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"initmovetolandmark:4000\">\n"
                     "<initMoveToLandmark   name=\"initMoveToLandmark\" r=\"pioneer\" /></Sequence>\n"
                     "<Sequence name=\"movetolandmark:5000\">\n"
                     "<moveToLandmark   name=\"moveToLandmark\" r=\"pioneer\" to=\"home\" /></Sequence>\n"
                     "<Sequence name=\"initguidepersontolandmarkattempt:6000\">\n"
                     "<InitguidePersonToLandmarkAttempt   name=\"InitguidePersonToLandmarkAttempt\" r=\"pioneer\" p=\"nathan\" to=\"home\" /></Sequence>\n"
                     "<Sequence name=\"guidepersontolandmarkattempt2:7000\">\n"
                     "<guidePersonToLandmarkAttempt2   name=\"guidePersonToLandmarkAttempt2\" r=\"pioneer\" p=\"nathan\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"checkguidetosucceeded2:8000\">\n"
                     "<checkGuideToSucceeded2   name=\"checkGuideToSucceeded2\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"updatepersonloc2:9000\">\n"
                     "<UpdatePersonLoc2   name=\"UpdatePersonLoc2\" p=\"nathan\" from=\"home\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyautomatedmedicineat:10000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess1:11000\">\n"
                     "<UpdateSuccess1   name=\"UpdateSuccess1\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyautomatedmedicineat:10000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyrecordedmedicineat:11000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess2:12000\">\n"
                     "<UpdateSuccess2   name=\"UpdateSuccess2\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyrecordedmedicineat:11000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"askcaregiverhelpmedicine1:12000\">\n"
                     "<askCaregiverHelpMedicine1   name=\"askCaregiverHelpMedicine1\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess3:13000\">\n"
                     "<UpdateSuccess3   name=\"UpdateSuccess3\" p=\"nathan\" /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"checkguidetosucceeded2:8000\">\n"
                     "<checkGuideToSucceeded2   name=\"checkGuideToSucceeded2\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"initmovetolandmark:9000\">\n"
                     "<initMoveToLandmark   name=\"initMoveToLandmark\" r=\"pioneer\" /></Sequence>\n"
                     "<Sequence name=\"movetolandmark:10000\">\n"
                     "<moveToLandmark   name=\"moveToLandmark\" r=\"pioneer\" to=\"home\" /></Sequence>\n"
                     "<Sequence name=\"askcaregiverhelpmedicine2:11000\">\n"
                     "<askCaregiverHelpMedicine2   name=\"askCaregiverHelpMedicine2\" r=\"pioneer\" p=\"nathan\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess3:12000\">\n"
                     "<UpdateSuccess3   name=\"UpdateSuccess3\" p=\"nathan\" /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"detectperson:0\">\n"
                     "<detectPerson   name=\"detectPerson\" r=\"pioneer\" p=\"nathan\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"initmovetolandmark:1000\">\n"
                     "<initMoveToLandmark   name=\"initMoveToLandmark\" r=\"pioneer\" /></Sequence>\n"
                     "<Sequence name=\"movetolandmark:2000\">\n"
                     "<moveToLandmark   name=\"moveToLandmark\" r=\"pioneer\" to=\"couch\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"detectperson:3000\">\n"
                     "<detectPerson   name=\"detectPerson\" r=\"pioneer\" p=\"nathan\" loc=\"couch\" /></Sequence>\n"
                     "<Sequence name=\"initguidepersontolandmarkattempt:4000\">\n"
                     "<InitguidePersonToLandmarkAttempt   name=\"InitguidePersonToLandmarkAttempt\" r=\"pioneer\" p=\"nathan\" to=\"couch\" /></Sequence>\n"
                     "<Sequence name=\"guidepersontolandmarkattempt1:5000\">\n"
                     "<guidePersonToLandmarkAttempt1   name=\"guidePersonToLandmarkAttempt1\" r=\"pioneer\" p=\"nathan\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"checkguidetosucceeded1:6000\">\n"
                     "<checkGuideToSucceeded1   name=\"checkGuideToSucceeded1\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"updatepersonloc1:7000\">\n"
                     "<UpdatePersonLoc1   name=\"UpdatePersonLoc1\" p=\"nathan\" from=\"couch\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyautomatedmedicineat:8000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess1:9000\">\n"
                     "<UpdateSuccess1   name=\"UpdateSuccess1\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyautomatedmedicineat:8000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyrecordedmedicineat:9000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess2:10000\">\n"
                     "<UpdateSuccess2   name=\"UpdateSuccess2\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyrecordedmedicineat:9000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"askcaregiverhelpmedicine1:10000\">\n"
                     "<askCaregiverHelpMedicine1   name=\"askCaregiverHelpMedicine1\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess3:11000\">\n"
                     "<UpdateSuccess3   name=\"UpdateSuccess3\" p=\"nathan\" /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"checkguidetosucceeded1:6000\">\n"
                     "<checkGuideToSucceeded1   name=\"checkGuideToSucceeded1\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"initmovetolandmark:7000\">\n"
                     "<initMoveToLandmark   name=\"initMoveToLandmark\" r=\"pioneer\" /></Sequence>\n"
                     "<Sequence name=\"movetolandmark:8000\">\n"
                     "<moveToLandmark   name=\"moveToLandmark\" r=\"pioneer\" to=\"couch\" /></Sequence>\n"
                     "<Sequence name=\"initguidepersontolandmarkattempt:9000\">\n"
                     "<InitguidePersonToLandmarkAttempt   name=\"InitguidePersonToLandmarkAttempt\" r=\"pioneer\" p=\"nathan\" to=\"couch\" /></Sequence>\n"
                     "<Sequence name=\"guidepersontolandmarkattempt2:10000\">\n"
                     "<guidePersonToLandmarkAttempt2   name=\"guidePersonToLandmarkAttempt2\" r=\"pioneer\" p=\"nathan\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"checkguidetosucceeded2:11000\">\n"
                     "<checkGuideToSucceeded2   name=\"checkGuideToSucceeded2\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"updatepersonloc2:12000\">\n"
                     "<UpdatePersonLoc2   name=\"UpdatePersonLoc2\" p=\"nathan\" from=\"couch\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyautomatedmedicineat:13000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess1:14000\">\n"
                     "<UpdateSuccess1   name=\"UpdateSuccess1\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyautomatedmedicineat:13000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyrecordedmedicineat:14000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess2:15000\">\n"
                     "<UpdateSuccess2   name=\"UpdateSuccess2\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyrecordedmedicineat:14000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"askcaregiverhelpmedicine1:15000\">\n"
                     "<askCaregiverHelpMedicine1   name=\"askCaregiverHelpMedicine1\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess3:16000\">\n"
                     "<UpdateSuccess3   name=\"UpdateSuccess3\" p=\"nathan\" /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"checkguidetosucceeded2:11000\">\n"
                     "<checkGuideToSucceeded2   name=\"checkGuideToSucceeded2\" loc=\"home\" /></Sequence>\n"
                     "<Sequence name=\"initmovetolandmark:12000\">\n"
                     "<initMoveToLandmark   name=\"initMoveToLandmark\" r=\"pioneer\" /></Sequence>\n"
                     "<Sequence name=\"movetolandmark:13000\">\n"
                     "<moveToLandmark   name=\"moveToLandmark\" r=\"pioneer\" to=\"couch\" /></Sequence>\n"
                     "<Sequence name=\"askcaregiverhelpmedicine2:14000\">\n"
                     "<askCaregiverHelpMedicine2   name=\"askCaregiverHelpMedicine2\" r=\"pioneer\" p=\"nathan\" loc=\"couch\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess3:15000\">\n"
                     "<UpdateSuccess3   name=\"UpdateSuccess3\" p=\"nathan\" /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"detectperson:3000\">\n"
                     "<detectPerson   name=\"detectPerson\" r=\"pioneer\" p=\"nathan\" loc=\"couch\" /></Sequence>\n"
                     "<Sequence name=\"initmovetolandmark:4000\">\n"
                     "<initMoveToLandmark   name=\"initMoveToLandmark\" r=\"pioneer\" /></Sequence>\n"
                     "<Sequence name=\"movetolandmark:5000\">\n"
                     "<moveToLandmark   name=\"moveToLandmark\" r=\"pioneer\" to=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyautomatedmedicineat:6000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess1:7000\">\n"
                     "<UpdateSuccess1   name=\"UpdateSuccess1\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyautomatedmedicineat:6000\">\n"
                     "<notifyAutomatedMedicineAt   name=\"notifyAutomatedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Fallback>\n"
                     "    <Sequence>\n"
                     "       <Sequence name=\"notifyrecordedmedicineat:7000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess2:8000\">\n"
                     "<UpdateSuccess2   name=\"UpdateSuccess2\"  /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "    <Sequence>\n"
                     "        <Sequence name=\"notifyrecordedmedicineat:7000\">\n"
                     "<notifyRecordedMedicineAt   name=\"notifyRecordedMedicineAt\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"askcaregiverhelpmedicine1:8000\">\n"
                     "<askCaregiverHelpMedicine1   name=\"askCaregiverHelpMedicine1\" r=\"pioneer\" p=\"nathan\" loc=\"kitchen\" /></Sequence>\n"
                     "<Sequence name=\"updatesuccess3:9000\">\n"
                     "<UpdateSuccess3   name=\"UpdateSuccess3\" p=\"nathan\" /></Sequence>\n"
                     "\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "    </Sequence>\n"
                     "</Fallback>\n"
                     "        </Sequence>\n"
                     "    </BehaviorTree>\n"
                     "</root>";


BT::NodeStatus
detectPerson::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus initMoveToLandmark::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus moveToLandmark::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus InitguidePersonToLandmarkAttempt::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus guidePersonToLandmarkAttempt1::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus guidePersonToLandmarkAttempt2::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus checkGuideToSucceeded1::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus checkGuideToSucceeded2::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdatePersonLoc1::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdatePersonLoc2::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdateSuccess1::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdateSuccess2::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus UpdateSuccess3::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus notifyAutomatedMedicineAt::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus notifyRecordedMedicineAt::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus askCaregiverHelpMedicine1::tick_action(const InstantiatedAction & action) {
    return BT::NodeStatus::SUCCESS;
}

BT::NodeStatus askCaregiverHelpMedicine2::tick_action(const InstantiatedAction & action) {
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
