


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
 * File: repeated_states.c
 *
 * Description: recognises seen states. quite complicated.
 *
 * Author: Joerg Hoffmann 2002
 *
 *********************************************************************/ 








#include "ff.h"

#include "output.h"
#include "memory.h"

#include "relax.h"
#include "state_transitions.h"
#include "repeated_states.h"
#include "search.h"
















/***********************************************************
 * LOCAL GLOBALS                                           *
 ***********************************************************/










/* memory (hash table) for states that are already members
 * of the best first search space
 */
BfsHashEntry_pointer lbfs_hash_entry[BFS_HASH_SIZE];





/* the clauses to be communicated to the SAT solver for
 * determining fact progress. indexed by nr (S1 | S2) / cl nr / lit nr
 */
TimedLiteral ***lrs_clauses;
int **lrs_clause_length;
int *lrs_num_clauses;
int *lrs_endtime;


/* array; maps 1|2 (ie 0|1) / ft / time triple to its number in CNF encoding.
 */
int ***lrs_codes;

/* inverse mapping, to undo changes in table.
 */
int *lrs_cn, *lrs_cf, *lrs_ct, lrs_num_c;



/* hitting set -- in CNF construction for noops in presence of non-unary antecedents.
 */
int *lrs_hitting_set;
int lrs_num_hitting_set;



/* for naive DP implementation
 */
int **lrs_dp_clauses;
int *lrs_dp_clause_length;
int lrs_dp_num_clauses;
int *lrs_assigned;



/* stores the current DP decisions including unit propagations.
 */
int *lrs_decision_stack;
int lrs_num_decision_stack;


/* for each possible ft code, a pointer to connected dynamic list
 * of member elements, ie the clauses in which it participates,
 * positive and negative.
 */
MemberList_pointer *lrs_pos_c_in_clause_start;
MemberList_pointer *lrs_pos_c_in_clause_end;
MemberList_pointer *lrs_neg_c_in_clause_start;
MemberList_pointer *lrs_neg_c_in_clause_end;
















void initialize_repeated_states( void )

{

  int i, j;

  for ( i = 0; i < BFS_HASH_SIZE; i++ ) {
    lbfs_hash_entry[i] = NULL;
  }

  lrs_clauses = ( TimedLiteral *** ) calloc( 2, sizeof( TimedLiteral ** ) );
  lrs_clauses[0] = ( TimedLiteral ** ) calloc( gmax_rs_clauses, sizeof( TimedLiteral * ) );
  lrs_clauses[1] = ( TimedLiteral ** ) calloc( gmax_rs_clauses, sizeof( TimedLiteral * ) );
  lrs_clause_length = ( int ** ) calloc( 2, sizeof( int * ) );
  lrs_clause_length[0] = ( int * ) calloc( gmax_rs_clauses, sizeof( int ) );
  lrs_clause_length[1] = ( int * ) calloc( gmax_rs_clauses, sizeof( int ) );
  lrs_dp_clauses = ( int ** ) calloc( gmax_rs_clauses, sizeof( int * ) );
  lrs_dp_clause_length = ( int * ) calloc( gmax_rs_clauses, sizeof( int ) );
  for ( i = 0; i < gmax_rs_clauses; i++ ) {
    lrs_clauses[0][i] = ( TimedLiteral * ) calloc( gmax_literals, sizeof( TimedLiteral ) );
    lrs_clause_length[0][i] = 0;
    lrs_clauses[1][i] = ( TimedLiteral * ) calloc( gmax_literals, sizeof( TimedLiteral ) );
    lrs_clause_length[1][i] = 0;
    lrs_dp_clauses[i] = ( int * ) calloc( gmax_literals, sizeof( int ) );
    lrs_dp_clause_length[i] = 0;
  }
  lrs_num_clauses = ( int * ) calloc( 2, sizeof( int ) );
  lrs_num_clauses[0] = 0;
  lrs_num_clauses[1] = 0;
  lrs_dp_num_clauses = 0;
  
  lrs_endtime = ( int * ) calloc( 2, sizeof( int ) );
  lrs_endtime[0] = -1;
  lrs_endtime[1] = -1;
  
  lrs_codes = ( int *** ) calloc( 2, sizeof( int ** ) );
  lrs_codes[0] = ( int ** ) calloc( MAX_PLAN_LENGTH + 1, sizeof( int * ) );
  lrs_codes[1] = ( int ** ) calloc( MAX_PLAN_LENGTH + 1, sizeof( int * ) );
  for ( i = 0; i <= MAX_PLAN_LENGTH; i++ ) {
    lrs_codes[0][i] = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    lrs_codes[1][i] = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    for ( j = 0; j < gnum_ft_conn; j++ ) {
      lrs_codes[0][i][j] = -1;
      lrs_codes[1][i][j] = -1;
    }
  }
  
  lrs_cn = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( int ) );
  lrs_cf = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( int ) );
  lrs_ct = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( int ) );
  
  lrs_hitting_set = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );


  lrs_assigned = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( int ) );
  for ( i = 0; i < (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1; i++ ) {
    lrs_assigned[i] = -1;
  }

  lrs_decision_stack = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2, sizeof( int ) );

  lrs_pos_c_in_clause_start = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( MemberList_pointer ) );
  lrs_pos_c_in_clause_end = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( MemberList_pointer ) );
  lrs_neg_c_in_clause_start = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( MemberList_pointer ) );
  lrs_neg_c_in_clause_end = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2 + 1, sizeof( MemberList_pointer ) );
  for ( i = 0; i <= (1 + MAX_PLAN_LENGTH) * gmax_CNFU * 2; i++ ) {
    lrs_pos_c_in_clause_start[i] = NULL;
    lrs_neg_c_in_clause_start[i] = NULL;
    lrs_pos_c_in_clause_end[i] = NULL;
    lrs_neg_c_in_clause_end[i] = NULL;
  }

}










/* FULL REPEATED STATES CHECK COMPLETELY "COMMENTED OUT", FOR THE TIME BEING.
 */




















/*********************************************************
 * EASY PRELIMINARY TEST: FOR GIVEN SOURCE AND DEST,   
 * IS THERE A STAGNATION IE NO IMPROVEMENT OVER S?
 * EASY TO IMPLEMENT AS CNF ALREADY COMPUTED INTO gclauses
 *********************************************************/

























Bool lnondets;

















/* here we test S' against *all states on the path to it*!!
 * this way we can still use the same CNF;
 *
 * implement it simply by calling the classical S x S' fn
 * for S going back into the past on the path.
 */
Bool stagnates( State *dest, BfsNode *dest_node, int endtime )

