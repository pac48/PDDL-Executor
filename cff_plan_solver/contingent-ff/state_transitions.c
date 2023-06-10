


/*
 * THIS SOURCE CODE IS SUPPLIED  ``AS IS'' WITHOUT WARRANTY OF ANY KIND, 
 * AND ITS AUTHOR AND THE JOURNAL OF ARTIFICIAL INTELLIGENCE RESEARCH 
 * (JAIR) AND JAIR'S PUBLISHERS AND DISTRIBUTORS, DISCLAIM ANY AND ALL 
 * WARRANTIES, INCLUDING BUT NOT LIMITED TO ANY IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND
 * ANY WARRANTIES OR NON INFRINGEMENT.  THE USER ASSUMES ALL LIABILITY AND
 * RESPONSIBILITY FOR USE OF THIS SOURCE CODE, AND NEITHER THE AUTHOR NOR
 * JAIR, NOR JAIR'S PUBLISHERS AND DISTRIBUTORS, WILL BE LIABLE FOR 
 * DAMAGES OF ANY KIND RES.LTING FROM ITS USE.  Without limiting the 
 * generality of the foregoing, neither the author, nor JAIR, nor JAIR's
 * publishers and distributors, warrant that the Source Code will be 
 * error-free, will operate without interruption, or will meet the needs 
 * of the user.
 */











/*********************************************************************
 *
 * File: state_transitions.c
 *
 * Description: computes new search states. quite complicated.
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
















/***********************************************************
 * LOCAL GLOBALS                                           *
 ***********************************************************/








/* hitting set -- in CNF construction for noops in presence of non-unary antecedents.
 */
int *lhitting_set;
int lnum_hitting_set;



/* for naive DP implementation
 */
int *lassigned;



/* for each possible ft code, a pointer to connected dynamic list
 * of member elements, ie the clauses in which it participates,
 * positive and negative.
 */
MemberList_pointer *gpos_c_in_clause_start;
MemberList_pointer *gpos_c_in_clause_fixed;/* up to here, list corresp. to fixed CNF */
MemberList_pointer *gpos_c_in_clause_end;/* this is current list end */
MemberList_pointer *gneg_c_in_clause_start;
MemberList_pointer *gneg_c_in_clause_fixed;
MemberList_pointer *gneg_c_in_clause_end;







/* make space for global arrays needed here
 */
void initialize_state_transitions( void )

