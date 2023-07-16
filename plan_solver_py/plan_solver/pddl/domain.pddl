(define (domain Rover)
(:types rover waypoint store camera mode lander objective)

(:predicates (at ?x - rover ?y - waypoint)
             (at_lander ?x - lander ?y - waypoint)
             (can_traverse ?r - rover ?x - waypoint ?y - waypoint)
             (equipped_for_imaging ?r - rover)
	     (equipped_for_soil_analysis ?r - rover)
             (equipped_for_rock_analysis ?r - rover)
             (empty ?s - store)
             (have_rock_analysis ?r - rover)
             (have_soil_analysis ?r - rover)
             (full ?s - store)
	     (calibrated ?c - camera ?r - rover ?w - waypoint ?o - objective)
	     (supports ?c - camera ?m - mode)
             (available ?r - rover)
             (visible ?w - waypoint ?p - waypoint)
             (have_image ?r - rover ?o - objective ?m - mode)
             (communicated_image_data ?o - objective ?m - mode)
             (communicated_soil_data)
             (communicated_rock_data)
 	     (at_soil_sample ?w - waypoint)
	     (at_rock_sample ?w - waypoint)
   	     (visible_from ?o - objective ?w - waypoint)
	     (store_of ?s - store ?r - rover)
	     (calibration_target ?i - camera ?o - objective)
	     (on_board ?i - camera ?r - rover)
	     (channel_free ?l - lander)

)

(:action sense_vis
 :parameters (?x - rover ?t - objective ?z - waypoint )
 :precondition (at ?x ?z)
 :observe (visible_from ?t ?z))

(:action sense_rock
 :parameters (?x - rover ?t - objective ?z - waypoint )
 :precondition (at ?x ?z)
 :observe (at_rock_sample ?z))

(:action sense_soil
 :parameters (?x - rover ?t - objective ?z - waypoint )
 :precondition (at ?x ?z)
 :observe (at_soil_sample ?z))

(:action navigate
:parameters (?x - rover ?y - waypoint ?z - waypoint ?c - camera ?p - objective)
:precondition (and (can_traverse ?x ?y ?z) (available ?x) (at ?x ?y)
                (visible ?y ?z)
	    )
:effect (and (not (at ?x ?y)) (at ?x ?z)
(not (calibrated ?c ?x ?z ?p))
		)

)


(:action calibrate
 :parameters (?r - rover ?i - camera ?t - objective ?w - waypoint)
 :precondition
(and
	(equipped_for_imaging ?r)
	(calibration_target ?i ?t)
	(at ?r ?w)
	(on_board ?i ?r))
 :effect
(calibrated ?i ?r ?w ?t)
)


(:action take_image
 :parameters (?r - rover ?p - waypoint ?o - objective ?i - camera ?m - mode)
 :precondition (and (calibrated ?i ?r ?p ?o)
			 (on_board ?i ?r)
                      (equipped_for_imaging ?r)
                      (supports ?i ?m)
			 (visible_from ?o ?p)
                     (at ?r ?p)
               )
 :effect (and (have_image ?r ?o ?m)
              (not (calibrated ?i ?r ?p ?o))
      )
)



(:action communicate_image_data
 :parameters (?r - rover ?l - lander ?o - objective ?m - mode ?x - waypoint ?y - waypoint)
 :precondition (and (at ?r ?x)
                    (at_lander ?l ?y)
	            (have_image ?r ?o ?m)
                    (visible ?x ?y)

               )
 :effect (and
(communicated_image_data ?o ?m)
          )
)

(:action sample_soil
:parameters (?x - rover ?s - store ?p - waypoint)
:precondition (and (at ?x ?p)
                  (at_soil_sample ?p)
                   (equipped_for_soil_analysis ?x)
                   (store_of ?s ?x)
                   (empty ?s)
		   (not (full ?s))
		)
:effect (and (not (empty ?s))
                        (full ?s)
                        (have_soil_analysis ?x)
                        (not (at_soil_sample ?p))
                   ))


(:action sample_rock
:parameters (?x - rover ?s - store ?p - waypoint)
:precondition (and (at ?x ?p)
                   (at_rock_sample ?p)
                   (equipped_for_rock_analysis ?x)
                   (store_of ?s ?x)
                   (empty ?s)
		)
:effect (and	(not (empty ?s))
                        (full ?s)
                        (have_rock_analysis ?x)
                        (not (at_rock_sample ?p))
                   ))

(:action drop
:parameters (?x - rover ?y - store)
:precondition (and
		(store_of ?y ?x)
	       )
:effect
              (and (not (full ?y))
                   (empty ?y)
	      )
)

(:action communicate_soil_data
 :parameters (?r - rover ?l - lander
              ?x - waypoint ?y - waypoint)
 :precondition (and (at ?r ?x)
                    (at_lander ?l ?y)
                    (have_soil_analysis ?r)
                    (visible ?x ?y)
                    (available ?r)
                    (channel_free ?l)
                )
 :effect
	      (communicated_soil_data)

)

(:action communicate_rock_data
 :parameters (?r - rover ?l - lander
 ?x - waypoint ?y - waypoint)
 :precondition (and (at ?r ?x)
                    (at_lander ?l ?y)
                    (have_rock_analysis ?r)
                    (visible ?x ?y)
                    (available ?r)
                    (channel_free ?l)
                )
 :effect
              (communicated_rock_data)

)
)