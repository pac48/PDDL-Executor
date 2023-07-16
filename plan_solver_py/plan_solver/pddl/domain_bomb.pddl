(define (domain bts)
  (:types package bomb toilet)
  (:predicates
   (in ?p - package ?b - bomb)
   (defused ?b - bomb)

)

  (:action senseP
   :parameters (?p - package ?b - bomb)
   :observe (in ?p ?b))

  (:action dunk
   :parameters (?p - package
                ?b - bomb
                ?t - toilet)
   :effect (and (when (in ?p ?b) (defused ?b))))

)