{

  int i, j;

  gclauses = ( TimedLiteral ** ) calloc( gmax_clauses, sizeof( TimedLiteral * ) );
  gclause_length = ( int * ) calloc( gmax_clauses, sizeof( int ) );
  for ( i = 0; i < gmax_clauses; i++ ) {
    gclauses[i] = ( TimedLiteral * ) calloc( gmax_literals, sizeof( TimedLiteral ) );
    gclause_length[i] = 0;
  }
  gnum_fixed_clauses = 0;
  gnum_clauses = 0;

  gcodes = ( int ** ) calloc( MAX_PLAN_LENGTH + 1, sizeof( int * ) );
  for ( i = 0; i < MAX_PLAN_LENGTH + 1; i++ ) {
    gcodes[i] = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    for ( j = 0; j < gnum_ft_conn; j++ ) {
      gcodes[i][j] = -1;
    }
  }
  gnum_fixed_c = 0;

  gcf = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( int ) );
  gct = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( int ) );

  gpos_c_in_clause_start = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
  gpos_c_in_clause_fixed = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
  gpos_c_in_clause_end = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
  gneg_c_in_clause_start = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
  gneg_c_in_clause_fixed = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
  gneg_c_in_clause_end = ( MemberList_pointer * ) 
    calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( MemberList_pointer ) );
  for ( i = 0; i <= (1 + MAX_PLAN_LENGTH) * gmax_CNFU; i++ ) {
    gpos_c_in_clause_start[i] = NULL;
    gneg_c_in_clause_start[i] = NULL;
    gpos_c_in_clause_end[i] = NULL;
    gneg_c_in_clause_end[i] = NULL;
    gpos_c_in_clause_fixed[i] = NULL;
    gneg_c_in_clause_fixed[i] = NULL;
  }

  lhitting_set = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );

  gdecision_stack = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU, sizeof( int ) );

  lassigned = ( int * ) calloc( (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1, sizeof( int ) );
  for ( i = 0; i < (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1; i++ ) {
    lassigned[i] = -1;
  }


  gsum_k_clauses = ( float * ) calloc( gmax_literals + 1, sizeof( float ) );
  for ( i = 0; i < gmax_literals + 1; i++ ) {
    gsum_k_clauses[i] = 0;
  }


}


















/***********************************************************
 * THIS HERE IS THE "MAIN" FN, CALLED DIRECTLY FROM SEARCH *
 ***********************************************************/
















/* function that computes state transition as induced by a
 * normalized ADL action under unknown literals.
 * (uses CNF decision mechanism for the semantics of the latter)
 *
 * implementation is largely naive, look at that again if it's
 * runtime relevant --- which I doubt.
 */
Bool result_to_dest( State *dest, BfsNode *dest_node, int observe_mode )

{

  static Bool first_call = TRUE;
  static Bool *F, *U, *Npp, *adds, *dels, *uadds, *udels, *nadds, *ndels;
  static int *check_pos, *check_neg, *check_both;

  State *source;

  int num_cp, num_cn, num_cb;
  int i, j, fcount, ucount, ft, ef, k, endtime, nextstart, nextcstart;
  int op = dest_node->ingoing_edge->op;
  int observed_ft;

  Bool retval = TRUE;

  if ( first_call ) {
    F = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    U = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    Npp = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    adds = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    dels = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    uadds = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    udels = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    nadds = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    ndels = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    check_pos = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    check_neg = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    check_both = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      F[i] = FALSE;
      U[i] = FALSE;
      Npp[i] = FALSE;
      adds[i] = FALSE;
      dels[i] = FALSE;
      uadds[i] = FALSE;
      udels[i] = FALSE;
      nadds[i] = FALSE;
      ndels[i] = FALSE;
    }
    
    first_call = FALSE;
  }

  if ( gcmd_line.T && gcmd_line.debug ) {
    printf("\nentering result to dest. obs mode %d action ", observe_mode);
    print_op_name( op );
  }
  source = &(dest_node->ingoing_edge->in_node->S);

  /* special case treatment for observation actions
   */
  if ( observe_mode != -1 ) {
    if ( gop_conn[op].num_O != 1 ) {
      printf("\n\nother than one O in result fn\n\n");
      exit( 1 );
    }
    /* first, if first call of this action on this state,
     * see if observed fact is unknown in source; otherwise,
     * action is not applicable (does not change the state)
     */
    observed_ft = gop_conn[op].O[0];
    if ( observe_mode == 1 ) {
      for ( i = 0; i < source->num_U; i++ ) {
	if ( source->U[i] == observed_ft ) break;
      }
      if ( i == source->num_U ) {
	return FALSE;
      }
      /* no effects --> no unknown effects;
       * do this here since setting these U effs up is the responsibility
       * of the results fn.
       */
      dest_node->ingoing_edge->num_unknown_E = 0;
    }
    /* Ok. now simply set up the the new state, and get back.
     * both is trivial under the current restriction that observe actions
     * have no effects (other than the observation and the resulting branch).
     *
     * have to take care of negation facts here, since these are elsewhere not
     * handled in the O fields.
     */
    for ( i = 0; i < source->num_F; i++ ) {
      dest->F[i] = source->F[i];
    }
    dest->num_F = source->num_F;
    if ( observe_mode == 1 ) {
      dest->F[dest->num_F++] = observed_ft;
    }
    if ( observe_mode == 0 && gft_conn[observed_ft].negation != -1 ) {
      dest->F[dest->num_F++] = gft_conn[observed_ft].negation;
    }
   
    dest->num_U = 0;
    for ( i = 0; i < source->num_U; i++ ) {
      if ( source->U[i] == observed_ft ) continue;
      if ( gft_conn[observed_ft].negation != -1 && 
	   source->U[i] == gft_conn[observed_ft].negation ) continue;
      dest->U[dest->num_U++] = source->U[i];
    }

    
    /* now see if any of the unknown facts in dest become true or false due to 
     * our observation, ie if they follow.
     *
     * naive: simply assume that any other fact may become true or false, and check
     * if it does by a SAT call. heuristics for restricting the fact set?
     */
  } /* observation action first setup steps */



  /* set up direct F and U info
   */
  for ( i = 0; i < source->num_F; i++ ) {
    F[source->F[i]] = TRUE;
  }
  for ( i = 0; i < source->num_U; i++ ) {
    U[source->U[i]] = TRUE;
  }
  /* setup known and unknown adds and dels.
   */
  dest_node->ingoing_edge->num_unknown_E = 0;
  for ( i = 0; i < gop_conn[op].num_E; i++ ) {
    ef = gop_conn[op].E[i];
    fcount = 0;
    ucount = 0;
    for ( j = 0; j < gef_conn[ef].num_C; j++ ) {
      if ( F[gef_conn[ef].C[j]] ) fcount++;
      if ( U[gef_conn[ef].C[j]] ) ucount++;
      /* DEBUGGING TEST. REMOVE LATER.
       */
      if ( F[gef_conn[ef].C[j]] && U[gef_conn[ef].C[j]] ) {
	printf("\nsource F and U intersec non-empty?\n\n");
	    exit(1);
      }
    }
    if ( fcount == gef_conn[ef].num_C ) {
      for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
	if ( !gef_conn[ef].A_nondet[j] ) {
	  adds[gef_conn[ef].A[j]] = TRUE;
	} else {
	  nadds[gef_conn[ef].A[j]] = TRUE;
	}
      }
      for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
	if ( !gef_conn[ef].D_nondet[j] ) {
	  dels[gef_conn[ef].D[j]] = TRUE;
	} else {
	  ndels[gef_conn[ef].D[j]] = TRUE;
	}
      }
      continue;
    }
    if ( fcount + ucount == gef_conn[ef].num_C ) {
      dest_node->ingoing_edge->unknown_E[dest_node->ingoing_edge->num_unknown_E++] = ef;
      for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
	uadds[gef_conn[ef].A[j]] = TRUE;
      }
      for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
	udels[gef_conn[ef].D[j]] = TRUE;
      }
    }
  }


  /* setup the CNF for the following SAT calls.
   */

  /* EFFICIENCY: HERE, AND BELOW, ONE COULD DO SIMPLE PRE-CHECKS TO SEE
   * WHETHER THERE'LL BE ANY SAT REASONING NEEDED::: ONLY IF EITHER
   * OP CAN CONTRADICT ITSELF OR WHEN THERE ARE POS/NEG CHECK FACTS!!
   */
 
  times( &end );
  TIME( gsearch_time );
  times( &start );    
  
  /* first, extend the current fixed clauses with the search path up to source.
   * will be the same for all literals to be checked (?)
   */
  endtime = extend_dynamic_clauses_base( dest,
					 dest_node,
					 Npp,
					 TRUE );

  nextstart = gnum_clauses;

  times( &end );
  TIME( gcnf_time );
  times( &start );
  
  /* then extend the encoding, ie map the literal / time pairs
   * into integers.
   *
   * Mar'04 NOTE: this is wasteful and completely unnecessary - we could just as well 
   *              implement DP directly on the timed actions encoding.
   *              well... the int IDs help in indexing into the "assigned" array...
   *
   * the translation code is in table gcodes for use below in the
   * single checks.
   */
  extend_dynamic_clauses_base_encoding( gnum_fixed_clauses, gnum_fixed_c );
  nextcstart = gnum_c;

  times( &end );
  TIME( genc_time );
  times( &start );


  /* check for possible contradictions.
   */
  for ( ft = 0; ft < gnum_ft_conn; ft++ ) {
    if ( adds[ft] && dels[ft] ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\naction ");
	print_op_name( op );
	printf(" contradicts itself. skipping it.");
      }
      retval = FALSE;
    }
    if ( nadds[ft] && ndels[ft] ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\naction ");
	print_op_name( op );
	printf(" can contradict itself. skipping it.");
      }
      retval = FALSE;
    }
    if ( adds[ft] && (udels[ft] || ndels[ft]) ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\naction ");
	print_op_name( op );
	printf(" can delete an add. skipping it.");
	print_ft_name( ft ); exit( 1 );
      }
      retval = FALSE;
    }
    if ( (uadds[ft] || nadds[ft]) && dels[ft] ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\naction ");
	print_op_name( op );
	printf(" can add a delete. skipping it.");
      }
      retval = FALSE;
    }
    if ( uadds[ft] && udels[ft] ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\naction ");
	print_op_name( op );
	printf(" may contradict itself on fact ");
	print_ft_name( ft );
	printf(". checking that.");
      }
      if ( can_contradict_in( ft, dest_node, endtime - 1 ) ) {
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\naction may contradict itself! skipping action!");
	}
	retval = FALSE;
      }
    }
  }



  if ( retval ) {
    if ( observe_mode == -1 ) {
      /* proceed over all facts and see whether they will be true,
       * false, or unknown for sure, and whether we must check if they
       * become true or false.
       */
      dest->num_F = 0;
      dest->num_U = 0;
      num_cp = 0;
      num_cn = 0;
      num_cb = 0;
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\n\n---------------------------state transition, setups");
      }
      for ( ft = 0; ft < gnum_ft_conn; ft++ ) {
	if ( F[ft] ) {
	  /* this was true; note that per assumption all adds and dels
	   * are cap empty
	   */
	  if ( dels[ft] ) {
	    /* remember, for use in CNF, that we *know* this is false
	     */
	    Npp[ft] = TRUE;
	    continue;/* state-wise, false <-> do not include ft into F' or U' */
	  }
	  if ( ndels[ft] ) {
	    /* this becomes completely unknown: it is deleted by a nondet effect,
	     * and was true before that.
	     */
	    /* note: order matters, ie membership in dels is asked prior to
	     * this here!!
	     */
	    if ( gcmd_line.T && gcmd_line.debug ) {
	      printf("\n---becomes U due to nondet del "); 
	      print_ft_name( ft );
	    }
	    dest->U[dest->num_U++] = ft;
	    continue;
	  }
	  /* note: order matters, ie the above are asked first!
	   */
	  if ( udels[ft] ) {
	    /* might become negative; can not stay pos as then all del effs
	     * would need to provably stay out, and we'd know that
	     */
	    check_neg[num_cn++] = ft;
	    continue;
	  }
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\n---remains true "); 
	    print_ft_name( ft );
	  }
	  dest->F[dest->num_F++] = ft;
	  continue;
	}
	if ( U[ft] ) {
	  /* this was unknown
	   */
	  if ( adds[ft] ) {
	    dest->F[dest->num_F++] = ft;
	    continue;
	  }
	  if ( dels[ft] ) {
	    /* remember, for use in CNF, that we *know* this is false
	     */
	    Npp[ft] = TRUE;
	    continue;
	  }
	  if ( uadds[ft] ) {
	    check_pos[num_cp++] = ft;
	    continue;
	  }	      
	  if ( udels[ft] ) {
	    check_neg[num_cn++] = ft;
	    continue;
	  }
	  dest->U[dest->num_U++] = ft;
	  continue;
	}
	/* this was false
	 */ 
	if ( adds[ft] ) {
	  dest->F[dest->num_F++] = ft;
	  continue;
	}
	if ( nadds[ft] ) {
	  /* this becomes completely unknown: it is added by a nondet effect,
	   * and was false before that.
	   */
	  /* note: order matters, ie membership in dels is asked prior to
	   * this here!!
	   */
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\n---becomes U due to nondet add "); 
	    print_ft_name( ft );
	  }
	  dest->U[dest->num_U++] = ft;
	  continue;
	}
	/* note: order matters, ie the above are asked first!
	 */
	if ( uadds[ft] ) {
	  /* similar to above this can not stay false, so if it not
	   * becomes pos then it becomes U.
	   */
	  check_pos[num_cp++] = ft;
	  continue;
	}
	/* if it's not added then we do nothing, leaving it false.
	 *
	 * we remember, though, for use in CNF, that we *know* this is false
	 */
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\n---remains false "); 
	  print_ft_name( ft );
	}
	Npp[ft] = TRUE;
      } /* loop over all fts */
    } else { /* observe mode != -1 */
      /* naive: check all unknown facts.
       */
      num_cp = 0;
      num_cn = 0;
      num_cb = 0;
      for ( i = 0; i < dest->num_U; i++ ) {
	/* new array to check both directions
	 */
	check_both[num_cb++] = dest->U[i];
      }
      dest->num_U = 0;
    }
    times( &end );
    TIME( gsearch_time );
    times( &start );    
    
    /* now, extend the current clauses with the step source -> dest.
     * will be the same for all literals to be checked (?)
     */
    endtime = extend_dynamic_clauses_base( dest,
					   dest_node,
					   Npp,
					   FALSE );
    times( &end );
    TIME( gcnf_time );
    times( &start );
    /* goes over fixed until num, so some of these have been encoded above;
     * don't matter as in these cases coded remain the same !?
     */
    extend_dynamic_clauses_base_encoding( nextstart, nextcstart );
    gcnfs++;
    times( &end );
    TIME( genc_time );
    times( &start );   
    
    /* hand the to-be-checked facts over to a SAT solver
     */
    handle_inferred_literals( dest,
			      check_pos, num_cp,
			      check_neg, num_cn,
			      check_both, num_cb,
			      Npp, endtime );
    
    
    
    /* DEBUGGING TEST. REMOVE LATER.
     * (unknown E s computed at start already can not interfere with
     *  F' and U' values, can they?)
     */
    if ( gcmd_line.T && gcmd_line.debug ) {
      for ( i = 0; i < dest_node->ingoing_edge->num_unknown_E; i++ ) {
	ef = dest_node->ingoing_edge->unknown_E[i];
	/* can the effect del an F' fact?
	 */
	for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
	  for ( k = 0; k < dest->num_F; k++ ) {
	    if ( gef_conn[ef].D[j] == dest->F[k] ) {
	      printf("\nunknown effect deletes F'?");
	    }
	  }
	}
	/* can the effect add an N' fact?
	 */
	for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
	  for ( k = 0; k < dest->num_F; k++ ) {
	    if ( gef_conn[ef].A[j] == dest->F[k] ) {
	      break;
	    }
	  }
	  if ( k < dest->num_F ) continue;
	  for ( k = 0; k < dest->num_U; k++ ) {
	    if ( gef_conn[ef].A[j] == dest->U[k] ) {
	      break;
	    }
	  }
	  if ( k < dest->num_U ) continue;
	  printf("\nunknown effect adds N'?");
	}
      }
    }
  
    /* finally, check whether this new state stagnates, ie whether
     * it is not the case that one fact can be true in it that isn't
     * along the path to it.
     * if domination not valid, the test is if the new state is equal to
     * any of the states on the path to it.
     * stagnating states are cut out by making the action
     * inapplicable.
     *
     * (NOTE: stagnation test easy to implement and set up here as 
     *  CNF already present in gclauses!!)
     */
    if ( gcmd_line.stagnating > 0 && observe_mode == -1 &&
	 stagnates( dest, dest_node, endtime ) ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\nsuccessor state stagnates! skipping action!");
      }
      retval = FALSE;
    }
  } /* if retval */

  /* undo the clauses and encoding infos
   */
  for ( i = gnum_fixed_clauses; i < gnum_clauses; i++ ) {
    gclause_length[i] = 0;
  }
  gnum_clauses = gnum_fixed_clauses;
  for ( i = gnum_fixed_c + 1; i <= gnum_c; i++ ) {
    gcodes[gct[i]][gcf[i]] = -1;
  }
  /* here, UNSET the extended lit-in-clause pointers!!
   */
  for ( i = 1; i <= gnum_fixed_c; i++ ) {
    gpos_c_in_clause_end[i] = gpos_c_in_clause_fixed[i];
    gneg_c_in_clause_end[i] = gneg_c_in_clause_fixed[i];
  }
  for ( ; i <= gnum_c; i++ ) {
    if ( gpos_c_in_clause_start[i] == NULL ||
	 gneg_c_in_clause_start[i] == NULL ) {
      printf("\nin reset, var %d has no dummy nodes in mem list yet??\n\n", i);
      exit( 1 );
    }
    if ( gpos_c_in_clause_fixed[i] != gpos_c_in_clause_start[i]->next ||
	 gneg_c_in_clause_fixed[i] != gneg_c_in_clause_start[i]->next ) {
      printf("\nin reset, for new dyn. code %d fixed != start->next??\n\n", i);
      exit( 1 );
    }
    gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
    gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
  }

  /* unset direct infos
   */
  for ( i = 0; i < gnum_ft_conn; i++ ) {
    F[i] = FALSE;
    U[i] = FALSE;
    Npp[i] = FALSE;
    adds[i] = FALSE;
    dels[i] = FALSE;
    uadds[i] = FALSE;
    udels[i] = FALSE;
    nadds[i] = FALSE;
    ndels[i] = FALSE;
  }

  return retval;

}
























/*********************************************************************
 * POSSIBLE SELF-CONTRADICTION (CALLED BY STATE TRANSITION FUNCTION) *
 *********************************************************************/





























/* NOTE: THIS IS VERY INEFFICIENT IN THE SENSE THAT THE SAME PAIR OF EFFECTS
 * MAY BE CHECKED MULTIPLE TIMES. AVOID THAT AT SOME POINT!!
 */
Bool can_contradict_in( int ft, BfsNode *dest_node, int endtime )

