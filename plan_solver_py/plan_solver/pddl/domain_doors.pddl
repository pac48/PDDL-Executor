
(define (domain doors) 

   (:requirements :strips :typing :contingent)
   (:types pos )
   (:predicates (adj ?i ?j - pos) (at ?i - pos)  (opened ?i - pos)  )

   (:action sense_door
      :parameters (?i - pos ?j - pos )
      :precondition   (and (at ?i) (adj ?i ?j))
      :observe (opened ?j) )

   (:action move
      :parameters (?i - pos ?j - pos )
      :precondition (and (adj ?i ?j) (at ?i) (opened ?j))
      :effect  (and (not (at ?i)) (at ?j))
      )
)
