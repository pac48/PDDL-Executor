(define (domain medicalPKS10)
(:requirements :contingent)
(:constants i0 i1 i2 i3 i4 i5 i6 i7 i8 i9 i10  - ILLNESS
s0 s1 s2 s3 s4 s5 s6 s7 s8 s9 s10 - STAIN)

 (:predicates
    ( ndead)
	(stained)
    (stain ?i - STAIN)
               (ill ?i - ILLNESS)
         )


(:action inspect-stain
   :parameters (?i - STAIN)
   :precondition (and (ndead) (stained))
   :observe (stain ?i))



(:action medicate1  :precondition (and (ndead) (ill i1) )  :effect       (when (ill i1) (ill i0)) )
 (:action medicate2  :precondition (and (ndead) (ill i2) )  :effect      (when (ill i2) (ill i0)) )
 (:action medicate3  :precondition (and (ndead) (ill i3) )  :effect      (when (ill i3) (ill i0)) )
 (:action medicate4  :precondition (and (ndead) (ill i4) )  :effect      (when (ill i4) (ill i0)) )
 (:action medicate5  :precondition (and (ndead) (ill i5) )  :effect      (when (ill i5) (ill i0)) )
 (:action medicate6  :precondition (and (ndead) (ill i6) )  :effect      (when (ill i6) (ill i0)) )
 (:action medicate7  :precondition (and (ndead) (ill i7) )  :effect      (when (ill i7) (ill i0)) )
 (:action medicate8  :precondition (and (ndead) (ill i8) )  :effect      (when (ill i8) (ill i0)) )
 (:action medicate9  :precondition (and (ndead) (ill i9) )  :effect      (when (ill i9) (ill i0)) )
 (:action medicate10  :precondition (and (ndead) (ill i10) )  :effect    (when (ill i10) (ill i0)) )

(:action stain
:parameters ()
:precondition (ndead)
:effect (and (stained)
	(when (ill i1) (stain s1))
 (when (ill i2) (stain s2))
 (when (ill i3) (stain s3))
 (when (ill i4) (stain s4))
 (when (ill i5) (stain s5))
 (when (ill i6) (stain s6))
 (when (ill i7) (stain s7))
 (when (ill i8) (stain s8))
 (when (ill i9) (stain s9))
 (when (ill i10) (stain s10))
))
)