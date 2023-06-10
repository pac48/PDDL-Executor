


/*
 * THIS SOURCE CODE IS SUPPLIED  ``AS IS'' WITHOUT WARRANTY OF ANY KIND, 
 * AND ITS AUTHOR AND THE JOURNAL OF ARTIFICIAL INTELLIGENCE RESEARCH 
 * (JAIR) AND JAIR'S PUBLISHERS AND DISTRIBUTORS, DISCLAIM ANY AND ALL 
 * WARRANTIES, INCLUDING BUT NOT LIMITED TO ANY IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND
 * ANY WARRANTIES OR NON INFRINGEMENT.  THE USER ASSUMES ALL LIABILITY AND
 * RESPONSIBILITY FOR USE OF THIS SOURCE CODE, AND NEITHER THE AUTHOR NOR
 * JAIR, NOR JAIR'S PUBLISHERS AND DISTRIBUTORS, WILL BE LIABLE FOR 
 * DAMAGES OF ANY KIND RESULTING FROM ITS USE.  Without limiting the 
 * generality of the foregoing, neither the author, nor JAIR, nor JAIR's
 * publishers and distributors, warrant that the Source Code will be 
 * error-free, will operate without interruption, or will meet the needs 
 * of the user.
 */











/*********************************************************************
 *
 * File: search.c
 *
 * Description: implementation of routines that search the "state" space
 *
 *
 * Author: Joerg Hoffmann 2004
 *
 *********************************************************************/









#include "ff.h"

#include "output.h"
#include "memory.h"

#include "relax.h"
#include "state_transitions.h"
#include "repeated_states.h"
#include "search.h"














/*****************
 * LOCAL GLOBALS *
 *****************/

























/************************************
 * BEST FIRST SEARCH IMPLEMENTATION *
 ************************************/


