{

  int anc = 1;
  BfsNode *ibfs;

  times( &end );
  TIME( gsearch_time );
  times( &start );    

  if ( !dest_node ||
       !dest_node->ingoing_edge ||
       !dest_node->ingoing_edge->in_node ) {
    printf("\nstagnation test missing in-edge/in-edge-in-node?\n\n");
    exit( 1 );
  }
  

  if ( gcmd_line.P && gcmd_line.debug ) {
    printf("\n\n**********************************checking stagnation of:");
    for ( ibfs = dest_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
      printf("\n");
      print_op_name( ibfs->ingoing_edge->op );
    }
  }
  
  lnondets = FALSE;
  if ( gop_conn[dest_node->ingoing_edge->op].nondeteff != 0 ) {
    lnondets = TRUE;
  }
/*   for ( ibfs = dest_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) { */
/*     if ( gop_conn[ibfs->ingoing_edge->op].nondeteff != 0 ) { */
/*       lnondets = TRUE; */
/*     } */
/*   } */
  if ( lnondets ) {
    /* for all predecessor states "as" (gcmd_line.stagnating > 1), see if there's an outcome of the action
     * such that "as == s'".
     */
    return do_nondet_stagnation_check( dest, dest_node, endtime );
  }



  if ( gcmd_line.stagnating == 1 ) {
    /* check only against direct predecessor
     */
    if ( !gop_conn[dest_node->ingoing_edge->op].observation ) {
      if ( single_stagnates( dest, &(dest_node->ingoing_edge->in_node->S), endtime, 1 ) ) {
	times( &end );
	TIME( gss_time );
	times( &start );
	gss_hits++;
	return TRUE;
      }    
      times( &end );
      TIME( gss_time );
      times( &start );
    }
    return FALSE;
  }

  /* check against all ancestors
   */
  for ( ibfs = dest_node->ingoing_edge->in_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
    if ( gop_conn[ibfs->ingoing_edge->op].observation ) break;
    if ( single_stagnates( dest, &(ibfs->S), endtime, anc ) ) {
      times( &end );
      TIME( gss_time );
      times( &start );
      gss_hits++;
      return TRUE;
    }
    anc++;
  }
  if ( !ibfs->ingoing_edge ) {
    if ( single_stagnates( dest, &(ibfs->S), endtime, anc ) ) {
      times( &end );
      TIME( gss_time );
      times( &start );
      gss_hits++;
      return TRUE;
    }
  }
  
  times( &end );
  TIME( gss_time );
  times( &start );
  return FALSE;

}



Bool do_nondet_stagnation_check( State *dest, BfsNode *dest_node, int endtime )

{

  BfsNode *ibfs;
  int anc = 1;

  if ( gcmd_line.stagnating == 1 ) {
    /* check only against direct predecessor
     */
    if ( !gop_conn[dest_node->ingoing_edge->op].observation ) {
      if ( rec_do_nondet_stagnation_check( 0, dest, dest_node, dest_node->ingoing_edge->in_node, endtime, anc ) ) {
	times( &end );
	TIME( gss_time );
	times( &start );
	gss_hits++;
	return TRUE;
      }    
      times( &end );
      TIME( gss_time );
      times( &start );
    }
    return FALSE;
  }

  /* check against all ancestors
   */
  for ( ibfs = dest_node->ingoing_edge->in_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
    if ( gop_conn[ibfs->ingoing_edge->op].observation ) break;
    if ( rec_do_nondet_stagnation_check( 0, dest, dest_node, ibfs, endtime, anc ) ) {
      times( &end );
      TIME( gss_time );
      times( &start );
      gss_hits++;
      return TRUE;
    }
    anc++;
  }
  if ( !ibfs->ingoing_edge ) {
    if ( rec_do_nondet_stagnation_check( 0, dest, dest_node, ibfs, endtime, anc ) ) {
      times( &end );
      TIME( gss_time );
      times( &start );
      gss_hits++;
      return TRUE;
    }
  }
  
  times( &end );
  TIME( gss_time );
  times( &start );
  return FALSE;

}



Bool rec_do_nondet_stagnation_check( int nondets_set_on_spath, 
				     State *dest, BfsNode *dest_node, BfsNode *source_node, 
				     int endtime, int anc )

