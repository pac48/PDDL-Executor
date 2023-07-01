(define (problem task_conditional)
(:domain paul_shr_conditional)
(:objects
    kitchen couch home couch2 - landmark
    pioneer - robot
    nathan - person
)
(:init
    (robot_at pioneer home)
    (medicine_location kitchen)
    (unknown (person_at nathan couch))
    (unknown (person_at nathan couch2))
    (unknown (person_at nathan home))
    (unknown (person_at nathan kitchen))
    (oneof (person_at nathan couch) (person_at nathan kitchen) (person_at nathan home)  (person_at nathan couch2))
    (unknown (guide_to_succeeded_attempt_1))
    (unknown (guide_to_succeeded_attempt_2))
    (unknown (notify_automated_succeeded))
    (unknown (notify_recorded_succeeded))

)

(:goal (success))
)