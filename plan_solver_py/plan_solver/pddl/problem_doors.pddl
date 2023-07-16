(define (problem doors-5)
(:domain doors)

 (:objects 
    p1_1
    p1_2
    p1_3
    p1_4
    p1_5
    p2_1
    p2_2
    p2_3
    p2_4
    p2_5
    p3_1
    p3_2
    p3_3
    p3_4
    p3_5
    p4_1
    p4_2
    p4_3
    p4_4
    p4_5
    p5_1
    p5_2
    p5_3
    p5_4
    p5_5
 - pos
    )
   (:init
      (at p1_3)
     (adj p1_1 p2_1)
     (adj p2_1 p1_1)

     (adj p1_2 p2_2)
     (adj p2_2 p1_2)

     (adj p1_3 p2_3)
     (adj p2_3 p1_3)

     (adj p1_4 p2_4)
     (adj p2_4 p1_4)

     (adj p1_5 p2_5)
     (adj p2_5 p1_5)

     (adj p2_1 p3_1)
     (adj p3_1 p2_1)

     (adj p2_2 p3_2)
     (adj p3_2 p2_2)

     (adj p2_3 p3_3)
     (adj p3_3 p2_3)

     (adj p2_4 p3_4)
     (adj p3_4 p2_4)

     (adj p2_5 p3_5)
     (adj p3_5 p2_5)

     (adj p3_1 p4_1)
     (adj p4_1 p3_1)

     (adj p3_2 p4_2)
     (adj p4_2 p3_2)

     (adj p3_3 p4_3)
     (adj p4_3 p3_3)

     (adj p3_4 p4_4)
     (adj p4_4 p3_4)

     (adj p3_5 p4_5)
     (adj p4_5 p3_5)

     (adj p4_1 p5_1)
     (adj p5_1 p4_1)

     (adj p4_2 p5_2)
     (adj p5_2 p4_2)

     (adj p4_3 p5_3)
     (adj p5_3 p4_3)

     (adj p4_4 p5_4)
     (adj p5_4 p4_4)

     (adj p4_5 p5_5)
     (adj p5_5 p4_5)


     (adj p1_1 p1_2)
     (adj p1_2 p1_1)

     (adj p2_1 p2_2)
     (adj p2_2 p2_1)

     (adj p3_1 p3_2)
     (adj p3_2 p3_1)

     (adj p4_1 p4_2)
     (adj p4_2 p4_1)

     (adj p5_1 p5_2)
     (adj p5_2 p5_1)

     (adj p1_2 p1_3)
     (adj p1_3 p1_2)

     (adj p2_2 p2_3)
     (adj p2_3 p2_2)

     (adj p3_2 p3_3)
     (adj p3_3 p3_2)

     (adj p4_2 p4_3)
     (adj p4_3 p4_2)

     (adj p5_2 p5_3)
     (adj p5_3 p5_2)

     (adj p1_3 p1_4)
     (adj p1_4 p1_3)

     (adj p2_3 p2_4)
     (adj p2_4 p2_3)

     (adj p3_3 p3_4)
     (adj p3_4 p3_3)

     (adj p4_3 p4_4)
     (adj p4_4 p4_3)

     (adj p5_3 p5_4)
     (adj p5_4 p5_3)

     (adj p1_4 p1_5)
     (adj p1_5 p1_4)

     (adj p2_4 p2_5)
     (adj p2_5 p2_4)

     (adj p3_4 p3_5)
     (adj p3_5 p3_4)

     (adj p4_4 p4_5)
     (adj p4_5 p4_4)

     (adj p5_4 p5_5)
     (adj p5_5 p5_4)


        (opened p1_1)
        (opened p1_2)
        (opened p1_3)
        (opened p1_4)
        (opened p1_5)
     (oneof
        (opened p2_1)
        (opened p2_2)
        (opened p2_3)
        (opened p2_4)
        (opened p2_5)
     )
        (opened p3_1)
        (opened p3_2)
        (opened p3_3)
        (opened p3_4)
        (opened p3_5)
     (oneof
        (opened p4_1)
        (opened p4_2)
        (opened p4_3)
        (opened p4_4)
        (opened p4_5)
     )
        (opened p5_1)
        (opened p5_2)
        (opened p5_3)
        (opened p5_4)
        (opened p5_5)
    )
    (:goal (and (at p5_3))
))