{

  State *oldnewancs;

  MemberList_pointer *prev_pos_end;
  MemberList_pointer *prev_neg_end;

  int newanc = 1;
  BfsNode *ibfs;
  int pathnondet;

  int i, op, j;

  int lowop, lowft, lowadded, lownegft, lowff, lowm;
  State *newancs, *newancs_pred;
  Bool lowftFds = FALSE, lowftUds = FALSE, lowftFs = FALSE, lowftUs = FALSE;
  int prevnumcl, prevnumc;

  int numnondets_to_s, numnondets_to_sd_source;
  int testff, testm, testcode, testft;
  Bool sat;
  
  Bool forall_mode;

  Bool rec_result;


  oldnewancs = ( State * ) calloc( 1, sizeof( State ) );
  make_state( oldnewancs, gnum_ft_conn );

  op = dest_node->ingoing_edge->op; 


  numnondets_to_s = 0;
  for ( ibfs = source_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
    if ( gop_conn[ibfs->ingoing_edge->op].nondeteff != 0 ) numnondets_to_s++;
  }
  if ( nondets_set_on_spath < numnondets_to_s ) {
    /* select the next later nondet eff on the path to source
     * for all settings we need stagnation
     */
    forall_mode = TRUE;
  } else {
    /* select the next later nondet eff on the path from source to dest
     * for one setting we need stagnation
     */
    forall_mode = FALSE;
  }

  numnondets_to_sd_source = 0;
  for ( ibfs = dest_node->ingoing_edge->in_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
    if ( gop_conn[ibfs->ingoing_edge->op].nondeteff != 0 ) numnondets_to_sd_source++;
  }


  if ( numnondets_to_sd_source - nondets_set_on_spath > 0 ) {
    prev_pos_end = ( MemberList_pointer * ) 
      calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
    prev_neg_end = ( MemberList_pointer * ) 
      calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );

    pathnondet = 0;
    newanc = 1;
    for ( ibfs = dest_node->ingoing_edge->in_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
      if ( gop_conn[ibfs->ingoing_edge->op].nondeteff != 0 ) pathnondet++;
      if ( pathnondet >= (numnondets_to_sd_source-nondets_set_on_spath) ) break;
      newanc++;
    }

    if ( !ibfs->ingoing_edge ) {
      printf("\n\ndid not find nondet eff that should be there?\n\n");
      exit( 1 );
    }

    /* now ibfs/newanc is a state in which we came with an action whose nondeteff was not yet set.
     * set the effect, recurse, unset it.
     */
    newancs = &(ibfs->S);
    newancs_pred = &(ibfs->ingoing_edge->in_node->S);

    /* remember state of CNF
     */
    prevnumcl = gnum_clauses;
    prevnumc = gnum_c;
    for ( i = 1; i <= gnum_c; i++ ) {
      prev_pos_end[i] = gpos_c_in_clause_end[i];
      prev_neg_end[i] = gneg_c_in_clause_end[i];
    }

    lowft = -1; 
    lowadded = FALSE;
    lowop = ibfs->ingoing_edge->op;
    if ( gop_conn[lowop].nondeteff > 0 ) {
      lowadded = TRUE;
      lowft = gop_conn[lowop].nondeteff-1;
    }
    if ( gop_conn[lowop].nondeteff < 0 ) {
      lowadded = FALSE;
      lowft = ((-1)*gop_conn[lowop].nondeteff)-1;
    }
    lownegft = gft_conn[lowft].negation;
    /* remember old newanc state.
     */
    oldnewancs->num_F = 0;
    oldnewancs->num_U = 0;
    for ( i = 0; i < newancs->num_F; i++ ) {
      oldnewancs->F[oldnewancs->num_F++] = newancs->F[i];
    }
    for ( i = 0; i < newancs->num_U; i++ ) {
      oldnewancs->U[oldnewancs->num_U++] = newancs->U[i];
    }
    lowff = lowft;
    if ( gft_conn[lowff].CNF ) {
      lowm = 1;
    } else {
      lowm = -1;
      lowff = gft_conn[lowff].negation;
    }
    /* remember values of lowft
     */
    for ( i = 0; i < newancs_pred->num_F; i++ ) {
      if ( newancs_pred->F[i] == lowft ) lowftFs = TRUE;
    }
    for ( i = 0; i < newancs_pred->num_U; i++ ) {
      if ( newancs_pred->U[i] == lowft ) lowftUs = TRUE;
    }
    for ( i = 0; i < newancs->num_F; i++ ) {
      if ( newancs->F[i] == lowft ) lowftFds = TRUE;
    }
    for ( i = 0; i < newancs->num_U; i++ ) {
      if ( newancs->U[i] == lowft ) lowftUds = TRUE;
    }
    if ( gcmd_line.P && gcmd_line.debug  > 1) {
      printf("\n--------------low ft ");
      print_ft_name( lowft );
      printf(" Fs %d, Us %d, Fds %d, Uds %d -- low ff ", lowftFs, lowftUs, lowftFds, lowftUds);
      print_ft_name( lowff );
      fflush(stdout);
    }
 

    /* first, say that the effect did NOT occur.
     * insert the respective newanc state.
     */
    newancs->num_F = 0;
    newancs->num_U = 0;
    for ( i = 0; i < newancs_pred->num_F; i++ ) {
      if ( newancs_pred->F[i] == lowft ) break;
    }
    if ( i < newancs_pred->num_F ) {
      /* ft F, negft N
       */
      newancs->F[newancs->num_F++] = lowft;
    } else {
      /* ft either U or N
       */
      for ( i = 0; i < newancs_pred->num_U; i++ ) {
	if ( newancs_pred->U[i] == lowft ) break;
      }
      if ( i < newancs_pred->num_U ) {
	/* ft, negft U
	 */
	newancs->U[newancs->num_U++] = lowft;
	if ( lownegft != -1 ) {
	  newancs->U[newancs->num_U++] = lownegft;
	}
      } else {
	/* ft N
	 */
	if ( lownegft != -1 ) {
	  newancs->F[newancs->num_F++] = lownegft;
	}
      }
    }
    /* copy over the rest from old newancs
     */
    for ( i = 0; i < oldnewancs->num_F; i++ ) {
      if ( oldnewancs->F[i] == lowft || oldnewancs->F[i] == lownegft ) continue;
      newancs->F[newancs->num_F++] = oldnewancs->F[i];
    }
    for ( i = 0; i < oldnewancs->num_U; i++ ) {
      if ( oldnewancs->U[i] == lowft || oldnewancs->U[i] == lownegft ) continue;
      newancs->U[newancs->num_U++] = oldnewancs->U[i];
    }
    /* now insert the NOOP clause.
     */
    if ( lowadded ) {
      /* neg NOOP
       */
      if ( !lowftFs && (lowftFds || lowftUds) ) {/* else, clause true */
	/* here we assume that there are no other effects of the action that
	 * add the lowft!!! (--> no prevention conditions)
	 */
	if ( lowftUs ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    lowm * (lowff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime-newanc-1;
	}
	if ( lowftUds ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    (-1) * lowm * (lowff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime-newanc;
	}
	gnum_clauses++;
	if ( gcmd_line.P && gcmd_line.debug  > 1) {
	  printf("\n--------------low inserted neg NOOP clause ");
	  print_clause( gnum_clauses-1 );
	  fflush(stdout);
	}
      } else {
	if ( gcmd_line.P && gcmd_line.debug  > 1) {
	  printf("\n--------------low neg NOOP clause TRUE");
	  fflush(stdout);
	}
      }
    } else {
      /* pos NOOP
       */
      if ( (lowftFs || lowftUs) && !lowftFds ) {/* else, clause true */
	/* here we assume that there are no other effects of the action that
	 * del the lowft!!! (--> no prevention conditions)
	 */
	if ( lowftUs ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    (-1) * lowm * (lowff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime-newanc-1;
	}
	if ( lowftUds ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    lowm * (lowff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime-newanc;
	}
	gnum_clauses++;
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n--------------low inserted pos NOOP clause ");
	  print_clause( gnum_clauses-1 );
	  fflush(stdout);
	}
      } else {
	if ( gcmd_line.P && gcmd_line.debug  > 1) {
	  printf("\n--------------low pos NOOP clause TRUE");
	  fflush(stdout);
	}
      }
    } /* added or deleted */


    /* tell the new clause(s) to the planner.
     */
    extend_dynamic_clauses_base_encoding( prevnumcl, prevnumc );



    /* infer new known facts in newancs
     */
    i = 0;
    while ( i < newancs->num_U ) {
      testft = newancs->U[i];
      testff = testft;
      if ( gft_conn[testff].CNF ) {
	testm = 1;
      } else {
	testm = -1;
	testff = gft_conn[testff].negation;
      }
      testcode = gcodes[endtime-newanc][testff];
      if ( testcode == -1 ) {
	printf("\nno testcode up here?\n\n");
	exit( 1 );
      }
      gnum_decision_stack = 0;
      gdecision_stack[gnum_decision_stack++] = testm * (-1) * testcode;
      gss_sat_calls++;
      sat = dp_CNF();
      if ( !sat ) {
	for ( j = i; j < newancs->num_U-1; j++ ) {
	  newancs->U[j] = newancs->U[j+1];
	}
	newancs->num_U--;
	newancs->F[newancs->num_F++] = testft;
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n---------------inferred to be true at newancs:");
	  print_ft_name( testft );
	}
      } else {
	i++;
      }
    }
    i = 0;
    while ( i < newancs->num_U ) {
      testft = newancs->U[i];
      testff = testft;
      if ( gft_conn[testff].CNF ) {
	testm = 1;
      } else {
	testm = -1;
	testff = gft_conn[testff].negation;
      }
      testcode = gcodes[endtime-newanc][testff];
      if ( testcode == -1 ) {
	printf("\nno testcode up here?\n\n");
	exit( 1 );
      }
      gnum_decision_stack = 0;
      gdecision_stack[gnum_decision_stack++] = testm * testcode;
      gss_sat_calls++;
      sat = dp_CNF();
      if ( !sat ) {
	for ( j = i; j < newancs->num_U-1; j++ ) {
	  newancs->U[j] = newancs->U[j+1];
	}
	newancs->num_U--;
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n---------------inferred to be false at newancs:");
	  print_ft_name( testft );
	}
      } else {
	i++;
      }
    }
    if ( gcmd_line.P && gcmd_line.debug > 1 ) {
      printf("\n--------------------------forall mode %d, newanc %d nondet eff NO occur; created newanc as follows",
	     forall_mode, newanc);
      print_state( *newancs );
      fflush(stdout);
    }




    /* now everything is set up: the nondet eff is said to not occur. carry on
     * in recursion.
     */
    rec_result = rec_do_nondet_stagnation_check( nondets_set_on_spath + 1, 
						 dest, dest_node, source_node, endtime, anc );
    if ( forall_mode && !rec_result ) {
      /* unset, and return false
       */
      newancs->num_F = 0;
      newancs->num_U = 0;
      for ( i = 0; i < oldnewancs->num_F; i++ ) {
	newancs->F[newancs->num_F++] = oldnewancs->F[i];
      }
      for ( i = 0; i < oldnewancs->num_U; i++ ) {
	newancs->U[newancs->num_U++] = oldnewancs->U[i];
      }


      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      free( prev_pos_end );
      free( prev_neg_end );


      free( oldnewancs->F );
      free( oldnewancs->U );
      free( oldnewancs );
      return FALSE;
    }
    if ( !forall_mode && rec_result ) {
      /* unset, and return true
       */
      newancs->num_F = 0;
      newancs->num_U = 0;
      for ( i = 0; i < oldnewancs->num_F; i++ ) {
	newancs->F[newancs->num_F++] = oldnewancs->F[i];
      }
      for ( i = 0; i < oldnewancs->num_U; i++ ) {
	newancs->U[newancs->num_U++] = oldnewancs->U[i];
      }


      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      free( prev_pos_end );
      free( prev_neg_end );


      free( oldnewancs->F );
      free( oldnewancs->U );
      free( oldnewancs );
      return TRUE;
    }

    /* have to try other option. say that the nondet effect DID occur.
     */
    for ( i = prevnumcl; i < gnum_clauses; i++ ) {
      gclause_length[i] = 0;
    }
    gnum_clauses = prevnumcl;
    for ( i = prevnumc+1; i <= gnum_c; i++ ) {
      gcodes[gct[i]][gcf[i]] = -1;
    }
    for ( i = 1; i <= prevnumc; i++ ) {
      gpos_c_in_clause_end[i] = prev_pos_end[i];
      gneg_c_in_clause_end[i] = prev_neg_end[i];
    }
    for ( ; i <= gnum_c; i++ ) {
      gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
      gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
    }
    /* lowft has value given by nondet eff
     */
    newancs->num_F = 0;
    newancs->num_U = 0;
    if ( lowadded ) {
      newancs->F[newancs->num_F++] = lowft;     
      lowftFds = TRUE;
    } else {
      if ( lownegft != -1 ) {
	newancs->F[newancs->num_F++] = lownegft;
      }
    }
    /* copy over the rest from old newancs
     */
    for ( i = 0; i < oldnewancs->num_F; i++ ) {
      if ( oldnewancs->F[i] == lowft || oldnewancs->F[i] == lownegft ) continue;
      newancs->F[newancs->num_F++] = oldnewancs->F[i];
    }
    for ( i = 0; i < oldnewancs->num_U; i++ ) {
      if ( oldnewancs->U[i] == lowft || oldnewancs->U[i] == lownegft ) continue;
      newancs->U[newancs->num_U++] = oldnewancs->U[i];
    }
    /* we need the right unit clause
     */
    if ( lowadded ) {
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	lowm * (lowff+1);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	endtime-newanc;
      gnum_clauses++;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n--------------low inserted add-unit clause ");
	print_clause( gnum_clauses-1 );
	fflush(stdout);
      }
    } /* add effect appears */ else {
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	(-1) * lowm * (lowff+1);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	endtime-newanc;
      gnum_clauses++;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n--------------low inserted del-unit clause ");
	print_clause( gnum_clauses-1 );
	fflush(stdout);
      }
    } /* del effect appears */


    /* tell the new clause(s) to the planner.
     */
    extend_dynamic_clauses_base_encoding( prevnumcl, prevnumc );



    /* infer new known facts in newancs
     */
    i = 0;
    while ( i < newancs->num_U ) {
      testft = newancs->U[i];
      testff = testft;
      if ( gft_conn[testff].CNF ) {
	testm = 1;
      } else {
	testm = -1;
	testff = gft_conn[testff].negation;
      }
      testcode = gcodes[endtime-newanc][testff];
      if ( testcode == -1 ) {
	printf("\nno testcode up here?\n\n");
	exit( 1 );
      }
      gnum_decision_stack = 0;
      gdecision_stack[gnum_decision_stack++] = testm * (-1) * testcode;
      gss_sat_calls++;
      sat = dp_CNF();
      if ( !sat ) {
	for ( j = i; j < newancs->num_U-1; j++ ) {
	  newancs->U[j] = newancs->U[j+1];
	}
	newancs->num_U--;
	newancs->F[newancs->num_F++] = testft;
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n---------------inferred to be true at newancs:");
	  print_ft_name( testft );
	}
      } else {
	i++;
      }
    }
    i = 0;
    while ( i < newancs->num_U ) {
      testft = newancs->U[i];
      testff = testft;
      if ( gft_conn[testff].CNF ) {
	testm = 1;
      } else {
	testm = -1;
	testff = gft_conn[testff].negation;
      }
      testcode = gcodes[endtime-newanc][testff];
      if ( testcode == -1 ) {
	printf("\nno testcode up here?\n\n");
	exit( 1 );
      }
      gnum_decision_stack = 0;
      gdecision_stack[gnum_decision_stack++] = testm * testcode;
      gss_sat_calls++;
      sat = dp_CNF();
      if ( !sat ) {
	for ( j = i; j < newancs->num_U-1; j++ ) {
	  newancs->U[j] = newancs->U[j+1];
	}
	newancs->num_U--;
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n---------------inferred to be false at newancs:");
	  print_ft_name( testft );
	}
      } else {
	i++;
      }
    }
    if ( gcmd_line.P && gcmd_line.debug > 1 ) {
      printf("\n--------------------------forall mode %d, newanc %d nondet eff YES occur; created newanc as follows",
	     forall_mode, newanc);
      print_state( *newancs );
      fflush(stdout);
    }





    /* now everything is set up: the nondet eff is said to occur. carry on
     * in recursion.
     */
    rec_result = rec_do_nondet_stagnation_check( nondets_set_on_spath + 1, 
						 dest, dest_node, source_node, endtime, anc );
    if ( forall_mode && !rec_result ) {
      /* unset, and return false
       */
      newancs->num_F = 0;
      newancs->num_U = 0;
      for ( i = 0; i < oldnewancs->num_F; i++ ) {
	newancs->F[newancs->num_F++] = oldnewancs->F[i];
      }
      for ( i = 0; i < oldnewancs->num_U; i++ ) {
	newancs->U[newancs->num_U++] = oldnewancs->U[i];
      }


      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      free( prev_pos_end );
      free( prev_neg_end );


      free( oldnewancs->F );
      free( oldnewancs->U );
      free( oldnewancs );
      return FALSE;
    }
    if ( !forall_mode && rec_result ) {
      /* unset, and return true
       */
      newancs->num_F = 0;
      newancs->num_U = 0;
      for ( i = 0; i < oldnewancs->num_F; i++ ) {
	newancs->F[newancs->num_F++] = oldnewancs->F[i];
      }
      for ( i = 0; i < oldnewancs->num_U; i++ ) {
	newancs->U[newancs->num_U++] = oldnewancs->U[i];
      }


      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      free( prev_pos_end );
      free( prev_neg_end );


      free( oldnewancs->F );
      free( oldnewancs->U );
      free( oldnewancs );
      return TRUE;
    }

    newancs->num_F = 0;
    newancs->num_U = 0;
    for ( i = 0; i < oldnewancs->num_F; i++ ) {
      newancs->F[newancs->num_F++] = oldnewancs->F[i];
    }
    for ( i = 0; i < oldnewancs->num_U; i++ ) {
      newancs->U[newancs->num_U++] = oldnewancs->U[i];
    }


    for ( i = prevnumcl; i < gnum_clauses; i++ ) {
      gclause_length[i] = 0;
    }
    gnum_clauses = prevnumcl;
    for ( i = prevnumc+1; i <= gnum_c; i++ ) {
      gcodes[gct[i]][gcf[i]] = -1;
    }
    for ( i = 1; i <= prevnumc; i++ ) {
      gpos_c_in_clause_end[i] = prev_pos_end[i];
      gneg_c_in_clause_end[i] = prev_neg_end[i];
    }
    for ( ; i <= gnum_c; i++ ) {
      gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
      gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
    }
    free( prev_pos_end );
    free( prev_neg_end );


    free( oldnewancs->F );
    free( oldnewancs->U );
    free( oldnewancs );

    if ( forall_mode ) {
      /* tried both without failure
       */ 
      return TRUE;
    } else {
      /* tried both without success
       */ 
      return FALSE;
    }
  } /* if we set another nondet eff */








  /* else, we've set all the nondet effs on the paths. now, let's see if there's a setting
   * for our current nondet eff so that equality can be proved.
   */
  /* first, infer new known facts in source
   */
  newancs = &(source_node->S);
  oldnewancs->num_F = 0;
  oldnewancs->num_U = 0;
  for ( i = 0; i < newancs->num_F; i++ ) {
    oldnewancs->F[oldnewancs->num_F++] = newancs->F[i];
  }
  for ( i = 0; i < newancs->num_U; i++ ) {
    oldnewancs->U[oldnewancs->num_U++] = newancs->U[i];
  }

  i = 0;
  while ( i < newancs->num_U ) {
    testft = newancs->U[i];
    testff = testft;
    if ( gft_conn[testff].CNF ) {
      testm = 1;
    } else {
      testm = -1;
      testff = gft_conn[testff].negation;
    }
    testcode = gcodes[endtime-anc][testff];
    if ( testcode == -1 ) {
      printf("\nno testcode up here?\n\n");
      exit( 1 );
    }
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = testm * (-1) * testcode;
    gss_sat_calls++;
    sat = dp_CNF();
    if ( !sat ) {
      for ( j = i; j < newancs->num_U-1; j++ ) {
	newancs->U[j] = newancs->U[j+1];
      }
      newancs->num_U--;
      newancs->F[newancs->num_F++] = testft;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------inferred to be true at source:");
	print_ft_name( testft );
      }
    } else {
      i++;
    }
  }
  i = 0;
  while ( i < newancs->num_U ) {
    testft = newancs->U[i];
    testff = testft;
    if ( gft_conn[testff].CNF ) {
      testm = 1;
    } else {
      testm = -1;
      testff = gft_conn[testff].negation;
    }
    testcode = gcodes[endtime-anc][testff];
    if ( testcode == -1 ) {
      printf("\nno testcode up here?\n\n");
      exit( 1 );
    }
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = testm * testcode;
    gss_sat_calls++;
    sat = dp_CNF();
    if ( !sat ) {
      for ( j = i; j < newancs->num_U-1; j++ ) {
	newancs->U[j] = newancs->U[j+1];
      }
      newancs->num_U--;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------inferred to be false at source:");
	print_ft_name( testft );
      }
    } else {
      i++;
    }
  }
  if ( gcmd_line.P && gcmd_line.debug > 1 ) {
    printf("\n--------------------------created source as follows");
    print_state( *newancs );
    fflush(stdout);
  }
  

  if ( nondet_single_stagnates( FALSE, dest, dest_node, source_node, endtime, anc ) ) {
    newancs->num_F = 0;
    newancs->num_U = 0;
    for ( i = 0; i < oldnewancs->num_F; i++ ) {
      newancs->F[newancs->num_F++] = oldnewancs->F[i];
    }
    for ( i = 0; i < oldnewancs->num_U; i++ ) {
      newancs->U[newancs->num_U++] = oldnewancs->U[i];
    }
    free( oldnewancs->F );
    free( oldnewancs->U );
    free( oldnewancs );
    return TRUE;
  }
  if ( nondet_single_stagnates( TRUE, dest, dest_node, source_node, endtime, anc ) ) {
    newancs->num_F = 0;
    newancs->num_U = 0;
    for ( i = 0; i < oldnewancs->num_F; i++ ) {
      newancs->F[newancs->num_F++] = oldnewancs->F[i];
    }
    for ( i = 0; i < oldnewancs->num_U; i++ ) {
      newancs->U[newancs->num_U++] = oldnewancs->U[i];
    }
    free( oldnewancs->F );
    free( oldnewancs->U );
    free( oldnewancs );
    return TRUE;
  }
  newancs->num_F = 0;
  newancs->num_U = 0;
  for ( i = 0; i < oldnewancs->num_F; i++ ) {
    newancs->F[newancs->num_F++] = oldnewancs->F[i];
  }
  for ( i = 0; i < oldnewancs->num_U; i++ ) {
    newancs->U[newancs->num_U++] = oldnewancs->U[i];
  }
  free( oldnewancs->F );
  free( oldnewancs->U );
  free( oldnewancs );
  return FALSE;

}



