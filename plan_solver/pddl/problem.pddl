(define (problem midnight_wondering)
(:domain midnight_wondering_domain)
(:objects
    bed kitchen couch home door outside - Landmark
    nathan - Person
    t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 - Time
    automated_msg recorded call_caregiver_outside call_caregiver_bed_msg call_emergency - Msg
    give_up caregiver_call emergency_call automated_reminder caregiver_call_bed - ActionInstance
)
(:init

	  ;; needed actions
    (DetectPerson_enabled)
    ;; (GiveReminder_enabled)
    (MakeCall_enabled)
    (WaitForPersonToGoToLocation_enabled)

    (current_time t1)
    (next_time t1 t2)
    (next_time t2 t3)
    (next_time t3 t4)
    (next_time t4 t5)
    (next_time t5 t6)
    (next_time t6 t7)
    (next_time t7 t8)
    (next_time t8 t9)
    (next_time t9 t10)
    (time_limit t10)

    (robot_at kitchen)
    (person_at t1 nathan kitchen)
    (person_at t5 nathan outside)
    (person_at t7 nathan bed)
    (unknown (person_at t2 nathan outside))
    (unknown (person_at t2 nathan door))
    (unknown (person_at t2 nathan bed))
    (unknown (person_at t3 nathan outside))
    (unknown (person_at t3 nathan door))
    (unknown (person_at t3 nathan bed))
    ;;(unknown (person_at t4 nathan outside))
    ;;(unknown (person_at t4 nathan door))
    ;;(unknown (person_at t4 nathan bed))
    (oneof (person_at t2 nathan door) (person_at t2 nathan outside) (person_at t2 nathan bed) )
    (oneof (person_at t3 nathan door) (person_at t3 nathan outside) (person_at t3 nathan bed) )
    ;;(oneof (person_at t4 nathan door) (person_at t4 nathan outside) (person_at t4 nathan bed) )

    (traversable bed kitchen)
    (traversable kitchen bed)
    (traversable door kitchen)
    (traversable kitchen door)
    (door_location door)
    (bed_location bed)

    ;;success states
    (message_given_success call_emergency)
    (message_given_success call_caregiver_bed_msg)
    (person_at_success nathan bed)

    ;; specify which actions must come before others
    (blocks caregiver_call emergency_call)
    (blocks automated_reminder caregiver_call_bed)
    ;;(blocks emergency_call give_up)
    ;;(blocks automated_reminder give_up)


    ;; specify valid input argument combinations for all actions
    (valid_message caregiver_call call_caregiver_outside)
    (valid_message emergency_call call_emergency)
    (valid_message caregiver_call_bed call_caregiver_bed_msg)
    (valid_message automated_reminder automated_msg)

    ;; specify world state constraints for all actions
    (person_location_constraint caregiver_call nathan outside)
    (robot_location_constraint caregiver_call door)

    (person_location_constraint automated_reminder nathan door)
    (robot_location_constraint automated_reminder door)

    (person_location_constraint emergency_call nathan outside)
    (person_location_constraint emergency_call nathan outside)
    (robot_location_constraint emergency_call door)

    (not_person_location_constraint caregiver_call_bed nathan bed)

    ;;types
    (call_action_type caregiver_call)
    (call_action_type emergency_call)
    (call_action_type caregiver_call_bed)
    (give_up_action_type give_up)


)
(:goal (success)
)
)  