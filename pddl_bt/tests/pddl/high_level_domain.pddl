(define (domain high_level)

(:requirements
  :strips
  :typing
)

(:types
)

(:predicates
	;; medicine
	(time_to_take_medicine)
	(already_took_medicine)
  ;; eating
  (time_to_eat_breakfast)
  (already_ate_breakfast)
  (time_to_eat_lunch)
  (already_ate_lunch)
  (time_to_eat_dinner)
  (already_ate_dinner)
  ;; wondering
  (too_late_to_go_outside)
  ;; fall
  (person_on_ground)
  ;; priority
  (priority_1)
  (priority_2)
  (priority_3)
  (priority_4)
  (priority_5)

	(success)

)

(:action ChangePriority_1_2
	:parameters ()
	:precondition (and
	    (priority_1)
		)
	:effect (and (priority_2) (not (priority_1)))
)
(:action ChangePriority_2_3
	:parameters ()
	:precondition (and
	    (priority_2)
		)
	:effect (and (priority_3) (not (priority_2)))
)
(:action ChangePriority_3_4
	:parameters ()
	:precondition (and
	    (priority_3)
		)
	:effect (and (priority_4) (not (priority_3)))
)
(:action ChangePriority_4_5
	:parameters ()
	:precondition (and
	    (priority_4)
		)
	:effect (and (priority_5) (not (priority_4)))
)

(:action StartMedicineProtocol
	:parameters ()
	:precondition (and
	    (priority_2)
      (time_to_take_medicine)
      (not (already_took_medicine))
		)
	:effect (success)
)

(:action StartFallProtocol
	:parameters ()
	:precondition (and
	    (priority_1)
      (person_on_ground)
		)
	:effect (success)
)

(:action StartIdle
	:parameters ()
	:precondition (and
	    (priority_5)
		)
	:effect (success)
)



)