{

  int i, j, k, l, ef, ef_, ff, m;
  Bool sat;
  State *source;

  times( &end );
  TIME( gsearch_time );
  times( &start );    

  if ( !dest_node ||
       !dest_node->ingoing_edge ||
       !dest_node->ingoing_edge->in_node ) {
    printf("\nself-contradiction test missing in-edge/in-edge-in-node?\n\n");
    exit( 1 );
  }
  source = &(dest_node->ingoing_edge->in_node->S);

  for ( i = 0; i < dest_node->ingoing_edge->num_unknown_E; i++ ) {
    ef = dest_node->ingoing_edge->unknown_E[i];
    for ( k = 0; k < gef_conn[ef].num_A; k++ ) {
      if ( gef_conn[ef].A[k] == ft ) break;
    }
    if ( k == gef_conn[ef].num_A ) continue;
    
    /* this one adds ft. look over all others that del ft.
     */
    for ( j = 0; j < dest_node->ingoing_edge->num_unknown_E; j++ ) {
      ef_ = dest_node->ingoing_edge->unknown_E[j];
      /* we also admit i == j, checking for contras inside an effect.
       * (quite useless, but may be part of a specification)
       * (we could test this special case above already... who cares...)
       */
      for ( k = 0; k < gef_conn[ef_].num_D; k++ ) {
	if ( gef_conn[ef_].D[k] == ft ) break;
      }
      if ( k == gef_conn[ef_].num_D ) continue;
      
      /* ef and ef_ contradict each other. add their conds as unit clauses
       * at source time and see whether the result is sat. if it is, we got
       * a potential contradiction. if not, not.
       */
      /* NEW: we don't actually insert unit clauses, but push the respective
       * decisions onto the stack used in DP. that is, we initialise these 
       * values here!!
       */
      gnum_decision_stack = 0;
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\nchecking C achievable: ");
      }
      for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	ff = gef_conn[ef].C[k];
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\n");
	  print_ft_name( ff ); printf("(%d)", endtime);
	}
	for ( l = 0; l < source->num_F; l++ ) {
	  if ( source->F[l] == ff ) break;
	}
	if ( l < source->num_F ) {
	  if ( gcmd_line.T && gcmd_line.debug ) printf(" is TRUE!");
	  continue;
	}
	/* DEBUGGING, REMOVE LATER
	 */
	for ( l = 0; l < source->num_U; l++ ) {
	  if ( source->U[l] == ff ) break;
	}
	if ( l == source->num_U ) {
	  printf(" is not U??");
	  exit( 1 );
	}
	if ( gft_conn[ff].CNF ) {
	  m = 1;
	} else {
	  m = -1;
	  ff = gft_conn[ff].negation;
	}
	if ( gcodes[endtime][ff] == -1 ) {
	  /* what I *think* holds true is this. if there are no clauses
	   * on the var, then it is either endtime = 0 and the var is free
	   * wrspt. the others (ie we can make it true whenever we want to),
	   * or endtime > 0 but no pos noop on the var; now, the pos noop is
	   * out only if a del eff is in for sure, in which case ff wouldn't
	   * be U in the 1st place, contradiction so endtime = 0. 
	   * a little debug print to verify the latter.
	   */
	  if ( endtime != 0 ) {
	    printf("\nC check var not encoded, but U at time > 0??\n\n");
	    exit( 1 );
	  }
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("is not encoded, therefore unconstrained");
	  }
	  continue;
	}
	/* finally, check if we recorded that literal already.
	 */
	for ( l = 0; l < gnum_decision_stack; l++ ) {
	  if ( gdecision_stack[l] == m * gcodes[endtime][ff] ) break;
	}
	if ( l < gnum_decision_stack ) {
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("on dec stack already, skipping it");
	  }
	  continue;
	}
	gdecision_stack[gnum_decision_stack++] = m * gcodes[endtime][ff];
      }
      for ( k = 0; k < gef_conn[ef_].num_C; k++ ) {
	ff = gef_conn[ef_].C[k];
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\n");
	  print_ft_name( ff ); printf("(%d)", endtime);
	}
	for ( l = 0; l < source->num_F; l++ ) {
	  if ( source->F[l] == ff ) break;
	}
	if ( l < source->num_F ) {
	  if ( gcmd_line.T && gcmd_line.debug ) printf(" is TRUE!");
	  continue;
	}
	/* DEBUGGING, REMOVE LATER
	 */
	for ( l = 0; l < source->num_U; l++ ) {
	  if ( source->U[l] == ff ) break;
	}
	if ( l == source->num_U ) {
	  printf(" is not U??");
	  exit( 1 );
	}
	if ( gft_conn[ff].CNF ) {
	  m = 1;
	} else {
	  m = -1;
	  ff = gft_conn[ff].negation;
	}
	if ( gcodes[endtime][ff] == -1 ) {
	  /* s.a.
	   */
	  if ( endtime != 0 ) {
	    printf("\nC check var not encoded, but U at time > 0??\n\n");
	    exit( 1 );
	  }
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("is not encoded, therefore unconstrained");
	  }
	  continue;
	}
	for ( l = 0; l < gnum_decision_stack; l++ ) {
	  if ( gdecision_stack[l] == m * gcodes[endtime][ff] ) break;
	}
	if ( l < gnum_decision_stack ) {
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("on dec stack already, skipping it");
	  }
	  continue;
	}
	gdecision_stack[gnum_decision_stack++] = m * gcodes[endtime][ff];
      }
      if ( gcmd_line.T && gcmd_line.debug >= 4 ) {
	printf("\nconflict check coded CNF\n");
	print_encoded_clauses();
      }
      if ( gnum_decision_stack == 0 ) {
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nno var encoded! all free, sat, conflict!");
	}
	times( &end );
	TIME( gsc_time );
	times( &start );
	return TRUE;	
      }
      /* here it comes..
       */
      gsc_sat_calls++;
      sat = dp_CNF();
      if ( sat ) {
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nsat! conflict!");
	}
	times( &end );
	TIME( gsc_time );
	times( &start );
	return TRUE;
      } else {
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nunsat! no conflict!");
	}
      }
    }
  }
  
  times( &end );
  TIME( gsc_time );
  times( &start );
  return FALSE;

}






























































/**************************************************************
 * CONFORMANT SEMANTICS (CALLED BY STATE TRANSITION FUNCTION) *
 **************************************************************/









































/* implementation largely naive. if critical, reconsider.
 */
void handle_inferred_literals( State *dest, 
			       int *check_pos, int num_cp,
			       int *check_neg, int num_cn,
			       int *check_both, int num_cb,
			       Bool *Npp, int endtime )

{

  int i, j, m, ff;
  Bool sat;

  if ( num_cp + num_cn + num_cb == 0 ) {
    /* naught to check --> get back.
     */
    return;
  }
  
  /* proceed over the queries, check, and update dest info.
   *
   * NOTE: the to-be-checked facts all appear in unknown effects
   * so they appear in the clauses and thus in partic in the gcodes
   * table! make sure that it is so with a little debugging test.
   */
  if ( gcmd_line.T && gcmd_line.debug ) {
    printf("\n\n-------------------------checks, entry state");
    print_state( *dest );
  }
  for ( i = 0; i < num_cp; i++ ) {
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\nchecking positive: "); 
      print_ft_name( check_pos[i] );
      printf("(%d)", endtime);
    }
    ff = check_pos[i];
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    /* DEBUGGING, REMOVE LATER
     */
    if ( gcodes[endtime][ff] == -1 ) {
      printf("\n\npos to-be-checked var not encoded?\n\n");
      exit(1);
    }
    /* NEW: we don't actually insert unit clauses, but push the respective
     * decisions onto the stack used in DP. that is, we initialise these 
     * values here!!
     */
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * (-1) * gcodes[endtime][ff];
    if ( gcmd_line.T && gcmd_line.debug  >= 4 ) {
      print_encoded_clauses();
    }
    /* here goes the SAT solver!
     *
     * curr CNF unsolvable --> fact proved true at endtime
     */
    /* do not count time for communicating clauses to Chaff.
     */
    times( &end );
    TIME( gsearch_time );
    times( &start );
    gsat_calls++;
    sat = dp_CNF();
    times( &end );
    TIME( gsat_time );
    times( &start );
    if ( sat ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\nsat! unknown "); print_ft_name( check_pos[i] );
      }
      dest->U[dest->num_U++] = check_pos[i];
    } else {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\nunsat! proved true");
      }
      dest->F[dest->num_F++] = check_pos[i];
    }
  }
  for ( i = 0; i < num_cn; i++ ) {
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\nchecking negative: "); 
      print_ft_name( check_neg[i] );
      printf("(%d)", endtime);
    }
    if ( gft_conn[check_neg[i]].negation != -1 ) {
      for ( j = 0; j < dest->num_F; j++ ) {
	if ( dest->F[j] == gft_conn[check_neg[i]].negation ) break;
      }
      if ( j < dest->num_F ) {
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nnegation proved true!");
	}
	continue;
      }
      for ( j = 0; j < dest->num_U; j++ ) {
	if ( dest->U[j] == gft_conn[check_neg[i]].negation ) break;
      }
      if ( j < dest->num_U ) {
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nnegation unknown!");
	}
	dest->U[dest->num_U++] = check_neg[i];
	continue;
      }
      printf("\nnegation of neg check there, but neither F nor U yet??\n\n");
      exit( 1 );
    }
    ff = check_neg[i];
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    if ( gcodes[endtime][ff] == -1 ) {
      printf("\n\nneg to-be-checked var not encoded?\n\n");
      exit(1);
    }
    /* NEW: we don't actually insert unit clauses, but push the respective
     * decisions onto the stack used in DP. that is, we initialise these 
     * values here!!
     */
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * gcodes[endtime][ff];
    if ( gcmd_line.T && gcmd_line.debug  >= 4 ) {
      printf("\n");
      print_encoded_clauses();
    }
    /* here goes the SAT solver!
     * curr CNF unsolvable --> fact proved false at endtime
     */
    times( &end );
    TIME( gsearch_time );
    times( &start );
    gsat_calls++;
    sat = dp_CNF();
    times( &end );
    TIME( gsat_time );
    times( &start );
    if ( sat ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\nsat! unknown");
      }
      dest->U[dest->num_U++] = check_neg[i];
    } else {
      /* false <-> neither U nor F
       */
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\nunsat! proved false");
      }
    }
  }

  /* bi-directional checks... only for observation actions!!!
   */
  for ( i = 0; i < num_cb; i++ ) {
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\nchecking both: "); 
      print_ft_name( check_both[i] );
      printf("(%d)", endtime);
    }
    if ( gft_conn[check_both[i]].negation != -1 ) {
      /* see if we had the negation already
       */
      for ( j = 0; j < i; j++ ) {
	if ( gft_conn[check_both[i]].negation == check_both[j] ) break;
      }
      if ( j < i ) {
	for ( j = 0; j < dest->num_F; j++ ) {
	  if ( dest->F[j] == gft_conn[check_both[i]].negation ) break;
	}
	if ( j < dest->num_F ) {
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\nnegation proved true!");
	  }
	  continue;
	}
	for ( j = 0; j < dest->num_U; j++ ) {
	  if ( dest->U[j] == gft_conn[check_both[i]].negation ) break;
	}
	if ( j < dest->num_U ) {
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\nnegation unknown!");
	  }
	  dest->U[dest->num_U++] = check_both[i];
	  continue;
	}
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nnegation proved false!");
	}
	dest->F[dest->num_F++] = check_both[i];
	continue;
      } /* had negation already */
    } /* negation exists */
    ff = check_both[i];
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    if ( gcodes[endtime][ff] == -1 ) {
      printf("\n\nneg to-be-checked var not encoded?\n\n");
      exit(1);
    }
    gnum_decision_stack = 0;
    gdecision_stack[gnum_decision_stack++] = m * (-1) * gcodes[endtime][ff];
    if ( gcmd_line.T && gcmd_line.debug  >= 4 ) {
      print_encoded_clauses();
    }
    times( &end );
    TIME( gsearch_time );
    times( &start );
    gsat_calls++;
    sat = dp_CNF();
    times( &end );
    TIME( gsat_time );
    times( &start );
    if ( !sat ) {
      if ( gcmd_line.T && gcmd_line.debug ) {
	printf("\nunsat! proved true");
      }
      dest->F[dest->num_F++] = check_both[i];
    } else {
      gnum_decision_stack = 0;
      gdecision_stack[gnum_decision_stack++] = m * gcodes[endtime][ff];
      if ( gcmd_line.T && gcmd_line.debug  >= 4 ) {
	printf("\n");
	print_encoded_clauses();
      }
      times( &end );
      TIME( gsearch_time );
      times( &start );
      gsat_calls++;
      sat = dp_CNF();
      times( &end );
      TIME( gsat_time );
      times( &start );
      if ( sat ) {
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nsat! unknown");
	}
	dest->U[dest->num_U++] = check_both[i];
      } else {
	/* false <-> noeither U nor F
	 */
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nunsat! proved false");
	}
      }
    } /* pos check succeeded or not */
  } /* all both-check facts */

  if ( gcmd_line.T && gcmd_line.debug ) {
    printf("\n\n-------------------------checks, exit state");
    print_state( *dest );
  }

}



void create_fixed_initial_clauses( void )

