(define (problem blocksworld_problem)
(:domain blocksworld)
(:objects
)
(:init
	(on-table b3)
	(unknown (on-table b4))
	(unknown (on-table b5))
	(unknown (on-table b2))
	(unknown (on-table b7))
	(unknown (clear b6))
	(unknown (clear b1))
	(unknown (clear b5))
	(unknown (clear b2))
	(unknown (clear b4))
	(unknown (clear b7))
	(unknown (on b7 b4))
	(unknown (on b4 b7))
	(unknown (on b2 b5))
	(unknown (on b5 b2))
	(unknown (on b1 b3))
	(unknown (on b6 b1))
	(unknown (on b6 b3))
	(unknown (on b1 b6))
	(or (and
(not (on b2 b5) )
(not (on b5 b2) )
))
	(or (and
(not (on b5 b2) )
(not (on b2 b5) )
))
	(oneof (clear b2) (clear b5))
	(oneof (on-table b2) (on-table b5))
	(oneof (on-table b2) (on b2 b5))
	(oneof (on-table b5) (on b5 b2))
	(oneof (clear b2) (on b5 b2))
	(oneof (clear b5) (on b2 b5))
	(or (and
(not (on b1 b6) )
(not (on b6 b1) )
))
	(or (and
(not (on b6 b1) )
(not (on b1 b6) )
))
	(oneof (clear b1) (clear b6))
	(oneof (on b1 b3) (on b6 b3))
	(oneof (on b1 b3) (on b1 b6))
	(oneof (on b6 b3) (on b6 b1))
	(oneof (clear b1) (on b6 b1))
	(oneof (clear b6) (on b1 b6))
	(or (and
(not (on b4 b7) )
(not (on b7 b4) )
))
	(or (and
(not (on b7 b4) )
(not (on b4 b7) )
))
	(oneof (clear b4) (clear b7))
	(oneof (on-table b4) (on-table b7))
	(oneof (on-table b4) (on b4 b7))
	(oneof (on-table b7) (on b7 b4))
	(oneof (clear b4) (on b7 b4))
	(oneof (clear b7) (on b4 b7))
)
(:goal (success))
)