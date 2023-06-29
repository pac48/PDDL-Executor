(define (domain midnight_wondering_domain)

(:requirements :strips :typing)

(:types
	person
	robot
	landmark
)

(:predicates
	(robot_at ?v - robot ?lm - landmark)
	(person_at ?p - person ?lm - landmark)
	(door_location ?lm - landmark)

	;; control flow
	(init_move_to_landmark)
	(init_detect_person_left_house_1)
	(init_detect_person_left_house_2)
	(init_check_bed_after_return)
	(init_check_bed_after_return2)

	;; keep track of actions performed
	(tried_notify_automated)
	(tried_notify_recorded)
	(tried_detect_person_left_house_1)
	(tried_detect_person_left_house_2)

	(called_emergency)
	(called_caregiver_wondering)
	(called_caregiver_ask_to_go_to_bed)

	;; unknowns
	(notify_automated_succeeded)
	(notify_recorded_succeeded)
	(person_decides_to_go_outside_1)
	(person_decides_to_go_outside_2)
	(person_decides_to_return_1)
	(person_decides_to_return_2)
	(person_decides_to_go_to_bed_1)
	(person_decides_to_go_to_bed_2)
	(person_goes_to_bed_after_return_1)
	(person_goes_to_bed_after_return_2)

	(success)
)

;; Notify message at landmark
(:action notifyAutomatedMidnightAt
	:parameters (?r - robot ?p - person ?loc - landmark)
	:precondition  (and
                        (robot_at ?r ?loc)
                        (person_at ?p ?loc)
                        (not (tried_notify_automated))
                        (not (init_detect_person_left_house_1))
                        (not (init_detect_person_left_house_2))
                        (not (init_check_bed_after_return))
	        	(not (init_move_to_landmark))
               	   )
	:effect (tried_notify_automated)
)

;; Notify message at landmark
(:action notifyRecordedMidnightAt
	:parameters (?r - robot ?p - person ?loc - landmark)
	:precondition (and
		        (robot_at ?r ?loc)
		        (person_at ?p ?loc)
		        (tried_notify_automated)
		        (not (person_decides_to_go_to_bed_1))
		        (not (tried_notify_recorded))
		        (not (init_detect_person_left_house_1))
		        (not (init_detect_person_left_house_2))
		        (not (init_check_bed_after_return))
	        	(not (init_move_to_landmark))
               	   )
	:effect (tried_notify_recorded)
)

;; detect if person is at location
(:action DetectPerson
    :parameters (?r - robot ?p - person ?loc - landmark)
    :precondition (and
    			(robot_at ?r ?loc)
			(not (init_detect_person_left_house_1))
            (not (init_detect_person_left_house_2))
            (not (init_check_bed_after_return))
        (not (init_move_to_landmark))
   		 )
    :observe (person_at ?p ?loc)
)

(:action initDetectPersonLeftHouse1
    :parameters (?p - person ?r - robot ?loc - landmark)
    :precondition (and
    			(tried_notify_automated)
    			(door_location ?loc)
    			(robot_at ?r ?loc)
    			(person_at ?p ?loc)
	   		(not (init_detect_person_left_house_1))
            (not (init_detect_person_left_house_2))
            (not (init_check_bed_after_return))
        (not (init_move_to_landmark))
		)
    :effect (init_detect_person_left_house_1)
)
(:action initDetectPersonLeftHouse2
    :parameters (?p - person ?r - robot ?loc - landmark)
    :precondition (and
    			(tried_notify_recorded)
    			(door_location ?loc)
    			(robot_at ?r ?loc)
    			(person_at ?p ?loc)
	   		(not (init_detect_person_left_house_1))
            (not (init_detect_person_left_house_2))
            (not (init_check_bed_after_return))
        (not (init_move_to_landmark))
		)
    :effect (init_detect_person_left_house_2)
)
;; detect if person leaves house
(:action detectPersonLeftHouse1
    :parameters (?r - robot ?loc - landmark)
    :precondition (and
    			(init_detect_person_left_house_1)
		)
    :observe (person_decides_to_go_outside_1)
)
;; detect if person leaves house
(:action detectPersonLeftHouse2
    :parameters (?r - robot ?loc - landmark)
    :precondition (and
    			(init_detect_person_left_house_2)
		)
    :observe (person_decides_to_go_outside_2)
)

;; detect human action, either: go to bed, open door, go outside
(:action personGoOutside1
    :parameters (?p - person)
    :precondition (and
    			(person_decides_to_go_outside_1)
    			(init_detect_person_left_house_1)
		)
    :effect (and
    		(forall (?loc - landmark) (not (person_at ?p ?loc)))
    		(not (init_detect_person_left_house_1))
	    )
)
(:action personGoOutside2
    :parameters (?p - person)
    :precondition (and
    			(person_decides_to_go_outside_2)
    			(init_detect_person_left_house_2)
		)
    :effect (and
    		(forall (?loc - landmark) (not (person_at ?p ?loc)))
    		(not (init_detect_person_left_house_2))
	    )
)

(:action finishDetectPerson1
    :parameters (?p - person)
    :precondition (and
			(init_detect_person_left_house_1)
			(not (person_decides_to_go_outside_1))
		)
    :effect (and
    	(not (init_detect_person_left_house_1))
    	(tried_detect_person_left_house_1)
    )
)
(:action finishDetectPerson2
    :parameters (?p - person)
    :precondition (and
			(init_detect_person_left_house_2)
			(not (person_decides_to_go_outside_2))
		)
    :effect (and
    	(not (init_detect_person_left_house_2))
    	(tried_detect_person_left_house_2)
    )
)

;; check if person went to bed
(:action checkIfPersonWentToBed1
    :parameters (?p - person ?loc - landmark)
    :precondition (and
    			(door_location ?loc)
    			(person_at ?p ?loc)
    			(tried_detect_person_left_house_1)
		)
    :observe (person_decides_to_go_to_bed_1)
)

(:action checkIfPersonWentToBed2
    :parameters (?p - person ?loc - landmark)
    :precondition (and
    			(door_location ?loc)
    			(person_at ?p ?loc)
    			(tried_detect_person_left_house_2)
		)
    :observe (person_decides_to_go_to_bed_2)
)


;;call caregiver and ask person to go to sleep
(:action callCaregiverAskToGoToBed
    :parameters (?p - person)
    :precondition (and
    			(tried_notify_automated)
         		(tried_notify_recorded)
    			(not (person_decides_to_go_to_bed_2))
    			(not (person_decides_to_go_to_bed_1))
		)
    :effect (called_caregiver_ask_to_go_to_bed)
)

(:action waitForPersonToReturn1
    :parameters (?p - person)
    :precondition (and
			(forall (?loc - landmark) (not (person_at ?p ?loc)))
		)
    :observe (person_decides_to_return_1)
)
(:action updatePersonLocation1
    :parameters (?p - person ?loc - landmark)
    :precondition (and
	                (door_location ?loc)
    			(person_decides_to_return_1)
		)
    :effect (person_at ?p ?loc)
)
(:action updatePersonLocation2
    :parameters (?p - person ?loc - landmark)
    :precondition (and
	                (door_location ?loc)
    			(person_decides_to_return_2)
		)
    :effect (person_at ?p ?loc)
)


;; call caregiver
(:action callCaregiverWondering
    :parameters (?p - person)
    :precondition (and
    			(not (person_decides_to_return_1))
    			(not (init_detect_person_left_house_1))
                (not (init_detect_person_left_house_2))
                (not (init_check_bed_after_return))
            (not (init_move_to_landmark))
		)
    :effect (called_caregiver_wondering)
)


(:action waitForPersonToReturn2
    :parameters (?p - person)
    :precondition (and
    			(called_caregiver_wondering)
			(forall (?loc - landmark) (not (person_at ?p ?loc)))
		)
    :observe (person_decides_to_return_2)
)


;; call emergency services
(:action callEmergency
    :parameters (?p - person)
    :precondition (and
    			(not (person_decides_to_return_2))
    			(not (init_detect_person_left_house_1))
                (not (init_detect_person_left_house_2))
                (not (init_check_bed_after_return))
            (not (init_move_to_landmark))
		)
    :effect (called_emergency)
)

;; Update success status
(:action UpdateSuccess2
	:parameters ()
	:precondition (and
		(not (person_decides_to_return_2))
		(called_emergency)
		)
    :effect (success)
)
;; Update success status
(:action UpdateSuccess3
	:parameters ()
	:precondition (and
		(person_decides_to_go_to_bed_1)
		)
    :effect (success)
)

;; Update success status
(:action UpdateSuccess4
	:parameters ()
	:precondition (and
		(person_decides_to_go_to_bed_2)
		)
    :effect (success)
)
;; Update success status
(:action UpdateSuccess5
	:parameters ()
	:precondition (and
		(called_caregiver_ask_to_go_to_bed)
		)
    :effect (success)
)

;; Init move
(:action initMoveToLandmark
	:parameters (?r - robot)
	:precondition (and
		        (not (init_detect_person_left_house_1))
		        (not (init_detect_person_left_house_2))
		        (not (init_check_bed_after_return))
			(not (init_move_to_landmark))
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
		      )
	:effect (and
                (robot_at ?r ?to)
                (not (init_move_to_landmark))
            )
)


 ;;Update success status
 (:action InitCheckBedAfterReturn1
 :parameters  (?p - person ?loc - landmark)
  	:precondition (and
  		(door_location ?loc)
         (person_at ?p ?loc)
		 (person_decides_to_return_1)
	 	)
     :effect (init_check_bed_after_return)
)

 ;;notify_check
 (:action CheckBedAfterReturn1
 :parameters ()
     :precondition (and
		 (init_check_bed_after_return)
	 	)
     :observe (person_goes_to_bed_after_return_1)
)
;;Update success status
 (:action UpdateSuccess0
 :parameters (?p - person ?loc - landmark)
     :precondition (and
                 (person_goes_to_bed_after_return_1)
                 )
     :effect (and
                 (not (init_check_bed_after_return))
                 (success))
)
;;call caregiver and ask person to go to sleep
(:action callCaregiverAskToGoToBedAfterReturn1
    :parameters (?p - person)
    :precondition (and
                (init_check_bed_after_return)
    			(not (person_goes_to_bed_after_return_1))
		)
    :effect (and
    (not (init_check_bed_after_return))
    (called_caregiver_ask_to_go_to_bed))
)

 ;;Update success status
 (:action InitCheckBedAfterReturn2
 :parameters  (?p - person ?loc - landmark)
 	:precondition (and
 		(door_location ?loc)
         (person_at ?p ?loc)
		 (person_decides_to_return_2)
	 	)
     :effect (init_check_bed_after_return2)
)

 ;;Update success status
 (:action CheckBedAfterReturn2
 :parameters ()
     :precondition (and
		 (init_check_bed_after_return2)
	 	)
     :observe (person_goes_to_bed_after_return_2)
)

;; Update success status

(:action UpdateSuccess1
	:parameters  ()
	:precondition (and
		(person_goes_to_bed_after_return_2)
		(not (person_decides_to_return_1))
		(called_caregiver_wondering)
	)
    :effect (success)
)
;;call caregiver and ask person to go to sleep
(:action callCaregiverAskToGoToBedAfterReturn2
    :parameters ()
    :precondition (and
                (init_check_bed_after_return2)
    			(not (person_goes_to_bed_after_return_2))
		)
    :effect (and
    (not (init_check_bed_after_return2))
    (called_caregiver_ask_to_go_to_bed))
)


)