void do_best_first_search(void) {

    static Bool fc = TRUE;
    static State S;
    static int *A;

    BfsNode *open_leaf, *tmpbfs;
    long i, minlf = INFINITY, minif = INFINITY, maxd = -1;
    long num_A;
    Bool start = TRUE;
    BfsEdge *tmpedge;

    if (fc) {
        make_state(&S, gnum_ft_conn);
        A = (int *) calloc(gnum_op_conn, sizeof(int));
        fc = FALSE;
    }

    /* create initial search node; first, its (single-leaf-node) plantree
     */
    gbfs_initial_state = new_BfsNode();
    make_state(&(gbfs_initial_state->S), ginitial_state.num_F);
    source_to_dest(&(gbfs_initial_state->S), &(ginitial_state));
    if (contains_goal(&(gbfs_initial_state->S))) {
        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("\ninitial state contains goal!");
        }
        gbfs_initial_state->solved = TRUE;
        gbfs_initial_state->f = 0;
    } else {
        if (gcmd_line.helpful) {
            gbfs_initial_state->f = get_1P_and_H(gbfs_initial_state);
            gbfs_initial_state->H = (int *) calloc(gnum_H, sizeof(int));
            for (i = 0; i < gnum_H; i++) {
                gbfs_initial_state->H[i] = gH[i];
            }
            gbfs_initial_state->num_H = gnum_H;
        } else {
            gbfs_initial_state->f = get_1P(gbfs_initial_state);
        }
        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("\ninitial state 1P: %f, weighted %f",
                   gbfs_initial_state->f, gcmd_line.hweight * gbfs_initial_state->f);
        }
    }
    if (gbfs_initial_state->f == INFINITY) {
        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("\ninitial h value infty!");
        }
        gbfs_initial_state->failed = TRUE;
    } else {
        gbfs_initial_state->f *= gcmd_line.hweight;
    }
    gbfs_initial_state->astar_f = gbfs_initial_state->f;
    gbfs_initial_state->depth = 0;

    /* search: expand nongoal leaf of current best subtree, until
     * the best subtree has no nongoal leaf -> is a plan
     */
    while (TRUE) {

        /* plan existence and non-existence are propagated through
         * the search space when they are detected.
         */
        if (gbfs_initial_state->solved ||
            gbfs_initial_state->failed) {
            break;
        }

        /* none of both -> find an open leaf of currently best subtree
         */
        open_leaf = get_open_leaf_of_best_subtree();
        if (open_leaf->depth == MAX_PLAN_LENGTH) {
            printf("\nincrease MAX_PLAN_LENGTH! currently %d\n\n", MAX_PLAN_LENGTH);
            exit(1);
        }

        /* screen printout
         */
        if (LESS(open_leaf->f, minlf) ||
            LESS(gbfs_initial_state->f, minif) ||
            maxd < open_leaf->depth) {
            if (LESS(open_leaf->f, minlf)) {
                minlf = open_leaf->f;
            }
            if (LESS(gbfs_initial_state->f, minif)) {
                minif = gbfs_initial_state->f;
            }
            if (maxd < open_leaf->depth) {
                maxd = open_leaf->depth;
            }
            if (start) {
                printf("\nlowest leaf goal distance/initial state f value seen: %6ld/%6ld; maxdepth reached: %4ld",
                       minlf, minif, maxd);
                start = FALSE;
                fflush(stdout);
            } else {
                printf("\n                                                      %6ld/%6ld                    %4ld",
                       minlf, minif, maxd);
                fflush(stdout);
            }
        }

        /* expand the leaf
         */
        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("\nopen leaf:");
            print_state(open_leaf->S);
        }
        /* with either all applicable actions, or the helpful actions
         */
        if (gcmd_line.helpful) {
            for (i = 0; i < open_leaf->num_H; i++) {
                A[i] = open_leaf->H[i];
            }
            num_A = open_leaf->num_H;
        } else {
            get_A(&(open_leaf->S));
            for (i = 0; i < gnum_A; i++) {
                A[i] = gA[i];
            }
            num_A = gnum_A;
        }
        /* now apply all the A actions
         */
        for (i = 0; i < num_A; i++) {
            if (gcmd_line.A && gcmd_line.debug >= 1) {
                printf("\napplying action (observe %d) ", gop_conn[A[i]].observation);
                print_op_name(A[i]);
            }
            if (!gop_conn[A[i]].observation) {
                /* pre-set up the data structures, for ease of implementation
                 * in state transition code.
                 */
                tmpedge = new_BfsEdge();
                tmpedge->op = A[i];
                tmpedge->in_node = open_leaf;
                tmpbfs = new_BfsNode();
                tmpbfs->ingoing_edge = tmpedge;
                if (!result_to_dest(&S, tmpbfs, -1)) {
                    /* action may be self-contradictory, or state may
                     * stagnate.
                     */
                    if (gcmd_line.A && gcmd_line.debug >= 1) {
                        printf("\naction rejected by result_to_dest");
                    }
                    free(tmpedge->unknown_E);
                    free(tmpedge);
                    free(tmpbfs);
                    continue;
                }
                /* insert new node etc.
                 */
                if (gcmd_line.A && gcmd_line.debug >= 2) {
                    printf("\nresult state:");
                    print_state(S);
                }
                add_to_bfs_space(&S, tmpbfs, tmpedge, open_leaf, -1);
                continue;
            } /* no observation */
            /* is an observation!
             */
            tmpedge = new_BfsEdge();
            tmpedge->op = A[i];
            tmpedge->in_node = open_leaf;
            /* "true" branch
             */
            tmpbfs = new_BfsNode();
            tmpbfs->ingoing_edge = tmpedge;
            if (!result_to_dest(&S, tmpbfs, 1)) {
                if (gcmd_line.A && gcmd_line.debug >= 1) {
                    printf("\naction 'true' rejected by result_to_dest");
                }
                free(tmpedge->unknown_E);
                free(tmpedge);
                free(tmpbfs);
                continue;
            } else {
                if (gcmd_line.A && gcmd_line.debug >= 2) {
                    printf("\nresult 'true' state:");
                    print_state(S);
                }
                add_to_bfs_space(&S, tmpbfs, tmpedge, open_leaf, 1);
            }
            /* "false" branch
             */
            tmpbfs = new_BfsNode();
            tmpbfs->ingoing_edge = tmpedge;
            /* will return TRUE, check already made above for mode 1
             */
            result_to_dest(&S, tmpbfs, 0);
            if (gcmd_line.A && gcmd_line.debug >= 2) {
                printf("\nresult 'false' state:");
                print_state(S);
            }
            add_to_bfs_space(&S, tmpbfs, tmpedge, open_leaf, 0);
        } /* for all A applicable actions */

        /* JOERGPARTIALUNDO: Insert here new_BfsEdge() with a single
           new_BfsNode() for the give-up action (not using a sub-call to
           add_to_bfs_space, just do everything here). That action will
           just get a bogus id "-117" or whatever, something unique; when
           we look at its successor state that should be treated as if it
           was a goal state. This *should* work by just leaving S
           unassigned, setting "solved" to TRUE and setting "f" to 0, and
           making a special case for the give-up action below in bestedge
           finding, see there. For correct pointer handling, see above and
           add_to_bfs_space. Actually, make "give-up action yes/no" a
           command line parameter; same for the "1000000".
        */

        if (gcmd_line.giveup_action) {
            /* pre-set up the data structures, for ease of implementation
             * in state transition code.
             */
            tmpedge = new_BfsEdge();
            tmpedge->op = GIVEUP_ID;
            tmpedge->in_node = open_leaf;
            tmpbfs = new_BfsNode();
            tmpbfs->ingoing_edge = tmpedge;

            /*add_to_bfs_space( &S, tmpbfs, tmpedge, open_leaf, -1 );*/
            tmpbfs->solved = TRUE;
            tmpbfs->f = gcmd_line.giveup_cost;
            tmpbfs->astar_f = gcmd_line.giveup_cost;

            tmpedge->out_node = tmpbfs;

            tmpedge->next = open_leaf->outgoing_edges;
            open_leaf->outgoing_edges = tmpedge;
        }

        /* re-adjust the f values and the pointers to best edges,
         * as well as solved and failed values,
         * along path from expanded leaf back to initial state
         */
        /* JOERGPARTIALUNDO: In this call here, as soon as one of the
           actions applicable to open_leaf leads to a solved state (as
           would the giveup action), open_leaf itself is marked as solved
           and then recursively back to the initial state ... NOTE: This
           was in original CFF and might be a bug ie lead to incorrect
           plans... see below... if a node is marked as solved then its
           bestedge poointer MUST be an action whose outcome state(s) are
           actually solved ...
           Alvaro: Solved.
        */
        update_fvalues_and_markers(open_leaf);

    } /* search ends here */

    if (gbfs_initial_state->solved) {
        print_plan();
    } else {
        if (!gcmd_line.helpful) {
            printf("\n\ninitial state failed! problem proved unsolvable!\n\n");
        } else {
            printf("\n\ninitial state failed! try without helpful actions/greedy successor pruning!\n\n");
        }
    }

}


