(define (domain wandering_protocol)

(:requirements :strips :typing)

(:types
	Person
	Robot
	Landmark
	WanderingProtocol
)

(:predicates
  (wandering_protocol_enabled ?w - WanderingProtocol)

	(robot_at ?r - Robot ?lm - Landmark)
	(person_at ?p - Person ?lm - Landmark)
	(door_location ?w - WanderingProtocol ?lm - Landmark)

	;; control flow
	(init_move_to_landmark ?w - WanderingProtocol)
	(init_detect_person_left_house_1 ?w - WanderingProtocol)
	(init_detect_person_left_house_2 ?w - WanderingProtocol)
	(init_check_bed_after_return ?w - WanderingProtocol)
	(init_check_bed_after_return2 ?w - WanderingProtocol)

	;; keep track of actions performed
	(tried_notify_automated ?w - WanderingProtocol)
	(tried_notify_recorded ?w - WanderingProtocol)
	(tried_detect_person_left_house_1 ?w - WanderingProtocol)
	(tried_detect_person_left_house_2 ?w - WanderingProtocol)

	(called_emergency ?w - WanderingProtocol)
	(called_caregiver_wandering ?w - WanderingProtocol)
	(called_caregiver_ask_to_go_to_bed ?w - WanderingProtocol)

	;; unknowns
	(notify_automated_succeeded ?w - WanderingProtocol)
	(notify_recorded_succeeded ?w - WanderingProtocol)
	(person_decides_to_go_outside_1 ?w - WanderingProtocol)
	(person_decides_to_go_outside_2 ?w - WanderingProtocol)
	(person_decides_to_return_1 ?w - WanderingProtocol)
	(person_decides_to_return_2 ?w - WanderingProtocol)
	(person_decides_to_go_to_bed_1 ?w - WanderingProtocol)
	(person_decides_to_go_to_bed_2 ?w - WanderingProtocol)
	(person_goes_to_bed_after_return_1 ?w - WanderingProtocol)
	(person_goes_to_bed_after_return_2 ?w - WanderingProtocol)

	(success)
)

;; Notify message at landmark
(:action notifyAutomatedMidnightAt
	:parameters (?w - WanderingProtocol ?r - Robot ?p - Person ?loc - Landmark)
	:precondition  (and
	                      (wandering_protocol_enabled ?w)
                        (robot_at ?r ?loc)
                        (person_at ?p ?loc)
                        (not (tried_notify_automated ?w))
                        (not (init_detect_person_left_house_1 ?w))
                        (not (init_detect_person_left_house_2 ?w))
                        (not (init_check_bed_after_return ?w))
	        	(not (init_move_to_landmark ?w))
               	   )
	:effect (tried_notify_automated ?w)
)

;; Notify message at landmark
(:action notifyRecordedMidnightAt
	:parameters (?w - WanderingProtocol ?r - Robot ?p - Person ?loc - Landmark)
	:precondition (and
	          (wandering_protocol_enabled ?w)
		        (robot_at ?r ?loc)
		        (person_at ?p ?loc)
		        (tried_notify_automated ?w)
		        (not (person_decides_to_go_to_bed_1 ?w))
		        (not (tried_notify_recorded ?w))
		        (not (init_detect_person_left_house_1 ?w))
		        (not (init_detect_person_left_house_2 ?w))
		        (not (init_check_bed_after_return ?w))
	        	(not (init_move_to_landmark ?w))
               	   )
	:effect (tried_notify_recorded ?w)
)

;; detect if person is at location
(:action DetectPerson
    :parameters (?w - WanderingProtocol ?r - Robot ?p - Person ?loc - Landmark)
    :precondition (and
                    (wandering_protocol_enabled ?w)
                    (robot_at ?r ?loc)
                    (not (init_detect_person_left_house_1 ?w))
                    (not (init_detect_person_left_house_2 ?w))
                    (not (init_check_bed_after_return ?w))
                    (not (init_move_to_landmark ?w))
   		 )
    :observe (person_at ?p ?loc)
)

(:action initDetectPersonLeftHouse1
    :parameters (?w - WanderingProtocol ?p - Person ?r - Robot ?loc - Landmark)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(tried_notify_automated ?w)
    			(door_location ?w ?loc)
    			(robot_at ?r ?loc)
    			(person_at ?p ?loc)
	   		(not (init_detect_person_left_house_1 ?w))
            (not (init_detect_person_left_house_2 ?w))
            (not (init_check_bed_after_return ?w))
        (not (init_move_to_landmark ?w))
		)
    :effect (init_detect_person_left_house_1 ?w)
)
(:action initDetectPersonLeftHouse2
    :parameters (?w - WanderingProtocol ?p - Person ?r - Robot ?loc - Landmark)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(tried_notify_recorded ?w)
    			(door_location ?w ?loc)
    			(robot_at ?r ?loc)
    			(person_at ?p ?loc)
	   		(not (init_detect_person_left_house_1 ?w))
            (not (init_detect_person_left_house_2 ?w))
            (not (init_check_bed_after_return ?w))
        (not (init_move_to_landmark ?w))
		)
    :effect (init_detect_person_left_house_2 ?w)
)
;; detect if person leaves house
(:action detectPersonLeftHouse1
    :parameters (?w - WanderingProtocol ?r - Robot ?loc - Landmark)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(init_detect_person_left_house_1 ?w)
		)
    :observe (person_decides_to_go_outside_1 ?w)
)
;; detect if person leaves house
(:action detectPersonLeftHouse2
    :parameters (?w - WanderingProtocol ?r - Robot ?loc - Landmark)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(init_detect_person_left_house_2 ?w)
		)
    :observe (person_decides_to_go_outside_2 ?w)
)

