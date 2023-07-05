(define (problem midnight_wondering)
(:domain midnight_wondering_domain)
(:objects
    couch kitchen - Landmark
    nathan - Person
    t1 t2 t3 t4 t5 t6 t7 - Time
    ;;  t8 t9 t10 - Time
    guide_1_msg guide_2_msg automated_msg recorded_msg call_caregiver_guide_msg call_caregiver_msg - Msg
    automated_reminder recorded_reminder guide_reminder_1 guide_reminder_2 - ReminderAction
    caregiver_call caregiver_call_guide - CallAction
    guide_person_1 guide_person_2 - GuideAction
    go_to_person_1 go_to_person_2 - GoToPersonAction
)
(:init

	  ;; needed actions
    (DetectPerson_enabled)
    (UpdatePersonLocation_enabled)
    (GiveReminder_enabled)
    (MakeCall_enabled)

    (current_time t1)
    (next_time t1 t2)
    (next_time t2 t3)
    (next_time t3 t4)
    (next_time t4 t5)
    (next_time t5 t6)
    (next_time t6 t7)

    ;;(next_time t5 t6)
    ;;(next_time t6 t7)
    ;;(next_time t7 t8)
    ;;(next_time t8 t9)
    ;;(next_time t9 t10)
    ;;(time_limit t10)

    (robot_at couch)
    (person_at t1 nathan couch)
    (unknown (person_at t2 nathan kitchen))
    (unknown (person_at t2 nathan couch))
    (unknown (person_at t3 nathan kitchen))
    (unknown (person_at t3 nathan couch))
    (unknown (person_at t4 nathan kitchen))
    (unknown (person_at t4 nathan couch))
    (unknown (person_at t5 nathan kitchen))
    (unknown (person_at t5 nathan couch))
    (unknown (person_at t6 nathan kitchen))
    (unknown (person_at t6 nathan couch))
    (unknown (person_at t7 nathan kitchen))
    (unknown (person_at t7 nathan couch))
    ;;(unknown (person_at t8 nathan kitchen))
    ;;(unknown (person_at t8 nathan couch))
    ;;(unknown (person_at t9 nathan kitchen))
    ;;(unknown (person_at t9 nathan couch))
    ;;(unknown (person_at t10 nathan kitchen))
    ;;(unknown (person_at t10 nathan couch))
    (oneof (person_at t2 nathan couch) (person_at t2 nathan kitchen) )
    (oneof (person_at t3 nathan couch) (person_at t3 nathan kitchen) )
    (oneof (person_at t4 nathan couch) (person_at t4 nathan kitchen)  )
    (oneof (person_at t5 nathan couch) (person_at t5 nathan kitchen)  )
    (oneof (person_at t6 nathan couch) (person_at t6 nathan kitchen)  )
    (oneof (person_at t7 nathan couch) (person_at t7 nathan kitchen)  )
    ;;(oneof (person_at t8 nathan couch) (person_at t8 nathan kitchen)  )
    ;;(oneof (person_at t9 nathan couch) (person_at t9 nathan kitchen)  )
    ;;(oneof (person_at t10 nathan couch) (person_at t10 nathan kitchen) )

    (traversable kitchen couch)
    (traversable couch kitchen)
    ;;(traversable kitchen bed)
    ;;(traversable bed kitchen)
    ;;(traversable couch bed)
    ;;(traversable bed couch)

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
    (reminder_person_location_constraint recorded_reminder nathan kitchen)

    (reminder_not_person_location_constraint guide_reminder_1 nathan kitchen)
    (reminder_not_person_location_constraint guide_reminder_2 nathan kitchen)
    (call_not_person_location_constraint caregiver_call_guide nathan kitchen)

)
(:goal (and (success)  ) )

)