{

  
  int i, j, ff, m;
  
  if ( gcmd_line.T && gcmd_line.debug ) {
    printf("\n\n-----------------------creating fixed initial clauses");
  }
  gnum_clauses = 0;
  gnum_fixed_clauses = 0;
  /* insert the initially valid ORs given by user
   */
  for ( i = 0; i < gnum_initial_or; i++ ) {
    if ( gnum_clauses == gmax_clauses ) {
      printf("\n\ntoo many clauses? %d\n\n", gnum_clauses);
      exit( 1 );
    }
    for ( j = 0; j < ginitial_or_length[i]; j++ ) {
      ff = ginitial_or[i][j];
      if ( gft_conn[ff].CNF ) {
	m = 1;
      } else {
	m = -1;
	ff = gft_conn[ff].negation;
      }
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal = m * (1 + ff);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].time = 0;
      gclause_length[gnum_clauses]++;
    }
    gnum_clauses++;
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\ninitial OR clause"); print_clause( gnum_clauses-1 ); fflush(stdout);
    }
  } /* i over initial ors */
  gnum_fixed_clauses = gnum_clauses;

}



/* collection of hitting sets largely naive. reconsider if critical.
 */
void insert_posnoop_hitting_set_clauses( int ft, int time, 
					 BfsEdge *dest_edge,
					 Bool *Ft, Bool *Ut )

{

  lnum_hitting_set = 0;
  next_posnoop_hitting_set_step( 0,
				 ft, time,
				 dest_edge,
				 Ft, Ut );
 
}



void next_posnoop_hitting_set_step( int Uindex,
				    int ft, int time,
				    BfsEdge *dest_edge,
				    Bool *Ft, Bool *Ut )

{

  int i, j, k, ef = -1, ff, m;

  for ( i = Uindex; i < dest_edge->num_unknown_E; i++ ) {
    ef = dest_edge->unknown_E[i];
    for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
      if ( gef_conn[ef].D[j] == ft ) break;
    }
    if ( j < gef_conn[ef].num_D ) break;
  }

  if ( i == dest_edge->num_unknown_E ) {
    /* we got the hitting set. create the clause.
     *
     * apply several checks to avoid redundancies.
     */
    if ( gnum_clauses == gmax_clauses ) {
      printf("\n\ntoo many clauses? %d\n\n", gnum_clauses);
      exit( 1 );
    }
    if ( !Ft[ft] ) {
      ff = ft;
      if ( gft_conn[ff].CNF ) {
	m = 1;
      } else {
	m = -1;
	ff = gft_conn[ff].negation;
      }
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	m * (-1) * (1 + ff);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	time;
    }
    for ( j = 0; j < lnum_hitting_set; j++ ) {
      /* if c == ft then the clause is true
       */
      if ( lhitting_set[j] == ft ) {
	/* clause not recorded since gnum_clauses not incremented.
	 */
	gclause_length[gnum_clauses] = 0;
	return;
      }
      /* if c == not-ft then we can skip it. got that already.
       */
      if ( gft_conn[lhitting_set[j]].negation == ft ) {
	continue;
      }
      /* now, similar checks for the previously inserted hitting 
       * set members.
       *
       * NOTE: the member k might not have been inserted, in 1st check?
       *       but then the reason must be another identical member.
       */
      for ( k = 0; k < j; k++ ) {
	if ( lhitting_set[j] == lhitting_set[k] ) {
	  /* had this.
	   */
	  break;
	}
	if ( gft_conn[lhitting_set[j]].negation == lhitting_set[k] ) {
	  /* clause true.
	   */
	  gclause_length[gnum_clauses] = 0;
	  return;
	}
      }
      if ( k < j ) {
	/* found duplicate; skip this j
	 */
	continue;
      }
      ff = lhitting_set[j];
      if ( gft_conn[ff].CNF ) {
	m = 1;
      } else {
	m = -1;
	ff = gft_conn[ff].negation;
      }
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	m * (1 + ff);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	time;
    }
    ff = ft;
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
      m * (1 + ff);
    gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
      time + 1;
    gnum_clauses++;
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\npos noop clause"); print_clause( gnum_clauses-1 );
    }

    return;
  }

  /* DEBUGGING, REMOVE LATER
   */
  if ( ef == -1 ) {
    printf("\n\nHae?\n\n");
    exit( 1 );
  }

  if ( gcmd_line.T && gcmd_line.debug >= 2 ) {
    printf("\nhitting set for ef %d", ef);
  }

  lnum_hitting_set++;
  for ( j = 0; j < gef_conn[ef].num_C; j++ ) {
    /* remember the hitting set fact
     */
    lhitting_set[lnum_hitting_set-1] = gef_conn[ef].C[j];
    /* now, recursively, selct the possible completions of the set, and create the clauses.
     */
    next_posnoop_hitting_set_step( i + 1,
				   ft, time,
				   dest_edge,
				   Ft, Ut );
  }
  lnum_hitting_set--;

}



void insert_negnoop_hitting_set_clauses( int ft, int time, 
					 BfsEdge *dest_edge,
					 Bool *Ft, Bool *Ut )

{

  lnum_hitting_set = 0;
  next_negnoop_hitting_set_step( 0,
				 ft, time,
				 dest_edge,
				 Ft, Ut );
 
}



void next_negnoop_hitting_set_step( int Uindex,
				    int ft, int time,
				    BfsEdge *dest_edge,
				    Bool *Ft, Bool *Ut )

{

  int i, j, k, ef = -1, m, ff;

  for ( i = Uindex; i < dest_edge->num_unknown_E; i++ ) {
    ef = dest_edge->unknown_E[i];
    for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
      if ( gef_conn[ef].A[j] == ft ) break;
    }
    if ( j < gef_conn[ef].num_A ) break;
  }

  if ( i == dest_edge->num_unknown_E ) {
    /* we got the hitting set. create the clause.
     *
     * apply several checks to avoid redundancies.
     */
    if ( gnum_clauses == gmax_clauses ) {
      printf("\n\ntoo many clauses? %d\n\n", gnum_clauses);
      exit( 1 );
    }
    if ( Ut[ft] ) {
      ff = ft;
      if ( gft_conn[ff].CNF ) {
	m = 1;
      } else {
	m = -1;
	ff = gft_conn[ff].negation;
      }
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	m * (1 + ff);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	time;
    }
    for ( j = 0; j < lnum_hitting_set; j++ ) {
      /* if c == ft then skip it. got that already.
       */
      if ( lhitting_set[j] == ft ) {
	continue;
      }
      /* if c == not-ft then the clause is true.
       */
      if ( gft_conn[lhitting_set[j]].negation == ft ) {
	gclause_length[gnum_clauses] = 0;
	return;
      }
      /* now, similar checks for the previously inserted hitting 
       * set members.
       *
       * NOTE: the member k might not have been inserted, in 1st check?
       *       but then the reason must be another identical member.
       */
      for ( k = 0; k < j; k++ ) {
	if ( lhitting_set[j] == lhitting_set[k] ) {
	  /* had this.
	   */
	  break;
	}
	if ( gft_conn[lhitting_set[j]].negation == lhitting_set[k] ) {
	  /* clause true.
	   */
	  gclause_length[gnum_clauses] = 0;
	  return;
	}
      }
      if ( k < j ) {
	/* found duplicate; skip this j
	 */
	continue;
      }
      ff = lhitting_set[j];
      if ( gft_conn[ff].CNF ) {
	m = 1;
      } else {
	m = -1;
	ff = gft_conn[ff].negation;
      }
      gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	m * (1 + ff);
      gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	time;
    }
    ff = ft;
    if ( gft_conn[ff].CNF ) {
      m = 1;
    } else {
      m = -1;
      ff = gft_conn[ff].negation;
    }
    gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
      m * (-1) * (1 + ff);
    gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
      time + 1;
    gnum_clauses++;
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\nneg noop clause"); print_clause( gnum_clauses-1 );
    }

    return;
  }

  /* DEBUGGING, REMOVE LATER
   */
  if ( ef == -1 ) {
    printf("\n\nHae?\n\n");
    exit( 1 );
  }

  lnum_hitting_set++;
  for ( j = 0; j < gef_conn[ef].num_C; j++ ) {
    /* remember the hitting set fact
     */
    lhitting_set[lnum_hitting_set-1] = gef_conn[ef].C[j];
    /* now, recursively, selct the possible completions of the set, and create the clauses.
     */
    next_negnoop_hitting_set_step( i + 1,
				   ft, time,
				   dest_edge,
				   Ft, Ut );
  }
  lnum_hitting_set--;

}



int extend_dynamic_clauses_base( State *dest, 
				 BfsNode *dest_node, 
				 Bool *Npp,
				 Bool firstpart )