BfsNode *get_open_leaf_of_best_subtree(void) {

    BfsNode *i_bfs;

    /* currently, simply follow non-solved best path in
     * true-branch-preceedes-false-branch order, ie no
     * (insertion point for) selection heuristic.
     * NOTE: assumes there are no cycles! (think about that
     * in detail when doing more than stagnating repeated-checks)
     */

    i_bfs = gbfs_initial_state;
    while (i_bfs) {
        if (i_bfs->best_outgoing_edge == NULL) break;

        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("   best subtree: %d: ", i_bfs->best_outgoing_edge->op);
            print_op_name(i_bfs->best_outgoing_edge->op);
            printf("\n");
        }

        if (i_bfs->best_outgoing_edge->op == GIVEUP_ID || /* Alvaro: Giveup action is not an observation */
            !gop_conn[i_bfs->best_outgoing_edge->op].observation) {

            i_bfs = i_bfs->best_outgoing_edge->out_node;
            if (i_bfs->solved || i_bfs->failed) {
                printf("\n\n1 solved or failed in open leaf getting?\n\n");
                exit(1);
            }
            continue;
        }
        /* observation!
         */
        if (!i_bfs->best_outgoing_edge->true_out_node->solved &&
            (i_bfs->best_outgoing_edge->false_out_node->solved ||
             i_bfs->best_outgoing_edge->false_out_node->f >=
             i_bfs->best_outgoing_edge->true_out_node->f)) {
            i_bfs = i_bfs->best_outgoing_edge->true_out_node;
            if (i_bfs->failed) {
                printf("\n\n2 failed in open leaf getting?\n\n");
                exit(1);
            }
        } else {
            i_bfs = i_bfs->best_outgoing_edge->false_out_node;
            if (i_bfs->solved || i_bfs->failed) {
                printf("\n\n3 solved or failed in open leaf getting?\n\n");
                exit(1);
            }
        }
    }


    return i_bfs;

}


