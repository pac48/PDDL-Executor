(define (problem medicine_reminder)
(:domain shr_domain)
(:objects
    couch kitchen - Landmark
    nathan - Person
    t1 t2 t3 t4 t5 - Time
    guide_1_msg guide_2_msg automated_msg recorded_msg call_caregiver_guide_msg call_caregiver_msg - Msg
    automated_reminder recorded_reminder guide_reminder_1 guide_reminder_2 - ReminderAction
    caregiver_call caregiver_call_guide - CallAction
)
(:init

	  ;; needed actions
    (DetectPerson_enabled)
    (GiveReminder_enabled)
    (MakeCall_enabled)
    (DetectTakingMedicine_enabled)

    (current_time t1)
    (next_time t1 t2)
    (next_time t2 t3)
    (next_time t3 t4)
    (next_time t4 t5)

    (unknown (person_taking_medicine t1))
    (unknown (person_taking_medicine t2))
    (unknown (person_taking_medicine t3))
    (unknown (person_taking_medicine t4))
    (unknown (person_taking_medicine t5))

    (robot_at couch)
    (unknown (person_at t1 nathan kitchen))
    (unknown (person_at t1 nathan couch))
    (unknown (person_at t2 nathan kitchen))
    (unknown (person_at t2 nathan couch))
    (unknown (person_at t3 nathan kitchen))
    (unknown (person_at t3 nathan couch))
    (unknown (person_at t4 nathan kitchen))
    (unknown (person_at t4 nathan couch))
    (unknown (person_at t5 nathan kitchen))
    (unknown (person_at t5 nathan couch))
    (oneof (person_at t1 nathan couch) (person_at t1 nathan kitchen) )
    (oneof (person_at t2 nathan couch) (person_at t2 nathan kitchen) )
    (oneof (person_at t3 nathan couch) (person_at t3 nathan kitchen) )
    (oneof (person_at t4 nathan couch) (person_at t4 nathan kitchen) )
    (oneof (person_at t5 nathan couch) (person_at t5 nathan kitchen) )

    (traversable kitchen couch)
    (traversable couch kitchen)

    ;;success states
    (message_given_success call_caregiver_msg)
    (message_given_success call_caregiver_guide_msg)
    ;;should be kitchen (person_at_success nathan bed)

    ;; specify which actions must come before others
    (reminder_blocks_reminder automated_reminder recorded_reminder)
    (reminder_blocks_call recorded_reminder caregiver_call)
    (reminder_blocks_reminder guide_reminder_1 guide_reminder_2)
    (reminder_blocks_call guide_reminder_2 caregiver_call_guide)

    ;; specify valid input argument combinations for all actions
    (valid_call_message caregiver_call call_caregiver_msg)
    (valid_call_message caregiver_call_guide call_caregiver_guide_msg)
    (valid_reminder_message automated_reminder automated_msg)
    (valid_reminder_message recorded_reminder recorded_msg)
    (valid_reminder_message guide_reminder_1 guide_1_msg)
    (valid_reminder_message guide_reminder_2 guide_2_msg)

    ;; specify world state constraints for all actions
    (reminder_person_location_constraint automated_reminder nathan kitchen)
    (reminder_person_not_taking_medicine_constraint automated_reminder nathan)
    (reminder_person_location_constraint recorded_reminder nathan kitchen)
    (reminder_person_not_taking_medicine_constraint recorded_reminder nathan)
    (reminder_not_person_location_constraint guide_reminder_1 nathan kitchen)
    (reminder_not_person_location_constraint guide_reminder_2 nathan kitchen)
    (call_not_person_location_constraint caregiver_call_guide nathan kitchen)
    (call_person_not_taking_medicine_constraint caregiver_call nathan)

)
(:goal (and (success)  ) )

)