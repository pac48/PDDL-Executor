(define (domain midnight_wondering_domain)

(:requirements :strips :typing)

(:types
	Person
	Landmark
	Msg
	Time
	ActionInstance
)

(:predicates
	;; physical modeling
	(robot_at ?lm - Landmark)
	(person_at ?t - Time ?p - Person ?lm - Landmark)
	;; physical constants
	(traversable ?to ?from - Landmark)
	(bed_location ?lm - Landmark)
	(door_location ?lm - Landmark)

	;; effects of actions
	(message_given ?m - Msg)

  ;; success conditions
	(person_at_success ?p - Person ?loc - Landmark)
	(message_given_success ?m - Msg)

	;; enable/disable actions
  (DetectPerson_enabled)
  (GiveReminder_enabled)
  (MakeCall_enabled)
  (WaitForPersonToGoToLocation_enabled)

  ;; action types
  (call_action_type ?a - ActionInstance)
  (give_up_action_type ?a - ActionInstance)

  ;; enforce action sequence dependencies
  (blocks ?a1 ?a2 - ActionInstance)
  (executed_action ?a - ActionInstance)

  ;; enforce that actions are called with valid object instances
  (valid_message ?a - ActionInstance ?m - Msg)
  (valid_landmark ?a - ActionInstance ?loc - Landmark)
  (valid_person ?a - ActionInstance ?p - Person)
  (valid_time ?a - ActionInstance ?t - Time)

  ;; time management predicates
  (should_tick)
  (current_time ?tc - Time)
  (next_time ?tc ?tn - Time)
  (time_limit ?t - Time)

  ;; constraints on the state of the world. object instances here refer to non-input instances
  (person_location_constraint ?a - ActionInstance ?p - Person ?loc - Landmark)
  (not_person_location_constraint ?a - ActionInstance ?p - Person ?loc - Landmark)
  (robot_location_constraint ?a - ActionInstance ?loc - Landmark)
  (message_given_constraint ?a - ActionInstance ?m - Msg)

	(success)
)

;; change time
(:action Tick
	:parameters (?tc - Time ?tn  - Time)
	:precondition (and
	    (should_tick)
	    (current_time ?tc)
	    (next_time ?tc ?tn)
		)
	:effect (and (not (current_time ?tc)) (current_time ?tn) (not (should_tick)) )
)

;;give reminder
(:action GiveReminder
    :parameters (?a - ActionInstance ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (GiveReminder_enabled)
            (current_time ?t)
            (valid_message ?a ?m)
            ;; certain action instances block others, for example, we must call caregiver before calling emergency
            (forall (?ai - ActionInstance)
              (not (and (blocks ?ai ?a)  (not (executed_action ?ai) ) ) )
            )
            ;; certain things must be true about the world state for the specific action instance
            ;; this condition enforces that the person is at the location specified in person_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (person_at ?t ?p ?loc)) (person_location_constraint ?a ?p ?loc) ) )
            )
           ;; this condition enforces that the robot is at the location specified in robot_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (robot_at ?loc)) (robot_location_constraint ?a ?loc) ) )
            )
           ;; this condition enforces that other messages have already been given
            (forall (?mi - Msg)
              (not (and (not (message_given ?mi)) (message_given_constraint ?a ?mi) ) )
            )
		)
    :effect (and (message_given ?m)  (executed_action ?a)  )
)


;; detect if person is at location
(:action DetectPersonLocation
    :parameters (?t - Time ?p - Person ?loc - Landmark)
    :precondition (and
    		            (DetectPerson_enabled)
                    (current_time ?t)
	                )
    :observe (person_at ?t ?p ?loc)
)

;;(:action WaitForPersonToGoToLocation
;;    :parameters (?tc - Time ?tn - Time ?p - Person ?loc - Landmark)
;;    :precondition (and
;;                    (WaitForPersonToGoToLocation_enabled)
;;                    (current_time ?tc)
;;                    (next_time ?tc ?tn)
;;		)
;;     :observe (person_at ?tn ?p ?loc)
;;)

;; wait for clock
(:action Wait
	:parameters ()
	:precondition (and (not (should_tick)))
	:effect (and (should_tick) )
)

;; Move to any landmark, avoiding terrain
(:action moveToLandmark
	:parameters (?from - Landmark ?to - Landmark)
	:precondition (and
	                (robot_at ?from)
	                (traversable ?to ?from)
	          )
	:effect (and (robot_at ?to) (not (robot_at ?from)) (should_tick) )
)

;;make call
(:action MakeCall
    :parameters (?a - ActionInstance ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (MakeCall_enabled)
            (current_time ?t)
            (valid_message ?a ?m)
            (call_action_type ?a)
            ;; certain action instances block others, for example, we must call caregiver before calling emergency
            (forall (?ai - ActionInstance)
              (not (and (blocks ?ai ?a)  (not (executed_action ?ai) ) ) )
            )
            ;; certain things must be true about the world state for the specific action instance
            ;; this condition enforces that the person is at the location specified in person_location_constraint
            (forall (?loc - Landmark)
              (not (and (person_location_constraint ?a ?p ?loc)  (not (person_at ?t ?p ?loc)) ) )
            )
            ;; this condition enforces that the person is not at the location specified in not_person_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (person_at ?t ?p ?loc)) (not_person_location_constraint ?a ?p ?loc) ) )
            )
           ;; this condition enforces that the robot is at the location specified in robot_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (robot_at ?loc)) (robot_location_constraint ?a ?loc) ) )
            )
           ;; this condition enforces that other messages have already been given
            (forall (?mi - Msg)
              (not (and (not (message_given ?mi)) (message_given_constraint ?a ?mi) ) )
            )
		)
    :effect (and (message_given ?m)  (executed_action ?a))
)


;; Update success status
(:action UpdateSuccess
	:parameters (?t - Time ?p - Person ?loc - Landmark)
	:precondition (and
	                (current_time ?t)
                  (bed_location ?loc)
                  (person_at ?t ?p ?loc)
                )
    :effect (success)
)

;; Update success status
(:action MessageGivenSuccess
	:parameters (?m - Msg)
	:precondition (and
                  (message_given_success ?m)
                  (message_given ?m)
                )
    :effect (success)
)

;; Update success status
(:action PersonAtSuccess
	:parameters (?t - Time ?p - Person ?loc - Landmark)
	:precondition (and
	                (current_time ?t)
                  (person_at_success ?p ?loc)
                  (person_at ?t ?p ?loc)
                )
    :effect (success)
)

;; Update success status
(:action GiveUpAction
  :parameters (?a - ActionInstance ?p - Person ?t - Time)
	:precondition (and
	                (give_up_action_type ?a)
	                (current_time ?t)
	                (time_limit ?t)
                  (forall (?ai - ActionInstance)
                    (not (and (blocks ?ai ?a)  (not (executed_action ?ai) ) ) )
                  )
                )
    :effect (success)
)



)