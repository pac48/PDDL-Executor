(define (problem midnight_wondering)
(:domain shr_domain)
(:objects
    bed door outside home - Landmark
    nathan - Person
    t1 t2 t3 t4 t5 t6 t7 t8 - Time
    automated_msg recorded_msg recorded_msg_2 recorded_msg_3 call_caregiver_outside_msg call_caregiver_bed_msg call_caregiver_bed_msg_2 call_emergency_msg call_emergency_msg_2 - Msg
    automated_reminder recorded_reminder recorded_reminder_2 recorded_reminder_3 - ReminderAction
    caregiver_call emergency_call emergency_call_2 caregiver_call_bed caregiver_call_bed_2 - CallAction
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
    (next_time t5 t6)
    (next_time t6 t7)
    (next_time t7 t8)

    (robot_at home)
    (unknown (person_at t1 nathan door))
    (unknown (person_at t1 nathan outside))
    (unknown (person_at t2 nathan outside))
    (unknown (person_at t2 nathan door))
    (unknown (person_at t2 nathan bed))
    (unknown (person_at t3 nathan outside))
    (unknown (person_at t3 nathan door))
    (unknown (person_at t3 nathan bed))
    (unknown (person_at t4 nathan outside))
    (unknown (person_at t4 nathan door))
    (unknown (person_at t4 nathan bed))
    (unknown (person_at t5 nathan outside))
    (unknown (person_at t5 nathan door))
    (unknown (person_at t5 nathan bed))
    (unknown (person_at t6 nathan outside))
    (unknown (person_at t6 nathan door))
    (unknown (person_at t6 nathan bed))
    (unknown (person_at t7 nathan outside))
    (unknown (person_at t7 nathan door))
    (unknown (person_at t7 nathan bed))
    (unknown (person_at t8 nathan outside))
    (unknown (person_at t8 nathan door))
    (unknown (person_at t8 nathan bed))
    (oneof (person_at t1 nathan door) (person_at t1 nathan outside) )
    (oneof (person_at t2 nathan door) (person_at t2 nathan outside) (person_at t2 nathan bed) )
    (oneof (person_at t3 nathan door) (person_at t3 nathan outside) (person_at t3 nathan bed) )
    (oneof (person_at t4 nathan door) (person_at t4 nathan outside) (person_at t4 nathan bed) )
    (oneof (person_at t5 nathan door) (person_at t5 nathan outside) (person_at t5 nathan bed) )
    (oneof (person_at t6 nathan door) (person_at t6 nathan outside) (person_at t6 nathan bed) )
    (oneof (person_at t7 nathan door) (person_at t7 nathan outside) (person_at t7 nathan bed) )
    (oneof (person_at t8 nathan door) (person_at t8 nathan outside) (person_at t8 nathan bed) )

    (traversable home door)

    ;;success states
    (message_given_success call_emergency_msg_2)
    (message_given_success call_caregiver_bed_msg_2)
    (person_at_success nathan bed)

    ;; specify which actions must come before others
    (call_blocks_call caregiver_call emergency_call)
    (call_blocks_call emergency_call emergency_call_2)
    (reminder_blocks_reminder automated_reminder recorded_reminder)
    (reminder_blocks_reminder recorded_reminder recorded_reminder_2)
    (reminder_blocks_reminder recorded_reminder_2 recorded_reminder_3)
    (reminder_blocks_call recorded_reminder_3 caregiver_call_bed)
    (call_blocks_call caregiver_call_bed caregiver_call_bed_2)

    ;; domain specific time blocking

    ;; specify valid input argument combinations for all actions
    (valid_call_message caregiver_call call_caregiver_outside_msg)
    (valid_call_message emergency_call call_emergency_msg)
    (valid_call_message emergency_call_2 call_emergency_msg_2)
    (valid_call_message caregiver_call_bed call_caregiver_bed_msg)
    (valid_call_message caregiver_call_bed_2 call_caregiver_bed_msg_2)
    (valid_reminder_message automated_reminder automated_msg)
    (valid_reminder_message recorded_reminder recorded_msg)
    (valid_reminder_message recorded_reminder_2 recorded_msg_2)
    (valid_reminder_message recorded_reminder_3 recorded_msg_3)

    ;; specify world state constraints for all actions
    (call_person_location_constraint caregiver_call nathan outside)

    (reminder_person_location_constraint automated_reminder nathan door)

    (reminder_person_location_constraint recorded_reminder nathan door)
    (reminder_person_location_constraint recorded_reminder_2 nathan door)
    (reminder_person_location_constraint recorded_reminder_3 nathan door)

    (call_person_location_constraint emergency_call nathan outside)
    (call_person_location_constraint emergency_call_2 nathan outside)

    (call_person_location_constraint caregiver_call_bed nathan door)
    (call_person_location_constraint caregiver_call_bed_2 nathan door)

)
(:goal (and (success)  ) )

)