void add_to_bfs_space(State *S, BfsNode *tmpbfs, BfsEdge *tmpedge, BfsNode *open_leaf,
                      int observe_mode) {

    int i;

    /* complete the new leaf: state, f value, depth
     */
    make_state(&(tmpbfs->S), S->num_F);
    source_to_dest(&(tmpbfs->S), S);
    if (contains_goal(&(tmpbfs->S))) {
        /* mark this node as solved!
         */
        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("\ncontains goal!");
        }
        tmpbfs->solved = TRUE;
        tmpbfs->f = 0;
    } else {
        if (gcmd_line.helpful) {
            tmpbfs->f = get_1P_and_H(tmpbfs);
            tmpbfs->H = (int *) calloc(gnum_H, sizeof(int));
            for (i = 0; i < gnum_H; i++) {
                tmpbfs->H[i] = gH[i];
            }
            tmpbfs->num_H = gnum_H;
        } else {
            tmpbfs->f = get_1P(tmpbfs);
        }
        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("\n1P: %f, weighted: %f",
                   tmpbfs->f, tmpbfs->f * gcmd_line.hweight);
        }
    }
    if (tmpbfs->f == INFINITY) {
        /* mark this node as failed!
         */
        if (gcmd_line.A && gcmd_line.debug >= 1) {
            printf("\nh value infty!");
        }
        tmpbfs->failed = TRUE;
    } else {
        tmpbfs->f *= gcmd_line.hweight;
    }
    /* JOERGPARTIALUNDO: the next two guys are not in use, might be
       easier to just remove, to reduce confusion.  ALVARO: Removed
       greedy_f but astar_f is used for tie-breaking in greedy search.
     */
    tmpbfs->astar_f = tmpbfs->f;
    tmpbfs->depth = open_leaf->depth + 1;

    /* complete the edge node: just insert out node, here
     *
     * (op, in_node set in search main fn,
     * unknown efs set in result to dest)
     */
    if (observe_mode == -1) {
        tmpedge->out_node = tmpbfs;
    }
    if (observe_mode == 0) {
        tmpedge->false_out_node = tmpbfs;
    }
    if (observe_mode == 1) {
        tmpedge->true_out_node = tmpbfs;
    }

    /* insert the edge into outgoing list of expanded leaf;
     * if we're in observe mode 0, this was already done.
     */
    if (observe_mode != 0) {
        tmpedge->next = open_leaf->outgoing_edges;
        open_leaf->outgoing_edges = tmpedge;
    }

}


