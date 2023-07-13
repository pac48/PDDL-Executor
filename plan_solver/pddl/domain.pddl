(define (domain shr_domain)

(:requirements :strips :typing)

(:types
	Person
	Landmark
	Msg
	Time
	CallAction
	WaitAction
	ReminderAction
	GuideAction
	GoToPersonAction
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
    (DetectMove_enabled)
    (UpdatePersonLocation_enabled)
    (GiveReminder_enabled)
    (MakeCall_enabled)
    (WaitForPersonToGoToLocation_enabled)

    ;; enforce action sequence dependencies
    (call_blocks_call ?a1 ?a2 - CallAction)
    (reminder_blocks_reminder ?a1 ?a2 - ReminderAction)
    (reminder_blocks_call ?a1 - ReminderAction ?a2 - CallAction)
    (wait_blocks_wait ?a1 - WaitAction ?a2 - WaitAction)

    (executed_call ?a - CallAction)
    (executed_reminder ?a - ReminderAction)
    (executed_guide ?a - GuideAction)
    (executed_go_to_person ?a - GoToPersonAction)
    (executed_wait ?a - WaitAction)

    ;; enforce that actions are called with valid object instances
    (valid_reminder_message ?a - ReminderAction ?m - Msg)
    (valid_call_message ?a - CallAction ?m - Msg)
    (valid_guide_message ?a - GuideAction ?m - Msg)

    ;; time management predicates
    (should_tick)
    (used_move ?tc - Time)
    (current_time ?tc - Time)
    (next_time ?tc ?tn - Time)
    (time_limit ?t - Time)

    ;; constraints on the state of the world. object instances here refer to non-input instances
    (call_person_location_constraint ?a - CallAction ?p - Person ?loc - Landmark)
    (reminder_person_location_constraint ?a - ReminderAction ?p - Person ?loc - Landmark)
    (guide_person_location_constraint ?a - GuideAction ?p - Person ?loc - Landmark)

    (call_not_person_location_constraint ?a - CallAction ?p - Person ?loc - Landmark)
    (reminder_not_person_location_constraint ?a - ReminderAction ?p - Person ?loc - Landmark)
    (guide_not_person_location_constraint ?a - GuideAction ?p - Person ?loc - Landmark)

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

;;(:action FakeTick
;;	:parameters (?tc - Time ?p - Person )
;;	:precondition (and
;;	    (current_time ?tc)
;;	    (time_limit ?tc)
;;	    (should_tick)
;;	    (forall (?loc - Landmark)
;;            (forall (?ti - Time)
;;                (not (and (not (person_at ?ti ?p ?loc)) (person_at ?ti ?p ?loc) ) )
;;           )
;;	    )
;;	    )
;;	:effect (and (not (should_tick)) )
;;)

;;give reminder
(:action GiveReminder
    :parameters (?a - ReminderAction ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (GiveReminder_enabled)
            (current_time ?t)
            (valid_reminder_message ?a ?m)
            (not (should_tick))
            (not (executed_reminder ?a))
            (not
              (forall (?loc - Landmark)
                (not (and (person_at ?t ?p ?loc) (robot_at ?loc)) )
              )
            )
            ;; certain action instances block others, for example, we must call caregiver before calling emergency
            (forall (?ai - ReminderAction)
              (not (and (reminder_blocks_reminder ?ai ?a)  (not (executed_reminder ?ai) ) ) )
            )
            ;; certain things must be true about the world state for the specific action instance
            ;; this condition enforces that the person is at the location specified in person_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (person_at ?t ?p ?loc)) (reminder_person_location_constraint ?a ?p ?loc) ) )
            )
            ;; this condition enforces that the person is not at the location specified in not_person_location_constraint
            (forall (?loc - Landmark)
              (not (and (person_at ?t ?p ?loc) (reminder_not_person_location_constraint ?a ?p ?loc) ) )
            )
		)
    :effect (and (message_given ?m)  (executed_reminder ?a)  (should_tick) )
)


;; Move to the landmark that the person is currently at
;;(:action GoToPerson
;;	:parameters (?a - GoToPersonAction ?from - Landmark ?to - Landmark ?tc - Time ?p - Person)
;;	:precondition (and
;;	                (not (executed_go_to_person ?a))
;;	                (not (should_tick))
;;	                (current_time ?tc)
;;	                (robot_at ?from)
;;	                (not (person_at ?tc ?p ?from))
;;	                (person_at ?tc ?p ?to)
;;	                ;;(traversable ?from ?to)
;;	          )
;;	:effect (and (robot_at ?to) (not (robot_at ?from))  (executed_go_to_person ?a)  (should_tick) )
;;)

