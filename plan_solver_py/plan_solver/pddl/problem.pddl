(define (problem high_level_domain_problem)
(:domain high_level_domain)
(:objects
FoodProtocol1 FoodProtocol2 FoodProtocol3 - FoodProtocol
WanderingProtocol1 - WanderingProtocol
MedicineProtocol1 - MedicineProtocol
Time1 Time2 Time3 Time4 Time5 - Time
Landmark1 Landmark2 Landmark3 Landmark4 Landmark5 Landmark6 Landmark7 Landmark8 - Landmark
Person1 - Person
)
(:init
	(person_at Time5 Person1 Landmark7)
	(time_to_take_medicine WanderingProtocol1)
	(priority_1)
)
(:goal
(success))
)