/* JOERGPARTIALUNDO: cleanup this whole function, see comments below.
   Alvaro: Done. Now we mark the node as solved only if bestedge is
   solved.  For greedy search, solved states are always better
   independently of their f value
   Alvaro: TODO: cleanup a bit more....
 */
void update_fvalues_and_markers(BfsNode *open_leaf) {
    BfsNode *i_bfs;
    BfsEdge *i_edge, *bestedge;
    long min, min_astar, f, astar_f;
    Bool ex_nonfailed;
    Bool bestedge_solved;

    /* use astar_f as secondary heuristic for bestedge
     * selection in greedy search!
     */
    i_bfs = open_leaf;
    while (TRUE) {
        /* adjust the f value of the node, and the best edge,
         * and the solved and failed values.
         */
        min = -2;
        min_astar = -2;
        bestedge = NULL;
        ex_nonfailed = FALSE;
        bestedge_solved = FALSE;
        for (i_edge = i_bfs->outgoing_edges; i_edge; i_edge = i_edge->next) {
            /*printf("Analysis edge: %d \n", i_edge->op);*/
            if (i_edge->op == GIVEUP_ID || /* Alvaro: Giveup action is not an observation */
                !gop_conn[i_edge->op].observation) {
                if (i_edge->out_node->failed) {
                    if (i_edge->out_node->f != INFINITY) {
                        printf("\n1 failed node with finite f value?\n\n");
                        exit(1);
                    }
                    continue;
                }
                ex_nonfailed = TRUE;

                if (i_edge->out_node->f == INFINITY) {
                    printf("\n1 unfailed node with infinite f value?\n\n");
                    exit(1);
                }
                if (i_edge->out_node->f < 0) {
                    printf("\n1 unfailed node with < 0 f value?\n\n");
                    exit(1);
                }
                astar_f = action_cost(i_edge->op) + i_edge->out_node->astar_f;
                f = i_edge->out_node->f;
                if (gcmd_line.search_alg == 0) f += action_cost(i_edge->op);


                /* JOERGPARTIALUNDO. Alvaro: For greedy search, a solved edge
                 is always better than a non-solved */
                if (gcmd_line.search_alg != 1 /* not greedy */ ||
                    !bestedge_solved || i_edge->out_node->solved) {
                    if (min == -2 ||
                        f <= min) {
                        min = f;
                        min_astar = astar_f;
                        bestedge = i_edge;
                        bestedge_solved = bestedge->out_node->solved;
                    }
                    if (gcmd_line.search_alg == 1 /* greedy */ && (
                            (f == min &&
                             astar_f < min_astar)
                            || (i_edge->out_node->solved && !bestedge_solved))) {
                        min_astar = astar_f;
                        bestedge = i_edge;
                        bestedge_solved = bestedge->out_node->solved;
                    }
                }
                continue;
            }
            /* observation edge!
             */
            if (i_edge->true_out_node->failed) {
                if (i_edge->true_out_node->f != INFINITY) {
                    printf("\n2 failed node with finite f value?\n\n");
                    exit(1);
                }
                continue;
            }
            if (i_edge->false_out_node->failed) {
                if (i_edge->false_out_node->f != INFINITY) {
                    printf("\n3 failed node with finite f value?\n\n");
                    exit(1);
                }
                continue;
            }
            ex_nonfailed = TRUE;
            if (i_edge->true_out_node->f == INFINITY ||
                i_edge->false_out_node->f == INFINITY) {
                printf("\n2 unfailed node with infinite f value?\n\n");
                exit(1);
            }
            if (i_edge->true_out_node->f < 0 ||
                i_edge->false_out_node->f < 0) {
                printf("\n2 unfailed node with < 0 f value?\n\n");
                exit(1);
            }
            /* JOERGPARTIALUNDO: In the arrangement right now, the first
           "if" part is SUM aggregation in AND nodes, the 2nd one is MAX
           aggregation in AND nodes. Probably keep these options. But
           add a 3rd one AVERAGE is 0.5*true+0.5*false.
             */
            if (gcmd_line.search_mode == 0 /* sum */) {
                f = i_edge->true_out_node->f + i_edge->false_out_node->f;
                astar_f = i_edge->true_out_node->astar_f + i_edge->false_out_node->astar_f;
            } else if (gcmd_line.search_mode == 1 /* max */) {
                f = (i_edge->true_out_node->f > i_edge->false_out_node->f) ? i_edge->true_out_node->f
                                                                           : i_edge->false_out_node->f;
                astar_f = (i_edge->true_out_node->astar_f > i_edge->false_out_node->astar_f) ?
                          i_edge->true_out_node->astar_f : i_edge->false_out_node->astar_f;
            } else /* avg */ { /* gcmd_line.search_mode == 2 */
                f = avg(i_edge->true_out_node->f, i_edge->false_out_node->f);
                astar_f = avg(i_edge->true_out_node->astar_f, i_edge->false_out_node->astar_f);
            }
            astar_f += action_cost(i_edge->op);
            if (gcmd_line.search_alg == 0) f += action_cost(i_edge->op);

            /* JOERGPARTIALUNDO. Alvaro: For greedy search, a solved edge
           is always better than a non-solved */
            if (gcmd_line.search_alg != 1 /* not greedy */ ||
                !bestedge_solved || (i_edge->true_out_node->solved &&
                                     i_edge->false_out_node->solved)) {
                /* prefer, with equal heuristic values, observations!
                 * do so by saying that min is updated under equality also
                 */
                if (min == -2 || f < min) {
                    min = f;
                    min_astar = astar_f;
                    bestedge = i_edge;
                    bestedge_solved = (i_edge->true_out_node->solved &&
                                       i_edge->false_out_node->solved);
                }
                if (gcmd_line.search_alg == 1 /* greedy */ &&
                    ((f == min && astar_f <= min_astar) ||
                     (i_edge->true_out_node->solved &&
                      i_edge->false_out_node->solved &&
                      !bestedge_solved))) {
                    min_astar = astar_f;
                    bestedge = i_edge;
                    bestedge_solved = (i_edge->true_out_node->solved &&
                                       i_edge->false_out_node->solved);
                }
            }
        } /* all outgoing edges i_edge */
        if (!ex_nonfailed) {
            i_bfs->failed = TRUE;
            i_bfs->f = INFINITY;
        } else {
            /* JOERGPARTIALUNDO: DANGER! The "bestedge" is what we will
           actually output as the contingent plan; ie, the sub-graph of
           our explored search tree reached when using these outgoing
           actions. If a node is marked "solved" then bestedge MUST
           point to a solved successor state. Maybe this follows from
           the setup here, with uniform action costs, or maybe only in
           particular settings, like max'ing over AND nodes
           (observations). Not clear. Anyway, CORRECT this: FIRST find
           the bestedge ie minimum f-value (which is infinite in case
           all successors are failed); THEN set node as solved if and
           only if all outcome states of bestedge are sdolved (ie the
           single one in case of regular action, both true and false
           outcomes in case of observation).
           Alvaro: solved
             */

            if (bestedge_solved) {
                i_bfs->solved = TRUE;
            }
            /*if ( i_edge->op == GIVEUP_ID || / Alvaro: Giveup action is not an observation /
              &&  !gop_conn[bestedge->op].observation ){
             }else if ( i_edge->true_out_node->solved &&
                i_edge->false_out_node->solved ) {

             }*/

            if (min == -2 || bestedge == NULL) {
                printf("\n\nmin/bestedge not set?\n\n");
                exit(1);
            }
            /* JOERGPARTIALUNDO: This is the definition of "f" we want
           to use. Make a case distinction: is this the give-up action
             ("-117")? Or is it some acvtual action? If the former, add
             1000000 resp this cmd line parameter; if the latter, add
             1. NOTE: This needs to be done already up above when
             finding "bestedge"!
             Alvaro: min_astar and min already take into account action cost now
           */
            i_bfs->astar_f = min_astar;
            i_bfs->f = min;
            i_bfs->best_outgoing_edge = bestedge;

            if (gcmd_line.A && gcmd_line.debug >= 1) {
                printf("\n SELECTED ACTION: %d: ", i_bfs->best_outgoing_edge->op);
                printf(" my f-value is %f with f-value of %f: ", i_bfs->f, i_bfs->best_outgoing_edge->in_node->f);

                printf("\n");
            }

        }

        /* JOERGPARTIALUNDO: Actually there is no full duplicate checking
           because that would be too expensive over belief states. Instead
           I nly check "stagnating paths" which means the same path visits
           the same belief state again.
        */
        /* this will be more complicated later when there can be several
         * ingoing edges?!
         */
        if (i_bfs->ingoing_edge != NULL) {
            i_bfs = i_bfs->ingoing_edge->in_node;
        } else {
            break;
        }
    } /* while loop backwards over bfs nodes from open leaf */

}



