

(define (problem BW-rand-7)
(:domain blocksworld)
(:objects b1 b2 b3 b4 b5 b6 b7 )
(:init
(unknown (ontable b2))
(unknown (clear b2))
(unknown (on b2 b5))
(unknown (ontable b5))
(unknown (clear b5))
(unknown (on b5 b2))
(or
(not (on b2 b5))
(not (on b5 b2))
)
(or
(not (on b5 b2))
(not (on b2 b5))
)
(oneof
(clear b2)
(clear b5)
)
(oneof
(ontable b2)
(ontable b5)
)
(oneof
(ontable b2)
(on b2 b5)
)
(oneof
(ontable b5)
(on b5 b2)
)
(oneof
(clear b2)
(on b5 b2)
)
(oneof
(clear b5)
(on b2 b5)
)
(ontable b3)
(unknown (on b1 b3))
(unknown (clear b1))
(unknown (on b1 b6))
(unknown (on b6 b3))
(unknown (clear b6))
(unknown (on b6 b1))
(or
(not (on b1 b6))
(not (on b6 b1))
)
(or
(not (on b6 b1))
(not (on b1 b6))
)
(oneof
(clear b1)
(clear b6)
)
(oneof
(on b1 b3)
(on b6 b3)
)
(oneof
(on b1 b3)
(on b1 b6)
)
(oneof
(on b6 b3)
(on b6 b1)
)
(oneof
(clear b1)
(on b6 b1)
)
(oneof
(clear b6)
(on b1 b6)
)
(unknown (ontable b4))
(unknown (clear b4))
(unknown (on b4 b7))
(unknown (ontable b7))
(unknown (clear b7))
(unknown (on b7 b4))
(or
(not (on b4 b7))
(not (on b7 b4))
)
(or
(not (on b7 b4))
(not (on b4 b7))
)
(oneof
(clear b4)
(clear b7)
)
(oneof
(ontable b4)
(ontable b7)
)
(oneof
(ontable b4)
(on b4 b7)
)
(oneof
(ontable b7)
(on b7 b4)
)
(oneof
(clear b4)
(on b7 b4)
)
(oneof
(clear b7)
(on b4 b7)
)
)
(:goal
(and
(on b1 b4)
(on b2 b1)
(on b3 b6)
(on b5 b7))
)
)