{
  
  static Bool fc = TRUE;
  static BfsNode_pointer *path;
  static Bool *Ft, *Ut, *Ftpp, *Utpp;
  
  int i, j, k, ef, num_path, t, time, ft, m, ff;
  BfsNode *ibfs;

  if ( fc ) {
    path = ( BfsNode_pointer * ) calloc( MAX_PLAN_LENGTH+1, sizeof( BfsNode_pointer ) );
   
    Ft = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    Ut = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    Ftpp = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    Utpp = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      Ft[i] = FALSE;
      Ut[i] = FALSE;
      Ftpp[i] = FALSE;
      Utpp[i] = FALSE;
    }
    
    fc = FALSE;
  }
  
  if ( gcmd_line.T && gcmd_line.debug >= 1 ) {
    printf("\n\n-----------------------dynamic exct path, final dest state not set");
    fflush(stdout);
  }
  num_path = MAX_PLAN_LENGTH;
  for ( ibfs = dest_node; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
    if ( num_path == 0 ) {
      printf("\n\nincrease MAX_PLAN_LENGTH! currently %d\n\n",
	     MAX_PLAN_LENGTH);
      exit( 1 );
    }
    path[num_path--] = ibfs;
    if ( gcmd_line.T && gcmd_line.debug >= 1 ) {
      printf("\n-----state %d", num_path+1); print_state( path[num_path+1]->S );
      fflush(stdout);
    }
    if ( gcmd_line.T && gcmd_line.debug >= 1 ) {
      printf("\n-----op "); print_op_name( path[num_path+1]->ingoing_edge->op );
      fflush(stdout);
    }
  }
  path[num_path--] = ibfs;

  
  /* the bfsnodes leading to dest are in 
   * path[num_path + 1] .. path[MAX_PLAN_LENGTH] (in that last one, state dest is not set)
   * unknown efs are stored along the way and we can
   * produce our CNF based on these.
   */
  if ( firstpart ) {
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\nfirst part dynamic CNF");
    }
    if ( gnum_clauses != gnum_fixed_clauses ) {
      printf("\n\nat start dyn. extend gnum_clauses != gnum_fixed_clauses??\n\n");
      fflush(stdout);
      exit( 1 );
    }
  }
  if ( gcmd_line.T && gcmd_line.debug ) {
    printf("\n\n-----------------------adding clauses from %d",
	   gnum_clauses);
  }
  if ( firstpart ) {
    /* insert the t -> t+1 implications for all t on the path.
     *
     * the times here are t-(num_path+1) -> t-(num_path+1)+1 for t in:
     */
    for ( t = num_path + 1; t < MAX_PLAN_LENGTH-1; t++ ) {
      time = t - (num_path + 1);
      
      /* first, make fast accessible the F and U info at t and t+1
       */
      for ( i = 0; i < path[t]->S.num_F; i++ ) {
	Ft[path[t]->S.F[i]] = TRUE;
      }
      for ( i = 0; i < path[t]->S.num_U; i++ ) {
	Ut[path[t]->S.U[i]] = TRUE;
      }
      for ( i = 0; i < path[t+1]->S.num_F; i++ ) {
	Ftpp[path[t+1]->S.F[i]] = TRUE;
      }
      for ( i = 0; i < path[t+1]->S.num_U; i++ ) {
	Utpp[path[t+1]->S.U[i]] = TRUE;
      }
      
      /* for known efs these clauses are fulfilled anyway so we can skip them.
       */
      for ( i = 0; i < path[t+1]->ingoing_edge->num_unknown_E; i++ ) {
	ef = path[t+1]->ingoing_edge->unknown_E[i];
	/* for all non-true adds ad:
	 * -u1(t) vee .. -un(t) vee ad(t+1)
	 * where u1 .. un are the unknown conds of the ef.
	 */
	for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
	  if ( !gft_conn[gef_conn[ef].A[j]].CNF ) continue;
	  if ( Ftpp[gef_conn[ef].A[j]] ) continue;
	  if ( gef_conn[ef].A_nondet[j] ) continue;
	  if ( gnum_clauses == gmax_clauses ) {
	    printf("\n\ntoo many clauses? %d\n\n", gnum_clauses);
	    exit( 1 );
	  }
	  for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	    if ( Ft[gef_conn[ef].C[k]] ) continue;
	    /* DEBUGGING; REMOVE LATER
	     */
	    if ( !Ut[gef_conn[ef].C[k]] ) {
	      printf("\nunknown ef cond neither F nor U in CNF gen\n\n");
	      exit(1);
	    }
	    ff = gef_conn[ef].C[k];
	    if ( gft_conn[ff].CNF ) {
	      m = 1;
	    } else {
	      m = -1;
	      ff = gft_conn[ff].negation;
	    }
	    gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	      m * (-1) * (1 + ff);
	    gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	      time;
	  }
	  ff = gef_conn[ef].A[j];
	  m = 1;
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (1 + ff);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    time + 1;
	  gnum_clauses++;
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\nunknown add eff clause"); print_clause( gnum_clauses-1 );
	  }
	}
	/* for all non-false dels de:
	 * -u1(t) vee .. -un(t) vee -de(t+1)
	 * where u1 .. un are the unknown conds of the ef.
	 */
	for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
	  if ( !gft_conn[gef_conn[ef].D[j]].CNF ) continue;
	  if ( !Ftpp[gef_conn[ef].D[j]] && !Utpp[gef_conn[ef].D[j]] ) continue;
	  if ( gef_conn[ef].D_nondet[j] ) continue;
	  if ( gnum_clauses == gmax_clauses ) {
	    printf("\n\ntoo many clauses? %d\n\n", gnum_clauses);
	    exit( 1 );
	  }
	  for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	    if ( Ft[gef_conn[ef].C[k]] ) continue;
	    /* DEBUGGING; REMOVE LATER
	     */
	    if ( !Ut[gef_conn[ef].C[k]] ) {
	      printf("\nunknown ef cond neither F nor U in CNF gen\n\n");
	      exit(1);
	    }
	    ff = gef_conn[ef].C[k];
	    if ( gft_conn[ff].CNF ) {
	      m = 1;
	    } else {
	      m = -1;
	      ff = gft_conn[ff].negation;
	    }
	    gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	      m * (-1) * (1 + ff);
	    gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	      time;
	  }
	  ff = gef_conn[ef].D[j];
	  m = 1;
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (-1) * (1 + ff);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    time + 1;
	  gnum_clauses++;
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\nunknown del eff clause"); print_clause( gnum_clauses-1 );
	  }
	}
      }
      
      /* now, the noop - implications; first, the positives then the negatives
       */
      for ( ft = 0; ft < gnum_ft_conn; ft++ ) {
	if ( !gft_conn[ft].CNF ) continue;
	if ( Ftpp[ft] ) continue;/* clause satisfied */
	if ( !Utpp[ft] ) {
	  /* proved false; so prev step, or conds, are proved false
	   * and clause thus satisfied.
	   *
	   * note: this check is necessary for validity of clause;
	   * in partic the uncond dels are false so no clause is
	   * generated in this case.
	   *
	   * note: the check also covers the case where ft became 
	   * false at tpp due to a branching (observe) action --
	   * in that case, we do not want to insert this noop clause at all.
	   */
	  continue;
	}
	if ( !Ft[ft] && !Ut[ft] ) continue;/* clause satisfied */
	if ( unconditional_nondet_del_on( ft, Ft, path[t+1]->ingoing_edge->op ) ) {
	  /* there's a nondet del effect with fulfilled cond. no NOOP.
	   */
	  continue;
	}
	/* we deal with Ft || Ut -> Utpp;
	 * clauses are all -ft(t) vee c1 .. cn vee ft(t+1)
	 * where c1 .. cn are conds of unknown efs deleting ft
	 */
	insert_posnoop_hitting_set_clauses( ft, time, 
					    path[t+1]->ingoing_edge,
					    Ft, Ut );
      }
      /* negative noops
       */
      for ( ft = 0; ft < gnum_ft_conn; ft++ ) {
	if ( !gft_conn[ft].CNF ) continue;
	if ( !Ftpp[ft] && !Utpp[ft] ) continue;/* clause satisfied */
	if ( Ftpp[ft] ) {
	  /* proved true; so prev step, or conds, are proved false
	   * and clause thus satisfied.
	   *
	   * also covers true-branch case, see above.
	   */
	  continue;
	}
	if ( Ft[ft] ) continue;/* clause satisfied */
	if ( unconditional_nondet_add_on( ft, Ft, path[t+1]->ingoing_edge->op ) ) {
	  /* there's a nondet add effect with fulfilled cond. no NOOP.
	   */
	  continue;
	}
	/* we deal with Ft || Ut -> Utpp;
	 * clauses are all -ft(t) vee c1 .. cn vee ft(t+1)
	 * where c1 .. cn are conds of unknown efs deleting ft
	 */
	insert_negnoop_hitting_set_clauses( ft, time, 
					    path[t+1]->ingoing_edge,
					    Ft, Ut );
      }
      
      /* finally, observation unary clauses: if I observed p at t+1,
       * then that must have been true at t
       */
      for ( i = 0; i < gop_conn[path[t+1]->ingoing_edge->op].num_O; i++ ) {
	ft = gop_conn[path[t+1]->ingoing_edge->op].O[i];
	if ( Utpp[ft] ) {
	  printf("\nobserved fact unknown at Tpp?\n");
	  exit( 1 );
	}
	if ( Ftpp[ft] ) {
	  /* we observed that this is true
	   */
	  ff = ft;
	  if ( gft_conn[ff].CNF ) {
	    m = 1;
	  } else {
	    m = -1;
	    ff = gft_conn[ff].negation;
	  }
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (1 + ff);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    time;
	  gnum_clauses++;
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\nobserved 'true' clause"); print_clause( gnum_clauses-1 );
	  }
	  /* don't need a clause for negation since only one of them
	   * is talked about in CNF anyway.
	   */
	} else {
	  /* we observed that this is false
	   */
	  ff = ft;
	  if ( gft_conn[ff].CNF ) {
	    m = 1;
	  } else {
	    m = -1;
	    ff = gft_conn[ff].negation;
	  }
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (-1) * (1 + ff);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    time;
	  gnum_clauses++;
	  if ( gcmd_line.T && gcmd_line.debug ) {
	    printf("\nobserved 'false' clause"); print_clause( gnum_clauses-1 );
	  }
	}
      } /* observed facts of edge-op */
      
      
      /* undo F U info.
       */
      for ( i = 0; i < path[t]->S.num_F; i++ ) {
	Ft[path[t]->S.F[i]] = FALSE;
      }
      for ( i = 0; i < path[t]->S.num_U; i++ ) {
	Ut[path[t]->S.U[i]] = FALSE;
      }
      for ( i = 0; i < path[t+1]->S.num_F; i++ ) {
	Ftpp[path[t+1]->S.F[i]] = FALSE;
      }
      for ( i = 0; i < path[t+1]->S.num_U; i++ ) {
	Utpp[path[t+1]->S.U[i]] = FALSE;
      }
    }/* end path clauses loop */
  }/* end 1st part */

  /* in any case, get the appropriate (end)time
   */
  t = MAX_PLAN_LENGTH-1;
  time = t - (num_path + 1);

  if ( !firstpart ) {
    if ( gcmd_line.T && gcmd_line.debug ) {
      printf("\nsecond part dynamic CNF state dest");
      print_state( *dest );
    }
    
    /* now we do pretty much the same, except that the proved false info is
     * now given by   Npp, and proved completely unknown by Upp
     */
    for ( i = 0; i < path[t]->S.num_F; i++ ) {
      Ft[path[t]->S.F[i]] = TRUE;
    }
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      Ut[path[t]->S.U[i]] = TRUE;
    }
    for ( i = 0; i < dest->num_F; i++ ) {
      Ftpp[dest->F[i]] = TRUE;
    }
    for ( i = 0; i < dest->num_U; i++ ) {
      Utpp[dest->U[i]] = TRUE;
    }
    /* the unknown effect - implications; these efs are stored
     * at the state which the resp op leads to.
     * (for known efs these clauses are fulfilled anyway so we can skip them.)
     */
    for ( i = 0; i < path[t+1]->ingoing_edge->num_unknown_E; i++ ) {
      ef = path[t+1]->ingoing_edge->unknown_E[i];
      /* for all non-true adds ad:
       * -u1(t) vee .. -un(t) vee ad(t+1)
       * where u1 .. un are the unknown conds of the ef.
       */
      for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
	if ( !gft_conn[gef_conn[ef].A[j]].CNF ) continue;
	if ( Ftpp[gef_conn[ef].A[j]] ) continue;
	if ( gef_conn[ef].A_nondet[j] ) continue;
	if ( gnum_clauses == gmax_clauses ) {
	  printf("\n\ntoo many clauses? %d\n\n", gnum_clauses);
	  exit( 1 );
	}
	for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	  if ( Ft[gef_conn[ef].C[k]] ) continue;
	  /* DEBUGGING; REMOVE LATER
	   */
	  if ( !Ut[gef_conn[ef].C[k]] ) {
	    printf("\nunknown ef cond neither F nor U in CNF gen\n\n");
	    exit(1);
	  }
	  ff = gef_conn[ef].C[k];
	  if ( gft_conn[ff].CNF ) {
	    m = 1;
	  } else {
	    m = -1;
	    ff = gft_conn[ff].negation;
	  }
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (-1) * (1 + ff);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    time;
	}
	ff = gef_conn[ef].A[j];
	m = 1;
	gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	  m * (1 + ff);
	gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	  time + 1;
	gnum_clauses++;
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nunknown add eff clause, final"); print_clause( gnum_clauses-1 );
	}
      }
      /* for all non-false dels de:
       * -u1(t) vee .. -un(t) vee -de(t+1)
       * where u1 .. un are the unknown conds of the ef.
       */
      for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
	if ( !gft_conn[gef_conn[ef].D[j]].CNF ) continue;
	if ( Npp[gef_conn[ef].D[j]] ) continue;
	if ( gef_conn[ef].D_nondet[j] ) continue;
	if ( gnum_clauses == gmax_clauses ) {
	  printf("\n\ntoo many clauses? %d\n\n", gnum_clauses);
	  exit( 1 );
	}
	for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	  if ( Ft[gef_conn[ef].C[k]] ) continue;
	  /* DEBUGGING; REMOVE LATER
	   */
	  if ( !Ut[gef_conn[ef].C[k]] ) {
	    printf("\nunknown ef cond neither F nor U in CNF gen\n\n");
	    exit(1);
	  }
	  ff = gef_conn[ef].C[k];
	  if ( gft_conn[ff].CNF ) {
	    m = 1;
	  } else {
	    m = -1;
	    ff = gft_conn[ff].negation;
	  }
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	    m * (-1) * (1 + ff);
	  gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	    time;
	}
	ff = gef_conn[ef].D[j];
	m = 1;
	gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	  m * (-1) * (1 + ff);
	gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	  time + 1;
	gnum_clauses++;
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nunknown del eff clause, final"); print_clause( gnum_clauses-1 );
	}
      }
    }
    
    /* now, the noop - implications; first, the positives then the negatives
     */
    for ( ft = 0; ft < gnum_ft_conn; ft++ ) {
      if ( !gft_conn[ft].CNF ) continue;
      if ( Ftpp[ft] ) continue;/* clause satisfied */
      if ( Npp[ft] ) {
	/* note: this check is necessary for validity of code below;
	 * *un*conditional, or satisfied, dels are not taken into account there.
	 */
	continue;
      }
      if ( !Ft[ft] && !Ut[ft] ) continue;/* clause satisfied */
      if ( unconditional_nondet_del_on( ft, Ft, path[t+1]->ingoing_edge->op ) ) {
	/* there's a nondet del effect with fulfilled cond. no NOOP.
	 */
	continue;
      }
      /* we deal with Ft || Ut -> Utpp;
       * clauses are all -ft(t) vee c1 .. cn vee ft(t+1)
       * where c1 .. cn are conds of unknown efs deleting ft
       */
      insert_posnoop_hitting_set_clauses( ft, time, 
					  path[t+1]->ingoing_edge,
					  Ft, Ut );
    }
    /* negative noops
     */
    for ( ft = 0; ft < gnum_ft_conn; ft++ ) {
      if ( !gft_conn[ft].CNF ) continue;
      if ( Npp[ft] ) continue;/* clause satisfied */
      if ( Ftpp[ft] ) {
	/* proved true; so prev step, or conds, are proved false
	 * and clause thus satisfied.
	 */
	continue;
      }
      if ( Ft[ft] ) continue;/* clause satisfied */
      if ( unconditional_nondet_add_on( ft, Ft, path[t+1]->ingoing_edge->op ) ) {
	/* there's a nondet del effect with fulfilled cond. no NOOP.
	 */
	continue;
      }
      /* we deal with Ft || Ut -> Utpp;
       * clauses are all -ft(t) vee c1 .. cn vee ft(t+1)
       * where c1 .. cn are conds of unknown efs deleting ft
       */
      insert_negnoop_hitting_set_clauses( ft, time, 
					  path[t+1]->ingoing_edge,
					  Ft, Ut );
    }      
    /* finally, observation unary clauses: if I observed p at t+1,
     * then that must have been true at t
     */
    for ( i = 0; i < gop_conn[path[t+1]->ingoing_edge->op].num_O; i++ ) {
      ft = gop_conn[path[t+1]->ingoing_edge->op].O[i];
      if ( Utpp[ft] ) {
	printf("\ndest U should be empty here?\n");
	exit( 1 );
      }
      if ( Ftpp[ft] ) {
	/* we observed that this is true
	 */
	ff = ft;
	if ( gft_conn[ff].CNF ) {
	  m = 1;
	} else {
	  m = -1;
	  ff = gft_conn[ff].negation;
	}
	gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	  m * (1 + ff);
	gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	  time;
	gnum_clauses++;
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nobserved 'true' clause"); print_clause( gnum_clauses-1 );
	}
	/* don't need a clause for negation since only one of them
	 * is talked about in CNF anyway.
	 */
      } else {
	/* we observed that this is false
	 */
	ff = ft;
	if ( gft_conn[ff].CNF ) {
	  m = 1;
	} else {
	  m = -1;
	  ff = gft_conn[ff].negation;
	}
	gclauses[gnum_clauses][gclause_length[gnum_clauses]].literal =
	  m * (-1) * (1 + ff);
	gclauses[gnum_clauses][gclause_length[gnum_clauses]++].time =
	  time;
	gnum_clauses++;
	if ( gcmd_line.T && gcmd_line.debug ) {
	  printf("\nobserved 'false' clause"); print_clause( gnum_clauses-1 );
	}
      }
    } /* observed facts of edge-op */
    for ( i = 0; i < path[t]->S.num_F; i++ ) {
      Ft[path[t]->S.F[i]] = FALSE;
    }
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      Ut[path[t]->S.U[i]] = FALSE;
    }
    for ( i = 0; i < dest->num_F; i++ ) {
      Ftpp[dest->F[i]] = FALSE;
    }
    for ( i = 0; i < dest->num_U; i++ ) {
      Utpp[dest->U[i]] = FALSE;
    }
  }
  
  /* debugging: see what that looks like!
   */
  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
    if ( gcmd_line.T && gcmd_line.debug >= 8 ) {
      printf("\n\n----------------------------- to execution path\n");
      if ( gcmd_line.T && gcmd_line.debug >= 9 ) {
	printf("\n---------state:");
	print_state( path[num_path+1]->S );
      }
      for ( t = num_path + 2; t <= MAX_PLAN_LENGTH; t++ ) {
	printf("\n-----------op ");
	print_op_name( path[t]->ingoing_edge->op );
	if ( gcmd_line.T && gcmd_line.debug >= 9 ) {
	  printf("\n----------state:");
	  print_state( path[t]->S );
	}
      }
    }
    printf("\n\n----------------------------- I get the CNF\n");
    print_clauses();
  }
  
  if ( time == -1 ) {
    printf("\n\ntime == -1 in dynamic extend??\n\n");
    exit( 1 );
  }
  
  return time + 1;

}