/* void backpropagate_fvalue( BfsNode *open_leaf, long newleaff ) */

/* { */

/*   BfsNode *i_bfs; */
/*   long newsuccf = newleaff; */

/*   /\* use astar_f as secondary heuristic for bestedge */
/*    * selection in greedy search! */
/*    *\/ */
/*   i_bfs = open_leaf; */
/*   while ( TRUE ) { */
/*     /\* adjust the f value of the node, and the best edge, */
/*      * and the solved and failed values. */
/*      *\/ */
/*     if ( newsuccf == INFINITY ) { */
/*       /\* this clearly can't give any improvements */
/*        *\/ */
/*       break; */
/*     } */
/*     if ( i_bfs->f == INFINITY ) { */
/*       printf("\nbackpropagating f value over infty node?\n\n"); */
/*       exit( 1 ); */
/*     } */
/*     if ( gcmd_line.search_mode == 0 ) { */
/*       if ( newsuccf < i_bfs->f ) { */
/* 	i_bfs->f = newsuccf; */
/*       } else { */
/* 	/\* no more changes */
/* 	 *\/ */
/* 	break; */
/*       } */
/*     } */
/*     if ( gcmd_line.search_mode == 1 ) { */
/*       if ( 1 + newsuccf < i_bfs->f ) { */
/* 	i_bfs->f = 1 + newsuccf; */
/*       } else { */
/* 	/\* no more changes */
/* 	 *\/ */
/* 	break; */
/*       } */
/*     } */
/*     /\* we got here means we do a new iteration unless we're at the initial  */
/*      * state already */
/*      *\/ */
/*     if ( i_bfs->ingoing_edge != NULL ) { */
/*       i_bfs = i_bfs->ingoing_edge->in_node; */
/*     } else { */
/*       break; */
/*     } */
/*   } */

