(define (problem high_level_domain_problem)
(:domain high_level_domain)
(:objects
	 daily_med - MedicineProtocol
	 breakfast - FoodProtocol
	 dinner - FoodProtocol
	 dinning_room - Landmark
	 daily_wand - WanderingProtocol
	 door - Landmark
	 t2 - Time
	 nathan - Person
	 kitchen - Landmark
	 living_room - Landmark
	 couch - Landmark
	 t1 - Time
	 outside - Landmark
	 t3 - Time
	 t4 - Time
	 bed - Landmark
	 bathroom - Landmark
	 t5 - Time
	 lunch - FoodProtocol
)
(:init
	(priority_1)
	(medicine_protocol_enabled daily_med)
	(person_at t1 nathan door)
	(person_at_door daily_wand)
	(time_to_take_medicine daily_med)
	(unknown (person_at t2 nathan bed))
	(unknown (person_at t2 nathan outside))
	(unknown (person_at t2 nathan door))
	(unknown (person_at t3 nathan outside))
	(unknown (person_at t3 nathan door))
	(unknown (person_at t2 nathan couch))
	(unknown (person_at t2 nathan kitchen))
	(unknown (person_at t5 nathan bed))
	(unknown (person_at t5 nathan outside))
	(unknown (person_at t5 nathan door))
	(unknown (person_at t4 nathan bed))
	(unknown (person_at t4 nathan outside))
	(unknown (person_at t4 nathan door))
	(unknown (person_at t3 nathan bed))
	(unknown (person_at t5 nathan kitchen))
	(unknown (person_at t5 nathan couch))
	(unknown (person_at t4 nathan kitchen))
	(unknown (person_at t4 nathan couch))
	(unknown (person_at t3 nathan kitchen))
	(unknown (person_at t3 nathan couch))
	(oneof (person_at t2 nathan couch) (person_at t2 nathan kitchen))
	(oneof (person_at t3 nathan couch) (person_at t3 nathan kitchen))
	(oneof (person_at t4 nathan couch) (person_at t4 nathan kitchen))
	(oneof (person_at t5 nathan couch) (person_at t5 nathan kitchen))
	(oneof (person_at t2 nathan door) (person_at t2 nathan outside) (person_at t2 nathan bed))
	(oneof (person_at t3 nathan door) (person_at t3 nathan outside) (person_at t3 nathan bed))
	(oneof (person_at t4 nathan door) (person_at t4 nathan outside) (person_at t4 nathan bed))
	(oneof (person_at t5 nathan door) (person_at t5 nathan outside) (person_at t5 nathan bed))
	(oneof (person_at t2 nathan couch) (person_at t2 nathan kitchen))
	(oneof (person_at t3 nathan couch) (person_at t3 nathan kitchen))
	(oneof (person_at t4 nathan couch) (person_at t4 nathan kitchen))
	(oneof (person_at t5 nathan couch) (person_at t5 nathan kitchen))
)
(:goal (success))
)