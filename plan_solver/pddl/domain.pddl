(define (domain bomb)
  (:types obj)
   (:predicates (bomb ?x - obj)
                (toilet ?x - obj)
                (armed ?x - obj)
		(clogged ?x - obj)
		)

   (:action dunk
       :parameters  (?bomb ?toilet - obj)
       :precondition (and (bomb ?bomb) (toilet ?toilet) 
                          (not (clogged ?toilet)))
       :effect (and (when (armed ?bomb) (not (armed ?bomb)))
                    (clogged ?toilet)))

   (:action flush
       :parameters  (?toilet - obj)
       :precondition (toilet ?toilet) 
       :effect (when (clogged ?toilet) (not (clogged ?toilet))))

)