/* } */



void print_plan(void) {

    BfsEdge_pointer upperlayer[2048], lowerlayer[2048];
    int num_upperlayer, lowerlayerson[2048][2], num_lowerlayer;
    int i, t, totalcount = 0;

    printf("\n\nff: found plan as follows");
    fflush(stdout);
    if (!gbfs_initial_state->best_outgoing_edge) {
        printf("\n(empty plan)");
        return;
    }
    lowerlayer[0] = gbfs_initial_state->best_outgoing_edge;
    num_lowerlayer = 1;

    t = 0;
    while (num_lowerlayer > 0) {
        num_upperlayer = 0;
        for (i = 0; i < num_lowerlayer; i++) {
            if (lowerlayer[i]->op == GIVEUP_ID || /* Alvaro: Giveup action is not an observation */
                !gop_conn[lowerlayer[i]->op].observation) {
                if (lowerlayer[i]->out_node->best_outgoing_edge != NULL) {
                    if (num_upperlayer == 2048) {
                        printf("\n\ntoo many actions in one level for plan print!\n\n");
                        exit(1);
                    }
                    upperlayer[num_upperlayer] = lowerlayer[i]->out_node->best_outgoing_edge;
                    lowerlayerson[i][0] = num_upperlayer;
                    num_upperlayer++;
                } else {
                    lowerlayerson[i][0] = -1;
                }
                continue;
            }
            /* observation!
             */
            if (lowerlayer[i]->true_out_node->best_outgoing_edge != NULL) {
                if (num_upperlayer == 2048) {
                    printf("\n\ntoo many actions in one level for plan print!\n\n");
                    exit(1);
                }
                upperlayer[num_upperlayer] = lowerlayer[i]->true_out_node->best_outgoing_edge;
                lowerlayerson[i][0] = num_upperlayer;
                num_upperlayer++;
            } else {
                lowerlayerson[i][0] = -1;
            }
            if (lowerlayer[i]->false_out_node->best_outgoing_edge != NULL) {
                if (num_upperlayer == 2048) {
                    printf("\n\ntoo many actions in one level for plan print!\n\n");
                    exit(1);
                }
                upperlayer[num_upperlayer] = lowerlayer[i]->false_out_node->best_outgoing_edge;
                lowerlayerson[i][1] = num_upperlayer;
                num_upperlayer++;
            } else {
                lowerlayerson[i][1] = -1;
            }
        } /* end loop over edges in lower layer */

        /* print this (lower) layer
         */
        printf("\n-------------------------------------------------");
        for (i = 0; i < num_lowerlayer; i++) {
            printf("\n%3d||%d --- ", t, i);
            print_op_name(lowerlayer[i]->op);
            totalcount++;
            if (lowerlayer[i]->op == GIVEUP_ID || /* Alvaro: Giveup action is not an observation */
                !gop_conn[lowerlayer[i]->op].observation) {
                printf(" --- SON: %d||%d", t + 1, lowerlayerson[i][0]);
            } else {
                printf(" --- TRUESON: %d||%d --- FALSESON: %d||%d",
                       t + 1, lowerlayerson[i][0], t + 1, lowerlayerson[i][1]);
            }
        }

        /* copy upper into lower
         */
        for (i = 0; i < num_upperlayer; i++) {
            lowerlayer[i] = upperlayer[i];
        }
        num_lowerlayer = num_upperlayer;

        t++;
    } /* end loop over time steps t */

    printf("\n-------------------------------------------------");
    printf("\n\ntree layers: %d", t);
    printf("\ntotal nr. actions: %d", totalcount);

}


