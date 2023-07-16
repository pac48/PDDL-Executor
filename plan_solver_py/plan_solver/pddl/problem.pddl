(define (problem roverprob1234) (:domain Rover)
(:objects
        general - lander
        colour high_res low_res - mode
        rover0 - rover
        rover0store - store
        waypoint0 waypoint1 waypoint2 waypoint3 waypoint4 waypoint5 - waypoint
        camera0 - camera
        objective1 - objective
        )
(:init
	(visible waypoint1 waypoint4)
	(visible waypoint4 waypoint1)
	(visible waypoint5 waypoint4)
	(visible waypoint4 waypoint5)


	(visible waypoint1 waypoint0)
	(visible waypoint2 waypoint0)
	(visible waypoint3 waypoint0)
        (visible waypoint0 waypoint1)

        (visible waypoint0 waypoint2)
        (visible waypoint2 waypoint1)
        (visible waypoint1 waypoint2)

        (visible waypoint0 waypoint3)
        (visible waypoint3 waypoint1)
        (visible waypoint1 waypoint3)
        (visible waypoint3 waypoint2)
        (visible waypoint2 waypoint3)





        (at_lander general waypoint0)
       (channel_free general)
        (at rover0 waypoint3)
        (available rover0)
       (equipped_for_imaging rover0)
          (can_traverse rover0 waypoint1 waypoint4)
        (can_traverse rover0 waypoint4 waypoint1)
         (can_traverse rover0 waypoint5 waypoint4)
        (can_traverse rover0 waypoint4 waypoint5)
       (can_traverse rover0 waypoint3 waypoint0)
        (can_traverse rover0 waypoint0 waypoint3)
        (can_traverse rover0 waypoint3 waypoint1)
        (can_traverse rover0 waypoint1 waypoint3)
        (can_traverse rover0 waypoint1 waypoint2)
        (can_traverse rover0 waypoint2 waypoint1)


       (on_board camera0 rover0)
        (calibration_target camera0 objective1)
        (supports camera0 colour)
        (supports camera0 high_res)
         (store_of rover0store rover0)
          (equipped_for_soil_analysis rover0)
        (equipped_for_rock_analysis rover0)
 	(empty rover0store)



	(unknown (visible_from objective1 waypoint0))
	(unknown (visible_from objective1 waypoint4))
	(unknown (visible_from objective1 waypoint5))
	(oneof
         (visible_from objective1 waypoint0)
         (visible_from objective1 waypoint4)
         (visible_from objective1 waypoint5)
	)

	(unknown (at_soil_sample waypoint4))
	(unknown (at_soil_sample waypoint5))
	(unknown (at_rock_sample waypoint4))
	(unknown (at_rock_sample waypoint5))
	(oneof
	 (at_soil_sample waypoint4)
	 (at_soil_sample waypoint5)
	)
	(oneof
          (at_rock_sample waypoint4)
   	  (at_rock_sample waypoint5)
 	)

)

(:goal (and (communicated_image_data objective1 high_res)
          (communicated_soil_data)
            (communicated_rock_data)
       )
)
)
