(define (problem BW-rand-11)
(:domain blocksworld)
(:objects b1 b2 b3 b4 b5 b6 b7 b8 b9 b10 b11 - block)
(:init
(on-table b3)
(on b5 b3)
(unknown (on b10 b5))
(unknown (clear b10))
(unknown (on b10 b2))
(unknown (on b2 b5))
(unknown (clear b2))
(unknown (on b2 b10))
(or
(not (on b10 b2))
(not (on b2 b10))
)
(or
(not (on b2 b10))
(not (on b10 b2))
)
(oneof
(clear b10)
(clear b2)
)
(oneof
(on b10 b5)
(on b2 b5)
)
(oneof
(on b10 b5)
(on b10 b2)
)
(oneof
(on b2 b5)
(on b2 b10)
)
(oneof
(clear b10)
(on b2 b10)
)
(oneof
(clear b2)
(on b10 b2)
)
(unknown (on-table b8))
(unknown (clear b8))
(unknown (on b8 b4))
(unknown (on-table b4))
(unknown (clear b4))
(unknown (on b4 b8))
(or
(not (on b8 b4))
(not (on b4 b8))
)
(or
(not (on b4 b8))
(not (on b8 b4))
)
(oneof
(clear b8)
(clear b4)
)
(oneof
(on-table b8)
(on-table b4)
)
(oneof
(on-table b8)
(on b8 b4)
)
(oneof
(on-table b4)
(on b4 b8)
)
(oneof
(clear b8)
(on b4 b8)
)
(oneof
(clear b4)
(on b8 b4)
)
(on-table b9)
(on b11 b9)
(on b6 b11)
(unknown (on b1 b6))
(unknown (clear b1))
(unknown (on b1 b7))
(unknown (on b7 b6))
(unknown (clear b7))
(unknown (on b7 b1))
(or
(not (on b1 b7))
(not (on b7 b1))
)
(or
(not (on b7 b1))
(not (on b1 b7))
)
(oneof
(clear b1)
(clear b7)
)
(oneof
(on b1 b6)
(on b7 b6)
)
(oneof
(on b1 b6)
(on b1 b7)
)
(oneof
(on b7 b6)
(on b7 b1)
)
(oneof
(clear b1)
(on b7 b1)
)
(oneof
(clear b7)
(on b1 b7)
)
)
(:goal
(and
(on b1 b9)
(on b2 b11)
(on b3 b5)
(on b4 b8)
(on b5 b10)
(on b7 b2)
(on b9 b4)
(on b10 b6))
)
)