/***********************************
 * SIMPLE STATE HANDLING FUNCTIONS *
 ***********************************/
















void source_to_dest(State *dest, State *source) {

    int i;

    for (i = 0; i < source->num_F; i++) {
        dest->F[i] = source->F[i];
    }
    dest->num_F = source->num_F;

    for (i = 0; i < source->num_U; i++) {
        dest->U[i] = source->U[i];
    }
    dest->num_U = source->num_U;

}


void print_state(State S) {

    int i;

    printf("\nF:");
    for (i = 0; i < S.num_F; i++) {
        printf("\n");
        print_ft_name(S.F[i]);
    }
    printf("\nU:");
    for (i = 0; i < S.num_U; i++) {
        printf("\n");
        print_ft_name(S.U[i]);
    }

}


int avg(int v1, int v2) {
    return (v1 + v2) / 2;
    /* int sum = v1 + v2; */
    /* if(sum % 2) { */
    /* 	return (sum+1)/2; */
    /* }else{ */
    /* 	return (sum)/2; */
    /* } */
}

/*Alvaro: Auxiliar function to know the cost of an operator (useful
  for giveup action and possibly if we want to add support for action
  costs*/
int action_cost(int operator) {
    if (operator == GIVEUP_ID)
        return gcmd_line.giveup_cost;
    else
        return 1;
}
