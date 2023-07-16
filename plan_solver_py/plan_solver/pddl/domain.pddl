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
    (person_eating ?t - Time)
    (person_taking_medicine ?t - Time)
    (person_eating_food ?t - Time)
    (person_on_ground ?t - Time)

    ;; physical constants
    (traversable ?from ?to - Landmark)
    (bed_location ?lm - Landmark)
    (door_location ?lm - Landmark)

    ;; effects of actions
    (message_given ?m - Msg)

    ;; success conditions
    (person_at_success ?p - Person ?loc - Landmark)
    (message_given_success ?m - Msg)
    (medicine_taken_success)
    (food_eaten_success)

    ;; enable/disable actions
    (DetectPerson_enabled)
    (DetectTakingMedicine_enabled)
    (DetectEatingFood_enabled)
    (GiveReminder_enabled)
    (MakeCall_enabled)

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
    (time_critical)
    (used_move ?tc - Time)
    (current_time ?tc - Time)
    (next_time ?tc ?tn - Time)
    (time_limit ?t - Time)

    ;; constraints on the state of the world. object instances here refer to non-input instances
    (call_person_location_constraint ?a - CallAction ?p - Person ?loc - Landmark)
    (reminder_person_location_constraint ?a - ReminderAction ?p - Person ?loc - Landmark)
    (reminder_person_not_taking_medicine_constraint ?a - ReminderAction ?p - Person)
    (reminder_person_not_eating_food_constraint ?a - ReminderAction ?p - Person)
    (call_person_not_taking_medicine_constraint ?a - CallAction ?p - Person)
    (call_person_not_eating_food_constraint ?a - CallAction ?p - Person)
    (guide_person_location_constraint ?a - GuideAction ?p - Person ?loc - Landmark)

    (call_not_person_location_constraint ?a - CallAction ?p - Person ?loc - Landmark)
    (reminder_not_person_location_constraint ?a - ReminderAction ?p - Person ?loc - Landmark)
    (guide_not_person_location_constraint ?a - GuideAction ?p - Person ?loc - Landmark)

    (success)
)

;; detect if person is at location
(:action DetectTakingMedicine
    :parameters (?t - Time)
    :precondition (and
                    (DetectTakingMedicine_enabled)
                    (current_time ?t)
	                )
    :observe (person_taking_medicine ?t)
)

;; detect if person is at location
(:action DetectEatingFood
    :parameters (?t - Time)
    :precondition (and
                    (DetectEatingFood_enabled)
                    (current_time ?t)
	                )
    :observe (person_eating_food ?t)
)

;; detect if person is at location
(:action DetectPersonLocation
    :parameters (?t - Time ?p - Person ?loc - Landmark)
    :precondition (and
                    (current_time ?t)
                    (DetectPerson_enabled)
	                )
    :observe (person_at ?t ?p ?loc)
)

;;give reminder
(:action GiveReminder
    :parameters (?a - ReminderAction ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (GiveReminder_enabled)
            (current_time ?t)
            (valid_reminder_message ?a ?m)
            (not (executed_reminder ?a))
            ;; enforce that the person didn't taking medicine constraint
            (not (and (reminder_person_not_taking_medicine_constraint ?a ?p)  (not (not (person_taking_medicine ?t)) ) ) )
            ;; enforce that the person didn't eat food constraint
            (not (and (reminder_person_not_eating_food_constraint ?a ?p)  (not (not (person_eating_food ?t)) ) ) )

            ;; the robot and person must be at the same location
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
    :effect (and (message_given ?m)  (executed_reminder ?a)
              (forall (?tn - Time)
                (when (next_time ?t ?tn) (and (not (current_time ?t)) (current_time ?tn)) )
              )
            )
)

;; Wait for timestep
(:action Wait
	:parameters (?a - WaitAction)
	:precondition (and
	                (not (executed_wait ?a))
                  (forall (?ai - WaitAction)
                    (not (and (wait_blocks_wait ?ai ?a)  (not (executed_wait ?ai) ) ) )
                  )
	             )
	:effect (and (executed_wait ?a)
            (forall (?tn - Time)
              (when (next_time ?t ?tn) (and (not (current_time ?t)) (current_time ?tn)) )
            )
	)
)

;; Move to any landmark, avoiding terrain
(:action moveToLandmark
	:parameters (?t - Time ?from - Landmark ?to - Landmark)
	:precondition (and
	                (current_time ?t)
	                (not (used_move ?t))
	                (robot_at ?from)
	                (traversable ?from ?to)
	          )
	:effect (and (robot_at ?to) (not (robot_at ?from)) (used_move ?t)
	          (when (time_critical)
              (forall (?tn - Time)
                (when (next_time ?t ?tn) (and (not (current_time ?t)) (current_time ?tn)) )
              )
	          )
	        )
)

;;make call
(:action MakeCall
    :parameters (?a - CallAction ?t - Time ?p - Person ?m - Msg)
    :precondition (and
            (MakeCall_enabled)
            (current_time ?t)
            (valid_call_message ?a ?m)
            (not (executed_call ?a))
            ;; enforce that the person didn't taking medicine constraint
            (not (and (call_person_not_taking_medicine_constraint ?a ?p)  (not (not (person_taking_medicine ?t)) ) ) )
            ;; enforce that the person didn't eat food constraint
            (not (and (call_person_not_eating_food_constraint ?a ?p)  (not (not (person_eating_food ?t)) ) ) )
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
    :effect (and (message_given ?m)  (executed_call ?a)
              (forall (?tn - Time)
                (when (next_time ?t ?tn) (and (not (current_time ?t)) (current_time ?tn)) )
              )
    )
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

;; taking medicine
(:action MedicineTakenSuccess
	:parameters ()
	:precondition (and
	                (not (forall (?t - Time)
                          (not (and (medicine_taken_success) (person_taking_medicine ?t) ) )
                       )
	                )
                )
    :effect (success)
)

;; eating food
(:action FoodEatenSuccess
	:parameters ()
	:precondition (and
	                (not (forall (?t - Time)
	                        (not (and (food_eaten_success) (person_eating_food ?t) ) )
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

)