Bool single_stagnates( State *dest, State *source, int endtime, int ancestornr )

{

  int i, j, k, ft, m, ff;
  int ft_source_code, ft_dest_code;
  Bool sat;

  
  if ( gcmd_line.P && gcmd_line.debug > 1) {
    printf("\n-------------------------------------checking stagnation, S anc %d",
	   ancestornr);
    print_state( *source );
    printf("\n-------------------------------------S'");
    print_state( *dest );
  }

  if ( gdomination_valid ) {
    for ( i = 0; i < dest->num_F; i++ ) {
      for ( j = 0; j < source->num_F; j++ ) {
	if ( source->F[j] == dest->F[i] ) {
	  break;
	}
      }
      if ( j == source->num_F ) {
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n---------------S.F !supseteq S'.F|r:");
	  print_ft_name( dest->F[i] );
	}
	return FALSE;
      }
    }
    
    for ( i = 0; i < dest->num_U; i++ ) {
      for ( j = 0; j < source->num_F; j++ ) {
	if ( source->F[j] == dest->U[i] ) {
	  break;
	}
      }
      if ( j == source->num_F ) {
	for ( j = 0; j < source->num_U; j++ ) {
	  if ( source->U[j] == dest->U[i] ) {
	    break;
	  }
	}
	if ( j == source->num_U ) {
	  if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	    printf("\n---------------S.F cup S.U !sup S'.U|r:");
	    print_ft_name( dest->U[i] );
	  }
	  return FALSE;
	}
      }
    }
  } else {/* domination not valid */
    if ( source->num_F != dest->num_F ) {
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------|S.F| != |S'.F|");
      }  
      return FALSE;
    }
    if ( dest->num_U != source->num_U ) {
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------|S.U| != |S'.U|");
      }
      return FALSE;
    }
    for ( i = 0; i < dest->num_F; i++ ) {
      for ( j = 0; j < source->num_F; j++ ) {
	if ( source->F[j] == dest->F[i] ) {
	  break;
	}
      }
      if ( j == source->num_F ) {
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n---------------S.F != S'.F:");
	  print_ft_name( dest->F[i] );
	}
	return FALSE;
      }
    }    
    for ( i = 0; i < dest->num_U; i++ ) {
      for ( j = 0; j < source->num_U; j++ ) {
	if ( source->U[j] == dest->U[i] ) {
	  break;
	}
      }
      if ( j == source->num_U ) {
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n---------------S.U != S'.U:");
	  print_ft_name( dest->U[i] );
	}
	return FALSE;
      }
    }
  } /* end pre-checks */
  
  
  /* and now the real thing: for all U (in dest) fts f,
   * see whether f can be true in dest but not in source, and,
   * if domination not valid, also other way round.
   */
  for ( i = 0; i < dest->num_U; i++ ) {
    ft = dest->U[i];
    if ( gcmd_line.P && gcmd_line.debug > 1 ) {
      printf("\n--------------potentially non-stagnating ft ");
      print_ft_name( ft );
    }
    
    if ( gdomination_valid ) {
      /* if that ft is true in S it stagnates.
       */
      for ( k = 0; k < source->num_F; k++ ) {
	if ( source->F[k] == ft ) {
	  break;
	}
      }
      if ( k < source->num_F ) {
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf(" in S.F!");
	}
	continue;
      }
    }

    /* we're getting close to SAT. get the codes and do some debugging.
     */
    if ( endtime < 1 ) {
      printf("\ndest ends at endtime %d?\n\n", endtime);
      exit( 1 );
    }
    if ( endtime - ancestornr < 0 ) {
      printf("\ndest ends at endtime %d, but ancestor is %d?\n\n", 
	     endtime, ancestornr);
      exit( 1 );
    }
    ff = ft;
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    ft_source_code = gcodes[endtime-ancestornr][ff];
    ft_dest_code = gcodes[endtime][ff];
    /* if there is no code for ft in source, which is U, then (see text in
     * state_transitions.c, conflict check fn) it should hold that
     * endtime - 1 = 0, ie source is initial state. debug test.
     */ 
    if ( ft_source_code == -1 ) {
      if ( endtime - ancestornr != 0 ) {
	printf("\nft S var not encoded, but U at time > 0??\n\n");
	exit( 1 );
      }
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf(" not encoded at S, therefore unconstrained wrpt (transitive?) conds so these can trigger");
      }
      /* no code -> in particular, unconstrained and can be true whenever it wants to.
       */
      return FALSE;
    }
    if ( ft_dest_code == -1 ) {
      printf("\nft S' var not encoded??\n\n");
      exit( 1 );
    }
    if ( gcmd_line.P && gcmd_line.debug >= 3 ) {
      printf("\nft S code %d, ft S' code %d",
	     ft_source_code, ft_dest_code);
    }
    
    
    /* here we go. must test sat.
     */
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * (-1) * ft_source_code;
    gdecision_stack[gnum_decision_stack++] = m * ft_dest_code;
    if ( gcmd_line.P && gcmd_line.debug  >= 3 ) {
      printf("\nstagnation check 1 coded CNF\n");
      print_encoded_clauses();
    }
    /* here it comes..
     */
    gss_sat_calls++;
    sat = dp_CNF();
    if ( sat ) {
      if ( gcmd_line.P && gcmd_line.debug  > 1) {
	printf("\nsat! can be false at S but true at S'!");
      }
      return FALSE;
    }  
    if ( lnondets || !gdomination_valid ) {
      /* 2nd test to ensure equality
       */
      gnum_decision_stack = 0;
      gdecision_stack[gnum_decision_stack++] = m * ft_source_code;
      gdecision_stack[gnum_decision_stack++] = m * (-1) * ft_dest_code;
      if ( gcmd_line.P && gcmd_line.debug  >= 3 ) {
	printf("\nstagnation check 2 coded CNF\n");
	print_encoded_clauses();
      }
      /* here it comes..
       */
      gss_sat_calls++;
      sat = dp_CNF();
      if ( sat ) {
	if ( gcmd_line.P && gcmd_line.debug  > 1) {
	  printf("\nsat! can be true at S but false at S'!");
	}
	return FALSE;
      }
    }
    if ( gcmd_line.P && gcmd_line.debug  > 1) {
      printf("\ntests unsat! stagnates!");
    }
  } /* for all dest->U facts */
  
  if ( gcmd_line.P && gcmd_line.debug ) {
    printf("\nall tests done! stagnates!");
  }
  return TRUE;

}