;; detect human action, either: go to bed, open door, go outside
(:action personGoOutside1
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(person_decides_to_go_outside_1 ?w)
    			(init_detect_person_left_house_1 ?w)
		)
    :effect (and
    		(forall (?loc - Landmark) (not (person_at ?p ?loc)))
    		(not (init_detect_person_left_house_1 ?w))
	    )
)
(:action personGoOutside2
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(person_decides_to_go_outside_2 ?w)
    			(init_detect_person_left_house_2 ?w)
		)
    :effect (and
    		(forall (?loc - Landmark) (not (person_at ?p ?loc)))
    		(not (init_detect_person_left_house_2 ?w))
	    )
)

(:action finishDetectPerson1
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
      (wandering_protocol_enabled ?w)
			(init_detect_person_left_house_1 ?w)
			(not (person_decides_to_go_outside_1 ?w))
		)
    :effect (and
    	(not (init_detect_person_left_house_1 ?w))
    	(tried_detect_person_left_house_1 ?w)
    )
)
(:action finishDetectPerson2
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
      (wandering_protocol_enabled ?w)
			(init_detect_person_left_house_2 ?w)
			(not (person_decides_to_go_outside_2 ?w))
		)
    :effect (and
    	(not (init_detect_person_left_house_2 ?w))
    	(tried_detect_person_left_house_2 ?w)
    )
)

;; check if person went to bed
(:action checkIfPersonWentToBed1
    :parameters (?w - WanderingProtocol ?p - Person ?loc - Landmark)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(door_location ?w ?loc)
    			(person_at ?p ?loc)
    			(tried_detect_person_left_house_1 ?w)
		)
    :observe (person_decides_to_go_to_bed_1 ?w)
)

(:action checkIfPersonWentToBed2
    :parameters (?w - WanderingProtocol ?p - Person ?loc - Landmark)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(door_location ?w ?loc)
    			(person_at ?p ?loc)
    			(tried_detect_person_left_house_2 ?w)
		)
    :observe (person_decides_to_go_to_bed_2 ?w)
)