Bool unconditional_nondet_del_on( int ft, Bool *Ft, int op )

{

  int i, j, ef;
  
  if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
    printf("\nchecking if ");
    print_op_name( op );
    printf(" uncond nondet del ");
    print_ft_name( ft );
  }

  for ( i = 0; i < gop_conn[op].num_E; i++ ) {
    ef = gop_conn[op].E[i];
    for ( j = 0; j < gef_conn[ef].num_C; j++ ) {
      if ( !Ft[gef_conn[ef].C[j]] ) break;
    }
    if ( j < gef_conn[ef].num_C ) continue;

    for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
      if ( gef_conn[ef].D[j] == ft ) {
	if ( gef_conn[ef].D_nondet[j] ) {
	  if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
	    printf(" -- yes it does!");
	  }
	  return TRUE;
	} else {
	  printf("\n\nT uncond det del eff in noop check\n\n");
	  exit( 1 );
	}
      }
    }
  }

  if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
    printf(" -- no it doesn't!");
  }
  return FALSE;

}



Bool unconditional_nondet_add_on( int ft, Bool *Ft, int op )

{

  int i, j, ef;
  
  if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
    printf("\nchecking if ");
    print_op_name( op );
    printf(" uncond nondet add ");
    print_ft_name( ft );
  }

  for ( i = 0; i < gop_conn[op].num_E; i++ ) {
    ef = gop_conn[op].E[i];
    for ( j = 0; j < gef_conn[ef].num_C; j++ ) {
      if ( !Ft[gef_conn[ef].C[j]] ) break;
    }
    if ( j < gef_conn[ef].num_C ) continue;

    for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
      if ( gef_conn[ef].A[j] == ft ) {
	if ( gef_conn[ef].A_nondet[j] ) {
	  if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
	    printf(" -- yes it does!");
	  }
	  return TRUE;
	} else {
	  printf("\n\nT uncond det add eff in noop check\n\n");
	  exit( 1 );
	}
      }
    }
  }

  if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
    printf(" -- no it doesn't!");
  }
  return FALSE;

}



void print_clauses( void )

{

  int i, j;
  
  for ( i = 0; i < gnum_clauses; i++ ) {
    printf("\n%d lits: ", gclause_length[i]);
    for ( j = 0; j < gclause_length[i]; j++ ) {
      if ( gclauses[i][j].time >= 0 ) {
	if ( gclauses[i][j].literal < 0 ) {
	  printf("-");
	  print_ft_name( (-1) * gclauses[i][j].literal - 1 );
	} else {
	  print_ft_name( gclauses[i][j].literal - 1 );
	}
	printf("(%d) ", gclauses[i][j].time);
      } else {
	/* this is a code!
	 */
	if ( gclauses[i][j].literal < 0 ) {
	  printf("-");
	  print_ft_name( gcf[(-1) * gclauses[i][j].literal] );
	  printf("(%d) ", gct[(-1) * gclauses[i][j].literal] );
	} else {
	  print_ft_name( gcf[gclauses[i][j].literal] );
	  printf("(%d) ", gct[gclauses[i][j].literal] );
	}
      }
    }
  }

}



void print_clause( int i )

{

  int j;
  
  printf("\n%d lits: ", gclause_length[i]);
  for ( j = 0; j < gclause_length[i]; j++ ) {
    if ( gclauses[i][j].time >= 0 ) {
      if ( gclauses[i][j].literal < 0 ) {
	printf("-");
	print_ft_name( (-1) * gclauses[i][j].literal - 1 );
      } else {
	print_ft_name( gclauses[i][j].literal - 1 );
      }
      printf("(%d) ", gclauses[i][j].time);
    } else {
      /* this is a code!
       */
      if ( gclauses[i][j].literal < 0 ) {
	printf("-");
	print_ft_name( gcf[(-1) * gclauses[i][j].literal] );
	printf("(%d) ", gct[(-1) * gclauses[i][j].literal] );
      } else {
	print_ft_name( gcf[gclauses[i][j].literal] );
	printf("(%d) ", gct[gclauses[i][j].literal] );
      }
    }
  }

}



void create_fixed_initial_clauses_encoding( void )

