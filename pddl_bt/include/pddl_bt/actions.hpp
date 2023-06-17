#include "bt_medicine_domain.hpp"

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
    return BT::NodeStatus::FAILURE;
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
    return BT::NodeStatus::FAILURE;
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