;;call caregiver and ask person to go to sleep
(:action callCaregiverAskToGoToBed
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(tried_notify_automated ?w)
         		(tried_notify_recorded ?w)
    			(not (person_decides_to_go_to_bed_2 ?w))
    			(not (person_decides_to_go_to_bed_1 ?w))
		)
    :effect (called_caregiver_ask_to_go_to_bed ?w)
)

(:action waitForPersonToReturn1
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
      (wandering_protocol_enabled ?w)
			(forall (?loc - Landmark) (not (person_at ?p ?loc)))
		)
    :observe (person_decides_to_return_1 ?w)
)
(:action updatePersonLocation1
    :parameters (?w - WanderingProtocol ?p - Person ?loc - Landmark)
    :precondition (and
                   (wandering_protocol_enabled ?w)
	                (door_location ?w ?loc)
    			        (person_decides_to_return_1 ?w)
		)
    :effect (person_at ?p ?loc)
)
(:action updatePersonLocation2
    :parameters (?w - WanderingProtocol ?p - Person ?loc - Landmark)
    :precondition (and
     	            (wandering_protocol_enabled ?w)
	                (door_location ?w ?loc)
    			(person_decides_to_return_2 ?w)
		)
    :effect (person_at ?p ?loc)
)


;; call caregiver
(:action callCaregiverwandering
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(not (person_decides_to_return_1 ?w))
    			(not (init_detect_person_left_house_1 ?w))
                (not (init_detect_person_left_house_2 ?w))
                (not (init_check_bed_after_return ?w))
            (not (init_move_to_landmark ?w))
		)
    :effect (called_caregiver_wandering ?w)
)


(:action waitForPersonToReturn2
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(called_caregiver_wandering ?w)
			(forall (?loc - Landmark) (not (person_at ?p ?loc)))
		)
    :observe (person_decides_to_return_2 ?w)
)


;; call emergency services
(:action callEmergency
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
          (wandering_protocol_enabled ?w)
    			(not (person_decides_to_return_2 ?w))
    			(not (init_detect_person_left_house_1 ?w))
                (not (init_detect_person_left_house_2 ?w))
                (not (init_check_bed_after_return ?w))
            (not (init_move_to_landmark ?w))
		)
    :effect (called_emergency ?w)
)

;; Update success status
(:action UpdateSuccess2
	:parameters (?w - WanderingProtocol )
	:precondition (and
    (wandering_protocol_enabled ?w)
		(not (person_decides_to_return_2 ?w))
		(called_emergency ?w)
		)
    :effect (and (success) (not (wandering_protocol_enabled ?w)))
)
;; Update success status
(:action UpdateSuccess3
	:parameters (?w - WanderingProtocol )
	:precondition (and
    (wandering_protocol_enabled ?w)
		(person_decides_to_go_to_bed_1 ?w)
		)
    :effect (and (success) (not (wandering_protocol_enabled ?w)))
)

;; Update success status
(:action UpdateSuccess4
	:parameters (?w - WanderingProtocol )
	:precondition (and
    (wandering_protocol_enabled ?w)
		(person_decides_to_go_to_bed_2 ?w)
		)
    :effect (and (success) (not (wandering_protocol_enabled ?w)))
)
;; Update success status
(:action UpdateSuccess5
	:parameters (?w - WanderingProtocol )
	:precondition (and
    (wandering_protocol_enabled ?w)
		(called_caregiver_ask_to_go_to_bed ?w)
		)
    :effect (and (success) (not (wandering_protocol_enabled ?w)))
)

;; Init move
(:action initMoveToLandmark
	:parameters (?w - WanderingProtocol ?r - Robot)
	:precondition (and
            (wandering_protocol_enabled ?w)
		        (not (init_detect_person_left_house_1 ?w))
		        (not (init_detect_person_left_house_2 ?w))
		        (not (init_check_bed_after_return ?w))
			(not (init_move_to_landmark ?w))
		      )
	  :effect (and
		      (forall (?loc - Landmark)
			  (not (robot_at ?r ?loc))
		      )
		      (init_move_to_landmark ?w)
  	        )
)