/* do as if nondet eff of op into source_node appears ?loweff_appears,
 * nondet eff of op into dest_node appears ?eff_appears, 
 * if then the states are the same, return TRUE
 */
Bool nondet_single_stagnates( Bool eff_appears, 
			      State *dest, BfsNode *dest_node, BfsNode *source_node, int endtime, int ancestornr )

{

  static Bool fc = TRUE;
  static State *newdest;

  static MemberList_pointer *prev_pos_end;
  static MemberList_pointer *prev_neg_end;


  State *source = &(source_node->S);

  int i, j, ft, m, ff;
  int ft_source_code, ft_dest_code;
  Bool sat;
  int op = dest_node->ingoing_edge->op; 
  int negft;
  Bool added;
  State *direct_source = &(dest_node->ingoing_edge->in_node->S);
  Bool ftFds = FALSE, ftUds = FALSE, ftFs = FALSE, ftUs = FALSE;
  int prevnumcl, prevnumc;

  if ( fc ) {
    newdest = ( State * ) calloc( 1, sizeof( State ) );
    make_state( newdest, gnum_ft_conn );
    prev_pos_end = ( MemberList_pointer * ) 
      calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
    prev_neg_end = ( MemberList_pointer * ) 
      calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
    fc = FALSE;
  }



  if ( gcmd_line.P && gcmd_line.debug > 1 ) {
    printf("\n-------------------------------------checking nondet stagnation, %d, S anc %d",
	   eff_appears, ancestornr);
    print_state( *source );
    printf("\n-------------------------------------S'");
    print_state( *dest );
    fflush(stdout);
  }

  ft = -1; 
  added = FALSE;
  if ( gop_conn[op].nondeteff == 0 ) {
    printf("\nHAEEEEEDDDD??\n\n");
    exit( 1 );
  }
  if ( gop_conn[op].nondeteff > 0 ) {
    added = TRUE;
    ft = gop_conn[op].nondeteff-1;
  }
  if ( gop_conn[op].nondeteff < 0 ) {
    added = FALSE;
    ft = ((-1)*gop_conn[op].nondeteff)-1;
  }
  negft = gft_conn[ft].negation;
  /* remember ft values
   */
  for ( i = 0; i < direct_source->num_F; i++ ) {
    if ( direct_source->F[i] == ft ) ftFs = TRUE;
  }
  for ( i = 0; i < direct_source->num_U; i++ ) {
    if ( direct_source->U[i] == ft ) ftUs = TRUE;
  }
  for ( i = 0; i < dest->num_F; i++ ) {
    if ( dest->F[i] == ft ) ftFds = TRUE;
  }
  for ( i = 0; i < dest->num_U; i++ ) {
    if ( dest->U[i] == ft ) ftUds = TRUE;
  }




/*   if ( (added && ftFs) || */
/*        (!added && !ftFs && !ftUs) ) { */
/*     if ( gcmd_line.P && gcmd_line.debug ) { */
/*       printf("\nnondet eff introduces no nondeterminism! doing normal!"); */
/*     } */
/*     return single_stagnates( dest, source, endtime, ancestornr ); */
/*   } */




  ff = ft;
  if ( gft_conn[ff].CNF ) {
    m = 1;
  } else {
    m = -1;
    ff = gft_conn[ff].negation;
  }
  if ( gcmd_line.P && gcmd_line.debug  > 1) {
    printf("\n--------------ft ");
    print_ft_name( ft );
    printf(" Fs %d, Us %d, Fds %d, Uds %d -- ff ", ftFs, ftUs, ftFds, ftUds);
    print_ft_name( ff );
    fflush(stdout);
  }


  /* we've got to update the CNF, and do SAT tests
   */
  /* remember previous state of CNF
   */
  prevnumcl = gnum_clauses;
  prevnumc = gnum_c;
  for ( i = 1; i <= gnum_c; i++ ) {
    prev_pos_end[i] = gpos_c_in_clause_end[i];
    prev_neg_end[i] = gneg_c_in_clause_end[i];
  }

  if ( !eff_appears ) {
    if ( added ) {
      /* neg NOOP
       */
      if ( !ftFs && (ftFds || ftUds) ) {/* else, ft N in ds and clause true */
	/* here we assume that there are no other effects of the action that
	 * add the ft!!! (--> no prevention conditions)
	 */
	if ( ftUs ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (ff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime-1;
	}
	if ( ftUds ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    (-1) * m * (ff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime;
	}
	gnum_clauses++;
	if ( gcmd_line.P && gcmd_line.debug  > 1) {
	  printf("\n--------------inserted neg NOOP clause ");
	  print_clause( gnum_clauses-1 );
	  fflush(stdout);
	}
      } else {
	if ( gcmd_line.P && gcmd_line.debug  > 1) {
	  printf("\n--------------neg NOOP clause TRUE");
	  fflush(stdout);
	}
      }
    } else {
      /* pos NOOP
       */
      if ( (ftFs || ftUs) && !ftFds ) {/* else, ft F in ds and clause true */
	/* here we assume that there are no other effects of the action that
	 * del the ft!!! (--> no prevention conditions)
	 */
	if ( ftUs ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    (-1) * m * (ff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime-1;
	}
	if ( ftUds ) {
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (ff+1);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    endtime;
	}
	gnum_clauses++;
	if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	  printf("\n--------------inserted pos NOOP clause ");
	  print_clause( gnum_clauses-1 );
	  fflush(stdout);
	}
      } else {
	if ( gcmd_line.P && gcmd_line.debug  > 1) {
	  printf("\n--------------pos NOOP clause TRUE");
	  fflush(stdout);
	}
      }
    } /* added or deleted */
  } /* eff not appears therefore NOOP needed */ else {
    /* we need the right unit clause
     */
    if ( added ) {
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	m * (ff+1);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	endtime;
      gnum_clauses++;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n--------------inserted add-unit clause ");
	print_clause( gnum_clauses-1 );
	fflush(stdout);
      }
    } /* add effect appears */ else {
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	(-1) * m * (ff+1);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	endtime;
      gnum_clauses++;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n--------------inserted del-unit clause ");
	print_clause( gnum_clauses-1 );
	fflush(stdout);
      }
    } /* del effect appears */
  }


  /* tell the new clause(s) to the planner.
   */
  extend_dynamic_clauses_base_encoding( prevnumcl, prevnumc );
















  /* calculate by case distinctions, new F and U arrays of
   * dest state, under given situation
   */
  if ( !eff_appears ) {
    /* ft and negft have same value as in direct_source
     */
    newdest->num_F = 0;
    newdest->num_U = 0;
    for ( i = 0; i < direct_source->num_F; i++ ) {
      if ( direct_source->F[i] == ft ) break;
    }
    if ( i < direct_source->num_F ) {
      /* ft F, negft N
       */
      newdest->F[newdest->num_F++] = ft;
    } else {
      /* ft either U or N
       */
      for ( i = 0; i < direct_source->num_U; i++ ) {
	if ( direct_source->U[i] == ft ) break;
      }
      if ( i < direct_source->num_U ) {
	/* ft, negft U
	 */
	newdest->U[newdest->num_U++] = ft;
	if ( negft != -1 ) {
	  newdest->U[newdest->num_U++] = negft;
	}
      } else {
	/* ft N
	 */
	if ( negft != -1 ) {
	  newdest->F[newdest->num_F++] = negft;
	}
      }
    } /* end ft, negft pre-treatment */
    /* copy over the rest from dest
     */
    for ( i = 0; i < dest->num_F; i++ ) {
      if ( dest->F[i] == ft || dest->F[i] == negft ) continue;
      newdest->F[newdest->num_F++] = dest->F[i];
    }
    for ( i = 0; i < dest->num_U; i++ ) {
      if ( dest->U[i] == ft || dest->U[i] == negft ) continue;
      newdest->U[newdest->num_U++] = dest->U[i];
    }
  } else {
    /* ft has value given by nondet eff
     */
    newdest->num_F = 0;
    newdest->num_U = 0;
    if ( added ) {
      newdest->F[newdest->num_F++] = ft;     
      ftFds = TRUE;
    } else {
      if ( negft != -1 ) {
	newdest->F[newdest->num_F++] = negft;
      }
    }
    /* copy over the rest from dest
     */
    for ( i = 0; i < dest->num_F; i++ ) {
      if ( dest->F[i] == ft || dest->F[i] == negft ) continue;
      newdest->F[newdest->num_F++] = dest->F[i];
    }
    for ( i = 0; i < dest->num_U; i++ ) {
      if ( dest->U[i] == ft || dest->U[i] == negft ) continue;
      newdest->U[newdest->num_U++] = dest->U[i];
    }
  }





  /* see what U facts in dest can be inferred to be true now
   */
  i = 0;
  while ( i < newdest->num_U ) {
    ft = newdest->U[i];
    ff = ft;
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    ft_dest_code = gcodes[endtime][ff];
    if ( ft_dest_code == -1 ) {
      printf("\nno ft_dest_code up here?\n\n");
      exit( 1 );
    }
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * (-1) * ft_dest_code;
    /* here it comes..
     */
    gss_sat_calls++;
    sat = dp_CNF();
    if ( !sat ) {
      for ( j = i; j < newdest->num_U-1; j++ ) {
	newdest->U[j] = newdest->U[j+1];
      }
      newdest->num_U--;
      newdest->F[newdest->num_F++] = ft;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------inferred to be true at dest:");
	print_ft_name( ft );
      }
    } else {
      i++;
    }
  }
  /* see what U facts in dest can be inferred to be false now
   */
  i = 0;
  while ( i < newdest->num_U ) {
    ft = newdest->U[i];
    ff = ft;
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    ft_dest_code = gcodes[endtime][ff];
    if ( ft_dest_code == -1 ) {
      printf("\nno ft_dest_code up here?\n\n");
      exit( 1 );
    }
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * ft_dest_code;
    /* here it comes..
     */
    gss_sat_calls++;
    sat = dp_CNF();
    if ( !sat ) {
      for ( j = i; j < newdest->num_U-1; j++ ) {
	newdest->U[j] = newdest->U[j+1];
      }
      newdest->num_U--;
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------inferred to be false at dest:");
	print_ft_name( ft );
      }
    } else {
      i++;
    }
  }
  if ( gcmd_line.P && gcmd_line.debug > 1 ) {
    printf("\n-------------------------------------created newdest as follows");
    print_state( *newdest );
    fflush(stdout);
  }











  /* check if F and U of newdest are identical to those of source
   */
  if ( source->num_F != newdest->num_F ) {
    if ( gcmd_line.P && gcmd_line.debug  > 1) {
      printf("\n---------------|newS.F| != |newS'.F|");
    }  
    for ( i = prevnumcl; i < gnum_clauses; i++ ) {
      gclause_length[i] = 0;
    }
    gnum_clauses = prevnumcl;
    for ( i = prevnumc+1; i <= gnum_c; i++ ) {
      gcodes[gct[i]][gcf[i]] = -1;
    }
    for ( i = 1; i <= prevnumc; i++ ) {
      gpos_c_in_clause_end[i] = prev_pos_end[i];
      gneg_c_in_clause_end[i] = prev_neg_end[i];
    }
    for ( ; i <= gnum_c; i++ ) {
      gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
      gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
    }
    return FALSE;
  }
  if ( newdest->num_U != source->num_U ) {
    if ( gcmd_line.P && gcmd_line.debug  > 1) {
      printf("\n---------------|newS.U| != |newS'.U|");
    }
    for ( i = prevnumcl; i < gnum_clauses; i++ ) {
      gclause_length[i] = 0;
    }
    gnum_clauses = prevnumcl;
    for ( i = prevnumc+1; i <= gnum_c; i++ ) {
      gcodes[gct[i]][gcf[i]] = -1;
    }
    for ( i = 1; i <= prevnumc; i++ ) {
      gpos_c_in_clause_end[i] = prev_pos_end[i];
      gneg_c_in_clause_end[i] = prev_neg_end[i];
    }
    for ( ; i <= gnum_c; i++ ) {
      gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
      gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
    }
    return FALSE;
  }
  for ( i = 0; i < newdest->num_F; i++ ) {
    for ( j = 0; j < source->num_F; j++ ) {
      if ( source->F[j] == newdest->F[i] ) {
	break;
      }
    }
    if ( j == source->num_F ) {
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------newS.F != newS'.F:");
	print_ft_name( newdest->F[i] );
      }
      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      return FALSE;
    }
  }    
  for ( i = 0; i < newdest->num_U; i++ ) {
    for ( j = 0; j < source->num_U; j++ ) {
      if ( source->U[j] == newdest->U[i] ) {
	break;
      }
    }
    if ( j == source->num_U ) {
      if ( gcmd_line.P && gcmd_line.debug > 1 ) {
	printf("\n---------------newS.U != newS'.U:");
	print_ft_name( newdest->U[i] );
      }
      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      return FALSE;
    }
  }







  /* F and U checks Ok, let's do SAT
   */
  for ( i = 0; i < newdest->num_U; i++ ) {
    ft = newdest->U[i];
    if ( gcmd_line.P && gcmd_line.debug  > 1) {
      printf("\n--------------potentially non-stagnating ft ");
      print_ft_name( ft );
    }
    /* we're getting close to SAT. get the codes and do some debugging.
     */
    if ( endtime < 1 ) {
      printf("\ndest ends at endtime %d?\n\n", endtime);
      exit( 1 );
    }
    if ( endtime - ancestornr < 0 ) {
      printf("\ndest ends at endtime %d, but ancestor is %d?\n\n", 
	     endtime, ancestornr);
      exit( 1 );
    }
    ff = ft;
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    ft_source_code = gcodes[endtime-ancestornr][ff];
    ft_dest_code = gcodes[endtime][ff];
    /* if there is no code for ft in source, which is U, then (see text in
     * state_transitions.c, conflict check fn) it should hold that
     * endtime - 1 = 0, ie source is initial state. debug test.
     */ 
    if ( ft_source_code == -1 ) {
      if ( endtime - ancestornr != 0 ) {
	printf("\nft S var not encoded, but U at time > 0??\n\n");
	exit( 1 );
      }
      if ( gcmd_line.P && gcmd_line.debug ) {
	printf(" not encoded at S, therefore unconstrained wrpt (transitive?) conds so these can trigger");
      }
      /* no code -> in particular, unconstrained and can be true whenever it wants to.
       */
      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      printf("\nno ft source code bail out");
      return FALSE;
    }
    if ( ft_dest_code == -1 ) {
      printf("\nft S' var not encoded??\n\n");
      exit( 1 );
    }
    if ( gcmd_line.P && gcmd_line.debug >= 3 ) {
      printf("\nft S code %d, ft S' code %d",
	     ft_source_code, ft_dest_code);
    }
    /* here we go. must test sat.
     */
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * (-1) * ft_source_code;
    gdecision_stack[gnum_decision_stack++] = m * ft_dest_code;
    if ( gcmd_line.P && gcmd_line.debug  >= 3 ) {
      printf("\nstagnation check 17 coded CNF\n");
      print_clauses();
    }
    /* here it comes..
     */
    gss_sat_calls++;
    sat = dp_CNF();
    if ( sat ) {
      if ( gcmd_line.P && gcmd_line.debug  > 1) {
	printf("\nsat! can be false at S but true at S'!");
      }
      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      return FALSE;
    }  
    /* 2nd test to ensure equality
     */
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * ft_source_code;
    gdecision_stack[gnum_decision_stack++] = m * (-1) * ft_dest_code;
    if ( gcmd_line.P && gcmd_line.debug  >= 3 ) {
      printf("\nstagnation check 27 coded CNF\n");
      print_clauses();
    }
    /* here it comes..
     */
    gss_sat_calls++;
    sat = dp_CNF();
    if ( sat ) {
      if ( gcmd_line.P && gcmd_line.debug  > 1) {
	printf("\nsat! can be true at S but false at S'!");
      }
      for ( i = prevnumcl; i < gnum_clauses; i++ ) {
	gclause_length[i] = 0;
      }
      gnum_clauses = prevnumcl;
      for ( i = prevnumc+1; i <= gnum_c; i++ ) {
	gcodes[gct[i]][gcf[i]] = -1;
      }
      for ( i = 1; i <= prevnumc; i++ ) {
	gpos_c_in_clause_end[i] = prev_pos_end[i];
	gneg_c_in_clause_end[i] = prev_neg_end[i];
      }
      for ( ; i <= gnum_c; i++ ) {
	gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
	gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
      }
      return FALSE;
    }
    if ( gcmd_line.P && gcmd_line.debug > 1 ) {
      printf("\ntests unsat! stagnates!");
    }
  } /* for all dest->U facts */
  
  
  if ( gcmd_line.P && gcmd_line.debug ) {
    printf("\nall tests done! stagnates!");
  }
  for ( i = prevnumcl; i < gnum_clauses; i++ ) {
    gclause_length[i] = 0;
  }
  gnum_clauses = prevnumcl;
  for ( i = prevnumc+1; i <= gnum_c; i++ ) {
    gcodes[gct[i]][gcf[i]] = -1;
  }
  for ( i = 1; i <= prevnumc; i++ ) {
    gpos_c_in_clause_end[i] = prev_pos_end[i];
    gneg_c_in_clause_end[i] = prev_neg_end[i];
  }
  for ( ; i <= gnum_c; i++ ) {
    gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
    gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
  }
  return TRUE;

}



































