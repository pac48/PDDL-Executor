(define (domain paul_shr_conditional)

(:requirements :strips :typing)

(:types
	person
	robot
	landmark
)

(:predicates
	(robot_at ?r - robot ?lm - landmark)
	(person_at ?p - person ?lm - landmark)
	(medicine_location ?lm - landmark)

	(asked_caregiver_help ?p - person)
	(robot_updated_1)
	(robot_updated_2)

	(init_move_to_landmark)
	(init_guide_person_to_landmark_attempt)

	(guide_to_succeeded_attempt_1 )
	(guide_to_succeeded_attempt_2)
	(notify_automated_succeeded)
	(notify_recorded_succeeded)

	(tried_guide_person_landmark_1)
	(tried_guide_person_landmark_2)

	(enable_check_guide_1)
	(enable_check_guide_2)

	(success)

)

(:action detectPerson
    :parameters (?r - robot ?p - person ?loc - landmark)
    :precondition (and
    			(robot_at ?r ?loc)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
   		 )
    :observe (person_at ?p ?loc)
)

;; Init move
(:action initMoveToLandmark
	:parameters (?r - robot)
	:precondition (and
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
		      )
	  :effect (and
		      (forall (?loc - landmark)
			  (not (robot_at ?r ?loc))
		      )
		      (init_move_to_landmark)
  	        )
)

;; Move to any landmark, avoiding terrain
(:action moveToLandmark
	:parameters (?r - robot ?to - landmark)
	:precondition (and
			(init_move_to_landmark)
			(not (init_guide_person_to_landmark_attempt))
		      )
	:effect (and
                (robot_at ?r ?to)
                (not (enable_check_guide_1))
                (not (enable_check_guide_2))
		(not (init_move_to_landmark))
            )
)


 ;; Init Guide
(:action InitguidePersonToLandmarkAttempt
	:parameters (?r - robot ?p - person ?to - landmark)
	:precondition (and
			(robot_at ?r ?to)
			(person_at ?p ?to)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
		      )
	  :effect (and
		      (forall (?loc - landmark)
			    (not (robot_at ?r ?loc))
		      )
		      (init_guide_person_to_landmark_attempt)
  	        )
)


;; Guide person from one landmark to another
(:action guidePersonToLandmarkAttempt1
	:parameters (?r - robot ?p - person ?to - landmark)
	:precondition (and
	                (not (tried_guide_person_landmark_1))
                        (medicine_location ?to)
                        (not (init_move_to_landmark))
			(init_guide_person_to_landmark_attempt)
                   )
    :effect (and
                (robot_at ?r ?to)
                (tried_guide_person_landmark_1)
                (enable_check_guide_1)
		(not (init_guide_person_to_landmark_attempt))
            )
)

;; Guide person from one landmark to another
(:action guidePersonToLandmarkAttempt2
	:parameters (?r - robot ?p - person ?to - landmark)
	:precondition (and
                        (tried_guide_person_landmark_1)
                        (not (tried_guide_person_landmark_2))
                        (medicine_location ?to)
                        (not (init_move_to_landmark))
			(init_guide_person_to_landmark_attempt)
                   )
    :effect (and
                (robot_at ?r ?to)
                (tried_guide_person_landmark_2)
                (enable_check_guide_2)
		(not (init_guide_person_to_landmark_attempt))
            )
)

;; Notify message at landmark
(:action checkGuideToSucceeded1
	:parameters (?loc - landmark)
	:precondition  (and
		            (tried_guide_person_landmark_1)
		            (enable_check_guide_1)
			    (not (init_move_to_landmark))
		            (not (init_guide_person_to_landmark_attempt))
	                )
	:observe (guide_to_succeeded_attempt_1)
)
;; Notify message at landmark
(:action checkGuideToSucceeded2
	:parameters (?loc - landmark)
	:precondition  (and
	                    (tried_guide_person_landmark_2)
	                    (enable_check_guide_2)
	                    (not (init_move_to_landmark))
			    (not (init_guide_person_to_landmark_attempt))
	                )
	:observe (guide_to_succeeded_attempt_2)
)



;; Update person location
(:action UpdatePersonLoc1
	:parameters (?p - person ?from ?to - landmark)
	:precondition (and
	                (guide_to_succeeded_attempt_1)
	                (person_at ?p ?from)
	                (medicine_location ?to)
                        (not (init_move_to_landmark))
		        (not (init_guide_person_to_landmark_attempt))
	               )
    :effect ( and
                (not (person_at ?p ?from))
                (person_at ?p ?to)
            )
)

;; Update person location
(:action UpdatePersonLoc2
	:parameters (?p - person ?from ?to - landmark)
	:precondition (and
			(guide_to_succeeded_attempt_2)
			(person_at ?p ?from)
			(medicine_location ?to)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
                   )
	:effect ( and
                (not (person_at ?p ?from))
                (person_at ?p ?to)
            )
)

;; Update success status
(:action UpdateSuccess1
	:parameters ()
	:precondition (and
			(notify_automated_succeeded)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
		)
    :effect (success)
)
;; Update success status
(:action UpdateSuccess2
	:parameters ()
	:precondition (and
			(notify_recorded_succeeded)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
		)
	:effect (success)
)
;; Update success status
(:action UpdateSuccess3
	:parameters (?p - person)
	:precondition (and
			(asked_caregiver_help ?p)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
		)
	:effect (success)
)

;; Notify message at landmark
(:action notifyAutomatedMedicineAt
	:parameters (?r - robot ?p - person ?loc - landmark)
	:precondition  (and
                        (robot_at ?r ?loc)
                        (person_at ?p ?loc)
                        (medicine_location ?loc)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
               	   )
	:observe (notify_automated_succeeded)
)

;; Notify message at landmark
(:action notifyRecordedMedicineAt
	:parameters (?r - robot ?p - person ?loc - landmark)
	:precondition (and
		        (not (notify_automated_succeeded))
		        (robot_at ?r ?loc)
		        (person_at ?p ?loc)
		        (medicine_location ?loc)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
               	   )
	:observe (notify_recorded_succeeded)
)


;; ask for caregiver to convince person to do something
(:action askCaregiverHelpMedicine1
	:parameters (?r - robot ?p - person ?loc - landmark)
	:precondition (and
			(not (notify_automated_succeeded))
			(not (notify_recorded_succeeded))
			(robot_at ?r ?loc)
			(person_at ?p ?loc)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
                   )
	:effect (asked_caregiver_help ?p)
)

;; ask for caregiver to convince person to do something
(:action askCaregiverHelpMedicine2
	:parameters (?r - robot ?p - person ?loc - landmark)
	:precondition (and
			(not (guide_to_succeeded_attempt_1))
			(not (guide_to_succeeded_attempt_2))
			(robot_at ?r ?loc)
			(person_at ?p ?loc)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
                   )
	:effect (asked_caregiver_help ?p)
)


)