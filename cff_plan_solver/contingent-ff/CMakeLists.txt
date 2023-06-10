#cmake_minimum_required (VERSION 3.5)
#project(FF)


set(src_files
        memory.c
        output.c
        parse.c
        inst_pre.c
        inst_easy.c
        inst_hard.c
        inst_final.c
        relax.c
        relax_ff.c
        search.c
        state_transitions.c
        repeated_states.c
        scan-fct_pddl.tab.c
        scan-ops_pddl.tab.c)

add_executable(FF main.c ${src_files})
#target_link_libraries(FF LINK_PUBLIC Hello)