{

  int i, j, ft, time;
  Bool neg;
  
  for ( i = 0; i < gnum_fixed_clauses; i++ ) {
    for ( j = 0; j < gclause_length[i]; j++ ) {
      time = gclauses[i][j].time;
      if ( gclauses[i][j].literal < 0 ) {
	neg = TRUE;
	ft = ((-1) * gclauses[i][j].literal) - 1;
      } else {
	neg = FALSE;
	ft = gclauses[i][j].literal - 1;
      }
      if ( gcodes[time][ft] == -1 ) {
	gnum_fixed_c++;/* must be non-zero */
	gcodes[time][ft] = gnum_fixed_c;
	if ( gnum_fixed_c == (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1 ) {
	  printf("\n\ntoo many fixed codes? %d\n\n", gnum_fixed_c);
	  exit( 1 );
	}
	gcf[gnum_fixed_c] = ft;
	gct[gnum_fixed_c] = time;
      }
      gclauses[i][j].literal = gcodes[time][ft];
      gclauses[i][j].time = -1;/* for knowing (in print) that this is encoded */
      if ( neg ) {
	gclauses[i][j].literal *= (-1);
      }
      /* insert fixed mem list element
       */
      insert_fixed_membership( gcodes[time][ft], neg, i );
    }
  }
  
  if ( gcmd_line.T && gcmd_line.debug  >= 4 ) {
    printf("\n\n--------------------------------fixed CNF encoding");
    printf("\n-----codes up to %d:", gnum_fixed_c);
    for ( i = 1; i <= gnum_fixed_c; i++ ) {
      printf("\n"); print_ft_name( gcf[i] ); 
      printf("(%d) ---> %d", gct[i], gcodes[gct[i]][gcf[i]]);
    }
    printf("\n-----clauses:\n");
    print_encoded_clauses();
  }

}



void extend_dynamic_clauses_base_encoding( int startclauses, int startcodes )

{

  int i, j, ft, time;
  Bool neg;
  
  gnum_c = startcodes;
  for ( i = startclauses; i < gnum_clauses; i++ ) {
    for ( j = 0; j < gclause_length[i]; j++ ) {
      time = gclauses[i][j].time;
      if ( gclauses[i][j].literal < 0 ) {
	neg = TRUE;
	ft = ((-1) * gclauses[i][j].literal) - 1;
      } else {
	neg = FALSE;
	ft = gclauses[i][j].literal - 1;
      }
      if ( time < 0 || time > MAX_PLAN_LENGTH ||
	   ft < 0 || ft >= gnum_ft_conn ) {
	printf("\n\ntime %d, ft %d (of %d) in dyn clause encod??",
	       time, ft, gnum_ft_conn);fflush(stdout);
	exit( 1 );
      }
      if ( gcodes[time][ft] == -1 ) {
	gnum_c++;/* must be non-zero */
	gcodes[time][ft] = gnum_c;
	if ( gnum_c == (1 + MAX_PLAN_LENGTH) * gmax_CNFU + 1 ) {
	  printf("\n\ntoo many codes? %d\n\n", gnum_c);
	  exit( 1 );
	}
	gcf[gnum_c] = ft;
	gct[gnum_c] = time; 
      }
      gclauses[i][j].literal = gcodes[time][ft];
      gclauses[i][j].time = -1;/* for knowing (in print) that this is encoded */
      if ( neg ) {
	gclauses[i][j].literal *= (-1);
      }
      /* insert dynamic mem list element
       */
      insert_dynamic_membership( gcodes[time][ft], neg, i );
    }
  }
  
  if ( gcmd_line.T && gcmd_line.debug  >= 4 ) {
    printf("\n\n--------------------------------CNF encoding");
    printf("\n-----codes up to: %d", gnum_c);
    for ( i = 1; i <= gnum_c; i++ ) {
      printf("\n"); print_ft_name( gcf[i] ); 
      printf("(%d) ---> %d", gct[i], gcodes[gct[i]][gcf[i]]);
    }
    printf("\n-----clauses:\n");
    print_encoded_clauses();
  }

}



void insert_fixed_membership( int v, Bool neg, int c )

{

  if ( gpos_c_in_clause_end[v] !=
       gpos_c_in_clause_fixed[v] ) {
    printf("\nend != fixed mem list el for %d??\n\n", v);
    exit( 1 );
  }

  if ( gpos_c_in_clause_start[v] == NULL ) {
    if ( gneg_c_in_clause_start[v] != NULL ) {
      printf("\n%d pos start NULL, neg not??\n\n", v);
      exit( 1 );
    }
    gpos_c_in_clause_start[v] = new_MemberList();
    gpos_c_in_clause_start[v]->clause = -1;
    gpos_c_in_clause_end[v] = new_MemberList();
    gpos_c_in_clause_end[v]->clause = -1;
    gpos_c_in_clause_start[v]->next = gpos_c_in_clause_end[v];
    gpos_c_in_clause_fixed[v] = gpos_c_in_clause_end[v];

    gneg_c_in_clause_start[v] = new_MemberList();
    gneg_c_in_clause_start[v]->clause = -1;
    gneg_c_in_clause_end[v] = new_MemberList();
    gneg_c_in_clause_end[v]->clause = -1;
    gneg_c_in_clause_start[v]->next = gneg_c_in_clause_end[v];
    gneg_c_in_clause_fixed[v] = gneg_c_in_clause_end[v];
  }

  if ( !neg ) {
    if ( gpos_c_in_clause_end[v]->clause == -1 ) {
      /* we're at the end of the allocated list!
       */
      gpos_c_in_clause_end[v]->clause = c;
      gpos_c_in_clause_end[v]->next = new_MemberList();
      gpos_c_in_clause_end[v]->next->clause = -1;
      gpos_c_in_clause_end[v] = gpos_c_in_clause_end[v]->next;
      gpos_c_in_clause_fixed[v] = gpos_c_in_clause_end[v];
    } else {
      /* we're still in the middle of the list.
       */
      gpos_c_in_clause_end[v]->clause = c;
      gpos_c_in_clause_end[v] = gpos_c_in_clause_end[v]->next;
      gpos_c_in_clause_fixed[v] = gpos_c_in_clause_end[v];
    } /* case distinction for end pointer of v list */
    return;
  } /* !neg? */

  if ( gneg_c_in_clause_end[v]->clause == -1 ) {
    /* we're at the end of the allocated list!
     */
    gneg_c_in_clause_end[v]->clause = c;
    gneg_c_in_clause_end[v]->next = new_MemberList();
    gneg_c_in_clause_end[v]->next->clause = -1;
    gneg_c_in_clause_end[v] = gneg_c_in_clause_end[v]->next;
    gneg_c_in_clause_fixed[v] = gneg_c_in_clause_end[v];
  } else {
    /* we're still in the middle of the list.
     */
    gneg_c_in_clause_end[v]->clause = c;
    gneg_c_in_clause_end[v] = gneg_c_in_clause_end[v]->next;
    gneg_c_in_clause_fixed[v] = gneg_c_in_clause_end[v];
  } /* case distinction for end pointer of v list */

}



void insert_dynamic_membership( int v, Bool neg, int c )

{

  if ( gpos_c_in_clause_start[v] == NULL ) {
    if ( gneg_c_in_clause_start[v] != NULL ) {
      printf("\n%d pos start NULL, neg not??\n\n", v);
      exit( 1 );
    }
    gpos_c_in_clause_start[v] = new_MemberList();
    gpos_c_in_clause_start[v]->clause = -1;
    gpos_c_in_clause_end[v] = new_MemberList();
    gpos_c_in_clause_end[v]->clause = -1;
    gpos_c_in_clause_start[v]->next = gpos_c_in_clause_end[v];
    gpos_c_in_clause_fixed[v] = gpos_c_in_clause_end[v];

    gneg_c_in_clause_start[v] = new_MemberList();
    gneg_c_in_clause_start[v]->clause = -1;
    gneg_c_in_clause_end[v] = new_MemberList();
    gneg_c_in_clause_end[v]->clause = -1;
    gneg_c_in_clause_start[v]->next = gneg_c_in_clause_end[v];
    gneg_c_in_clause_fixed[v] = gneg_c_in_clause_end[v];
  }

  if ( !neg ) {
    if ( gpos_c_in_clause_end[v]->clause == -1 ) {
      /* we're at the end of the allocated list!
       */
      gpos_c_in_clause_end[v]->clause = c;
      gpos_c_in_clause_end[v]->next = new_MemberList();
      gpos_c_in_clause_end[v]->next->clause = -1;
      gpos_c_in_clause_end[v] = gpos_c_in_clause_end[v]->next;
    } else {
      /* we're still in the middle of the list.
       */
      gpos_c_in_clause_end[v]->clause = c;
      gpos_c_in_clause_end[v] = gpos_c_in_clause_end[v]->next;
    } /* case distinction for end pointer of v list */
    return;
  } /* !neg? */

  if ( gneg_c_in_clause_end[v]->clause == -1 ) {
    /* we're at the end of the allocated list!
     */
    gneg_c_in_clause_end[v]->clause = c;
    gneg_c_in_clause_end[v]->next = new_MemberList();
    gneg_c_in_clause_end[v]->next->clause = -1;
    gneg_c_in_clause_end[v] = gneg_c_in_clause_end[v]->next;
  } else {
    /* we're still in the middle of the list.
     */
    gneg_c_in_clause_end[v]->clause = c;
    gneg_c_in_clause_end[v] = gneg_c_in_clause_end[v]->next;
  } /* case distinction for end pointer of v list */

}



void print_encoded_clauses( void )

{

    int i, j;

    printf("\nc encoded conformant search state transition base clauses");
    printf("\np cnf %d %d\n", gnum_c, gnum_clauses);
    for ( i = 0; i < gnum_clauses; i++ ) {
	printf("%d: ", i);
	for ( j = 0; j < gclause_length[i]; j++ ) {
	    printf("%d", gclauses[i][j].literal);
	    if ( j < gclause_length[i] - 1 ) {
		printf(" ");
	    }
	}
	printf("\n");
    }

}































/***********************************************************
 * WE ALL LIVE IN A YELLOW SUBROUTINE:                     * 
 *   HERE COMES A SAT SOLVER                               *
 ***********************************************************/






























/* top level control fn for (extremely) naive DP implementation...
 */

Bool dp_CNF( void )

{

  int i, j, v, sign, sum, c;
  Bool sat;
  MemberList *i_ml, *j_ml;

  gdp_calls++;

  if ( gnum_decision_stack == 0 ) {
    printf("\nno decision stack at entering DP??\n\n"); 
    exit( 1 );
  }

  if ( gcmd_line.debug ) {
    for ( i = 0; i < gmax_literals + 1; i++ ) {
      for ( j = gnum_clauses - 1; j >= 0; j-- ) {
	if ( gclause_length[j] == i ) gsum_k_clauses[i]++;
	if ( gclause_length[j] > gmax_literals ) {
	  printf("???\n\n");
	}
      }
    }
    gsum_clauses += gnum_clauses;
  }

  for ( v = gnum_c; v > 0; v-- ) {
    if ( lassigned[v] != -1 ) {
      printf("\nDP entry assigned %d = %d??\n\n", v, lassigned[v]);
      exit( 1 );
    }
  }

  if ( gcmd_line.debug ) {
    sum = -1;
    for ( i = gnum_clauses - 1; i >= 0; i-- ) {
      for ( j = 0; j < gclause_length[i]; j++ ) {
	v = (gclauses[i][j].literal > 0) ? 
	  gclauses[i][j].literal : (-1) * gclauses[i][j].literal;
	if ( sum == -1 || v > sum ) {
	  sum = v;
	}
      }
    }
    if ( sum != gnum_c ) {
      printf("\ngnum_clauses: %d, counted gnum_c: %d, old: %d", gnum_clauses, sum, gnum_c);
    }
  }

  if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
    printf("\nmembership lists:");
    for ( i = 1; i <= gnum_c; i++ ) {
      printf("\nvar %d pos:", i);
      for ( i_ml = gpos_c_in_clause_start[i]->next; 
	    i_ml != gpos_c_in_clause_end[i]; i_ml = i_ml->next ) {
	printf(" %d", i_ml->clause);
      }
      printf("\nvar %d neg:", i);
      for ( i_ml = gneg_c_in_clause_start[i]->next; 
	    i_ml != gneg_c_in_clause_end[i]; i_ml = i_ml->next ) {
	printf(" %d", i_ml->clause);
      }
    }
  }
  if ( gcmd_line.debug ) {
    /* Oh... fuck you all: double-2-2-check that memlists are correct!!!
     */
    for ( i = 1; i <= gnum_c; i++ ) {
      for ( i_ml = gpos_c_in_clause_start[i]->next; 
	    i_ml != gpos_c_in_clause_end[i]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	for ( j = 0; j < gclause_length[c]; j++ ) {
	  if ( gclauses[c][j].literal == i ) break;
	}
	if ( j == gclause_length[c] ) {
	  printf("\nerror 1\n\n");
	  exit( 1 );
	}
	for ( j_ml = i_ml->next; 
	      j_ml != gpos_c_in_clause_end[i]; j_ml = j_ml->next ) {
	  if ( j_ml->clause == i_ml->clause ) {
	    printf("\nerror 1b\n\n");
	    exit( 1 );
	  }
	}
      }
      for ( i_ml = gneg_c_in_clause_start[i]->next; 
	    i_ml != gneg_c_in_clause_end[i]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	for ( j = 0; j < gclause_length[c]; j++ ) {
	  if ( gclauses[c][j].literal == (-1) * i ) break;
	}
	if ( j == gclause_length[c] ) {
	  printf("\nerror 2\n\n");
	  exit( 1 );
	}
	for ( j_ml = i_ml->next; 
	      j_ml != gneg_c_in_clause_end[i]; j_ml = j_ml->next ) {
	  if ( j_ml->clause == i_ml->clause ) {
	    printf("\nerror 2b\n\n");
	    exit( 1 );
	  }
	}
      }
    }
    for ( i = 0; i < gnum_clauses; i++ ) {
      for ( j = 0; j < gclause_length[i]; j++ ) {
	if ( gclauses[i][j].literal > 0 ) {
	  v = gclauses[i][j].literal;
	  if ( gpos_c_in_clause_start[v] == NULL ) {
	    printf("\nerror 3a\n\n");
	    exit( 1 );
	  }
	  for ( i_ml = gpos_c_in_clause_start[v]->next; 
		i_ml != gpos_c_in_clause_end[v]; i_ml = i_ml->next ) {
	    if ( i_ml->clause == i ) break;
	  }
	  if ( i_ml == gpos_c_in_clause_end[v] ) {
	    printf("\nerror 3\n\n");
	    exit( 1 );
	  }
	} else {
	  v = (-1) * gclauses[i][j].literal;
	  if ( gneg_c_in_clause_start[v] == NULL ) {
	    printf("\nerror 4a\n\n");
	    exit( 1 );
	  }
	  for ( i_ml = gneg_c_in_clause_start[v]->next; 
		i_ml != gneg_c_in_clause_end[v]; i_ml = i_ml->next ) {
	    if ( i_ml->clause == i ) break;
	  }
	  if ( i_ml == gneg_c_in_clause_end[v] ) {
	    printf("\nerror 4\n\n");
	    exit( 1 );
	  }
	}
      }
    }
  } /* end 2-2 mem list test */
    


  /* end debugging code
   */


  /* first, create assignments corresponding to decision stack;
   * check for contradictions.
   */
  i = 0;
  while ( i < gnum_decision_stack ) {
    if ( gdecision_stack[i] < 0 ) {
      v = (-1) * gdecision_stack[i];
      sign = 0;
    } else {
      v = gdecision_stack[i];
      sign = 1;
    }
    if ( lassigned[v] == sign ) {
      if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
	printf("\nskipping pre-insertion %d of var %d val %d, already set so",
	       i, v, sign);
      }
      for ( j = i; j < gnum_decision_stack-1; j++ ) {
	gdecision_stack[j] = gdecision_stack[j+1];
      }
      gnum_decision_stack--;
      continue;
    }
    if ( lassigned[v] != -1 ) {
      if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
	printf("\ncontradictory pre-insertion %d of var %d val %d!",
	       i, v, sign);
      }
      /* unset info
       */
      for ( j = 0; j < i; j++ ) {
	if ( gdecision_stack[j] < 0 ) {
	  v = (-1) * gdecision_stack[j];
	  sign = 0;
	} else {
	  v = gdecision_stack[j];
	  sign = 1;
	}
	lassigned[v] = -1;
      }
      return FALSE;
    }
    /* insert new assignment
     */
    if ( gcmd_line.T && gcmd_line.debug >= 5 ) {
      printf("\npre-inserting pos %d: var %d val %d",
	     i, v, sign);
    }
    lassigned[v] = sign;
    i++;
  }


  if ( gnum_decision_stack == 0 ) {
    printf("\nno decision stack after pre-inserting DP??\n\n"); 
    exit( 1 );
  }

  /* now do the corresponding unit props for all the pre-insertions
   * in sequence; is implicit because do_unit_props will move through
   * entire (growing) decision stack, starting from start index
   *
   * no contradiction --> do DP!
   */
  sat = FALSE;
  if ( do_unit_props( 0 ) ) {
    sat = dp();
  }

  /* undo assignments
   */
  for ( i = 0; i < gnum_decision_stack; i++ ) {
    if ( gdecision_stack[i] < 0 ) {
      v = (-1) * gdecision_stack[i];
    } else {
      v = gdecision_stack[i];
    }
    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
      printf("\ntop retracting dec stack %d, var %d val %d!",
	     i, v, gdecision_stack[i] < 0 ? 0 : 1);
    }
    lassigned[v] = -1;
  }
  /* not strictly necessary, as full (..) initialisation done above 
   * before DP calls... looks cleaner this way...
   */
  gnum_decision_stack = 0;

  return sat;

}



