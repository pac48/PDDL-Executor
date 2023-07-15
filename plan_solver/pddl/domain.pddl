;unix domain

(define (domain unix)

(:types FILE DIR)
(:predicates (file_in_dir ?file - FILE ?dir - DIR)
    (sub_dir ?par_dir - DIR ?child_dir - DIR)
    (is_cur_dir ?d - DIR)
    )

(:action cd_down
 :parameters (?cur_dir - DIR ?child_dir - DIR)
 :precondition (and (is_cur_dir ?cur_dir)(sub_dir ?cur_dir ?child_dir))
 :effect (and (is_cur_dir ?child_dir)(not (is_cur_dir ?cur_dir)))
)


(:action cd_up
 :parameters (?cur_dir - DIR ?par_dir - DIR)
 :precondition (and (is_cur_dir ?cur_dir)(sub_dir ?par_dir ?cur_dir))
 :effect (and (is_cur_dir ?par_dir)(not (is_cur_dir ?cur_dir)))
)

(:action ls
 :parameters (?cur_dir - DIR ?file - FILE)
 :precondition (is_cur_dir ?cur_dir)
 :observe (file_in_dir ?file ?cur_dir)
)

(:action mv
 :parameters (?file - FILE ?cur_dir - DIR ?target_dir - DIR)
 :precondition (and (is_cur_dir ?cur_dir)(file_in_dir ?file ?cur_dir))
 :effect (and (not (file_in_dir ?file ?cur_dir))(file_in_dir ?file ?target_dir))
)

)