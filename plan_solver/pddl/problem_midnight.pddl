(define (problem midnight_wandering)
(:domain shr_domain)
(:objects
    bed door outside home - Landmark
    nathan - Person
    t1 t2 t3 t4 t5 - Time
    automated_msg recorded_msg call_caregiver_outside_msg call_caregiver_bed_msg call_emergency_msg - Msg
    automated_reminder recorded_reminder - ReminderAction
    caregiver_call emergency_call caregiver_call_bed - CallAction
)
(:init

	  ;; needed actions
    (DetectPerson_enabled)
    (GiveReminder_enabled)
    (MakeCall_enabled)

    (current_time t1)
    (next_time t1 t2)
    (next_time t2 t3)
    (next_time t3 t4)
    (next_time t4 t5)
    (time_critical)

    (robot_at home)
    (oneof (person_at t1 nathan door) (person_at t1 nathan outside) )
    (oneof (person_at t2 nathan door) (person_at t2 nathan outside) (person_at t2 nathan bed) )
    (oneof (person_at t3 nathan door) (person_at t3 nathan outside) (person_at t3 nathan bed) )
    (oneof (person_at t4 nathan door) (person_at t4 nathan outside) (person_at t4 nathan bed) )
    (oneof (person_at t5 nathan door) (person_at t5 nathan outside) (person_at t5 nathan bed) )

    (traversable home door)

    ;;success states
    (message_given_success call_emergency_msg)
    (message_given_success call_caregiver_bed_msg)
    (person_at_success nathan bed)

    ;; specify which actions must come before others
    (call_blocks_call caregiver_call emergency_call)
    (reminder_blocks_reminder automated_reminder recorded_reminder)
    (reminder_blocks_call recorded_reminder caregiver_call_bed)

    ;; domain specific time blocking

    ;; specify valid input argument combinations for all actions
    (valid_call_message caregiver_call call_caregiver_outside_msg)
    (valid_call_message emergency_call call_emergency_msg)
    (valid_call_message caregiver_call_bed call_caregiver_bed_msg)
    (valid_reminder_message automated_reminder automated_msg)
    (valid_reminder_message recorded_reminder recorded_msg)

    ;; specify world state constraints for all actions
    (call_person_location_constraint caregiver_call nathan outside)

    (reminder_person_location_constraint automated_reminder nathan door)

    (reminder_person_location_constraint recorded_reminder nathan door)

    (call_person_location_constraint emergency_call nathan outside)

    (call_person_location_constraint caregiver_call_bed nathan door)

)
(:goal (and (success)  ) )

)