/* invariant: gnum_decision_stack is same on entry and exit; same holds for
 * lassigned array.
 */
Bool dp( void )

{

  int entry_num_decision_stack;
  int i, v, dec_v;

  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
    printf("\nentering naive dp");
  }

  /* store the entry nr. of decisions on the stack, for later
   * retraction of the new decisions.
   */
  entry_num_decision_stack = gnum_decision_stack;


  /* choose split vars from "backside of plan"
   */
  for ( dec_v = gnum_c; dec_v > 0; dec_v-- ) {
    if ( lassigned[dec_v] == -1 ) break;
  }
  if ( dec_v == 0 ) {
    /* all vars assigned and no contradiction --> sat!
     */
    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
      printf("\nall vars assigned, no contradiction -> sat!");
    }
    return TRUE;
  }

  /* split on v.
   *
   * first, positive.
   */
  if ( gnum_decision_stack == (1 + MAX_PLAN_LENGTH) * gmax_CNFU ) {
    printf("\ndec stack overflow??\n\n");
    exit( 1 );
  }
  gdecision_stack[gnum_decision_stack++] = dec_v;
  lassigned[dec_v] = 1;
  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
    printf("\ndecide %d true, doing unit props", dec_v);
  } 
  if ( do_unit_props( gnum_decision_stack-1 ) ) {
    if ( dp() ) {
      /* found solution.
       */
      return TRUE;
    }
  }
  for ( i = entry_num_decision_stack; i < gnum_decision_stack; i++ ) {
    if ( gdecision_stack[i] < 0 ) {
      v = (-1) * gdecision_stack[i];
    } else {
      v = gdecision_stack[i];
    }
    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
      printf("\ntrue tried retracting dec stack %d, var %d val %d!",
	     i, v, gdecision_stack[i] < 0 ? 0 : 1);
    }
    lassigned[v] = -1;
  }
  gnum_decision_stack = entry_num_decision_stack;


  /* now, negative.
   */
  gdecision_stack[gnum_decision_stack++] = (-1) * dec_v;
  lassigned[dec_v] = 0;
  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
    printf("\ndecide %d false, doing unit props", dec_v);
  } 
  if ( do_unit_props( gnum_decision_stack-1 ) ) {
    if ( dp() ) {
      /* found solution.
       */
      return TRUE;
    }
  }
  for ( i = entry_num_decision_stack; i < gnum_decision_stack; i++ ) {
    if ( gdecision_stack[i] < 0 ) {
      v = (-1) * gdecision_stack[i];
    } else {
      v = gdecision_stack[i];
    }
    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
      printf("\nfalse tried retracting dec stack %d, var %d val %d!",
	     i, v, gdecision_stack[i] < 0 ? 0 : 1);
    }
    lassigned[v] = -1;
  }
  gnum_decision_stack = entry_num_decision_stack;

  return FALSE;

}



Bool do_unit_props( int startidx )

{

  int idx, i, v, c, v_, numopenlits, lastopen;  
  MemberList *i_ml;

  gup_calls++;

  idx = startidx;
  while ( idx < gnum_decision_stack ) {
    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
      printf("\npropagating dec stack %d, %d!",
	     idx, gdecision_stack[idx]);
    }
    if ( gdecision_stack[idx] < 0 ) {
      v = (-1) * gdecision_stack[idx];
      for ( i_ml = gpos_c_in_clause_start[v]->next; 
	    i_ml != gpos_c_in_clause_end[v]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	numopenlits = 0;
	lastopen = 0;
	for ( i = 0; i < gclause_length[c]; i++ ) {
	  if ( numopenlits > 1 ) break;
	  if ( gclauses[c][i].literal > 0 ) {
	    v_ = gclauses[c][i].literal;
	    if ( lassigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = gclauses[c][i].literal;
	    }
	    if ( lassigned[v_] == 1 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  } else {
	    v_ = (-1) * gclauses[c][i].literal;
	    if ( lassigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = gclauses[c][i].literal;
	    }
	    if ( lassigned[v_] == 0 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  }
	}
	if ( i < gclause_length[c] ) {
	  /* clause sat or >1 open lit! ignore.
	   */
	  continue;
	}
	if ( numopenlits == 0 ) {
	  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	    printf("\nempty clause %d", c);
	  }
	  /* no fulfilled and no open lits --> contradiction!!
	   */
	  return FALSE;
	}
	if ( numopenlits == 1 ) {
	  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	    printf("\nunit clause %d", c);
	  }
	  if ( gnum_decision_stack == (1 + MAX_PLAN_LENGTH) * gmax_CNFU ) {
	    printf("\ndec stack overflow??\n\n");
	    exit( 1 );
	  }
	  /* this one's unit. constrain the variable.
	   */
	  /* DEBUGGING. REMOVE LATER.
	   */
	  if ( lastopen == 0 ) {
	    printf("\n\nlastopen 0?\n\n");
	    exit( 1 );
	  }
	  if ( lastopen > 0 ) {
	    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	      printf("\nUP constr %d true", lastopen);
	    }
	    lassigned[lastopen] = 1;
	  } else {
	    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	      printf("\nUP constr %d false", (-1) * lastopen);
	    }
	    lassigned[(-1) * lastopen] = 0;
	  }
	  gdecision_stack[gnum_decision_stack++] = lastopen;
	} /* numopenlits == 1 */
      } /* clauses in that negated v participates */
    } else { /* if decision here is positive */
      v = gdecision_stack[idx];
      for ( i_ml = gneg_c_in_clause_start[v]->next;
	    i_ml != gneg_c_in_clause_end[v]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	numopenlits = 0;
	lastopen = 0;
	for ( i = 0; i < gclause_length[c]; i++ ) {
	  if ( numopenlits > 1 ) break;
	  if ( gclauses[c][i].literal > 0 ) {
	    v_ = gclauses[c][i].literal;
	    if ( lassigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = gclauses[c][i].literal;
	    }
	    if ( lassigned[v_] == 1 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  } else {
	    v_ = (-1) * gclauses[c][i].literal;
	    if ( lassigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = gclauses[c][i].literal;
	    }
	    if ( lassigned[v_] == 0 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  }
	}
	if ( i < gclause_length[c] ) {
	  /* clause sat or >1 open lit! ignore.
	   */
	  continue;
	}
	if ( numopenlits == 0 ) {
	  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	    printf("\nempty clause %d", c);
	  }
	  /* no fulfilled and no open lits --> contradiction!!
	   */
	  return FALSE;
	}
	if ( numopenlits == 1 ) {
	  if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	    printf("\nunit clause %d", c);
	  }
	  if ( gnum_decision_stack == (1 + MAX_PLAN_LENGTH) * gmax_CNFU ) {
	    printf("\ndec stack overflow??\n\n");
	    exit( 1 );
	  }
	  /* this one's unit. constrain the variable.
	   */
	  /* DEBUGGING. REMOVE LATER.
	   */
	  if ( lastopen == 0 ) {
	    printf("\n\nlastopen 0?\n\n");
	    exit( 1 );
	  }
	  if ( lastopen > 0 ) {
	    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	      printf("\nUP constr %d true", lastopen);
	    }
	    lassigned[lastopen] = 1;
	  } else {
	    if ( gcmd_line.T && gcmd_line.debug >= 7 ) {
	      printf("\nUP constr %d false", (-1) * lastopen);
	    }
	    lassigned[(-1) * lastopen] = 0;
	  }
	  gdecision_stack[gnum_decision_stack++] = lastopen;
	} /* numopenlits == 1 */
      } /* clauses in that positive v participates */
    } /* neg or pos */
    idx++;
  } /* while not all decisions on stack propagated */

  return TRUE;

}


