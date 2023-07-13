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
	(traversable ?from ?to - Landmark)
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

  ;; enforce action sequence dependencies
  (call_blocks_call ?a1 ?a2 - CallAction)
  (reminder_blocks_reminder ?a1 ?a2 - ReminderAction)
  (reminder_blocks_call ?a1 - ReminderAction ?a2 - CallAction)

  (executed_action ?a - CallAction)
  (executed_action ?a - ReminderAction)
  (executed_action ?a - GuideAction)

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
	:parameters (?tc - Time ?tn - Time)
	:precondition (and
	    (current_time ?tc)
	    (next_time ?tc ?tn)
	    (should_tick)
		)
	:effect (and (not (current_time ?tc)) (current_time ?tn) (not (should_tick)) )
)

;;give reminder
(:action GiveReminder
    :parameters (?a - ActionInstance ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (GiveReminder_enabled)
            (reminder_action_type ?a)
            (current_time ?t)
            (valid_message ?a ?m)
            (not (should_tick))
            (not (executed_action ?a))
            (not
              (forall (?loc - Landmark)
                (not (and (person_at ?t ?p ?loc) (robot_at ?loc)) )
              )
            )

            ;; certain action instances block others, for example, we must call caregiver before calling emergency
            (forall (?ai - ActionInstance)
              (not (and (blocks ?ai ?a)  (not (executed_action ?ai) ) ) )
            )
            ;; certain things must be true about the world state for the specific action instance
            ;; this condition enforces that the person is at the location specified in person_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (person_at ?t ?p ?loc)) (person_location_constraint ?a ?p ?loc) ) )
            )
            ;; this condition enforces that the person is not at the location specified in not_person_location_constraint
            (forall (?loc - Landmark)
              (not (and (person_at ?t ?p ?loc) (not_person_location_constraint ?a ?p ?loc) ) )
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
    :effect (and (message_given ?m)  (executed_action ?a)  (should_tick) )
)


;; Move to the landmark that the person is currently at
;;(:action GoToPerson
;;	:parameters (?a - ActionInstance ?from - Landmark ?to - Landmark ?tc - Time ?p - Person)
;;	:precondition (and
;;	                (go_to_person_action_type ?a)
;;	                (not (executed_action ?a))
;;	                (not (should_tick))
;;	                (current_time ?tc)
;;	                (robot_at ?from)
;;	                (not (person_at ?tc ?p ?from))
;;	                (person_at ?tc ?p ?to)
;;                 (forall (?ai - ActionInstance)
;;                    (not (and (blocks ?ai ?a)  (not (executed_action ?ai) ) ) )
;;                  )
;;	                ;;(traversable ?from ?to)
;;	          )
;;	:effect (and (robot_at ?to) (not (robot_at ?from))  (executed_action ?a)  (should_tick) )
;;)

;; guide person to new location
(:action GuidePersonTo
    :parameters (?p - Person ?from - Landmark ?to - Landmark)
    :precondition (and
                    ;;TODO (DetectPerson_enabled)
                    (not (should_tick))
                    (forall (?tn - Time)
                      (forall (?tc - Time)
                         (not (and (current_time ?tc)  (not (and (next_time ?tc ?tn) (person_at ?tc ?p ?from) (robot_at ?from) ) ) ) )
                      )
                    )
	                )
	:effect (and (robot_at ?to) (not (robot_at ?from)) (should_tick) )
)

;; detect if person is at location
(:action DetectPersonLocation
    :parameters (?t - Time ?p - Person ?loc - Landmark)
    :precondition (and
                    (current_time ?t)
                    (not (should_tick))
                    (DetectPerson_enabled)
	                )
    :observe (person_at ?t ?p ?loc)
)

;; update person location
(:action UpdatePersonLocation
    :parameters (?tp - Time ?tc - Time ?p - Person ?loc - Landmark)
    :precondition (and
                    (not (should_tick))
                    (current_time ?tc)
                    (next_time ?tp ?tc)
                    (person_at ?tp ?p ?loc)
                    (forall (?lm - Landmark)
                      (not (person_at ?tc ?p ?lm))
                    )
	                )
    :effect (person_at ?tc ?p ?loc)
)



;; Move to any landmark, avoiding terrain
(:action moveToLandmark
	:parameters (?from - Landmark ?to - Landmark)
	:precondition (and
	                (not (should_tick))
	                (robot_at ?from)
	                (traversable ?from ?to)
	          )
	:effect (and (robot_at ?to) (not (robot_at ?from)) (should_tick) )
)

;;make call
(:action MakeCall
    :parameters (?a - ActionInstance ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (MakeCall_enabled)
            (call_action_type ?a)
            (current_time ?t)
            (valid_message ?a ?m)
            (not (should_tick))
            (not (executed_action ?a))
            ;; certain action instances block others, for example, we must call caregiver before calling emergency
            (forall (?ai - ActionInstance)
              (not (and (blocks ?ai ?a)  (not (executed_action ?ai) ) ) )
            )
            ;; certain things must be true about the world state for the specific action instance
            ;; this condition enforces that the person is at the location specified in person_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (person_at ?t ?p ?loc)) (person_location_constraint ?a ?p ?loc) ) )
            )
            ;; this condition enforces that the person is not at the location specified in not_person_location_constraint
            (forall (?loc - Landmark)
              (not (and (person_at ?t ?p ?loc) (not_person_location_constraint ?a ?p ?loc) ) )
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
    :effect (and (message_given ?m)  (executed_action ?a) (should_tick) )
)

;; Update success status
(:action MessageGivenSuccess
	:parameters ()
	:precondition (and
	                (not
                        (forall (?m - Msg)
                          (not (and (message_given_success ?m) (message_given ?m) ) )
                        )
                    )
                  )
    :effect (success)
)

;; Update success status
(:action PersonAtSuccess
	:parameters (?p - Person ?t - Time ?loc - Landmark)
	:precondition (and
	                (current_time ?t)
	                (person_at ?t ?p ?loc)
	                (person_at_success ?p ?loc)
                  )
    :effect (success)
)

;; Update success status
(:action PersonAvoidedAtSuccess
	:parameters ()
	:precondition (and
	              (forall (?a - ActionInstance)
	                (not (and (go_to_person_action_type ?a)  (not (executed_action ?a) ) ) )
	              )
                  )
    :effect (success)
)

)