;; guide person to new location
;;(:action GuidePersonTo
;;    :parameters (?a - GuideAction ?p - Person ?from - Landmark ?to - Landmark)
;;    :precondition (and
;;                    ;;TODO (DetectPerson_enabled)
;;                    (not (should_tick))
;;                    (not (executed_guide ?a))
;;                    (forall (?tn - Time)
;;                      (forall (?tc - Time)
;;                         (not (and (and (current_time ?tc) (next_time ?tc ?tn))  (not (and  (person_at ?tc ?p ?from) (robot_at ?from) ) ) ) )
;;                      )
;;                    )
;;                    (forall (?ai - CallAction)
;;                      (not (and (call_blocks_call ?ai ?a)  (not (executed_call ?ai) ) ) )
;;                    )
;;	                )
;;	:effect (and (robot_at ?to) (not (robot_at ?from)) (should_tick) (executed_guide ?a) )
;;)

;; detect if person is at location
;;(:action DetectPersonLocationNext
;;    :parameters (?tn - Time ?p - Person ?loc - Landmark)
;;    :precondition (and
;;                  (forall (?tc - Time)
;;                     (not (and (current_time ?tc)  (not (next_time ?tc ?tn) ) ) )
;;                  )
;;                    (not (should_tick))
;;                    (DetectPerson_enabled)
;;                )
;;    :observe (person_at ?tn ?p ?loc)
;;)

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

;; Move to any landmark, avoiding terrain
;;(:action DetectMove
;;	:parameters (?from - Landmark ?to - Landmark ?tc - Time ?tn - Time ?p - Person)
;;	:precondition (and
;;                    (current_time ?tc)
;;                    (next_time ?tc ?tn)
;;                    (DetectMove_enabled)
;;	                (not (should_tick))
;;	                (robot_at ?from)
;;	                (traversable ?from ?to)
;;	          )
;;	:effect (and (robot_at ?to) (not (robot_at ?from)) (should_tick) )
;;	:observe (person_at ?tn ?p ?to)
;;)

;; update person location
;;(:action UpdatePersonLocation
;;    :parameters (?tp - Time ?tc - Time ?p - Person ?loc - Landmark)
;;    :precondition (and
;;                    (UpdatePersonLocation_enabled)
;;                    (not (should_tick))
;;                    (current_time ?tc)
;;                    (next_time ?tp ?tc)
;;                    (person_at ?tp ?p ?loc)
;;                    (forall (?lm - Landmark)
;;                      (not (person_at ?tc ?p ?lm))
;;                    )
;;	                )
;;    :effect (person_at ?tc ?p ?loc)
;;)

;; Wait for timestep
(:action Wait
	:parameters (?a - WaitAction)
	:precondition (and
	                (not (should_tick))
	                (not (executed_wait ?a))
                  (forall (?ai - WaitAction)
                    (not (and (wait_blocks_wait ?ai ?a)  (not (executed_wait ?ai) ) ) )
                  )
	             )
	:effect (and (should_tick) (executed_wait ?a) )
)

;; Move to any landmark, avoiding terrain
(:action moveToLandmark
	:parameters (?t - Time ?from - Landmark ?to - Landmark)
	:precondition (and
	                (current_time ?t)
	                (not (should_tick))
	                (not (used_move ?t))
	                (robot_at ?from)
	                (traversable ?from ?to)
	          )
	:effect (and (robot_at ?to) (not (robot_at ?from)) (used_move ?t) ) ;;(should_tick)
)

;;make call
(:action MakeCall
    :parameters (?a - CallAction ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (MakeCall_enabled)
            (current_time ?t)
            (valid_call_message ?a ?m)
            (not (should_tick))
            (not (executed_call ?a))
            ;; certain action instances block others, for example, we must call caregiver before calling emergency
            (forall (?ai - CallAction)
              (not (and (call_blocks_call ?ai ?a)  (not (executed_call ?ai) ) ) )
            )
            (forall (?ai - ReminderAction)
              (not (and (reminder_blocks_call ?ai ?a)  (not (executed_reminder ?ai) ) ) )
            )
            ;; certain things must be true about the world state for the specific action instance
            ;; this condition enforces that the person is at the location specified in person_location_constraint
            (forall (?loc - Landmark)
              (not (and (not (person_at ?t ?p ?loc)) (call_person_location_constraint ?a ?p ?loc) ) )
            )
            ;; this condition enforces that the person is not at the location specified in not_person_location_constraint
            (forall (?loc - Landmark)
              (not (and (person_at ?t ?p ?loc) (call_not_person_location_constraint ?a ?p ?loc) ) )
            )
		)
    :effect (and (message_given ?m)  (executed_call ?a) (should_tick) )
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
;;(:action PersonAvoidedAtSuccess
;;	:parameters ()
;;	:precondition (and
;;                  (forall (?a - GuideAction)
;;                     (executed_guide ?a)
;;                   )
;;                  )
;;    :effect (success)
;;)

;; Update success status
;;(:action SuccessDEBUG
;;	:parameters (?t - Time)
;;	:precondition (and
;;	                  (current_time ?t)
;;                    (time_limit ?t)
;;                  )
;;    :effect (success)
;;)

)