;; Move to any landmark, avoiding terrain
(:action moveToLandmark
	:parameters (?w - WanderingProtocol ?r - Robot ?to - Landmark)
	:precondition (and
      (wandering_protocol_enabled ?w)
			(init_move_to_landmark ?w)
		      )
	:effect (and
                (robot_at ?r ?to)
                (not (init_move_to_landmark ?w))
            )
)


 ;;Update success status
 (:action InitCheckBedAfterReturn1
 :parameters  (?w - WanderingProtocol ?p - Person ?loc - Landmark)
  	:precondition (and
      (wandering_protocol_enabled ?w)
  		(door_location ?w ?loc)
         (person_at ?p ?loc)
		 (person_decides_to_return_1 ?w)
	 	)
     :effect (init_check_bed_after_return ?w)
)

 ;;notify_check
 (:action CheckBedAfterReturn1
 :parameters (?w - WanderingProtocol )
     :precondition (and
      (wandering_protocol_enabled ?w)
		 (init_check_bed_after_return ?w)
	 	)
     :observe (person_goes_to_bed_after_return_1 ?w)
)
;;Update success status
 (:action UpdateSuccess0
 :parameters (?w - WanderingProtocol ?p - Person ?loc - Landmark)
     :precondition (and
     	          (wandering_protocol_enabled ?w)
                 (person_goes_to_bed_after_return_1 ?w)
                 )
     :effect (and (not (init_check_bed_after_return ?w))
                  (success) (not (wandering_protocol_enabled ?w)) )
)
;;call caregiver and ask person to go to sleep
(:action callCaregiverAskToGoToBedAfterReturn1
    :parameters (?w - WanderingProtocol ?p - Person)
    :precondition (and
    	          (wandering_protocol_enabled ?w)
                (init_check_bed_after_return ?w)
    			(not (person_goes_to_bed_after_return_1 ?w))
		)
    :effect (and
    (not (init_check_bed_after_return ?w))
    (called_caregiver_ask_to_go_to_bed ?w))
)

 ;;Update success status
 (:action InitCheckBedAfterReturn2
 :parameters (?w - WanderingProtocol ?p - Person ?loc - Landmark)
 	:precondition (and
 		          (wandering_protocol_enabled ?w)
 		(door_location ?w ?loc)
         (person_at ?p ?loc)
		 (person_decides_to_return_2 ?w)
	 	)
     :effect (init_check_bed_after_return2 ?w)
)

 ;;Update success status
 (:action CheckBedAfterReturn2
 :parameters (?w - WanderingProtocol )
     :precondition (and
     (wandering_protocol_enabled ?w)
		 (init_check_bed_after_return2 ?w)
	 	)
     :observe (person_goes_to_bed_after_return_2 ?w)
)

;; Update success status

(:action UpdateSuccess1
	:parameters (?w - WanderingProtocol)
	:precondition (and
    (wandering_protocol_enabled ?w)
		(person_goes_to_bed_after_return_2 ?w)
		(not (person_decides_to_return_1 ?w))
		(called_caregiver_wandering ?w)
	)
    :effect (and (success) (not (wandering_protocol_enabled ?w)) )
)
;;call caregiver and ask person to go to sleep
(:action callCaregiverAskToGoToBedAfterReturn2
    :parameters (?w - WanderingProtocol )
    :precondition (and
                (wandering_protocol_enabled ?w)
                (init_check_bed_after_return2 ?w)
    			(not (person_goes_to_bed_after_return_2 ?w))
		)
    :effect (and
    (not (init_check_bed_after_return2 ?w))
    (called_caregiver_ask_to_go_to_bed ?w))
)


)