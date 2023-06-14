(define (domain paul_shr_conditional)
(:requirements
	:typing
	:strips
)
(:types
	landmark
	robot
	person
)
(:predicates
	(enable_check_guide_2)
	(tried_guide_person_landmark_2)
	(asked_caregiver_help ?p - person)
	(enable_check_guide_1)
	(guide_to_succeeded_attempt_2)
	(robot_at ?r - robot ?lm - landmark)
	(person_at ?p - person ?lm - landmark)
	(success)
	(medicine_location ?lm - landmark)
	(robot_updated_1)
	(notify_automated_succeeded)
	(init_move_to_landmark)
	(guide_to_succeeded_attempt_1)
	(notify_recorded_succeeded)
	(robot_updated_2)
	(init_guide_person_to_landmark_attempt)
	(tried_guide_person_landmark_1)
)
(:action detectPerson
	:parameters ( ?r - robot ?p - person ?loc - landmark)
	:precondition (and
(robot_at ?r ?loc)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:observe (person_at ?p ?loc)
)
(:action initMoveToLandmark
	:parameters ( ?r - robot)
	:precondition (and
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (and
(init_move_to_landmark)
(forall (?loc - landmark)
(not (robot_at ?r ?loc) )
)
)
)
(:action moveToLandmark
	:parameters ( ?r - robot ?to - landmark)
	:precondition (and
(init_move_to_landmark)
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (and
(robot_at ?r ?to)
(not (enable_check_guide_1) )
(not (enable_check_guide_2) )
(not (init_move_to_landmark) )
)
)
(:action InitguidePersonToLandmarkAttempt
	:parameters ( ?r - robot ?p - person ?to - landmark)
	:precondition (and
(robot_at ?r ?to)
(person_at ?p ?to)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (and
(init_guide_person_to_landmark_attempt)
(forall (?loc - landmark)
(not (robot_at ?r ?loc) )
)
)
)
(:action guidePersonToLandmarkAttempt1
	:parameters ( ?r - robot ?p - person ?to - landmark)
	:precondition (and
(medicine_location ?to)
(init_guide_person_to_landmark_attempt)
(not (tried_guide_person_landmark_1) )
(not (init_move_to_landmark) )
)
	:effect (and
(robot_at ?r ?to)
(tried_guide_person_landmark_1)
(enable_check_guide_1)
(not (init_guide_person_to_landmark_attempt) )
)
)
(:action guidePersonToLandmarkAttempt2
	:parameters ( ?r - robot ?p - person ?to - landmark)
	:precondition (and
(tried_guide_person_landmark_1)
(medicine_location ?to)
(init_guide_person_to_landmark_attempt)
(not (tried_guide_person_landmark_2) )
(not (init_move_to_landmark) )
)
	:effect (and
(robot_at ?r ?to)
(tried_guide_person_landmark_2)
(enable_check_guide_2)
(not (init_guide_person_to_landmark_attempt) )
)
)
(:action checkGuideToSucceeded1
	:parameters ( ?loc - landmark)
	:precondition (and
(tried_guide_person_landmark_1)
(enable_check_guide_1)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:observe (guide_to_succeeded_attempt_1)
)
(:action checkGuideToSucceeded2
	:parameters ( ?loc - landmark)
	:precondition (and
(tried_guide_person_landmark_2)
(enable_check_guide_2)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:observe (guide_to_succeeded_attempt_2)
)
(:action UpdatePersonLoc1
	:parameters ( ?p - person ?from - landmark ?to - landmark)
	:precondition (and
(guide_to_succeeded_attempt_1)
(person_at ?p ?from)
(medicine_location ?to)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (and
(person_at ?p ?to)
(not (person_at ?p ?from) )
)
)
(:action UpdatePersonLoc2
	:parameters ( ?p - person ?from - landmark ?to - landmark)
	:precondition (and
(guide_to_succeeded_attempt_2)
(person_at ?p ?from)
(medicine_location ?to)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (and
(person_at ?p ?to)
(not (person_at ?p ?from) )
)
)
(:action UpdateSuccess1
	:parameters ()
	:precondition (and
(notify_automated_succeeded)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (success)
)
(:action UpdateSuccess2
	:parameters ()
	:precondition (and
(notify_recorded_succeeded)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (success)
)
(:action UpdateSuccess3
	:parameters ( ?p - person)
	:precondition (and
(asked_caregiver_help ?p)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (success)
)
(:action notifyAutomatedMedicineAt
	:parameters ( ?r - robot ?p - person ?loc - landmark)
	:precondition (and
(robot_at ?r ?loc)
(person_at ?p ?loc)
(medicine_location ?loc)
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:observe (notify_automated_succeeded)
)
(:action notifyRecordedMedicineAt
	:parameters ( ?r - robot ?p - person ?loc - landmark)
	:precondition (and
(robot_at ?r ?loc)
(person_at ?p ?loc)
(medicine_location ?loc)
(not (notify_automated_succeeded) )
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:observe (notify_recorded_succeeded)
)
(:action askCaregiverHelpMedicine1
	:parameters ( ?r - robot ?p - person ?loc - landmark)
	:precondition (and
(robot_at ?r ?loc)
(person_at ?p ?loc)
(not (notify_automated_succeeded) )
(not (notify_recorded_succeeded) )
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (asked_caregiver_help ?p)
)
(:action askCaregiverHelpMedicine2
	:parameters ( ?r - robot ?p - person ?loc - landmark)
	:precondition (and
(robot_at ?r ?loc)
(person_at ?p ?loc)
(not (guide_to_succeeded_attempt_1) )
(not (guide_to_succeeded_attempt_2) )
(not (init_move_to_landmark) )
(not (init_guide_person_to_landmark_attempt) )
)
	:effect (asked_caregiver_help ?p)
)
)