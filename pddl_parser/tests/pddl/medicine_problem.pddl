(define (problem medicalPKS10)
(:domain medicalPKS10)

 (:init

 (stain s0)
 (ndead)


(oneof (ill i0) (ill i1) (ill i2) (ill i3) (ill i4) (ill i5) (ill i6) (ill i7) (ill i8) (ill i9) (ill i10) )

)

 (:goal (and (ill i0) (ndead))))