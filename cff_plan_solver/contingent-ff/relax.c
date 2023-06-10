


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




/* JOERGPARTIALUNDO: This heuristic function may return infty on
   belief states that have >=1 successful path. If we allow unsolved
   leaves we don't want that. Simple solution would be to take the
   states F and U instead, take the union ie pretend that all unknown
   facts are true, then compute a std relaxed plan on that.  */






/*********************************************************************
 * File: relax.c
 * Description: this file handles the relaxed planning problem, i.e.,
 *              the code is responsible for the heuristic evaluation
 *              of states during search.
 *
 *              --- THE HEART PEACE OF THE FF PLANNER ! ---
 *
 *              - fast (time critical) computation of the relaxed fixpoint
 *              - extraction of as short as possible plans, without search
 *
 *
 *
 *  ----- UPDATED VERSION TO HANDLE CONTINGENT SEMANTICS -----
 *
 *
 *
 * Author: Joerg Hoffmann 2004
 *
 *********************************************************************/ 









#include "ff.h"

#include "output.h"
#include "memory.h"

#include "relax.h"
#include "search.h"
#include "state_transitions.h"



#include "relax_ff.h" /*We redirect the calls to the FF version */











/* local globals
 */











/* fixpoint
 */
int *lF;
int lnum_F;
int *lE;
int lnum_E;

int *lch_E;
int lnum_ch_E;

int *l1ch_F;
int l1num_ch_F;

int *l0P_E;
int lnum_0P_E;

/* for get applicable actions
 */
int *lch_O;
int lnum_ch_O;
int *l0P_O;
int lnum_0P_O;

/* conformant part: the ``temporal implication graph''
 */
UftNode **lU;
int *lnum_U;
/* the U graph is built for the whole path to the state;
 * this int here is the number of the U layer that corresponds
 * to the state itself, ie to the first rplangraph layer.
 */
int lpath_U_length;


/* 1P extraction
 */
int **lgoals_at;
int *lnum_goals_at;

int **logoals_at;
int *lnum_ogoals_at;

int *lch_F;
int lnum_ch_F;

int **lused_O_at;
int *lnum_used_O_at;

long lh;


int lMAX_PATHNODES;






/* CNF time of vars in S -- for ini_ors == 5
 */
int lStime;






/* clauses for reasoning about initial formula implications
 */

TimedLiteral **lr_clauses;
int *lr_clause_length;
int lr_num_clauses;

/* we use our own codes in the CNF, to avoid confusion with this other shit I programmed
 * 2 years ago
 */
int *lr_codes;/* maps a fact at time 0 to its r-internal code */
int lr_num_c;/* nr. of codes */
int *lr_cf;/* maps a r-internal code to the resp. fact */

 

/* for naive DP implementation
 */
int *lr_assigned;


/* stores the current DP decisions including unit propagations.
 */
int *lr_decision_stack;
int lr_num_decision_stack;


/* for each possible ft code, a pointer to connected dynamic list
 * of member elements, ie the clauses in which it participates,
 * positive and negative.
 */
MemberList_pointer *lr_pos_c_in_clause_start;
MemberList_pointer *lr_pos_c_in_clause_end;
MemberList_pointer *lr_neg_c_in_clause_start;
MemberList_pointer *lr_neg_c_in_clause_end;





/* the observations backpropagate (under h 0 and 1) restrictions
 * into the initial state. namely, these facts here. computed in
 * path_implications
 */
int *lobs_backpropagation;
int lnum_obs_backpropagation;








/*************************************
 * helper, for -1 == INFINITY method *
 *************************************/












Bool LESS( int a, int b )

{

  if ( a == INFINITY ) {
    return FALSE;
  }

  if ( b == INFINITY ) {
    return TRUE;
  }

  return ( a < b ? TRUE : FALSE );

}












/***********************************
 * FUNCTIONS ACCESSED FROM OUTSIDE *
 ***********************************/






Bool contains_goal( State *S )

{

  int i, j;

  for ( i = 0; i < ggoal_state.num_F; i++ ) {
    for ( j = 0; j < S->num_F; j++ ) {
      if ( S->F[j] == ggoal_state.F[i] ) break;
    }

    if ( j == S->num_F ) {
      return FALSE;
    }
  }

  return TRUE;

}



void initialize_relax( void )

{



    if ( gcmd_line.heuristic == 3){
	int i;
	/* get A stuff
	 */
	l0P_O = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
	lch_O = ( int * ) calloc( gnum_op_conn, sizeof( int ) );

	gA = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
	gnum_A = 0;

	lnum_0P_O = 0;
	for ( i = 0; i < gnum_op_conn; i++ ) {     
	    gop_conn[i].is_in_A = FALSE;
	    gop_conn[i].is_in_H = FALSE;
	    gop_conn[i].ch = FALSE;
	    gop_conn[i].level = INFINITY;
    
	    if ( gop_conn[i].num_P == 0 && 
		 (gop_conn[i].num_E > 0 || gop_conn[i].num_O > 0) ) {
		l0P_O[lnum_0P_O++] = i;
	    }
	}
	for ( i = 0; i < gnum_ft_conn; i++ ) {
	    gft_conn[i].level = INFINITY;
	    gft_conn[i].in_F = FALSE;
	    gft_conn[i].ch1 = FALSE;
	    gft_conn[i].observed_level = INFINITY;
	}

	ff_initialize_relax();
	return;
    }
  int i, j, maxcl, m, ff, min, min_i;
  Bool *had;

  /* need to do this here as we do not call real gH setup fn
   */
  gH = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
  gnum_H = 0;


  /* the temporal implication graph between
   * unknown facts.
   */
  lU = ( UftNode ** ) calloc( RELAXED_STEPS + MAX_PLAN_LENGTH, sizeof( UftNode * ) );
  lnum_U = ( int * ) calloc( RELAXED_STEPS + MAX_PLAN_LENGTH, sizeof( int ) );
  for ( i = 0; i < RELAXED_STEPS + MAX_PLAN_LENGTH; i++ ) {
    lU[i] = ( UftNode * ) calloc( gmax_U, sizeof( UftNode ) );
    lnum_U[i] = 0;
    for ( j = 0; j < gmax_U; j++ ) {
      lU[i][j].num_in = 0;
    }
  }
  lMAX_PATHNODES = (RELAXED_STEPS + MAX_PLAN_LENGTH) * gmax_U;

  /* rp stuff
   */
  lF = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
  lE = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
  lch_E = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
  l0P_E = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
  l1ch_F = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );

  lobs_backpropagation = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );

  /* get A stuff
   */
  l0P_O = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
  lch_O = ( int * ) calloc( gnum_op_conn, sizeof( int ) );

  gA = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
  gnum_A = 0;


  /* various initializations
   */
  lnum_0P_E = 0;
  for ( i = 0; i < gnum_ef_conn; i++ ) {      
    gef_conn[i].level = INFINITY;    
    gef_conn[i].in_E = FALSE;
    gef_conn[i].num_active_PCs = 0;
    gef_conn[i].ch = FALSE;
    
    if ( gef_conn[i].num_PC == 0 ) {
      l0P_E[lnum_0P_E++] = i;
    }
  }
  lnum_0P_O = 0;
  for ( i = 0; i < gnum_op_conn; i++ ) {      
    gop_conn[i].is_in_A = FALSE;
    gop_conn[i].is_in_H = FALSE;
    gop_conn[i].ch = FALSE;
    gop_conn[i].level = INFINITY;
    
    if ( gop_conn[i].num_P == 0 ) {
      l0P_O[lnum_0P_O++] = i;
    }
  }
  for ( i = 0; i < gnum_ft_conn; i++ ) {
    gft_conn[i].level = INFINITY;
    gft_conn[i].in_F = FALSE;
    gft_conn[i].ch1 = FALSE;
    gft_conn[i].observed_level = INFINITY;
  }

  

  /* Mar04:
   * CREATE SAT FORMULA TO BE USED FOR INITIAL IMPLIES DISJUNCTION CHECKING!!
   * (if needed)
   *
   * (for h == 0 we do this just to be able to use the same pieces of code...
   *  can't be critical for performance...)
   */

  if ( gcmd_line.heuristic == 0 ||
       gcmd_line.heuristic == 1 ) {
    /* do it Baby
     */

    /* get the memory
     */
    maxcl = gnum_initial_or + gnum_ft_conn;
    lr_clauses = ( TimedLiteral ** ) calloc( maxcl, sizeof( TimedLiteral * ) );
    lr_clause_length = ( int * ) calloc( maxcl, sizeof( int ) );
    for ( i = 0; i < maxcl; i++ ) {
      lr_clauses[i] = ( TimedLiteral * ) calloc( gmax_literals, sizeof( TimedLiteral ) );
      lr_clause_length[i] = 0;
    }
    lr_num_clauses = 0;
    lr_codes = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    lr_cf = ( int * ) calloc( gnum_ft_conn + 1, sizeof( int ) );
    lr_num_c = 0; 
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      /* used to be non-trivial; replaced it with this trivial stuff here
       * to implement codes for ALL facts at time 0 (may be needed for
       * restrictions backpropagation; uncritical anyway)
       */
      if ( gft_conn[i].CNF ) {
	lr_num_c++;
	lr_codes[i] = lr_num_c;
	lr_cf[lr_num_c] = i;
      }
    }
    lr_assigned = ( int * ) calloc( gnum_ft_conn + 1, sizeof( int ) );
    for ( i = 0; i < gnum_ft_conn + 1; i++ ) {
      lr_assigned[i] = -1;
    }

    lr_decision_stack = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );

    lr_pos_c_in_clause_start = ( MemberList_pointer * ) 
      calloc( gnum_ft_conn + 1, sizeof( MemberList_pointer ) );
    lr_pos_c_in_clause_end = ( MemberList_pointer * ) 
      calloc( gnum_ft_conn + 1, sizeof( MemberList_pointer ) );
    lr_neg_c_in_clause_start = ( MemberList_pointer * ) 
      calloc( gnum_ft_conn + 1, sizeof( MemberList_pointer ) );
    lr_neg_c_in_clause_end = ( MemberList_pointer * ) 
      calloc( gnum_ft_conn + 1, sizeof( MemberList_pointer ) );
    for ( i = 0; i <= gnum_ft_conn; i++ ) {
      lr_pos_c_in_clause_start[i] = NULL;
      lr_neg_c_in_clause_start[i] = NULL;
      lr_pos_c_in_clause_end[i] = NULL;
      lr_neg_c_in_clause_end[i] = NULL;
    }


    /* now insert the explicit literal / time encoding
     *
     * NOTE: sort the clauses by increasing length; this way when, later, we 
     *       proceed over the clauses from front to back to see if one of them is
     *       contained in a leaf disjunction, we'll try the shortest clauses
     *       first (see also below in rplan extraction)
     */
    had = ( Bool * ) calloc( gnum_initial_or, sizeof( Bool ) );
    for ( i = 0; i < gnum_initial_or; i++ ) {
      had[i] = FALSE;
    }
    if ( gcmd_line.R && gcmd_line.debug ) {
      for ( i = 0; i < gnum_initial_or; i++ ) {
	printf("\nr initial OR ");
	for ( j = 0; j < ginitial_or_length[i]; j++ ) {
	  print_ft_name( ginitial_or[i][j] ); printf(" ");
	}
	fflush(stdout);
      }
    }
    for ( i = 0; i < gnum_initial_or; i++ ) {
      min = -1; min_i = -1;
      for ( j = 0; j < gnum_initial_or; j++ ) {
	if ( had[j] ) continue;
	if ( min == -1 || ginitial_or_length[j] < min ) {
	  min = ginitial_or_length[j];
	  min_i = j;
	}
      }
      if ( min_i < 0 || min_i >= gnum_initial_or ) {
	printf("\nsomething wrong with the min_i!\n\n");
	exit( 1 );
      }
      had[min_i] = TRUE;
      if ( lr_num_clauses == maxcl ) {
	printf("\n\ntoo many r clauses? %d\n\n", lr_num_clauses);
	exit( 1 );
      }
      for ( j = 0; j < ginitial_or_length[min_i]; j++ ) {
	ff = ginitial_or[min_i][j];
	if ( gft_conn[ff].CNF ) {
	  m = 1;
	} else {
	  m = -1;
	  ff = gft_conn[ff].negation;
	}
	if ( lr_codes[ff] == -1 ) {
	  /* this does not happen any longer, with the above
	   */
	  lr_num_c++;/* must be non-zero */
	  lr_codes[ff] = lr_num_c;
	  if ( lr_num_c == gnum_ft_conn ) {
	    printf("\n\ntoo many r codes? %d\n\n", lr_num_c);
	    exit( 1 );
	  }
	  lr_cf[lr_num_c] = ff;
	}
	lr_clauses[lr_num_clauses][lr_clause_length[lr_num_clauses]].literal = m * lr_codes[ff];
	lr_clauses[lr_num_clauses][lr_clause_length[lr_num_clauses]].time = -1;
	lr_clause_length[lr_num_clauses]++;
      }
      lr_num_clauses++;
      if ( gcmd_line.R && gcmd_line.debug ) {
	printf("\nr inserted OR clause"); print_r_clause( lr_num_clauses-1 ); fflush(stdout);
      }
    } /* for all initial ors */

    if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
      printf("\n\n--------------------------------r initial CNF encoding");
      printf("\n-----codes %d:", lr_num_c);
      for ( i = 1; i <= lr_num_c; i++ ) {
	printf("\n"); print_ft_name( lr_cf[i] ); 
	printf(" ---> %d", lr_codes[lr_cf[i]]);
      }
      fflush(stdout);
    }

    /* now create the membership lists.
     */
    r_do_membership_lists();
  }/* if heuristic == 1 */


}



void r_do_membership_lists( void )

{

  int i, j, v;
  MemberList *i_ml;

  /* first, "empty" the lists.
   */
  for ( i = 1; i <= lr_num_c; i++ ) {
    /* for new vars, create the dummy elements. else, just set the
     * "end marker" directly behind start.
     */
    if ( lr_pos_c_in_clause_start[i] == NULL ) {
      lr_pos_c_in_clause_start[i] = new_MemberList();
      lr_pos_c_in_clause_start[i]->clause = -1;
      lr_pos_c_in_clause_end[i] = new_MemberList();
      lr_pos_c_in_clause_end[i]->clause = -1;
      lr_pos_c_in_clause_start[i]->next = lr_pos_c_in_clause_end[i];
    } else {
      printf("\nvar in r CNF mem lists is not new\n\n");
      exit( 1 );
    }
    if ( lr_neg_c_in_clause_start[i] == NULL ) {
      lr_neg_c_in_clause_start[i] = new_MemberList();
      lr_neg_c_in_clause_start[i]->clause = -1;
      lr_neg_c_in_clause_end[i] = new_MemberList();
      lr_neg_c_in_clause_end[i]->clause = -1;
      lr_neg_c_in_clause_start[i]->next = lr_neg_c_in_clause_end[i];
    } else {
      printf("\nvar in r CNF mem lists is not new\n\n");
      exit( 1 );
    }
  }

  /* insert clauses from back to front; dunno if that has any significant effect
   * but it's become a kind of tradition..
   */
  for ( i = lr_num_clauses - 1; i >= 0; i-- ) {
    for ( j = 0; j < lr_clause_length[i]; j++ ) {
      if ( lr_clauses[i][j].literal > 0 ) {
	/* positive, is the var.
	 */
	v = lr_clauses[i][j].literal;
	if ( lr_pos_c_in_clause_end[v]->clause == -1 ) {
	  /* we're at the end of the allocated list!
	   */
	  lr_pos_c_in_clause_end[v]->clause = i;
	  lr_pos_c_in_clause_end[v]->next = new_MemberList();
	  lr_pos_c_in_clause_end[v]->next->clause = -1;
	  lr_pos_c_in_clause_end[v] = lr_pos_c_in_clause_end[v]->next;
	} else {
	  /* we're still in the middle of the list.
	   */
	  lr_pos_c_in_clause_end[v]->clause = i;
	  lr_pos_c_in_clause_end[v] = lr_pos_c_in_clause_end[v]->next;
	} /* case distinction for end pointer of v list */
      } else { /* literal negative; do the same, basically */
	v = (-1) * lr_clauses[i][j].literal;
	if ( lr_neg_c_in_clause_end[v]->clause == -1 ) {
	  /* we're at the end of the allocated list!
	   */
	  lr_neg_c_in_clause_end[v]->clause = i;
	  lr_neg_c_in_clause_end[v]->next = new_MemberList();
	  lr_neg_c_in_clause_end[v]->next->clause = -1;
	  lr_neg_c_in_clause_end[v] = lr_neg_c_in_clause_end[v]->next;
	} else {
	  /* we're still in the middle of the list.
	   */
	  lr_neg_c_in_clause_end[v]->clause = i;
	  lr_neg_c_in_clause_end[v] = lr_neg_c_in_clause_end[v]->next;
	} /* case distinction for end pointer of v list */
      } /* literal sign distinction */
    } /* clause elements */
  } /* clauses */

  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
    printf("\nr membership lists:");
    for ( i = 1; i <= lr_num_c; i++ ) {
      printf("\nr var %d pos:", i);
      for ( i_ml = lr_pos_c_in_clause_start[i]->next; 
	    i_ml != lr_pos_c_in_clause_end[i]; i_ml = i_ml->next ) {
	printf(" %d", i_ml->clause);
      }
      printf("\nr var %d neg:", i);
      for ( i_ml = lr_neg_c_in_clause_start[i]->next; 
	    i_ml != lr_neg_c_in_clause_end[i]; i_ml = i_ml->next ) {
	printf(" %d", i_ml->clause);
      }
    }
    fflush(stdout);
  }

}



long get_1P_and_H( BfsNode *tmpbfs )

{

  long h;
  int max;
  Bool reach;
  long retval;
  State *S = &(tmpbfs->S);

  if ( gcmd_line.heuristic == 3){
      get_A(S); /* Checks applicable actions*/
      int i;
      for ( i = 0; i < gnum_H; i++ ) {
	  gop_conn[gH[i]].is_in_H = FALSE;
      }
      gnum_H = 0;

      retval = ff_get_1P_and_H( S, &ggoal_state );
  }else{


      gevaluated_states++;
      if ( 0 ) {
	  printf("\nstates: %d", gevaluated_states); fflush(stdout);
      }

      times( &end );
      TIME( gsearch_time );
      times( &start );

      initialize_fixpoint( tmpbfs );
      reach = build_fixpoint( S, &max );
      if ( gcmd_line.display_info == 122 ||
	   (gcmd_line.R && gcmd_line.debug) ) {
	  print_fixpoint_result();
	  fflush(stdout);
      }

      if ( reach ) {
	  if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("\nnow extracting rplan!");
	      fflush(stdout);
	  }
	  h = extract_1P( max, TRUE );
      } else {
	  if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("\nrpg terminated unsuccessfully! no rplan!");
	      fflush(stdout);
	  }
	  h = INFINITY;
      }

      reset_fixpoint( max );

      times( &end );
      TIME( geval_time );
      times( &start );


      /* for heuristic 0, infinity doesn't mean the state is a dead end;
       */
      retval = h;
      if ( gcmd_line.heuristic == 0 &&
	   retval == INFINITY ) {
	  retval = BIG_H;
      }
  }

  /* if giveup action is activated, infinity doesn't mean the state is a dead end;
   */
  if(gcmd_line.giveup_action && (retval == INFINITY || retval > gcmd_line.giveup_cost)){
      retval = gcmd_line.giveup_cost;
  }

  return retval;
}



long get_1P( BfsNode *tmpbfs )

{
  long h;
  int max;
  Bool reach;
  long retval;
  State *S = &(tmpbfs->S);

  if ( gcmd_line.heuristic == 3){
      retval = ff_get_1P( S, &ggoal_state );
  }else{

      gevaluated_states++;
      if ( 0 ) {
	  printf("\nstates: %d", gevaluated_states); fflush(stdout);
      }

      times( &end );
      TIME( gsearch_time );
      times( &start );

      initialize_fixpoint( tmpbfs );
      reach = build_fixpoint( S, &max );
      if ( gcmd_line.display_info == 122 ||
	   (gcmd_line.R && gcmd_line.debug) ) {
	  print_fixpoint_result();
	  fflush(stdout);
      }

      if ( reach ) {
	  if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("\nnow extracting rplan!");
	      fflush(stdout);
	  }
	  h = extract_1P( max, FALSE );
      } else {
	  if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("\nrpg terminated unsuccessfully! no rplan!");
	      fflush(stdout);
	  }
	  h = INFINITY;
      }

      reset_fixpoint( max );

      times( &end );
      TIME( geval_time );
      times( &start );

      /* for heuristic 0, infinity doesn't mean the state is a dead end;
       */
      retval = h;
      if ( gcmd_line.heuristic == 0 &&
	   retval == INFINITY ) {
	  retval = BIG_H;
      }
  }

  /* if giveup action is activated, infinity doesn't mean the state is a dead end;
   */
  if(gcmd_line.giveup_action && (retval == INFINITY || retval > gcmd_line.giveup_cost)){
      retval = gcmd_line.giveup_cost;
  }

  return retval;

}



void get_A( State *S )

{

    /* if ( gcmd_line.heuristic == 3){ */
    /* 	ff_get_A( S ); */
    /* 	return; */
    /* } */

  int i, j, ft;
  /* unset prev.
   */
  for ( i = 0; i < gnum_A; i++ ) {
    gop_conn[gA[i]].is_in_A = FALSE;
  }
  gnum_A = 0;
  /* the 0p ops
   */
  for ( i = 0; i < lnum_0P_O; i++ ) {
    if ( gop_conn[l0P_O[i]].is_in_A ) {
      continue;
    }
    gop_conn[l0P_O[i]].is_in_A = TRUE;
    gA[gnum_A++] = l0P_O[i];
  }

  /* the ops activated by our facts.
   */
  lnum_ch_O = 0;
  for ( i = 0; i < S->num_F; i++ ) {
    ft = S->F[i];
    for ( j = 0; j < gft_conn[ft].num_P; j++ ) {
      gop_conn[gft_conn[ft].P[j]].num_active_Ps++;
      if ( !gop_conn[gft_conn[ft].P[j]].ch ) {
	gop_conn[gft_conn[ft].P[j]].ch = TRUE;
	lch_O[lnum_ch_O++] = gft_conn[ft].P[j];
      }
      if ( gop_conn[gft_conn[ft].P[j]].num_active_Ps >=
	   gop_conn[gft_conn[ft].P[j]].num_P ) {
	/* DEBUGGING, REMOVE LATER
	 */
	if ( gop_conn[gft_conn[ft].P[j]].is_in_A ) {
	  printf("\n\nactivated op already in\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	gop_conn[gft_conn[ft].P[j]].is_in_A = TRUE;
	gA[gnum_A++] = gft_conn[ft].P[j];
      }
    }
  }

  for ( i = 0; i < lnum_ch_O; i++ ) {
    gop_conn[lch_O[i]].num_active_Ps = 0;
    gop_conn[lch_O[i]].ch = FALSE;
  }

  /*printf("applicable actions: %d\n\n", gnum_A);*/

}






























/*******************************
 * RELAXED FIXPOINT ON A STATE *
 *******************************/





























Bool build_fixpoint( State *S, int *max )

{

  int start_ft, stop_ft, start_ef, stop_ef, i, time = 0;

  Bool new_U;

  start_ft = 0;
  start_ef = 0;
  while ( TRUE ) {
    if ( all_goals_activated( time ) ) {
      break;
    }

    stop_ft = lnum_F;
    for ( i = start_ft; i < stop_ft; i++ ) {
      activate_ft( lF[i], time );
    }

    if ( time == 0 ) {
      for ( i = 0; i < lnum_0P_E; i++ ) {
	if ( gef_conn[l0P_E[i]].in_E ) {
	  continue;
	}
	new_ef( l0P_E[i] );
      }
    }

    stop_ef = lnum_E;
    for ( i = start_ef; i < stop_ef; i++ ) {
      activate_ef( lE[i], time );
    }

    new_U = append_new_U_layer( time );

    if ( stop_ft == lnum_F && !new_U ) {
      *max = time;
      return FALSE;
    }

    start_ft = stop_ft;
    start_ef = stop_ef;
    time++;
  }

  *max = time;
  return TRUE;

}    



void initialize_fixpoint( BfsNode *tmpbfs )

{

  int i;
  State *S = &(tmpbfs->S);

  lnum_E = 0;
  lnum_ch_E = 0;
  lnum_ch_O = 0;
  l1num_ch_F = 0;

  /* the 0p ops
   */
  for ( i = 0; i < lnum_0P_O; i++ ) {
    if ( gop_conn[l0P_O[i]].level != INFINITY ) {
      continue;
    }
    gop_conn[l0P_O[i]].level = 0;    
    if ( !gop_conn[l0P_O[i]].ch ) {
      gop_conn[l0P_O[i]].ch = TRUE;
      lch_O[lnum_ch_O++] = l0P_O[i];
    }
  }

  lnum_F = 0;
  for ( i = 0; i < S->num_F; i++ ) {
    if ( gft_conn[S->F[i]].in_F ) {
      continue;
    }
    new_fact( S->F[i] );
  }

  /* create the U graph up to this state here.
   */
  insert_path_implications( tmpbfs );

}
   


void activate_ft( int index, int time )

{

  int i;

  gft_conn[index].level = time;

  for ( i = 0; i < gft_conn[index].num_PC; i++ ) {
    gef_conn[gft_conn[index].PC[i]].num_active_PCs++;
    if ( !gef_conn[gft_conn[index].PC[i]].ch ) {
      gef_conn[gft_conn[index].PC[i]].ch = TRUE;
      lch_E[lnum_ch_E++] = gft_conn[index].PC[i];
    }
    if ( gef_conn[gft_conn[index].PC[i]].num_active_PCs ==
	 gef_conn[gft_conn[index].PC[i]].num_PC ) {
      new_ef( gft_conn[index].PC[i] );
    }
  }

  /* also keep track of op levels. (needed for observations,
   * which may have no other effects)
   */
  for ( i = 0; i < gft_conn[index].num_P; i++ ) {
    gop_conn[gft_conn[index].P[i]].num_active_Ps++;
    if ( !gop_conn[gft_conn[index].P[i]].ch ) {
      gop_conn[gft_conn[index].P[i]].ch = TRUE;
      lch_O[lnum_ch_O++] = gft_conn[index].P[i];
    }
    if ( gop_conn[gft_conn[index].P[i]].num_active_Ps ==
	 gop_conn[gft_conn[index].P[i]].num_P ) {
      gop_conn[gft_conn[index].P[i]].level = time;
    }
  }

}



void activate_ef( int index, int time )

{

  int i;

  gef_conn[index].level = time;

  for ( i = 0; i < gef_conn[index].num_A; i++ ) {
    if ( gft_conn[gef_conn[index].A[i]].in_F ) {
      continue;
    }
    if ( gef_conn[index].A_nondet[i] ) {
      continue;
    }
    new_fact( gef_conn[index].A[i] );
  }

}



void new_fact( int index )

{

  lF[lnum_F++] = index;
  gft_conn[index].in_F = TRUE;

}



void new_ef( int index )

{

  lE[lnum_E++] = index;
  gef_conn[index].in_E = TRUE;

}



/* different versions of path implications are implemented; see -h option.
 */
void insert_path_implications( BfsNode *tmpbfs )

{

  static Bool fc = TRUE;
  static BfsNode_pointer *path;
  static Bool *Ut, *Utpp;
  static int *obsPtpp, *obsPt;
  
  int i, j, k, l, ef, num_path, t, c, conu, addu, op, ft;
  int Ucond, nowUft, notc, m;
  BfsNode *ibfs;
  int Utime, num_obsPtpp, num_obsPt;

  if ( fc ) {
    path = ( BfsNode_pointer * ) 
      calloc( MAX_PLAN_LENGTH + 1, sizeof( BfsNode_pointer ) );
    
    Ut = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    Utpp = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      Ut[i] = FALSE;
      Utpp[i] = FALSE;
    }
 
    obsPt = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    obsPtpp = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    
    fc = FALSE;
  }



  if ( gcmd_line.R && gcmd_line.debug ) {
    printf("\n\n-----------------------rp path");
    fflush(stdout);
  }
  num_path = MAX_PLAN_LENGTH;
  for ( ibfs = tmpbfs; ibfs->ingoing_edge; ibfs = ibfs->ingoing_edge->in_node ) {
    if ( num_path == 0 ) {
      printf("\n\nincrease MAX_PLAN_LENGTH! currently %d\n\n",
	     MAX_PLAN_LENGTH);
      exit( 1 );
    }
    path[num_path--] = ibfs;
    if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
      printf("\n-----state %d", num_path+1); print_state( path[num_path+1]->S );
      fflush(stdout);
    }
    if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
      printf("\n-----op "); print_op_name( path[num_path+1]->ingoing_edge->op );
      fflush(stdout);
    }
  }
  path[num_path--] = ibfs;
  if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
    printf("\n-----state %d", num_path+1); print_state( path[num_path+1]->S );
    fflush(stdout);
  }


  if ( gcmd_line.heuristic == 2 ) {
    /* we build impli graph only for the RPG, ie nothing much to build here!
     *
     * what we DO do here is to create the CNF encoding of the S semantics.
     */
    times( &end );
    TIME( geval_time );
    times( &start );    
    
    /* extend the current fixed clauses with the search path up to S.
     */
    lStime = r_extend_dynamic_clauses_base( path, num_path );

    times( &end );
    TIME( gr_cnf_time );
    times( &start );

    /* then extend the encoding, ie map the literal / time pairs
     * into integers.
     *
     * the translation code is in table gcodes for use below in the
     * single checks.
     */
    extend_dynamic_clauses_base_encoding( gnum_fixed_clauses, gnum_fixed_c );

    times( &end );
    TIME( gr_enc_time );
    times( &start );


    /* now build the U nodes for the initial RPG layer corresp. to S
     */
    lpath_U_length = 0; 
    t = MAX_PLAN_LENGTH;
    lnum_U[lpath_U_length] = 0;
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      if ( lnum_U[lpath_U_length] == gmax_U ) {
	printf("\n\ntoo many U facts in rpg?\n\n");
	fflush(stdout);
	exit( 1 );
      }
      /* DEBUGGING, REMOVE LATER
       */
      for ( j = 0; j < lnum_U[lpath_U_length]; j++ ) {
	if ( lU[lpath_U_length][j].ft == path[t]->S.U[i] ) {
	  printf("\n\npath state U contains same fact twice? ");
	  print_ft_name( path[t]->S.U[i] );
	  printf("\n\n");
	  exit( 1 );
	}
      }
      lU[lpath_U_length][lnum_U[lpath_U_length]].ft = path[t]->S.U[i];
      lU[lpath_U_length][lnum_U[lpath_U_length]].became_F = FALSE;
      lU[lpath_U_length][lnum_U[lpath_U_length]].observed = FALSE;
      lU[lpath_U_length][lnum_U[lpath_U_length]].num_in = 0;
      lnum_U[lpath_U_length]++;
    }
    lpath_U_length++;

    return;
  } /* gcmd_line.heuristic == 2 */




  /* heuristic is 0 or 1 -- we need the path to s
   */

  /* first, build the layer corresponding to the initial state.
   */  
  lpath_U_length = 0;
  t = num_path + 1;
  lnum_U[lpath_U_length] = 0;
  for ( i = 0; i < path[t]->S.num_U; i++ ) {
    if ( lnum_U[lpath_U_length] == gmax_U ) {
      printf("\n\ntoo many U facts in rpg?\n\n");
      fflush(stdout);
      exit( 1 );
    }
    /* DEBUGGING, REMOVE LATER
     */
    for ( j = 0; j < lnum_U[lpath_U_length]; j++ ) {
      if ( lU[lpath_U_length][j].ft == path[t]->S.U[i] ) {
	printf("\n\npath state U contains same fact twice? ");
	print_ft_name( path[t]->S.U[i] );
	printf("\n\n");
	exit( 1 );
      }
    }
    lU[lpath_U_length][lnum_U[lpath_U_length]].ft = path[t]->S.U[i];
    lU[lpath_U_length][lnum_U[lpath_U_length]].became_F = FALSE;
    lU[lpath_U_length][lnum_U[lpath_U_length]].observed = FALSE;
    lU[lpath_U_length][lnum_U[lpath_U_length]].num_in = 0;
    lnum_U[lpath_U_length]++;
  }
  /* now, proceed over all time steps t until s;
   * insert the U facts NOOPs and effects
   */
  lpath_U_length++;
  for ( t = num_path + 2; t <= MAX_PLAN_LENGTH; t++ ) {
    if ( lpath_U_length == RELAXED_STEPS + MAX_PLAN_LENGTH ) {
      printf("\n\nU graph too long! increase RELAXED_STEPS (now %d)\n\n",
	     RELAXED_STEPS );
      fflush(stdout);
      exit( 1 );
    }
    /* first, copy the U facts from state over. 
     */
    lnum_U[lpath_U_length] = 0;
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      if ( lnum_U[lpath_U_length] == gmax_U ) {
	printf("\n\ntoo many U facts in rpg?\n\n");
	fflush(stdout);
	exit( 1 );
      }
      /* DEBUGGING, REMOVE LATER
       */
      for ( j = 0; j < lnum_U[lpath_U_length]; j++ ) {
	if ( lU[lpath_U_length][j].ft == path[t]->S.U[i] ) {
	  printf("\n\npath state U contains same fact twice? ");
	  print_ft_name( path[t]->S.U[i] );
	  printf("\n\n");
	  exit( 1 );
	}
      }
      lU[lpath_U_length][lnum_U[lpath_U_length]].ft = path[t]->S.U[i];
      lU[lpath_U_length][lnum_U[lpath_U_length]].became_F = FALSE;
      lU[lpath_U_length][lnum_U[lpath_U_length]].observed = FALSE;
      lU[lpath_U_length][lnum_U[lpath_U_length]].num_in = 0;
      lnum_U[lpath_U_length]++;
      if ( gcmd_line.R && gcmd_line.debug ) {
	printf("\ninserted state U node %d: ", lpath_U_length);
	print_ft_name( path[t]->S.U[i] );
	fflush(stdout);
      }
    }
    /* here, we got all the U nodes at lpath_U_length, i.e. time t. now insert
     * the edges corresponding to NOOPs and unknown effs, as before.
     */
    for ( i = 0; i < lnum_U[lpath_U_length-1]; i++ ) {
      for ( j = 0; j < lnum_U[lpath_U_length]; j++ ) {
	if ( lU[lpath_U_length-1][i].ft == lU[lpath_U_length][j].ft ) break;
      }
      if ( j == lnum_U[lpath_U_length] ) continue;
      /* insert noop
       */
      if ( lU[lpath_U_length][j].num_in == MAX_UEDGES ) {
	printf("\n\ntoo many U edges! increase MAX_UEDGES (now %d)\n\n",
	       MAX_UEDGES );
	fflush(stdout);
	exit( 1 );
      }
      lU[lpath_U_length][j].in_edges[lU[lpath_U_length][j].num_in] = &(lU[lpath_U_length-1][i]);
      lU[lpath_U_length][j].in_efs[lU[lpath_U_length][j].num_in] = -1;
      lU[lpath_U_length][j].num_in++;
      if ( gcmd_line.R && gcmd_line.debug ) {
	printf("\ninserted NOOP edge %d -> %d: ", lpath_U_length-1, lpath_U_length);
	print_ft_name( lU[lpath_U_length-1][i].ft );
	printf(" -> ");
	print_ft_name( lU[lpath_U_length][j].ft );
	fflush(stdout);
      }
    } /* NOOPs */
    /* now, NOOPs for facts p that are F at t-1 and U at t
     * insert, for one eff that del p, (-c(t-1),p(t)) for all unknown
     * c of eff ==> is stronger than the real implication
     * (-c \or..\or -c) \and .. \and (-c \or..\or -c) (t-1) => p(t)
     */
    /* first, make fast accessible the U info at t-1 and t
     */
    for ( i = 0; i < path[t-1]->S.num_U; i++ ) {
      Ut[path[t-1]->S.U[i]] = TRUE;
    }
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      Utpp[path[t]->S.U[i]] = TRUE;
    }
    for ( i = 0; i < path[t-1]->S.num_F; i++ ) {
      ft =  path[t-1]->S.F[i];
      if ( !Utpp[ft] ) continue;
      /* find an unknown eff that del ft
       */
      ef = -1;
      for ( j = 0; j < path[t]->ingoing_edge->num_unknown_E; j++ ) {
	ef = path[t]->ingoing_edge->unknown_E[j];
	for ( k = 0; k < gef_conn[ef].num_D; k++ ) {
	  if ( ft == gef_conn[ef].D[k] ) break;
	}
	if ( k < gef_conn[ef].num_D ) break;
      }
      if ( j == path[t]->ingoing_edge->num_unknown_E ) {
	continue;/* must be nondet del */
	printf("\nft F at t-1, U at t, but no unknown del on it?\n\n");
	exit( 1 );
      }
      if ( ef < 0 ) {
	printf("\nef < 0 in rplan-nodel-noops?\n\n");
	exit( 1 );
      }
      for ( j = 0; j < gef_conn[ef].num_C; j++ ) {
	c = gef_conn[ef].C[j];
	if ( !Ut[c] ) continue;
	notc = gft_conn[c].negation;
	if ( notc == -1 ) {
	  printf("\neffcond -- in rplan-nodel-noops -- has no negation?\n\n");
	  exit( 1 );
	}
	if ( !Ut[notc] ) {
	  printf("\nnegated effcond -- in rplan-nodel-noops -- is not unknown?\n\n");
	  exit( 1 );
	}
	/* find lU nodes corresponding to notc(t-1) and ft(t)
	 */
	for ( Ucond = 0; Ucond < lnum_U[lpath_U_length-1]; Ucond++ ) {
	  if ( lU[lpath_U_length-1][Ucond].ft == notc ) break;
	}
	if ( Ucond == lnum_U[lpath_U_length-1] ) {
	  printf("can't find negated effcond -- in rplan-nodel-noops -- in lU?\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	for ( nowUft = 0; nowUft < lnum_U[lpath_U_length]; nowUft++ ) {
	  if ( lU[lpath_U_length][nowUft].ft == ft ) break;
	}
	if ( nowUft == lnum_U[lpath_U_length] ) {
	  printf("can't find ft(t) -- in rplan-nodel-noops -- in lU?\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	/* we're ready. insert the edge!
	 */
	lU[lpath_U_length][nowUft].in_edges[lU[lpath_U_length][nowUft].num_in] = 
	  &(lU[lpath_U_length-1][Ucond]);
	lU[lpath_U_length][nowUft].in_efs[lU[lpath_U_length][nowUft].num_in] = -1;
	lU[lpath_U_length][nowUft].num_in++;
	if ( gcmd_line.R && gcmd_line.debug ) {
	  printf("\ninserted nondel NOOP edge %d -> %d: ", lpath_U_length-1, lpath_U_length);
	  print_ft_name( notc );
	  printf(" -> ");
	  print_ft_name( ft );
	  fflush(stdout);
	}
      } /* for all conditions c of U eff that del ft */
    } /* for all i (ft) in F(t-1) */

    /*************************************************************
     * now, the OPs
     */
    /* go along the path just like in clause generation, and
     * if an unknown add effect on an unknown fact l' in S
     * occurs then insert the edge  l -> l' where l is the first
     * found cond that is unknown.
     */
    for ( i = 0; i < path[t]->ingoing_edge->num_unknown_E; i++ ) {
      ef = path[t]->ingoing_edge->unknown_E[i];
      for ( c = 0; c < gef_conn[ef].num_C; c++ ) {
	if ( Ut[gef_conn[ef].C[c]] ) break;
      }
      /* DEBUGGING; REMOVE LATER
       */
      if ( c == gef_conn[ef].num_C ) {
	printf("\n\nunknown ef has all conds known\n\n");
	fflush(stdout);
	exit( 1 );
      }
      /* node in path U
       */
      for ( k = 0; k < lnum_U[lpath_U_length-1]; k++ ) {
	if ( lU[lpath_U_length-1][k].ft == gef_conn[ef].C[c] ) break;
      }
      if ( k == lnum_U[lpath_U_length-1] ) {
	printf("can't find con ft on rp path?\n\n");
	fflush(stdout);
	exit( 1 );
      }
      conu = k;
      /* now, walk through the adds.
       */
      for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
	if ( !Utpp[gef_conn[ef].A[j]] ) continue;/* known in next step */
	if ( gef_conn[ef].A_nondet[j] ) {
	  /* no implication edges based on nondet effects!
	   */
	  continue;
	}
	/* we insert the edge gef_conn[ef].C[c] -> gef_conn[ef].A[j]
	 * where c is the first condition of ef we found above that is unknown both
	 * at execution point and in our state.
	 */
	/* edge U[lpath_U_length-1] -> U[lpath_U_length]; first find node.
	 */
	for ( k = 0; k < lnum_U[lpath_U_length]; k++ ) {
	  if ( lU[lpath_U_length][k].ft == gef_conn[ef].A[j] ) break;
	}
	/* DEBUGGING
	 */
	if ( k == lnum_U[lpath_U_length] ) {
	  printf("can't find added ft on rp path?\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	addu = k;
	if ( lU[lpath_U_length][addu].num_in == MAX_UEDGES ) {
	  printf("\n\ntoo many U edges! increase MAX_UEDGES (now %d)\n\n",
		 MAX_UEDGES );
	  fflush(stdout);
	  exit( 1 );
	}
	lU[lpath_U_length][addu].in_edges[lU[lpath_U_length][addu].num_in] = &(lU[lpath_U_length-1][conu]);
	lU[lpath_U_length][addu].in_efs[lU[lpath_U_length][addu].num_in] = ef;
	lU[lpath_U_length][addu].num_in++;
	if ( gcmd_line.R && gcmd_line.debug ) {
	  printf("\ninserted OP edge %d -> %d: ", lpath_U_length-1, lpath_U_length);
	  print_ft_name( gef_conn[ef].C[0] );
	  printf(" -> ");
	  print_ft_name( gef_conn[ef].A[j] );
	  fflush(stdout);
	}
      }/* A of u-ef */
    }/* u-efs at t */

    /* unset infos
     */
    for ( i = 0; i < path[t-1]->S.num_U; i++ ) {
      Ut[path[t-1]->S.U[i]] = FALSE;
    }
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      Utpp[path[t]->S.U[i]] = FALSE;
    }

    lpath_U_length++;
  } /* t over the "past" time steps */


  /* now, we must back-propagate the observations, ie go along path from s towards 
   * initial state; we propagate a set obsP of facts that make us end up in S. 
   * opsP is initially empty. if P was observed at t+1, then 
   * we insert P into opsP(t+1). when this is done for all P at t+1, we select a set opsP(t)
   * so that, if \phi(t) is the formula that is equivalent to \AND opsP(t+1) holding at t+1,
   * then (\AND opsP(t)) implies \phi(t); ie we consider a subset of the t states that make
   * opsP(t+1) true. when at the initial state, we insert opsP(0) as unary clauses.
   * this way the initial worlds we consider are a subset of the worlds that end up 
   * in S.
   */
  if ( gcmd_line.R && gcmd_line.debug ) {
    printf("\nbackpropagating observation restrictions");
  }
  num_obsPtpp = 0;
  for ( t = MAX_PLAN_LENGTH-1; t >= num_path+1; t-- ) {
    Utime = t - (num_path+1);
    /* make fast accessible the U info at t and t+1
     */
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      Ut[path[t]->S.U[i]] = TRUE;
    }
    for ( i = 0; i < path[t+1]->S.num_U; i++ ) {
      Utpp[path[t+1]->S.U[i]] = TRUE;
    }

    /* the propagations. first, insert all observed facts into obsPtpp
     */
    op = path[t+1]->ingoing_edge->op;
    for ( i = 0; i < gop_conn[op].num_O; i++ ) {
      ft = gop_conn[op].O[i];
      for ( j = 0; j < path[t+1]->S.num_F; j++ ) {
	if ( path[t+1]->S.F[j] == ft ) break;
      }
      if ( j < path[t+1]->S.num_F ) {
	/* ft was observed to be true
	 */
	for ( j = 0; j < num_obsPtpp; j++ ) {
	  if ( obsPtpp[j] == ft ) break;
	}
	if ( j < num_obsPtpp ) continue;/* no duplicates */
	if ( gcmd_line.R && gcmd_line.debug ) {
	  printf("\ninsert obs into obsPtpp(%d) ", t+1);
	  print_ft_name( ft );
	}
	obsPtpp[num_obsPtpp++] = ft;
	continue;
      }
      /* ft was observed to be false, propagate its negation.
       */
      if ( gft_conn[ft].negation == -1 ) {
	printf("\nobs prop. negation of neg observed fact not there?\n\n");
	exit( 1 );
      }
      for ( j = 0; j < num_obsPtpp; j++ ) {
	if ( obsPtpp[j] == gft_conn[ft].negation ) break;
      }
      if ( j < num_obsPtpp ) continue;/* no duplicates */
      if ( gcmd_line.R && gcmd_line.debug ) {
	printf("\ninsert neg obs obs into obsPtpp(%d) ", t+1);
	print_ft_name( gft_conn[ft].negation );
      }
      obsPtpp[num_obsPtpp++] = gft_conn[ft].negation;
    } /* for all observations ft or -ft (i) made by the op */

    /* now go over all facts in obsPtpp and insert a sufficient condition for the fact-\phi
     * into obsPt
     */
    num_obsPt = 0;
    for ( i = 0; i < num_obsPtpp; i++ ) {
      ft = obsPtpp[i];
      if ( gcmd_line.R && gcmd_line.debug ) {
	printf("\ntime tpp %d Utimepp %d, ft ", t+1, Utime+1);
	print_ft_name( ft );
      }
      /* debug...
       */
      for ( j = 0; j < path[t+1]->S.num_U; j++ ) {
	if ( path[t+1]->S.U[j] == ft ) break;
      }
      if ( j == path[t+1]->S.num_U ) {
	for ( j = 0; j < gop_conn[op].num_O; j++ ) {
	  if ( gop_conn[op].O[j] == ft ) break;
	  if ( gft_conn[gop_conn[op].O[j]].negation == ft ) break;
	}
	if ( j == gop_conn[op].num_O ) {
	  printf("\nobs prop. not U fact at tpp not observed?\n\n");
	  exit( 1 );
	}
	for ( j = 0; j < path[t+1]->S.num_F; j++ ) {
	  if ( path[t+1]->S.F[j] == ft ) break;
	}
	if ( j == path[t+1]->S.num_F ) {
	  printf("\nobs prop. not U fact at tpp not F?\n\n");
	  exit( 1 );
	}
      }
      /* NOOP implication if ft is U or F at t.
       * add effect implication if ft is false at t. (overall ft-phi = NOOPphi \or addeffphi;
       * we select a sufficient condition for just one of the two)
       *
       * first, we deal with the NOOP case. \phi for NOOP is
       * ft(t) \and (-c(t) \or .. \or -c(t)) \and .. \and (-c(t) \or .. \or -c(t)) [=> ft(t+1)]
       * where c's are conds of unknown effects that delete ft (there can't be any known 
       * effects that do this or else ft would be false at tpp)
       *
       * the suff cond we create is ft(t) \and -c(t) \and .. \and -c(t) where the c's are arbitarily
       * selected unknowns from the effconds; if ft is T at t, it is left out. 
       */
      for ( j = 0; j < path[t]->S.num_F; j++ ) {
	if ( path[t]->S.F[j] == ft ) break;
      }
      if ( j < path[t]->S.num_F || Ut[ft] ) {
	/* F or U; we use the NOOP!
	 */
	if ( gcmd_line.R && gcmd_line.debug ) {
	  if ( Ut[ft] ) {
	    printf("\nis U at t! using NOOP obsP(t)");
	  } else {
	    printf("\nis F at t! using NOOP obsP(t)");
	  }
	}
	if ( Ut[ft] ) {
	  /* it's U; insert into obsPt
	   */
	  for ( j = 0; j < num_obsPt; j++ ) {
	    if ( obsPt[j] == ft ) break;
	  }
	  if ( j == num_obsPt ) {
	    if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("\nnoop ft insert into obsPt(%d) ", t);
	      print_ft_name( ft );
	    }
	    obsPt[num_obsPt++] = ft;
	  }
	}
	/* now the conds as explained above.
	 */
	for ( j = 0; j < path[t+1]->ingoing_edge->num_unknown_E; j++ ) {
	  ef = path[t+1]->ingoing_edge->unknown_E[j];
	  for ( k = 0; k < gef_conn[ef].num_D; k++ ) {
	    if ( gef_conn[ef].D[k] == ft ) break;
	  }
	  if ( k == gef_conn[ef].num_D ) continue;
	  for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	    if ( Ut[gef_conn[ef].C[k]] ) break;
	  }
	  if ( k == gef_conn[ef].num_D ) {
	    printf("\nobs prop. U eff del no unknown cond?\n\n");
	    exit( 1 );
	  }
	  /* insert into obsPt
	   */
	  c = gft_conn[gef_conn[ef].C[k]].negation;
	  if ( c < 0 ) {
	    printf("\nobs prop. negated cond not there?\n\n");
	    exit( 1 );
	  }
	  for ( k = 0; k < num_obsPt; k++ ) {
	    if ( obsPt[k] == c ) break;
	  }
	  if ( k == num_obsPt ) {
	    if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("\nnoop c insert into obsPt(%d) ", t);
	      print_ft_name( c );
	    }
	    obsPt[num_obsPt++] = c;
	  }
	} /* unknown effs (j) that del ft */

	/* no add efffect impli in case NOOP is already done.
	 */
	continue;
      } /* ft is F or U at t */

      /* no NOOP, we take the add effect implications, ie phi is
       * (c(t) \and .. \and c(t)) \or .. \or (c(t) \and .. \and c(t)) [==> ft(t+1)]
       * where the c's are the effconds of all effects that add ft
       *
       * the suff cond we create is (c(t) \and .. \and c(t)) where the c's are the conds
       * of an arbitrary unknown eff.
       * F effconds are not inserted into obsPt since they are true anyway and not needed
       * for the implication opsP(t) => \phi to hold. therefore there's also no point in
       * selecting known effects -- well actually there can't be any such known effects
       * since either ft is U at t+1, or observed by the action.
       */
      if ( gcmd_line.R && gcmd_line.debug ) {
	printf("\nis neither F nor U at t! using add effect opsP(t)");
      }
      for ( j = 0; j < path[t+1]->ingoing_edge->num_unknown_E; j++ ) {
	ef = path[t+1]->ingoing_edge->unknown_E[j];
	for ( k = 0; k < gef_conn[ef].num_A; k++ ) {
	  if ( gef_conn[ef].A_nondet[k] ) continue;
	  if ( gef_conn[ef].A[k] == ft ) break;
	}
	if ( k == gef_conn[ef].num_A ) continue;
	for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	  if ( !Ut[gef_conn[ef].C[k]] ) continue;
	  for ( l = 0; l < num_obsPt; l++ ) {
	    if ( obsPt[l] == gef_conn[ef].C[k] ) break;
	  }
	  if ( l == num_obsPt ) {
	    if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("\nadd eff c insert into obsPt(%d) ", t);
	      print_ft_name( gef_conn[ef].C[k] );
	    }
	    obsPt[num_obsPt++] = gef_conn[ef].C[k];
	  }
	}
	/* one eff is enough, see above
	 */
	break;
      }
      /* if there was no unknown add eff then this means that
       * the ft was achieved by a nondet effect. no 
       * propagations!
       */
      if ( j == path[t+1]->ingoing_edge->num_unknown_E ) {
	if ( gcmd_line.R && gcmd_line.debug ) {
	  printf("\nobs prop. ft false at t, no U add eff on it! must be nondet. skipping. ");
	  print_ft_name( ft ); printf("\n\n");
	}
      }
    } /* for (i) ft in obsPtpp */

    /* undo info
     */
    for ( i = 0; i < path[t]->S.num_U; i++ ) {
      Ut[path[t]->S.U[i]] = FALSE;
    }
    for ( i = 0; i < path[t+1]->S.num_U; i++ ) {
      Utpp[path[t+1]->S.U[i]] = FALSE;
    }

    /* copy obsPt into obsPtpp 
     */
    for ( i = 0; i < num_obsPt; i++ ) {
      obsPtpp[i] = obsPt[i];
    }
    num_obsPtpp = num_obsPt;
  } /* t backwards from S */




  /* now we have the initial state restrictions in obsPtpp. "insert" this into
   * RPG initial state CNF.
   *
   * implemented by collecting codes of facts into an array; before r_DP
   * is called, this array is then pushed onto the decision stack.
   */
  if ( gcmd_line.R && gcmd_line.debug ) {
    printf("\nobs backpropagations:");
  }
  lnum_obs_backpropagation = 0;
  for ( i = 0; i < num_obsPtpp; i++ ) {
    ft = obsPtpp[i];
    for ( j = 0; j < path[num_path+1]->S.num_U; j++ ) {
      if ( path[num_path+1]->S.U[j] == ft ) break;
    }
    if ( j == path[num_path+1]->S.num_U ) {
      printf("\nobs prop. initial obsPtpp not U at %d?\n\n", num_path+1);
      exit( 1 );
    }

    if ( gft_conn[ft].CNF ) {
      m = 1;
    } else {
      m = -1;
      ft = gft_conn[ft].negation;
    }
    if ( lr_codes[ft] == -1 ) {
      printf("\ninitial obsPt not encoded?\n\n");
      exit( 1 );
    }
    /* we have the code, the literal occurs. write it into the code array.
     */
    if ( gcmd_line.R && gcmd_line.debug ) {
      printf("\nsign %d, code %d: ", m, lr_codes[ft]);
      print_ft_name( ft );
    }
    lobs_backpropagation[lnum_obs_backpropagation++] = m * lr_codes[ft];
  } /* facts in obsPt at time 0 */

}



/* returns TRUE if a new fact has become unknown
 * rather than false, or has become true rather than unknown, or
 * has become new facts in its backwards reachability tree.
 */
Bool append_new_U_layer( time )

{

  static Bool fc = TRUE;
  static int *Ups, *ce;

  int num_ce;
  int i, j, k, conu, addu, ft, ef, Utime, l;

  Bool new_U = FALSE;

  /* the state to be evaluated is in U layer
   * lpath_U_length - 1!!
   */
  Utime = time + lpath_U_length - 1;
  if ( Utime + 1 == RELAXED_STEPS + MAX_PLAN_LENGTH ) {
    printf("\n\nU graph is too long! increase RELAXED_STEPS (now %d)\n\n",
	   RELAXED_STEPS );
    fflush(stdout);
    exit( 1 );
  }

  if ( gcmd_line.R && gcmd_line.debug ) {
    printf("\nappend new U layer call, time %d, Utime %d", time, Utime);
  }

  if ( fc ) {
    Ups = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
    ce = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
    for ( i = 0; i < gnum_ef_conn; i++ ) {
      Ups[i] = 0;
    }

    fc = FALSE;
  }

  lnum_U[Utime+1] = 0;
  /* first, transfer U facts to new layer, if they're not yet added.
   */
  for ( i = 0; i < lnum_U[Utime]; i++ ) {
    if ( lU[Utime][i].became_F ) {
      /* ignore those that became true already
       */
      continue;
    }
    ft = lU[Utime][i].ft;
    if ( gft_conn[ft].in_F ) {
      /* this one's now added
       */
      continue;
    }
    if ( lnum_U[Utime+1] == gmax_U ) {
      printf("\n\ntoo many U facts in rpg?\n\n");
      fflush(stdout);
      exit( 1 );
    }
    lU[Utime+1][lnum_U[Utime+1]].ft = ft;
    lU[Utime+1][lnum_U[Utime+1]].became_F = FALSE;
    lU[Utime+1][lnum_U[Utime+1]].observed = FALSE;
    lU[Utime+1][lnum_U[Utime+1]].num_in = 0;
    /* the "noop" edge
     */
    if ( lU[Utime+1][lnum_U[Utime+1]].num_in == MAX_UEDGES ) {
      printf("\n\ntoo many U edges! increase MAX_UEDGES (now %d)\n\n",
	     MAX_UEDGES );
      fflush(stdout);
      exit( 1 );
    }
    lU[Utime+1][lnum_U[Utime+1]].in_edges[lU[Utime+1][lnum_U[Utime+1]].num_in] = 
      &(lU[Utime][i]);
    lU[Utime+1][lnum_U[Utime+1]].in_efs[lU[Utime+1][lnum_U[Utime+1]].num_in] = 
      -1;
    /* increment edges and U fact at Utime+1 counters.
     */
    lU[Utime+1][lnum_U[Utime+1]].num_in++;
    lnum_U[Utime+1]++;
  }

  if ( gcmd_line.R && gcmd_line.debug >= 3 ) {
    printf("\nentry U on efs propag time %d, Utime %d", time, Utime);
    fflush(stdout);
  }

  /* now, see which new facts become unknown; determine 
   * unknown efs by stepping through U facts here, incrementing
   * counters of supported effects.
   */ 
  num_ce = 0;
  for ( i = 0; i < lnum_U[Utime]; i++ ) {
    if ( gcmd_line.R && gcmd_line.debug >= 3 ) {
      printf("\npropag fact "); print_ft_name( lU[Utime][i].ft );
    }
    if ( lU[Utime][i].became_F ) {
      /* ignore those that became true already
       */
      continue;
    }
    ft = lU[Utime][i].ft;
    for ( j = 0; j < gft_conn[ft].num_C; j++ ) {
      /* NOTE: we only step through the *C* effects!!!
       * thereby, we only accept effects whose op is already full in,
       * but the effect antecedent is (partly) open... seems unclear how
       * to proceed otherwise, as several pre+ef conds might be open.
       *
       * we insert a stronger binary implication, ie  c -> a for all unknown a
       * where c is the first ef cond we meet that is unknown.
       */
      ef = gft_conn[ft].C[j];
      if ( Ups[ef] == 0 ) {
	ce[num_ce++] = ef;
      }
      Ups[ef]++;
      if ( gcmd_line.R && gcmd_line.debug >= 3 ) {
	printf("\nadded 1 to val of ef %d, now %d, fact ", ef, Ups[ef]); print_ft_name( ft );
	fflush(stdout);
      }
      if ( gef_conn[ef].num_active_PCs + Ups[ef] == gef_conn[ef].num_PC ) {
	/* first, find the unknown condition. (Mar04: we could also just use ft here!!)
	 */
	conu = -1;
	for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	  for ( conu = 0; conu < lnum_U[Utime]; conu++ ) {
	    if ( gef_conn[ef].C[k] == lU[Utime][conu].ft ) break;
	  }
	  if ( conu < lnum_U[Utime] ) break;
	}
	/* DEBUGGING, REMOVE LATER.
	 */
	if ( k == gef_conn[ef].num_C ) {
	  printf("\n\nsupported ef got no unknown C?\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	if ( conu == -1 ) {
	  printf("\n\ncon == -1\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	for ( k = 0; k < gef_conn[ef].num_A; k++ ) {
	  if ( gft_conn[gef_conn[ef].A[k]].in_F ) {
	    /* this add effect is known to be true already
	     */
	    continue;
	  }
	  if ( gef_conn[ef].A_nondet[k] ) {
	    /* no edges for nondet effects!
	     */
	    continue;
	  }
	  for ( addu = 0; addu < lnum_U[Utime+1]; addu++ ) {
	    if ( gef_conn[ef].A[k] == lU[Utime+1][addu].ft ) break;
	  }
	  if ( addu == lnum_U[Utime+1] ) {
	    /* this added ft is not registered unknown yet
	     */
	    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	      printf("\nnew U fact");
	      print_ft_name( gef_conn[ef].A[k] );
	      fflush(stdout);
	    }
	    if ( lnum_U[Utime+1] == gmax_U ) {
	      printf("\n\ntoo many U facts in rpg?\n\n");
	      fflush(stdout);
	      exit( 1 );
	    }
	    lU[Utime+1][lnum_U[Utime+1]].ft = gef_conn[ef].A[k];
	    lU[Utime+1][lnum_U[Utime+1]].became_F = FALSE;
	    lU[Utime+1][lnum_U[Utime+1]].observed = FALSE;
	    lU[Utime+1][lnum_U[Utime+1]].num_in = 0;
	    
	    if ( lU[Utime+1][lnum_U[Utime+1]].num_in == MAX_UEDGES ) {
	      printf("\n\ntoo many U edges! increase MAX_UEDGES (now %d)\n\n",
		     MAX_UEDGES );
	      fflush(stdout);
	      exit( 1 );
	    }
	    lU[Utime+1][lnum_U[Utime+1]].in_edges[lU[Utime+1][lnum_U[Utime+1]].num_in] = 
	      &(lU[Utime][conu]);
	    lU[Utime+1][lnum_U[Utime+1]].in_efs[lU[Utime+1][lnum_U[Utime+1]].num_in] = 
	      ef;
	    lU[Utime+1][lnum_U[Utime+1]].num_in++;
	    lnum_U[Utime+1]++;
	    /* hand this event info back. no graph fixpoint yet.
	     */
	    new_U = TRUE;
	  } else {
	    /* else only insert ef edge; 
	     *
	     * AND: ONLY INSERT THAT EDGE IF WE DON'T HAVE AN EDGE TO lU[Utime][conu]->ft
	     *      ie to the enabling fact, already!!
	     */
	    for ( l = 0; l < lU[Utime+1][addu].num_in; l++ ) {
	      if ( lU[Utime+1][addu].in_edges[l]->ft ==
		   lU[Utime][conu].ft ) break;
	    }
	    if ( l == lU[Utime+1][addu].num_in ) {
	      if ( lU[Utime+1][addu].num_in == MAX_UEDGES ) {
		printf("\n\ntoo many added U edges! increase MAX_UEDGES (now %d)\n\n",
		       MAX_UEDGES );
		printf("\nadded ft: "); print_ft_name( lU[Utime+1][addu].ft );
		for ( k = 0; k < lU[Utime+1][addu].num_in; k++ ) {
		  printf("\nedge to: "); print_ft_name( lU[Utime+1][addu].in_edges[k]->ft );
		  printf(" by "); 
		  print_op_name( gef_conn[lU[Utime+1][addu].in_efs[k]].op );
		}
		printf("\n\n");
		fflush(stdout);
		exit( 1 );
	      }
	      lU[Utime+1][addu].in_edges[lU[Utime+1][addu].num_in] = &(lU[Utime][conu]);
	      lU[Utime+1][addu].in_efs[lU[Utime+1][addu].num_in] = ef;
	      lU[Utime+1][addu].num_in++;
	    }
	  } /* else -- added ft not new */
	} /* all added facts of new ef */
      } /* if unknown ef comes in */
      /* DEBUGGING, REMOVE LATER
       */
      if ( gef_conn[ef].num_active_PCs + Ups[ef] > gef_conn[ef].num_PC ) {
	printf("\n\nat time %d Utime %d sum F + U %d %d ef PC higher than total %d? ",
	       time, Utime, gef_conn[ef].num_active_PCs, Ups[ef], gef_conn[ef].num_PC);
	print_op_name( gef_conn[ef].op );
	printf(" supp here by ");
	print_ft_name( ft );
	printf("\n\n");
	fflush(stdout);
	exit( 1 );
      }
    } /* all effconds the ft participates in */


    /* now do the same longish thing, for observed unknown facts, with 
     * the action preconditions they participate in
     *
     * --> if a new ef becomes unknown due to that, insert the implications
     * with an unknown observed action precondition.
     *
     * inefficient... never mind... not critical unless I'm very mistaken
     */
    if ( !lU[Utime][i].observed ) {
      continue;
    }
    for ( j = 0; j < gft_conn[ft].num_PC; j++ ) {
      ef = gft_conn[ft].PC[j];
      /* first make sure that this fact is a pre here!
       */
      for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	if ( ft == gef_conn[ef].C[k] ) break;
      }
      if ( k < gef_conn[ef].num_C ) {
	continue;
      }
      if ( Ups[ef] == 0 ) {
	ce[num_ce++] = ef;
      }
      Ups[ef]++;
      if ( gcmd_line.R && gcmd_line.debug >= 3 ) {
	printf("\nObserved P added 1 to val of ef %d, now %d, fact ", ef, Ups[ef]); print_ft_name( ft );
	fflush(stdout);
      }
      if ( gef_conn[ef].num_active_PCs + Ups[ef] == gef_conn[ef].num_PC ) {
	conu = -1;
	for ( k = 0; k < gop_conn[gef_conn[ef].op].num_P; k++ ) {
	  for ( conu = 0; conu < lnum_U[Utime]; conu++ ) {
	    if ( gop_conn[gef_conn[ef].op].P[k] == lU[Utime][conu].ft ) break;
	  }
	  if ( conu < lnum_U[Utime] ) break;
	}
	/* DEBUGGING, REMOVE LATER.
	 */
	if ( k == gop_conn[gef_conn[ef].op].num_P ) {
	  printf("\n\nObserved P supported ef got no unknown P?\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	if ( conu == -1 ) {
	  printf("\n\nObserved P con == -1\n\n");
	  fflush(stdout);
	  exit( 1 );
	}
	for ( k = 0; k < gef_conn[ef].num_A; k++ ) {
	  if ( gft_conn[gef_conn[ef].A[k]].in_F ) {
	    /* this add effect is known to be true already
	     */
	    continue;
	  }
	  if ( gef_conn[ef].A_nondet[k] ) {
	    /* no edges for nondet effects!
	     */
	    continue;
	  }
	  for ( addu = 0; addu < lnum_U[Utime+1]; addu++ ) {
	    if ( gef_conn[ef].A[k] == lU[Utime+1][addu].ft ) break;
	  }
	  if ( addu == lnum_U[Utime+1] ) {
	    /* this added ft is not registered unknown yet
	     */
	    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	      printf("\nObserved P new U fact");
	      print_ft_name( gef_conn[ef].A[k] );
	      fflush(stdout);
	    }
	    if ( lnum_U[Utime+1] == gmax_U ) {
	      printf("\n\nObserved P too many U facts in rpg?\n\n");
	      fflush(stdout);
	      exit( 1 );
	    }
	    lU[Utime+1][lnum_U[Utime+1]].ft = gef_conn[ef].A[k];
	    lU[Utime+1][lnum_U[Utime+1]].became_F = FALSE;
	    lU[Utime+1][lnum_U[Utime+1]].observed = FALSE;
	    lU[Utime+1][lnum_U[Utime+1]].num_in = 0;
	    
	    if ( lU[Utime+1][lnum_U[Utime+1]].num_in == MAX_UEDGES ) {
	      printf("\n\nObserved P too many U edges! increase MAX_UEDGES (now %d)\n\n",
		     MAX_UEDGES );
	      fflush(stdout);
	      exit( 1 );
	    }
	    lU[Utime+1][lnum_U[Utime+1]].in_edges[lU[Utime+1][lnum_U[Utime+1]].num_in] = 
	      &(lU[Utime][conu]);
	    lU[Utime+1][lnum_U[Utime+1]].in_efs[lU[Utime+1][lnum_U[Utime+1]].num_in] = 
	      ef;
	    lU[Utime+1][lnum_U[Utime+1]].num_in++;
	    lnum_U[Utime+1]++;
	    /* hand this event info back. no graph fixpoint yet.
	     */
	    new_U = TRUE;
	  } else {
	    /* else only insert ef edge; 
	     *
	     * AND: ONLY INSERT THAT EDGE IF WE DON'T HAVE AN EDGE TO lU[Utime][conu]->ft
	     *      ie to the enabling fact, already!!
	     */
	    for ( l = 0; l < lU[Utime+1][addu].num_in; l++ ) {
	      if ( lU[Utime+1][addu].in_edges[l]->ft ==
		   lU[Utime][conu].ft ) break;
	    }
	    if ( l == lU[Utime+1][addu].num_in ) {
	      if ( lU[Utime+1][addu].num_in == MAX_UEDGES ) {
		printf("\n\nObserved P too many added U edges! increase MAX_UEDGES (now %d)\n\n",
		       MAX_UEDGES );
		printf("\nadded ft: "); print_ft_name( lU[Utime+1][addu].ft );
		for ( k = 0; k < lU[Utime+1][addu].num_in; k++ ) {
		  printf("\nedge to: "); print_ft_name( lU[Utime+1][addu].in_edges[k]->ft );
		  printf(" by "); 
		  print_op_name( gef_conn[lU[Utime+1][addu].in_efs[k]].op );
		}
		printf("\n\n");
		fflush(stdout);
		exit( 1 );
	      }
	      lU[Utime+1][addu].in_edges[lU[Utime+1][addu].num_in] = &(lU[Utime][conu]);
	      lU[Utime+1][addu].in_efs[lU[Utime+1][addu].num_in] = ef;
	      lU[Utime+1][addu].num_in++;
	    }
	  } /* else -- added ft not new */
	} /* all added facts of new ef */
      } /* if unknown ef comes in */
      /* DEBUGGING, REMOVE LATER
       */
      if ( gef_conn[ef].num_active_PCs + Ups[ef] > gef_conn[ef].num_PC ) {
	printf("\n\nObserved P at time %d Utime %d sum F + U %d %d ef PC higher than total %d? ",
	       time, Utime, gef_conn[ef].num_active_PCs, Ups[ef], gef_conn[ef].num_PC);
	print_op_name( gef_conn[ef].op );
	printf(" supp here by ");
	print_ft_name( ft );
	printf("\n\n");
	fflush(stdout);
	exit( 1 );
      }
    } /* all preconds the ft participates in */


  } /* all unknown fts at Utime, implication edges loop */
  /* unset the Ups info
   */
  for ( i = 0; i < num_ce; i++ ) {
    if ( gcmd_line.R && gcmd_line.debug >= 3 ) {
      printf("\nsetting %d to 0", ce[i]);
      fflush(stdout);
    }
    Ups[ce[i]] = 0;
  }



  /* now say who is observed of the time+1 unknown facts.
   */
  for ( i = 0; i < lnum_U[Utime+1]; i++ ) {
    ft = lU[Utime+1][i].ft;
    /* see if there's an op with level below here and who observes ft.
     */
    for ( j = 0; j < gft_conn[ft].num_O; j++ ) {
      if ( gop_conn[gft_conn[ft].O[j]].level != INFINITY &&
	   gop_conn[gft_conn[ft].O[j]].level <= time ) {
	break;
      }
    }
    if ( j < gft_conn[ft].num_O ) {
      lU[Utime+1][i].observed = TRUE;
      /* check if that was unknown beforehand; if it wasn't observed then,
       * we have changes in the fixpoint procedure
       */
      if ( gft_conn[ft].observed_level == INFINITY ) {
	gft_conn[ft].observed_level = time+1;
	new_U = TRUE;
	if ( !gft_conn[ft].ch1 ) {
	  l1ch_F[l1num_ch_F++] = ft;
	  gft_conn[ft].ch1 = TRUE;
	}
      }
    }
  }



  /* now infere new facts! --- those for which there are paths from 
   * a fact p as well as its negation -p.
   */
  for ( i = 0; i < lnum_U[Utime+1]; i++ ) {
    /* DEBUGGING, REMOVE LATER
     */
    if ( lU[Utime+1][i].became_F ) {
      printf("\n\njust created U ft at time + 1 became F\n\n");
      fflush(stdout);
      exit( 1 );
    }
    if ( Uleaf_disjunction_implied_by_formula( Utime, &(lU[Utime+1][i]) ) ) {
      /* hand result over to the rest of the fixpoint algorithm!
       */
      lU[Utime+1][i].became_F = TRUE;
      new_fact( lU[Utime+1][i].ft );
      
      new_U = TRUE;
    }
  }



  /* it might be that no new ft became U, and no new fact
   * became F, but new fts became reachable from a node in
   * comparison to before so changes are still made.
   */
  if ( !new_U ) {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nno new Us or Fs at %d. checking reachable fts", time+1);
      fflush( stdout );
    }
    for ( i = 0; i < lnum_U[Utime+1]; i++ ) {
      for ( j = 0; j < lnum_U[Utime]; j++ ) {
	if ( lU[Utime][j].ft == lU[Utime+1][i].ft ) break;
      }
      /* DEBUGGING; REMOVE LATER
       */
      if ( j == lnum_U[Utime] ) {
	printf("\n\ntime + 1 U ft not in time U though no new-U\n\n");
	fflush(stdout);
	exit( 1 );
      }
      if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	printf("\n"); print_ft_name( lU[Utime+1][i].ft );
	fflush(stdout);
      }
      if ( more_reachable_facts( Utime, &(lU[Utime+1][i]), &(lU[Utime][j]) ) ) {
	new_U = TRUE;
	break;
      }
    }
  }

  return new_U;

}



Bool more_reachable_facts( int Utime, UftNode *npp, UftNode *n )

{

  static Bool fc = TRUE;
  static Bool *on_npath;
  static UftNode_pointer *p, *pp;

  int num_p, num_pp, prev_p, now_p;
  int i, j, t, ft, ft_, k;

  if ( fc ) {
    on_npath = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    p = ( UftNode_pointer * ) calloc( lMAX_PATHNODES, sizeof( UftNode_pointer ) );
    pp = ( UftNode_pointer * ) calloc( lMAX_PATHNODES, sizeof( UftNode_pointer ) );
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      on_npath[i] = FALSE;
    }

    fc = FALSE;
  }

  /* set up n paths info
   */
  ft = n->ft;
  on_npath[ft] = TRUE;
  now_p = 0;
  p[0] = n;
  num_p = 1;
  for ( t = Utime; t >= 0; t-- ) {
    prev_p = now_p;
    now_p = num_p;
    for ( i = prev_p; (t > 0) ? (i < now_p) : (i < num_p); i++ ) {
      for ( j = 0; j < p[i]->num_in; j++ ) {
	/* see if we got this node already: avoid cycles!
	 */
	for ( k = 0; k < num_p; k++ ) {
	  if ( p[k] == p[i]->in_edges[j] ) break;
	}
	if ( k < num_p ) {
	  /* had this. skip it.
	   */
	  continue;
	}
	ft_ = p[i]->in_edges[j]->ft;
	on_npath[ft_] = TRUE;
	if ( num_p >= lMAX_PATHNODES ) {
	  printf("\n\n1 -- too many nodes on paths! increase lMAX_PATHNODES (now %d)\n\n",
		 lMAX_PATHNODES);
	  fflush(stdout);
	  exit( 1 );
	}
	p[num_p++] = p[i]->in_edges[j];
      }
    }
  }

  /* now check the npp reachability
   */
  ft = npp->ft;
  now_p = 0;
  pp[0] = npp;
  num_pp = 1;
  for ( t = Utime + 1; t >= 0; t-- ) {
    prev_p = now_p;
    now_p = num_pp;
    for ( i = prev_p; (t > 0) ? (i < now_p) : (i < num_pp); i++ ) {
      for ( j = 0; j < pp[i]->num_in; j++ ) {
	/* see if we got this node already: avoid cycles!
	 */
	for ( k = 0; k < num_pp; k++ ) {
	  if ( pp[k] == pp[i]->in_edges[j] ) break;
	}
	if ( k < num_pp ) {
	  /* had this. skip it.
	   */
	  continue;
	}
	ft_ = pp[i]->in_edges[j]->ft;
	if ( !on_npath[ft_] ) {
	  for ( i = 0; i < num_p; i++ ) {
	    on_npath[p[i]->ft] = FALSE;
	  }
	  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	    printf(" --- new reachable "); print_ft_name( ft_ );
	    fflush(stdout);
	  }
	  return TRUE;
	}
	if ( num_pp >= lMAX_PATHNODES ) {
	  printf("\n\n2 -- too many nodes on paths! increase lMAX_PATHNODES (now %d)\n\n",
		 lMAX_PATHNODES);
	  fflush(stdout);
	  exit( 1 );
	}
	pp[num_pp++] = pp[i]->in_edges[j];
      }
    }
  }

  for ( i = 0; i < num_p; i++ ) {
    on_npath[p[i]->ft] = FALSE;
  }

  return FALSE;

}




/* for all heuristics, the backchaining to determine the leafs 
 * is exactly the same -- it's the different impligraph that makes
 * the difference.
 */
Bool Uleaf_disjunction_implied_by_formula( int Utime, UftNode *n ) 

{

  static Bool fc = TRUE;
  static int *leafs;
  static Bool *is_leaf;
  static UftNode_pointer *p;
  static int *reachedfts;
  static Bool *reachedft;

  int num_p, prev_p, now_p, num_leafs;
  int i, j, k, t, ft, ft_;
  int num_reachedfts;

  if ( fc ) {
    leafs = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    is_leaf = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    p = ( UftNode_pointer * ) calloc( lMAX_PATHNODES, sizeof( UftNode_pointer ) );

    for ( i = 0; i < gnum_ft_conn; i++ ) {
      is_leaf[i] = FALSE;
    }

    reachedfts = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    reachedft = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      reachedft[i] = FALSE;
    }
    
    fc = FALSE;
  }

  num_reachedfts = 0;
  ft = n->ft;
  reachedft[ft] = TRUE;
  reachedfts[num_reachedfts++] = ft;
  p[0] = n;
  now_p = 0;
  num_p = 1;
  num_leafs = 0;
  if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
    printf("\nr kaka starting at Utime %d + 1", Utime); print_ft_name( ft );
    fflush(stdout);
  }
  for ( t = Utime + 1; t > 0; t-- ) {
    /* include all edges from t to t-1
     *
     * Mar04: NOTE -- HERE, WE GOT NO EDGES AT TIME 0 SO WE CAN SIMPLIFY THE
     * COMPUTATION
     */
    prev_p = now_p;
    now_p = num_p;
    for ( i = prev_p; i < now_p; i++ ) {
      /* loop over all incoming edges of the node.
       */
      if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
	printf("\nr i at t %d: ", t); print_ft_name( p[i]->ft );
	fflush(stdout);
      }
      for ( j = 0; j < p[i]->num_in; j++ ) {
	/* see if we got this node already: avoid cycles!
	 */
	for ( k = 0; k < num_p; k++ ) {
	  if ( p[k] == p[i]->in_edges[j] ) break;
	}
	if ( k < num_p ) {
	  /* had this. skip it.
	   */
	  continue;
	}
	if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
	  printf("\nr new p: "); print_ft_name( p[i]->in_edges[j]->ft );
	  printf(" (by "); 
	  if ( p[i]->in_efs[j] == -1 ) {
	    printf("NOOP ");
	  } else {
	    if ( p[i]->in_efs[j] == -2 ) {
	      printf("OR constraint\n\n");
	      fflush(stdout);
	      exit( 1 );
	    } else {
	      print_op_name( gef_conn[p[i]->in_efs[j]].op );
	    }
	  }
	  printf(")");
	  fflush(stdout);
	}
	ft_ = p[i]->in_edges[j]->ft;
	if ( !reachedft[ft_] ) {
	  if ( gft_conn[ft_].negation != -1 &&
	       reachedft[gft_conn[ft_].negation] ) {
	    /* reached a contradiction already!!
	     */
	    for ( i = 0; i < num_leafs; i++ ) {
	      is_leaf[leafs[i]] = FALSE;
	    }
	    for ( i = 0; i < num_reachedfts; i++ ) {
	      reachedft[reachedfts[i]] = FALSE;
	    }
	    return TRUE;
	  }
	  reachedft[ft_] = TRUE;
	  reachedfts[num_reachedfts++] = ft_;
	}
	if ( t == 1 && !is_leaf[ft_] ) {
	  is_leaf[ft_] = TRUE;
	  leafs[num_leafs++] = ft_;
	  if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
	    printf("\nr set is-l "); print_ft_name( ft_ );
	    fflush(stdout);
	  }
	}
	if ( num_p >= lMAX_PATHNODES ) {
	  printf("\n\nr 3 -- too many nodes on paths! increase lMAX_PATHNODES (now %d)\n\n",
		 lMAX_PATHNODES);
	  fflush(stdout);
	  exit( 1 );
	}
	p[num_p++] = p[i]->in_edges[j];
      } /* in edges of node p[i] */
    } /* all p[i] in prev level */
  } /* all levels from top to bottom */

  if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
    printf("\nr fact "); print_ft_name( ft );
    printf(" has leafs:");
    for ( i = 0; i < num_leafs; i++ ) {
      printf(" ");print_ft_name( leafs[i] );
    }
    printf("\nr now checking for implication by initial/state formula.");
    fflush(stdout);
  }


  if ( gcmd_line.heuristic == 0 ||
       gcmd_line.heuristic == 1 ) {
    if ( disjunction_implied_by_initial_formula( leafs, num_leafs, is_leaf ) ) {
      for ( i = 0; i < num_leafs; i++ ) {
	is_leaf[leafs[i]] = FALSE;
      }
      for ( i = 0; i < num_reachedfts; i++ ) {
	reachedft[reachedfts[i]] = FALSE;
      }
      return TRUE;
    }
  }
  if ( gcmd_line.heuristic == 2 ) {
    if ( disjunction_implied_by_S_formula( leafs, num_leafs, is_leaf ) ) {
      for ( i = 0; i < num_leafs; i++ ) {
	is_leaf[leafs[i]] = FALSE;
      }
      for ( i = 0; i < num_reachedfts; i++ ) {
	reachedft[reachedfts[i]] = FALSE;
      }
      return TRUE;
    }
  }

  for ( i = 0; i < num_leafs; i++ ) {
    is_leaf[leafs[i]] = FALSE;
  }
  for ( i = 0; i < num_reachedfts; i++ ) {
    reachedft[reachedfts[i]] = FALSE;
  }

  return FALSE;

}



Bool disjunction_implied_by_initial_formula( int *dis, int num_dis, Bool *is_dis )

{

  static int *cdis;
  static Bool fc = TRUE;

  int i, j, k, m;
  int num_cdis;
  Bool sat;

  if ( fc ) {
    cdis = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    fc = FALSE;
  }


  /* first, see if two of the leafs are contradictory
   * --> disjunction trivially implied, step out
   */
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].negation != -1 &&
	 is_dis[gft_conn[dis[i]].negation] ) {
      if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	printf("\ncontradictory ft pair in backchain leafs disjunction: ");
	print_ft_name(dis[i]);
	print_ft_name(gft_conn[dis[i]].negation);
	fflush(stdout);
      }
      return TRUE;
    }
  }


  /* create array of codes of these facts; use implicit
   * negated facts, ie. make these codes for coded literals as in the 
   * CNF.
   */
  num_cdis = 0;
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].CNF ) {
      m = 1;
      cdis[num_cdis] = dis[i];
    } else {
      m = -1;
      cdis[num_cdis] = gft_conn[dis[i]].negation;
    }
    if ( lr_codes[cdis[num_cdis]] == -1 ) {
      /* special case: this is not encoded and therefore does not occur
       * in our initial state CNF. thus it can't be implied and we can
       * skip it from the disjunction; we just need to check if there are two
       * contradictory literals that are thrown away, in which case the implication
       * is true trivially; this has been done above already.
       */
      if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
	printf("\nun-encoded ft/0 pair in backchain leafs disjunction: ");
	print_ft_name(cdis[num_cdis]);
	fflush(stdout);
      }
      continue;
    }
    /* we have the code, the literal occurs. write it into the disj. code array.
     */
    cdis[num_cdis] = m * lr_codes[cdis[num_cdis]];
    num_cdis++;
  }

  if ( num_cdis == 0 ) {
    if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
      printf("\nencoded disjunction empty, bailing out");
      fflush(stdout);
    }
    return FALSE;
  }


  /* first pre-check: is any of the observation restrictions
   * a member of dis?
   */
  for ( i = 0; i < lnum_obs_backpropagation; i++ ) {
    for ( k = 0; k < num_cdis; k++ ) {
      if ( lobs_backpropagation[i] == cdis[k] ) break;
    }
    if ( k < num_cdis ) {
      /* yes, this one
       */
      if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
	printf("\nobs backpropagation %d is contained in leafs disjunction: ",
	       lobs_backpropagation[i]);
	fflush(stdout);
      }
      return TRUE;
    }
  }

  /* another pre-check: is any ini-or, minus negated restrictions, contained in dis?
   *
   * NOTE: clauses are ordered by increasing length so
   * this way we find a smallest appropriate clause (well, not considering
   * effects of varying restriction sets...)
   */
  for ( i = 0; i < lr_num_clauses; i++ ) {
    /* for each ini or
     */
    for ( j = 0; j < lr_clause_length[i]; j++ ) {
      /* for each literal in it
       */
      for ( k = 0; k < lnum_obs_backpropagation; k++ ) {
	if ( lobs_backpropagation[k] == (-1) * lr_clauses[i][j].literal ) break;
      }
      if ( k < lnum_obs_backpropagation ) {
	/* negation of this is backpropagated ==> we can skip this
	 */
	if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
	  printf("\nskipping, in ini-or inclusion check, ini-or literal %d whose negation is backpropagated",
		 lr_clauses[i][j].literal);
	  fflush(stdout);
	}
	continue;
      }
      for ( k = 0; k < num_cdis; k++ ) {
	if ( lr_clauses[i][j].literal == cdis[k] ) break;
      }
      if ( k == num_cdis ) {
	/* this one's not here -> abort
	 */
	break;
      }
    }
    if ( j == lr_clause_length[i] ) {
      /* yes, this one is contained in leafs
       */
      if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
	printf("\ninitial or minus backprops is contained in leafs disjunction: ");
	print_r_clause( i );
	fflush(stdout);
      }
      return TRUE;
    }
  }



  /* with h 0, don't do SAT reasoning and simply assume that there is no
   * implication if none of the pre-checks holds.
   */
  if ( gcmd_line.heuristic == 0 ) {
    if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
      printf("\nno pre-check applies. bailing out due to h == 0.");
      fflush(stdout);
    }
    return FALSE;
  }



  /* nada yet --> with h 1, now for the real sat test.
   *
   * extend the fixed ini state encoding with singleton clauses containing
   * the negations of the elements of the disjunction.
   */
  if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
    printf("\nno pre-check applies. doing full sat test now.");
    fflush(stdout);
  }

  /* stack is initialized with obs restrictions plus negs of disj;
   * check for duplicates.
   */
  lr_num_decision_stack = 0;
  for ( i = 0; i < lnum_obs_backpropagation; i++ ) {
    lr_decision_stack[lr_num_decision_stack++] = lobs_backpropagation[i];
  }
  for ( i = 0; i < num_cdis; i++ ) {
    for ( j = 0; j < lr_num_decision_stack; j++ ) {
      if ( lr_decision_stack[j] == (-1) * cdis[i] ) break;
    }
    if ( j < lr_num_decision_stack ) continue;
    lr_decision_stack[lr_num_decision_stack++] = (-1) * cdis[i];
  }

  times( &end );
  TIME( geval_time );
  times( &start );
  gr_sat_calls++;
  sat = r_dp_CNF();
  times( &end );
  TIME( gr_sat_time );
  times( &start );
  if ( sat ) {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nr sat! no implication");
      fflush(stdout);
    }
  } else {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nr unsat! implication");
      fflush(stdout);
    }
    return TRUE;
  }

  return FALSE;

}



Bool disjunction_implied_by_S_formula( int *dis, int num_dis, Bool *is_dis )

{

  static int *cdis;
  static Bool fc = TRUE;

  int i, m;
  int num_cdis;
  Bool sat;

  if ( fc ) {
    cdis = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    fc = FALSE;
  }


  /* first, see if two of the leafs are contradictory
   * --> disjunction trivially implied, step out
   */
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].negation != -1 &&
	 is_dis[gft_conn[dis[i]].negation] ) {
      if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	printf("\ncontradictory ft pair in backchain leafs disjunction: ");
	print_ft_name(dis[i]);
	print_ft_name(gft_conn[dis[i]].negation);
	fflush(stdout);
      }
      return TRUE;
    }
  }

  if ( lStime < 0 || lStime > MAX_PLAN_LENGTH ) {
    printf("\n\nsomething wrong with lStime!\n\n"); fflush(stdout);
    exit( 1 );
  }


  /* create array of codes of these facts; use implicit
   * negated facts, ie. make these codes for coded literals as in the 
   * CNF.
   */
  num_cdis = 0;
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].CNF ) {
      m = 1;
      cdis[num_cdis] = dis[i];
    } else {
      m = -1;
      cdis[num_cdis] = gft_conn[dis[i]].negation;
    }
    if ( gcodes[lStime][cdis[num_cdis]] == -1 ) {
      /* special case: this is not encoded and therefore does not occur
       * in our initial state CNF. thus it can't be implied and we can
       * skip it from the disjunction; we just need to check if there are two
       * contradictory literals that are thrown away, in which case the implication
       * is true trivially; this has been done above already.
       */
      if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
	printf("\nun-encoded ft/0 pair in backchain leafs disjunction: ");
	print_ft_name(cdis[num_cdis]);
	fflush(stdout);
      }
      continue;
    }
    /* we have the code, the literal occurs. write it into the disj. code array.
     */
    cdis[num_cdis] = m * gcodes[lStime][cdis[num_cdis]];
    num_cdis++;
  }

  if ( num_cdis == 0 ) {
    if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
      printf("\nencoded disjunction empty, bailing out");
      fflush(stdout);
    }
    return FALSE;
  }

  /* no pre-check if any ini or is contained in dis:
   * this is bound to fail anyway.
   *
   * extend the S CNF encoding with singleton clauses containing
   * the negations of the elements of the disjunction.
   */
  if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
    printf("\ndoing full sat test for implication now.");
    fflush(stdout);
  }

  gnum_decision_stack = 0;
  for ( i = 0; i < num_cdis; i++ ) {
    gdecision_stack[gnum_decision_stack++] = (-1) * cdis[i];
  }

  times( &end );
  TIME( geval_time );
  times( &start );
  gr_sat_calls++;
  sat = dp_CNF();
  times( &end );
  TIME( gr_sat_time );
  times( &start );

  if ( sat ) {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nr sat! no implication");
      fflush(stdout);
    }
  } else {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nr unsat! implication");
      fflush(stdout);
    }
    return TRUE;
  }

  return FALSE;

}



void reset_fixpoint( int max )

{

  int i;

  for ( i = 0; i < lnum_F; i++ ) {
    gft_conn[lF[i]].level = INFINITY;
    gft_conn[lF[i]].in_F = FALSE;
  }

  for ( i = 0; i < l1num_ch_F; i++ ) {
    gft_conn[l1ch_F[i]].observed_level = INFINITY;
    gft_conn[l1ch_F[i]].ch1 = FALSE;
  }
  l1num_ch_F = 0;

  for ( i = 0; i < lnum_E; i++ ) {
    gef_conn[lE[i]].level = INFINITY;
    gef_conn[lE[i]].in_E = FALSE;
  }

  for ( i = 0; i < lnum_ch_E; i++ ) {
    gef_conn[lch_E[i]].num_active_PCs = 0;
    gef_conn[lch_E[i]].ch = FALSE;
  }

  for ( i = 0; i < lnum_ch_O; i++ ) {
    gop_conn[lch_O[i]].num_active_Ps = 0;
    gop_conn[lch_O[i]].ch = FALSE;
    gop_conn[lch_O[i]].level = INFINITY;
  }


  if ( gcmd_line.heuristic == 2 ) {
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
	printf("\nin rpg reset, var %d has no dummy nodes in mem list yet\n\n", i);
	exit( 1 );
      }
      if ( gpos_c_in_clause_fixed[i] != gpos_c_in_clause_start[i]->next ||
	   gneg_c_in_clause_fixed[i] != gneg_c_in_clause_start[i]->next ) {
	printf("\nin rpg reset, for new dyn. code %d fixed != start->next\n\n", i);
	exit( 1 );
      }
      gpos_c_in_clause_end[i] = gpos_c_in_clause_start[i]->next;
      gneg_c_in_clause_end[i] = gneg_c_in_clause_start[i]->next;
    }
  }

  for ( i = 0; i < RELAXED_STEPS + MAX_PLAN_LENGTH; i++ ) {
    lnum_U[i] = 0;
  }

}



Bool all_goals_activated( int time ) 

{

  int i;

  for ( i = 0; i < ggoal_state.num_F; i++ ) {
    if ( !gft_conn[ggoal_state.F[i]].in_F ) {
      return FALSE;
    }
  }

  for ( i = 0; i < ggoal_state.num_F; i++ ) {
    if ( gft_conn[ggoal_state.F[i]].level == INFINITY ) {
      gft_conn[ggoal_state.F[i]].level = time;
    }
  }

  return TRUE;

}



void print_fixpoint_result( void )

{

  int time, i, j, Utime;
  Bool hit, hit_F, hit_E, hit_U, hit_O, hit_oF;

  printf("\n\n----------------------------relaxed plan graph");

  Utime = 0;
  while ( Utime < lpath_U_length - 1 ) {
    printf("\n\nU-LEVEL %d (ie Utime %d):", Utime - lpath_U_length + 1, Utime );
    for ( i = 0; i < lnum_U[Utime]; i++ ) {
      printf("\n");
      print_ft_name( lU[Utime][i].ft );
      if ( lU[Utime][i].became_F ) printf(" became F");
      printf(", edges to: ");
      for ( j = 0; j < lU[Utime][i].num_in; j++ ) {
	print_ft_name( lU[Utime][i].in_edges[j]->ft );
	printf(" (by "); 
	if ( lU[Utime][i].in_efs[j] == -1 ) {
	  printf("NOOP ");
	} else {
	  if ( lU[Utime][i].in_efs[j] == -2 ) {
	    printf("OR constraint");
	  } else {
	    print_op_name( gef_conn[lU[Utime][i].in_efs[j]].op );
	  }
	}
	printf("), ");
      }
    }
    Utime++;
  }

  time = 0;
  while ( 1 ) {
    hit = FALSE;
    hit_F = FALSE;
    hit_oF = FALSE;
    hit_U = FALSE;
    hit_E = FALSE;
    hit_O = FALSE;
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      if ( gft_conn[i].level == time ) {
	hit = TRUE;
	hit_F = TRUE;
	break;
      }
    }
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      if ( gft_conn[i].observed_level == time ) {
	hit = TRUE;
	hit_oF = TRUE;
	break;
      }
    }
    for ( i = 0; i < gnum_ef_conn; i++ ) {
      if ( gef_conn[i].level == time ) {
	hit = TRUE;
	hit_E = TRUE;
	break;
      }
    }
    for ( i = 0; i < gnum_op_conn; i++ ) {
      if ( gop_conn[i].level == time ) {
	hit = TRUE;
	hit_O = TRUE;
	break;
      }
    }
    if ( lnum_U[Utime] ) {
      hit_U = TRUE;
      hit = TRUE;
    }

    if ( !hit ) {
      break;
    }
 
    printf("\n\nLEVEL %d:", time);
    if ( hit_F ) {
      printf("\n\nFACTS:");
      for ( i = 0; i < gnum_ft_conn; i++ ) {
	if ( gft_conn[i].level == time ) {
	  printf("\n");
	  print_ft_name( i );
	}
      }
    }
    if ( hit_oF ) {
      printf("\n\nOBSERVED FACTS:");
      for ( i = 0; i < gnum_ft_conn; i++ ) {
	if ( gft_conn[i].observed_level == time ) {
	  printf("\n");
	  print_ft_name( i );
	}
      }
    }
    if ( hit_U ) {
      printf("\n\nUNKNOWN FACTS (Utime %d):", Utime);
      for ( i = 0; i < lnum_U[Utime]; i++ ) {
	printf("\n");
	print_ft_name( lU[Utime][i].ft );
	if ( lU[Utime][i].became_F ) printf(" became F");
	if ( lU[Utime][i].observed ) printf(" observed");
	printf(", edges to: ");
	for ( j = 0; j < lU[Utime][i].num_in; j++ ) {
	  print_ft_name( lU[Utime][i].in_edges[j]->ft );
	  printf(" (by "); 
	  if ( lU[Utime][i].in_efs[j] == -1 ) {
	    printf("NOOP ");
	  } else {
	    if ( lU[Utime][i].in_efs[j] == -2 ) {
	      printf("OR constraint");
	    } else {
	      print_op_name( gef_conn[lU[Utime][i].in_efs[j]].op );
	    }
	  }
	  printf("), ");
	}
      }
    }
    if ( hit_O ) {
      printf("\n\nOPS:");
      for ( i = 0; i < gnum_op_conn; i++ ) {
	if ( gop_conn[i].level == time ) {
	  printf("\nop %d: ", i);
	  print_op_name( i );
	}
      }
    }
    if ( hit_E ) {
      printf("\n\nEFS:");
      for ( i = 0; i < gnum_ef_conn; i++ ) {
	if ( gef_conn[i].level == time ) {
	  printf("\neffect %d to ", i);
	  print_op_name( gef_conn[i].op );
	}
      }
    }

    time++;
    Utime++;
  }
  fflush( stdout );

}
    



































/**************************************
 * FIRST RELAXED PLAN (1P) EXTRACTION *
 **************************************/









































long extract_1P( int max, Bool H_info )

{

  static Bool first_call = TRUE;
  int i, j, max_goal_level, time;

  if ( first_call ) {
    lgoals_at = ( int ** ) calloc( RELAXED_STEPS, sizeof( int * ) );
    lnum_goals_at = ( int * ) calloc( RELAXED_STEPS, sizeof( int ) );
    logoals_at = ( int ** ) calloc( RELAXED_STEPS, sizeof( int * ) );
    lnum_ogoals_at = ( int * ) calloc( RELAXED_STEPS, sizeof( int ) );
    lused_O_at = ( int ** ) calloc( RELAXED_STEPS, sizeof( int * ) );
    lnum_used_O_at = ( int * ) calloc( RELAXED_STEPS, sizeof( int ) );
    for ( i = 0; i < RELAXED_STEPS; i++ ) {
      lgoals_at[i] = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
      logoals_at[i] = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
      lused_O_at[i] = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    }
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      for ( j = 0; j < RELAXED_STEPS; j++ ) {
	gft_conn[i].is_goal_at[j] = FALSE;
	gft_conn[i].is_ogoal_at[j] = FALSE;
	gft_conn[i].is_goal_for_at[j] = -1;
	gft_conn[i].is_ogoal_for_at[j] = -1;
	gft_conn[i].is_goal_fortime_at[j] = -1;
	gft_conn[i].is_ogoal_fortime_at[j] = -1;
      }
      gft_conn[i].is_true = INFINITY;
      gft_conn[i].is_observed = INFINITY;
      gft_conn[i].ch = FALSE;
    }
    for ( i = 0; i < gnum_op_conn; i++ ) {
      for ( j = 0; j < RELAXED_STEPS; j++ ) {
	gop_conn[i].is_used_at[j] = FALSE;
	gop_conn[i].is_used_for_at[j] = -1;
	gop_conn[i].is_used_fortime_at[j] = -1;
      }
    }
    lch_F = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    lnum_ch_F = 0;
    first_call = FALSE;
  }

  reset_search_info();

  if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
    printf("\n\n----------------------------------------relaxed plan");
    fflush(stdout);
  }

  /* < infty due to previous checks
   */ 
  max_goal_level = initialize_goals( max );
  /* DEBUGGING, REMOVE LATER
   */
  if ( max_goal_level == INFINITY ) {
    printf("\n\ntrying to find rplan for infty goals?\n\n");
    fflush(stdout);
    exit( 1 );
  }

  /* part of the H info --- the path selected ops at time 0, namely ---
   * we are collecting along the way so we already initialze the stuff here.
   */
  for ( i = 0; i < gnum_H; i++ ) {
    gop_conn[gH[i]].is_in_H = FALSE;
  }
  gnum_H = 0;

  lh = 0;
  for ( time = max_goal_level; time > 0; time-- ) {
    achieve_goals( time );
    achieve_ogoals( time );
  }
  if ( H_info ) {
    collect_H_info();
  }

  if ( gcmd_line.observe_h > 0 ) {
    account_for_observation_dependencies();
  }

  return lh;

}



int initialize_goals( int max )

{

  int i, max_goal_level, ft;

  if ( max + 1 == RELAXED_STEPS ) {
    printf("\n\nRELAXED_STEPS too small due to max %d\n\n", max);
    fflush(stdout);
    exit( 1 );
  }

  for ( i = 0; i < max + 1; i++ ) {
    lnum_goals_at[i] = 0;
    lnum_ogoals_at[i] = 0;
    lnum_used_O_at[i] = 0;
  }

  max_goal_level = 0;
  for ( i = 0; i < ggoal_state.num_F; i++ ) {
    ft = ggoal_state.F[i];
    if ( gft_conn[ft].level == INFINITY ) {
      return INFINITY;
    }
    if ( gft_conn[ft].level > max_goal_level ) {
      max_goal_level = gft_conn[ft].level;
    }
    lgoals_at[gft_conn[ft].level][lnum_goals_at[gft_conn[ft].level]++] = ft;
    if ( gft_conn[ft].level >= RELAXED_STEPS ) {
      printf("\n\nRELAXED_STEPS too small due to ft->is_goal_at array, time %d\n\n",
	     gft_conn[ft].level);
      fflush(stdout);
      exit( 1 );
    }
    gft_conn[ft].is_goal_at[gft_conn[ft].level] = TRUE;
    if ( !gft_conn[ft].ch ) {
      lch_F[lnum_ch_F++] = ft;
      gft_conn[ft].ch = TRUE;
    }
  }

  return max_goal_level;

}



void achieve_goals( int time )
{

  int i, j, k, ft, min_p, min_e, ef, p, op, Utime;

  Utime =  time + lpath_U_length - 1;

  for ( i = 0; i < lnum_goals_at[time]; i++ ) {
    ft = lgoals_at[time][i];

    if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\nworking on goal ");
      print_ft_name( ft );
      printf(" at time %d", time);
      fflush(stdout);
    }

    if ( gft_conn[ft].is_true != INFINITY &&
	 gft_conn[ft].is_true <= time  ) {
      /* fact already true here.
       */
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	printf(" -- TRUE already!");
	fflush(stdout);
      }
      continue;
    }

    min_p = INFINITY;
    min_e = -1;
    for ( j = 0; j < gft_conn[ft].num_A; j++ ) {
      if ( gft_conn[ft].A_nondet[j] ) {
	/* no nondet achievers!
	 */
	continue;
      }
      ef = gft_conn[ft].A[j];
      if ( gef_conn[ef].level != time - 1 ) continue; 
      p = 0;
      for ( k = 0; k < gef_conn[ef].num_PC; k++ ) {
	p += gft_conn[gef_conn[ef].PC[k]].level;
      }
      if ( LESS( p, min_p ) ) {
	min_p = p;
	min_e = ef;
      }
    
    }
    /* has fact become true due to complementary unknown paths?
     * if yes, then select the ops on these paths!
     */
    if ( min_e == -1 ) {
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	printf("\nno real achiever. going to paths.");
	fflush(stdout);
      }
      for ( j = 0; j < lnum_U[Utime]; j++ ) {
	if ( lU[Utime][j].ft == ft ) break;
      }
      /* DEBUGGING; REMOVE LATER
       */
      if ( j == lnum_U[Utime] ) {
	printf("\n\nno ef in for subg ft but not in U either at %d U %d\n\n", time, Utime);
	fflush(stdout);
	exit( 1 );
      }
      if (!lU[Utime][j].became_F ) {
	printf("\n\nno ef in for subg ft but not became F either at %d U %d\n\n", time, Utime);
	fflush(stdout);
	exit( 1 );
      }
      select_implied_incoming_paths( time, &(lU[Utime][j]) );

      continue;
    } /* min_e == -1 */

    if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\nnormal. going to efs.");
      fflush(stdout);
    }
    /* else, proceed as normal.
     */
    if ( time - 1 >= RELAXED_STEPS ) {
      printf("\n\nRELAXED_STEPS too small due to op->is_used_at array, time %d\n\n",
	     time - 1);
      fflush(stdout);
      exit( 1 );
    }
    ef = min_e;
    op = gef_conn[ef].op;
    if ( !gop_conn[op].is_used_at[time-1] ) {
      lused_O_at[time-1][lnum_used_O_at[time-1]++] = op;
      gop_conn[op].is_used_at[time-1] = TRUE;
      if ( ft == -1 ) {
	printf("\n\nft -1 at point 1\n\n");
	exit( 1 );
      }
      gop_conn[op].is_used_for_at[time-1] = ft;
      gop_conn[op].is_used_fortime_at[time-1] = time;
      lh++;
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	printf("\nnormal selecting at step %3d: ", time-1);
	print_op_name( op );
	fflush(stdout);
      }
    } else {
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	printf("\nop used already at this time step. do not select it the 2nd time.");
	fflush(stdout);
      }
    }

    introduce_ef_PC_and_A( time, ef, -1 );
  } /* for all goals at time */

}



void achieve_ogoals( int time )

{

  int i, j, ft, op, optime;

  for ( i = 0; i < lnum_ogoals_at[time]; i++ ) {
    ft = logoals_at[time][i];

    if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\nworking on observation goal ");
      print_ft_name( ft );
      printf(" at time %d", time);
      fflush(stdout);
    }

    if ( gft_conn[ft].is_observed != INFINITY &&
	 gft_conn[ft].is_observed <= time  ) {
      /* fact already true here.
       */
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	printf(" -- observed already!");
	fflush(stdout);
      }
      continue;
    }

    op = -1;
    for ( j = 0; j < gft_conn[ft].num_O; j++ ) {
      op = gft_conn[ft].O[j];
      if ( gop_conn[op].level != INFINITY &&
	   gop_conn[op].level < time ) break; 
    }
    if ( j == gft_conn[ft].num_O ) {
      printf("\n1P: no observation op with required level\n\n");
      exit( 1 );
    }
    if ( op == -1 ) {
      printf("\n1P: no observation op at all\n\n");
      exit( 1 );
    }
    /* optime is the level of the ogoal when observed by the op
     */
    optime = gop_conn[op].level + 1;

    /* else, proceed as normal.
     */
    if ( optime >= RELAXED_STEPS ) {
      printf("\n\nRELAXED_STEPS too small due to op->is_used_at array, time %d\n\n",
	     optime);
      fflush(stdout);
      exit( 1 );
    }
    if ( !gop_conn[op].is_used_at[optime-1] ) {
      lused_O_at[optime-1][lnum_used_O_at[optime-1]++] = op;
      gop_conn[op].is_used_at[optime-1] = TRUE;
      if ( ft == -1 ) {
	printf("\n\nft -1 at point 2\n\n");
	exit( 1 );
      }
      gop_conn[op].is_used_for_at[optime-1] = ft;
      gop_conn[op].is_used_fortime_at[optime-1] = time;
      lh++;
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	printf("\nobservation selecting at step %3d: ", optime-1);
	print_op_name( op );
	fflush(stdout);
      }
    } else {
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	printf("\nobservation op used already at this time step. do not select it the 2nd time.");
	fflush(stdout);
      }
    }

    introduce_ef_PC_and_A( optime, -1, op );
  } /* for all ogoals at time */

}



void introduce_ef_PC_and_A( int time, int ef, int op )

{

  int i, j, k, ft, ef1;

  if ( ef != -1 ) {
    for ( j = 0; j < gef_conn[ef].num_PC; j++ ) {
      ft = gef_conn[ef].PC[j];
      if ( gft_conn[ft].level >= RELAXED_STEPS ) {
	printf("\n\nRELAXED_STEPS too small due to ft->is_goal_at array, time %d\n\n",
	       gft_conn[ft].level);
	fflush(stdout);
	exit( 1 );
      }
      if ( gft_conn[ft].level == INFINITY ||
	   gft_conn[ft].level > time - 1 ) {
	printf("\nPC fact from achievegoals not in P?\n\n");
	exit( 1 );
      }
      if ( gft_conn[ft].is_true != INFINITY &&
	   gft_conn[ft].is_true <= time - 1 ) {
	/* already true here.
	 */
	continue;
      }
      if ( gft_conn[ft].is_goal_at[gft_conn[ft].level] ) {
	/* this fact already is a goal at the point where we want to make it a goal.
	 */
	continue;
      }
      lgoals_at[gft_conn[ft].level][lnum_goals_at[gft_conn[ft].level]++] = ft;
      gft_conn[ft].is_goal_at[gft_conn[ft].level] = TRUE;
      gft_conn[ft].is_goal_for_at[gft_conn[ft].level] = gef_conn[ef].op;
      gft_conn[ft].is_goal_fortime_at[gft_conn[ft].level] = time-1;
      if ( !gft_conn[ft].ch ) {
	lch_F[lnum_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
    }
    
    for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
      ft = gef_conn[ef].A[j];
      if ( gft_conn[ft].is_true == INFINITY ||
	   gft_conn[ft].is_true > time ) {
	gft_conn[ft].is_true = time;
      }
      if ( !gft_conn[ft].ch ) {
	lch_F[lnum_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
    }
    for ( j = 0; j < gef_conn[ef].num_I; j++ ) {
      for ( k = 0; k < gef_conn[gef_conn[ef].I[j]].num_A; k++ ) {
	ft = gef_conn[gef_conn[ef].I[j]].A[k];
	if ( gft_conn[ft].is_true == INFINITY ||
	     gft_conn[ft].is_true > time - 1 ) {
	  gft_conn[ft].is_true = time - 1;
	}
	if ( !gft_conn[ft].ch ) {
	  lch_F[lnum_ch_F++] = ft;
	  gft_conn[ft].ch = TRUE;
	}
      }
    }

    /* this already handles the case where ops can observe and have effects
     */
    op = gef_conn[ef].op;
    for ( j = 0; j < gop_conn[op].num_O; j++ ) {
      ft = gop_conn[op].O[j];
      if ( gft_conn[ft].is_observed == INFINITY ||
	   gft_conn[ft].is_observed > time ) {
	gft_conn[ft].is_observed = time;
      }
      if ( !gft_conn[ft].ch ) {
	lch_F[lnum_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
    }

    return;
  } /* effect was given, ie called from achieve_goals */


  if ( op < 0 ) {
    printf("\nno op ?\n\n");
    exit( 1 );
  }

  for ( j = 0; j < gop_conn[op].num_P; j++ ) {
    ft = gop_conn[op].P[j];
    if ( gft_conn[ft].level >= RELAXED_STEPS ) {
      printf("\n\nRELAXED_STEPS too small due to ft->is_goal_at array, time %d\n\n",
	     gft_conn[ft].level);
      fflush(stdout);
      exit( 1 );
    }
    if ( gft_conn[ft].level == INFINITY ||
	 gft_conn[ft].level > time - 1 ) {
      /* must be in oP!
       */
      if ( gft_conn[ft].observed_level == INFINITY ||
	   gft_conn[ft].observed_level > time - 1 ) {
	printf("\nPC fact from achieve-ogoals neither in P nor in oP?\n\n");
	exit( 1 );
      }
      if ( gft_conn[ft].is_observed != INFINITY &&
	   gft_conn[ft].is_observed <= time - 1 ) {
	/* already observed here.
	 */
	continue;
      }
      if ( gft_conn[ft].is_ogoal_at[gft_conn[ft].observed_level] ) {
	/* this fact already is an ogoal at the point where we want to observe it.
	 */
	continue;
      }
      logoals_at[gft_conn[ft].observed_level][lnum_ogoals_at[gft_conn[ft].observed_level]++] = ft;
      gft_conn[ft].is_ogoal_at[gft_conn[ft].observed_level] = TRUE;
      gft_conn[ft].is_ogoal_for_at[gft_conn[ft].observed_level] = op;
      gft_conn[ft].is_ogoal_fortime_at[gft_conn[ft].observed_level] = time-1;
      if ( !gft_conn[ft].ch ) {
	lch_F[lnum_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
      continue;
    }
    /* in P!
     */
    if ( gft_conn[ft].is_true != INFINITY &&
	 gft_conn[ft].is_true <= time - 1 ) {
      /* already true here.
       */
      continue;
    }
    if ( gft_conn[ft].is_goal_at[gft_conn[ft].level] ) {
      /* this fact already is a goal at the point where we want to make it a goal.
       */
      continue;
    }
    lgoals_at[gft_conn[ft].level][lnum_goals_at[gft_conn[ft].level]++] = ft;
    gft_conn[ft].is_goal_at[gft_conn[ft].level] = TRUE;
    gft_conn[ft].is_goal_for_at[gft_conn[ft].level] = op;
    gft_conn[ft].is_goal_for_at[gft_conn[ft].level] = time-1;
    if ( !gft_conn[ft].ch ) {
      lch_F[lnum_ch_F++] = ft;
      gft_conn[ft].ch = TRUE;
    }
  }

  /* this already handles the case where ops can observe and have effects
   */
  for ( i = 0; i < gop_conn[op].num_E; i++ ) {
    ef1 = gop_conn[op].E[i];
    if ( gef_conn[ef1].num_C > 0 ) continue;

    for ( j = 0; j < gef_conn[ef1].num_A; j++ ) {
      ft = gef_conn[ef1].A[j];
      if ( gft_conn[ft].is_true == INFINITY ||
	   gft_conn[ft].is_true > time ) {
	gft_conn[ft].is_true = time;
      }
      if ( !gft_conn[ft].ch ) {
	lch_F[lnum_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
    }
  }
  for ( j = 0; j < gop_conn[op].num_O; j++ ) {
    ft = gop_conn[op].O[j];
    if ( gft_conn[ft].is_observed == INFINITY ||
	 gft_conn[ft].is_observed > time ) {
      gft_conn[ft].is_observed = time;
    }
    if ( !gft_conn[ft].ch ) {
      lch_F[lnum_ch_F++] = ft;
      gft_conn[ft].ch = TRUE;
    }
  }

}



void pathselect( int Utime, int ef, int ft, int fttime )

{

  int op, time, i;

  time = Utime - lpath_U_length + 1;

  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
    printf("\npath select call Utime %d (ie time %d) ef %d", Utime, time, ef);
    fflush(stdout);
  }
  if ( ef == -1 ) {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nskipping path select of noop constr.");
      fflush(stdout);
    }
    return;
  }
  if ( ef == -2 ) {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nskipping path select of OR constr.");
      fflush(stdout);
    }
    return;
  }
      
  if ( time <= 0 ) {
    if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
      printf("\nskipping path select at time %d", time - 1);
      print_op_name( gef_conn[ef].op );
      fflush(stdout);
    }
    return;
  }
  op = gef_conn[ef].op;
  if ( !gop_conn[op].is_used_at[time-1] ) {
    lused_O_at[time-1][lnum_used_O_at[time-1]++] = op;
    gop_conn[op].is_used_at[time-1] = TRUE;
    if ( ft == -1 ) {
      printf("\n\nft -1 at point 3\n\n");
      exit( 1 );
    }
    gop_conn[op].is_used_for_at[time-1] = ft;
    gop_conn[op].is_used_fortime_at[time-1] = fttime;
    lh++;
    if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\npath selecting at step %3d: ", time-1);
      print_op_name( op );
      fflush(stdout);
    }
    /* path ops are helpful -- if they are applicable!!
     */
    if ( !gop_conn[op].is_in_H ) {
      for ( i = 0; i < gop_conn[op].num_P; i++ ) {
	if ( gft_conn[gop_conn[op].P[i]].level != 0 ) break;
      }
      if ( i == gop_conn[op].num_P ) {
	gop_conn[op].is_in_H = TRUE;
	gH[gnum_H++] = op;
	if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug) ) {
	  printf("\nhelpful action from path time %d: ", time-1);
	  print_op_name( op );
	  fflush(stdout);
	}
      }
    }
  }

  introduce_path_ef_PC_and_A( time, ef );

}



void introduce_path_ef_PC_and_A( int time, int ef )

{

  int i, j, ft, Utime;

  Utime =  time + lpath_U_length - 1;

  for ( j = 0; j < gef_conn[ef].num_PC; j++ ) {
    ft = gef_conn[ef].PC[j];
    if ( gft_conn[ft].is_true != INFINITY &&
	 gft_conn[ft].is_true <= time - 1 ) {
      /* already true here.
       */
      continue;
    }
    if ( gft_conn[ft].level != INFINITY &&
	 gft_conn[ft].level < time ) {
      /* only enforce pathconds to be true if they can be enforced,
       * ie. if their level is below the current goal level.
       *
       * THIS MAINTAINS CORRECTNESS OF RPLAN!
       */
      if ( gcmd_line.display_info == 123  || (gcmd_line.R && gcmd_line.debug >= 2) ) {
	printf("\nintroducing path goal ");
	print_ft_name( ft );
	printf(" at time %d", gft_conn[ft].level);
	fflush(stdout);
      }
      if ( gft_conn[ft].level >= RELAXED_STEPS ) {
	printf("\n\nRELAXED_STEPS too small due to ft->is_goal_at array, time %d\n\n",
	       gft_conn[ft].level);
	fflush(stdout);
	exit( 1 );
      }
      if ( gft_conn[ft].is_goal_at[gft_conn[ft].level] ) {
	/* this fact already is a goal at the point where we want to make it a goal.
	 */
	continue;
      }
      lgoals_at[gft_conn[ft].level][lnum_goals_at[gft_conn[ft].level]++] = ft;
      gft_conn[ft].is_goal_at[gft_conn[ft].level] = TRUE;
      gft_conn[ft].is_goal_for_at[gft_conn[ft].level] = gef_conn[ef].op;
      gft_conn[ft].is_goal_fortime_at[gft_conn[ft].level] = time-1;
      if ( !gft_conn[ft].ch ) {
	lch_F[lnum_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
    } else {/* fact is not known at the effcond level */
      /* DEBUGGING! REMOVE THAT LATER
       */
      for ( i = 0; i < lnum_U[Utime-1]; i++ ) {
	if ( lU[Utime-1][i].ft == ft ) break;
      }
      if ( i == lnum_U[Utime-1] ) {
	printf("\n\nprec ft for path ef neither in nor U at time %d -1, U %d\n\n", 
	       time, Utime);
	fflush(stdout);
	exit( 1 );
      }
      /* if it's in P, it must be in oP!
       */
      for ( i = 0; i < gop_conn[gef_conn[ef].op].num_P; i++ ) {
	if ( ft == gop_conn[gef_conn[ef].op].P[i] ) break;
      }
      if ( i < gop_conn[gef_conn[ef].op].num_P ) {
	if ( gft_conn[ft].observed_level == INFINITY ||
	     gft_conn[ft].observed_level > time - 1 ) {
	  printf("\nP fact from pathselect neither in P nor in oP?\n\n");
	  exit( 1 );
	}
	if ( gft_conn[ft].is_observed != INFINITY &&
	     gft_conn[ft].is_observed <= time - 1 ) {
	  /* already observed here.
	   */
	  continue;
	}
	if ( gft_conn[ft].is_ogoal_at[gft_conn[ft].observed_level] ) {
	  /* this fact already is an ogoal at the point where we want to observe it.
	   */
	  continue;
	}
	logoals_at[gft_conn[ft].observed_level][lnum_ogoals_at[gft_conn[ft].observed_level]++] = ft;
	gft_conn[ft].is_ogoal_at[gft_conn[ft].observed_level] = TRUE;
	gft_conn[ft].is_ogoal_for_at[gft_conn[ft].observed_level] = gef_conn[ef].op;
	gft_conn[ft].is_ogoal_fortime_at[gft_conn[ft].observed_level] = time-1;
	if ( !gft_conn[ft].ch ) {
	  lch_F[lnum_ch_F++] = ft;
	  gft_conn[ft].ch = TRUE;
	}
      } /* it's a precond */
    } /* it's not in P at the ef time */
  }/* for all eff pre+conds */

  /* actually, this effect will only in part of our paths be executed (?!)
   * so we can and do not mark its add effects as true!
   */

}



void collect_H_info( void )

{

  static Bool first_call = TRUE;
  static int *H, num_H, *D;
  int i, j, k, ft, ef, op, d;

  if ( first_call ) {
    H = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    D = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    num_H = 0;
    first_call = FALSE;
  }

  /* gnum_H and the is in H info have been initialized at 
   * rplan start already!
   */

  num_H = 0;
  for ( i = 0; i < lnum_goals_at[1]; i++ ) {
    ft = lgoals_at[1][i];
    for ( j = 0; j < gft_conn[ft].num_A; j++ ) {
      if ( gft_conn[ft].A_nondet[j] ) {
	/* no nondet achievers!
	 */
	continue;
      }
      ef = gft_conn[ft].A[j];
      if ( gef_conn[ef].level != 0 ) {
	continue;
      }
      op = gef_conn[ef].op;
      if ( gop_conn[op].is_in_H ) {
	continue;
      }
      gop_conn[op].is_in_H = TRUE;
      H[num_H++] = op;
    }
  }
  for ( i = 0; i < lnum_ogoals_at[1]; i++ ) {
    ft = logoals_at[1][i];
    for ( j = 0; j < gft_conn[ft].num_O; j++ ) {
      op = gft_conn[ft].O[j];
      if ( gop_conn[op].level != 0 ) {
	continue;
      }
      if ( gop_conn[op].is_in_H ) {
	continue;
      }
      gop_conn[op].is_in_H = TRUE;
      H[num_H++] = op;
    }
  }

  /* H collected; now order it; just a stupid ad-hoc optimisation
   * that worked well in (classical) Logistics etc.
   *
   *       count number subgoal at 0 or 1 facts that
   *       op deletes (with level 0 effects). order less deletes
   *       before more deletes.
   *       start from back of H, to prefer down under
   *       goals to upper goals.
   */
  for ( i = num_H - 1; i > -1; i-- ) {
    d = 0;
    for ( j = 0; j < gop_conn[H[i]].num_E; j++ ) {
      ef = gop_conn[H[i]].E[j];
      if ( gef_conn[ef].level != 0 ) continue;
      for ( k = 0; k < gef_conn[ef].num_D; k++ ) {
	if ( gft_conn[gef_conn[ef].D[k]].is_goal_at[0] ||
	     gft_conn[gef_conn[ef].D[k]].is_goal_at[1] ) d++;
      }
    }
    for ( j = 0; j < gnum_H; j++ ) {
      if ( D[j] > d ) break;
    }
    for ( k = gnum_H; k > j; k-- ) {
      gH[k] = gH[k-1];
      D[k] = D[k-1];
    }
    gH[j] = H[i];
    D[j] = d;
    gnum_H++;
  }
  if ( gcmd_line.display_info == 124 || (gcmd_line.R && gcmd_line.debug) ) {
    printf("\n--------------------------------- collected H: ");
    for ( i = 0; i < gnum_H; i++ ) {
      print_op_name( gH[i] );
      printf("\n              ");
    }
    fflush(stdout);
  }

}



void reset_search_info( void )

{

  int i, j;

  for ( i = 0; i < lnum_ch_F; i++ ) {
    for ( j = 0; j < RELAXED_STEPS; j++ ) {
      gft_conn[lch_F[i]].is_goal_at[j] = FALSE;
      gft_conn[lch_F[i]].is_ogoal_at[j] = FALSE;
      gft_conn[lch_F[i]].is_goal_for_at[j] = -1;
      gft_conn[lch_F[i]].is_ogoal_for_at[j] = -1;
      gft_conn[lch_F[i]].is_goal_fortime_at[j] = -1;
      gft_conn[lch_F[i]].is_ogoal_fortime_at[j] = -1;
    }
    gft_conn[lch_F[i]].is_true = INFINITY;
    gft_conn[lch_F[i]].is_observed = INFINITY;
    gft_conn[lch_F[i]].ch = FALSE;
  }
  lnum_ch_F = 0;

  for ( i = 0; i < RELAXED_STEPS; i++ ) {
    for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
      gop_conn[lused_O_at[i][j]].is_used_at[i] = FALSE;
      gop_conn[lused_O_at[i][j]].is_used_for_at[i] = -1;
      gop_conn[lused_O_at[i][j]].is_used_fortime_at[i] = -1;
    }
    lnum_used_O_at[i] = 0;
  }
  
}



void select_implied_incoming_paths( int time, UftNode *n ) 

{

  static Bool fc = TRUE;
  static int *leafs, *min_leafs;
  static Bool *is_leaf;
  static UftNode_pointer *p;
  static int *pfather, *pef, *pt, *leafp;
  static int *reachedfts, *rftp;
  static Bool *reachedft;

  int num_p, prev_p, now_p, num_leafs, num_min_leafs;
  int i, j, k, t, ft, ft_;
  int Utime;
  int num_reachedfts;

  if ( fc ) {
    leafs = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    min_leafs = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    is_leaf = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    p = ( UftNode_pointer * ) calloc( lMAX_PATHNODES, sizeof( UftNode_pointer ) );
    pfather = ( int * ) calloc( lMAX_PATHNODES, sizeof( int ) );
    pef = ( int * ) calloc( lMAX_PATHNODES, sizeof( int ) );
    pt = ( int * ) calloc( lMAX_PATHNODES, sizeof( int ) );
    leafp = ( int * ) calloc( lMAX_PATHNODES, sizeof( int ) );

    for ( i = 0; i < gnum_ft_conn; i++ ) {
      is_leaf[i] = FALSE;
    }

    reachedfts = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    rftp = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    reachedft = ( Bool * ) calloc( gnum_ft_conn, sizeof( Bool ) );
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      reachedft[i] = FALSE;
    }

    fc = FALSE;
  }

  Utime =  time + lpath_U_length - 1;

  num_reachedfts = 0;
  ft = n->ft;
  reachedft[ft] = TRUE;
  rftp[ft] = 0;
  reachedfts[num_reachedfts++] = ft;
  p[0] = n;
  pfather[0] = -1;
  pef[0] = -1;
  pt[0] = Utime;
  now_p = 0;
  num_p = 1;
  num_leafs = 0;
  if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
    printf("\nstarting "); print_ft_name( ft );
    fflush(stdout);
  }
  for ( t = Utime; t > 0; t-- ) {
    /* include all edges from t to t-1
     *
     * Mar04: NOTE -- HERE, WE GOT NO EDGES AT TIME 0 SO WE CAN SIMPLIFY THE
     * COMPUTATION
     */
    prev_p = now_p;
    now_p = num_p;
    for ( i = prev_p; i < now_p; i++ ) {
      /* loop over all incoming edges of the node.
       */
      if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
	printf("\nrp i at t %d: ", t); print_ft_name( p[i]->ft );
	fflush(stdout);
      }
      /* NOTE: this is "NOOPs-first", as the NOOPs are the first edges inserted
       *       into this list by the functions that build the implication
       *       graph!
       *
       *       NOOPs-first is good: it makes the rplan extraction try to apply
       *       real actions as far down in the RPG/the impli graph as possible,
       *       in particular if actions already applied on the path to s can help,
       *       then they will be used!
       */
      for ( j = 0; j < p[i]->num_in; j++ ) {
	/* see if we got this node already: avoid cycles!
	 */
	for ( k = 0; k < num_p; k++ ) {
	  if ( p[k] == p[i]->in_edges[j] ) break;
	}
	if ( k < num_p ) {
	  /* had this. skip it.
	   */
	  continue;
	}
	if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
	  printf("\nrp new p: "); print_ft_name( p[i]->in_edges[j]->ft );
	  printf(" (by "); 
	  if ( p[i]->in_efs[j] == -1 ) {
	    printf("NOOP ");
	  } else {
	    if ( p[i]->in_efs[j] == -2 ) {
	      printf("OR constraint\n\n");
	      fflush(stdout);
	      exit( 1 );
	    } else {
	      print_op_name( gef_conn[p[i]->in_efs[j]].op );
	    }
	  }
	  printf(")");
	  fflush(stdout);
	}
	if ( num_p >= lMAX_PATHNODES ) {
	  printf("\n\nrp 3 -- too many nodes on paths! increase lMAX_PATHNODES (now %d)\n\n",
		 lMAX_PATHNODES);
	  fflush(stdout);
	  exit( 1 );
	}
	p[num_p] = p[i]->in_edges[j];
	pfather[num_p] = i;
	pt[num_p] = pt[i] - 1;
	pef[num_p++] = p[i]->in_efs[j];
	ft_ = p[i]->in_edges[j]->ft;
	if ( !reachedft[ft_] ) {
	  if ( gft_conn[ft_].negation != -1 &&
	       reachedft[gft_conn[ft_].negation] ) {
	    /* reached a contradiction already!!
	     * select actions ont he paths, and bail out!
	     * first, path to ft
	     */
	    for ( j = num_p-1; pfather[j] != -1; j = pfather[j] ) {
	      pathselect( pt[j] + 1, pef[j], ft, time );
	    }
	    /* now, path to negft
	     */
	    for ( j = rftp[gft_conn[ft_].negation]; pfather[j] != -1; j = pfather[j] ) {
	      pathselect( pt[j] + 1, pef[j], ft, time );
	    }
	    for ( i = 0; i < num_leafs; i++ ) {
	      is_leaf[leafs[i]] = FALSE;
	    }
	    for ( i = 0; i < num_reachedfts; i++ ) {
	      reachedft[reachedfts[i]] = FALSE;
	    }
	    return;
	  }
	  reachedft[ft_] = TRUE;  
	  rftp[ft_] = num_p-1;
	  reachedfts[num_reachedfts++] = ft_;
	}
	if ( t == 1 && !is_leaf[ft_] ) {
	  is_leaf[ft_] = TRUE;
	  leafs[num_leafs] = ft_;
	  leafp[num_leafs++] = num_p-1;
	  if ( gcmd_line.R && gcmd_line.debug  >= 7 ) {
	    printf("\nrp set is-l "); print_ft_name( ft_ );
	    fflush(stdout);
	  }
	}
      }
    }
  }

  if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
    printf("\nrp fact "); print_ft_name( ft );
    printf(" has leafs:");
    for ( i = 0; i < num_leafs; i++ ) {
      printf(" ");print_ft_name( leafs[i] );
    }
    printf("\nrp now checking for implication by initial/state formula.");
    fflush(stdout);
  }

  if ( gcmd_line.heuristic == 0 ||
       gcmd_line.heuristic == 1 ) {
    get_minimal_disjunction_implied_by_initial_formula( leafs, num_leafs, is_leaf,
							&(min_leafs), &(num_min_leafs), ft );
  }
  if ( gcmd_line.heuristic == 2 ) {
    get_minimal_disjunction_implied_by_state_formula( leafs, num_leafs, is_leaf,
						      &(min_leafs), &(num_min_leafs), ft );
  }

  for ( i = 0; i < num_leafs; i++ ) {
    is_leaf[leafs[i]] = FALSE;
  }
  for ( i = 0; i < num_reachedfts; i++ ) {
    reachedft[reachedfts[i]] = FALSE;
  }



  /* select the paths to the min_leafs
   */
  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
    printf("\nrp selecting minimal implied disj.: ");
    for ( i = 0; i < num_min_leafs; i++ ) {
      print_ft_name( leafs[min_leafs[i]] ); printf(" ");
    }
    fflush(stdout);
  }
  for ( i = 0; i < num_min_leafs; i++ ) {
    if ( leafs[min_leafs[i]] == ft ) {
      /* to ft, we have a path of noops; but this path might not have been
       * selected by backwards reasoning; we simply skip ft, to the same effect
       * as if we'd selected the noop edges leading to it.
       */
      continue;
    }
    for ( j = leafp[min_leafs[i]]; pfather[j] != -1; j = pfather[j] ) {
      pathselect( pt[j] + 1, pef[j], ft, time );
    }
  }


}



/* **mindis and *num_min_dis are the outputs of the fn -- an array of ints and the nr
 * of its elements, which at fn end contains a minimal implied subdisjunction in the
 * form of its indices in the dis array.
 */
void get_minimal_disjunction_implied_by_initial_formula( int *dis, int num_dis, Bool *is_dis,
							 int **min_dis, int *num_min_dis, int ft )

{

  static int *cdis, *cdis_to_dis;
  /* cdis can become a subset of dis; we need to remember, in cdis_to_dis,
   * what this subset is.
   */
  static Bool fc = TRUE;

  int i, j, k, m;
  int num_cdis;
  Bool sat;
  int curr_test_index;

  if ( fc ) {
    cdis = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    cdis_to_dis = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    fc = FALSE;
  }


  /* first, see if two of the leafs are contradictory
   */
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].negation != -1 &&
	 is_dis[gft_conn[dis[i]].negation] ) {
      if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	printf("\nrp contradictory ft pair in backchain leafs disjunction: ");
	print_ft_name(dis[i]);
	print_ft_name(gft_conn[dis[i]].negation);
	fflush(stdout);
      }
      (*min_dis)[0] = i;
      for ( k = 0; k < num_dis; k++ ) {
	if ( gft_conn[dis[i]].negation == dis[k] ) break;
      }
      if ( k == num_dis || k == i ) {
	printf("\nnegated fact not where it ought to be\n\n"); fflush(stdout);
	exit( 1 );
      }
      (*min_dis)[1] = k;
      (*num_min_dis) = 2;
      return;
    }
  }


  /* create array of codes of these facts; use implicit
   * negated facts, ie. make these codes for coded literals as in the 
   * CNF.
   */
  num_cdis = 0;
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].CNF ) {
      m = 1;
      cdis[num_cdis] = dis[i];
    } else {
      m = -1;
      cdis[num_cdis] = gft_conn[dis[i]].negation;
    }
    if ( lr_codes[cdis[num_cdis]] == -1 ) {
      /* special case: this is not encoded and therefore does not occur
       * in our initial state CNF. thus it can't be implied and we can
       * skip it from the disjunction; we just need to check if there are two
       * contradictory literals that are thrown away, in which case the implication
       * is true trivially; this has been done above already.
       */
      if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
	printf("\nrp un-encoded ft/0 pair in backchain leafs disjunction: ");
	print_ft_name(cdis[num_cdis]);
	fflush(stdout);
      }
      continue;
    }
    /* we have the code, the literal occurs. write it into the disj. code array.
     */
    cdis[num_cdis] = m * lr_codes[cdis[num_cdis]];
    cdis_to_dis[num_cdis] = i;
    num_cdis++;
  }

  if ( num_cdis == 0 ) {
    printf("\nrp encoded disjunction empty in rp extraction\n\n");
    fflush(stdout);
    exit( 1 );
  }
  if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
    printf("\nrp encoded disjunction: ");
    for ( i = 0; i < num_cdis; i++ ) {
      if ( cdis[i] < 0 ) {
	printf("-"); print_ft_name( lr_cf[(-1)*cdis[i]] ); 
      } else {
	print_ft_name( lr_cf[cdis[i]] ); 
      }
    }
    fflush(stdout);
  }



  /* first pre-check: is any of the observation restrictions
   * a member of dis?
   */
  for ( i = 0; i < lnum_obs_backpropagation; i++ ) {
    for ( k = 0; k < num_cdis; k++ ) {
      if ( lobs_backpropagation[i] == cdis[k] ) break;
    }
    if ( k < num_cdis ) {
      /* yes, this one
       */
      if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
	printf("\nobs backpropagation %d is contained in leafs disjunction: ",
	       lobs_backpropagation[i]);
	fflush(stdout);
      }
      /* record it into the min_dis array.
       */
      (*num_min_dis) = 1;
      (*min_dis)[0] = cdis_to_dis[k];
      return;
    }
  }

  /* another pre-check: is any ini-or, minus negated restrictions, contained in dis?
   *
   * NOTE: clauses are ordered by increasing length so
   * this way we find a smallest appropriate clause (well, not considering
   * effects of varying restriction sets...)
   */
  for ( i = 0; i < lr_num_clauses; i++ ) {
    /* for each ini or
     */
    for ( j = 0; j < lr_clause_length[i]; j++ ) {
      /* for each literal in it
       */
      for ( k = 0; k < lnum_obs_backpropagation; k++ ) {
	if ( lobs_backpropagation[k] == (-1) * lr_clauses[i][j].literal ) break;
      }
      if ( k < lnum_obs_backpropagation ) {
	/* negation of this is backpropagated ==> we can skip this
	 */
	if ( gcmd_line.R && gcmd_line.debug >= 1 ) {
	  printf("\nskipping, in plan extraction ini-or inclusion check, ini-or literal %d whose negation is backpropagated",
		 lr_clauses[i][j].literal);
	  fflush(stdout);
	}
	continue;
      }
      for ( k = 0; k < num_cdis; k++ ) {
	if ( lr_clauses[i][j].literal == cdis[k] ) break;
      }
      if ( k == num_cdis ) {
	/* this one's not here -> abort
	 */
	break;
      }
    }
    if ( j == lr_clause_length[i] ) {
      /* yes, this one is contained in leafs
       */
      if ( gcmd_line.R && gcmd_line.debug  >= 1 ) {
	printf("\nrp extract initial-or minus backprops is contained in leafs disjunction: ");
	print_r_clause( i );
	fflush(stdout);
      }
      /* take the clause as the minimal implied disjunction; ie, collect the
       * appropriate indices in the dis array.
       */
      (*num_min_dis) = 0; 
      for ( j = 0; j < lr_clause_length[i]; j++ ) {
	for ( k = 0; k < lnum_obs_backpropagation; k++ ) {
	  if ( lobs_backpropagation[k] == (-1) * lr_clauses[i][j].literal ) break;
	}
	if ( k < lnum_obs_backpropagation ) continue;
	for ( k = 0; k < num_cdis; k++ ) {
	  if ( lr_clauses[i][j].literal == cdis[k] ) break;
	}
	(*min_dis)[(*num_min_dis)++] = cdis_to_dis[k];
      }
      return;
    }
  } /* all fixed clauses, ie ini ors */


  
  /* if h == 0, and we get here, then something is wrong!!
   */
  if ( gcmd_line.heuristic == 0 ) {
    printf("\n\nno pre-check applies in h==0 - inferred ft leafs implication check?!\n\n");
    fflush(stdout);
    exit(1);
  }


  /* nada yet --> now do the real sat test, ie skip variables until
   * a minimal set remains.
   *
   * obviously (?) it is hard to find a smallest implied sub-disjunction.
   * we therefore go with a greedy minimisation, removing vars in some given 
   * fixed order. yet no heuristics at all on the order, we just proceed 
   * arbitrarily, ie from front to back in our array.
   *
   * some debugging stuff first...
   */
  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
    printf("\nrp no pre-check applies. doing full sat minimisation now.");
    fflush(stdout);
  }
  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
    printf("\ndoing debugging sat test");
    /* stack is initialized with obs restrictions plus negs of disj;
     * check for duplicates.
     */
    lr_num_decision_stack = 0;
    for ( i = 0; i < lnum_obs_backpropagation; i++ ) {
      lr_decision_stack[lr_num_decision_stack++] = lobs_backpropagation[i];
    }
    for ( i = 0; i < num_cdis; i++ ) {
      for ( j = 0; j < lr_num_decision_stack; j++ ) {
	if ( lr_decision_stack[j] == (-1) * cdis[i] ) break;
      }
      if ( j < lr_num_decision_stack ) continue;
      lr_decision_stack[lr_num_decision_stack++] = (-1) * cdis[i];
    }
    print_r_clauses();
    times( &end );
    TIME( geval_time );
    sat = r_dp_CNF();
    if ( sat ) {
      printf("\ndisj in full sat minimisation is not implied\n\n");
      fflush(stdout);
      exit( 1 );
    }    
  }


  /* and now for the actual code...
   */
  if ( num_cdis > 1 ) {
    /* else, nothing to minimise
     */
    curr_test_index = 0;
    while ( curr_test_index < num_cdis ) {
      if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	printf("\nr now testing ");
	if ( cdis[curr_test_index] < 0 ) {
	  printf("-"); print_ft_name( gcf[(-1)*cdis[curr_test_index]] ); 
	  printf("(%d)", lStime);
	} else {
	  print_ft_name( gcf[cdis[curr_test_index]] ); 
	  printf("(%d)", lStime);
	}
	fflush(stdout);
      }
      /* for the fact ft in question itself, we will select a noop path
       * anyway, see above, so we want to keep it in.
       */
      if ( cdis[curr_test_index] < 0 ) {
	if ( lr_cf[(-1)*cdis[curr_test_index]] == ft ) {
	  curr_test_index++;
	  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	    printf(" --- target, take NOOPs");
	    fflush(stdout);
	  }
	  continue;
	} 
      } else {
	if ( lr_cf[cdis[curr_test_index]] == ft ) {
	  curr_test_index++;
	  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	    printf(" --- target, take NOOPs");
	    fflush(stdout);
	  }
	  continue;
	} 
      }
      /* stack is initialized with obs restrictions plus negs of disj;
       * check for duplicates.
       */
      lr_num_decision_stack = 0;
      for ( i = 0; i < lnum_obs_backpropagation; i++ ) {
	lr_decision_stack[lr_num_decision_stack++] = lobs_backpropagation[i];
      }
      for ( i = 0; i < num_cdis; i++ ) {
	if ( i == curr_test_index ) {
	  /* skip this literal
	   */
	  continue;
	}
	for ( j = 0; j < lr_num_decision_stack; j++ ) {
	  if ( lr_decision_stack[j] == (-1) * cdis[i] ) break;
	}
	if ( j < lr_num_decision_stack ) continue;
	lr_decision_stack[lr_num_decision_stack++] = (-1) * cdis[i];
      }
      /* test if this is still implied
       */
      times( &end );
      TIME( geval_time );
      times( &start );
      grp_sat_calls++;
      sat = r_dp_CNF();
      times( &end );
      TIME( grp_sat_time );
      times( &start );
      if ( sat ) {
	if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	  printf(" rp sat! no implication, keeping literal");
	  fflush(stdout);
	}
	curr_test_index++;
      } else {
	if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	  printf(" rp unsat! implication, removing literal");
	  fflush(stdout);
	}
	/* statistics
	 */
	gremoved_lits++;
	for ( i = curr_test_index; i < num_cdis - 1; i++ ) {
	  cdis[i] = cdis[i+1];
	  cdis_to_dis[i] = cdis_to_dis[i+1];/* keep arrays synchronised */
	}
	num_cdis--;
      }
    } /* while curr test index < num cdis */
  } /* if num cdis > 1 */


  /* now cdis_to_dis contains exactly the minimal implied disj.
   * that we want to have. copy over.
   */
  for ( i = 0; i < num_cdis; i++ ) {
    (*min_dis)[i] = cdis_to_dis[i];
  }
  (*num_min_dis) = num_cdis;

}



/* as above, but with S formula.
 */
void get_minimal_disjunction_implied_by_state_formula( int *dis, int num_dis, Bool *is_dis,
						       int **min_dis, int *num_min_dis, int ft )

{

  static int *cdis, *cdis_to_dis;
  /* cdis can become a subset of dis; we need to remember, in cdis_to_dis,
   * what this subset is.
   */
  static Bool fc = TRUE;

  int i, k, m;
  int num_cdis;
  Bool sat;
  int curr_test_index;

  if ( fc ) {
    cdis = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    cdis_to_dis = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    fc = FALSE;
  }


  /* first, see if two of the leafs are contradictory
   */
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].negation != -1 &&
	 is_dis[gft_conn[dis[i]].negation] ) {
      if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	printf("\nrp contradictory ft pair in backchain leafs disjunction: ");
	print_ft_name(dis[i]);
	print_ft_name(gft_conn[dis[i]].negation);
	fflush(stdout);
      }
      (*min_dis)[0] = i;
      for ( k = 0; k < num_dis; k++ ) {
	if ( gft_conn[dis[i]].negation == dis[k] ) break;
      }
      if ( k == num_dis || k == i ) {
	printf("\nnegated fact not where it ought to be\n\n"); fflush(stdout);
	exit( 1 );
      }
      (*min_dis)[1] = k;
      (*num_min_dis) = 2;
      return;
    }
  }



  if ( lStime < 0 || lStime > MAX_PLAN_LENGTH ) {
    printf("\n\nsomething wrong with lStime!\n\n"); fflush(stdout);
    exit( 1 );
  }


  /* create array of codes of these facts; use implicit
   * negated facts, ie. make these codes for coded literals as in the 
   * CNF.
   */
  num_cdis = 0;
  for ( i = 0; i < num_dis; i++ ) {
    if ( gft_conn[dis[i]].CNF ) {
      m = 1;
      cdis[num_cdis] = dis[i];
    } else {
      m = -1;
      cdis[num_cdis] = gft_conn[dis[i]].negation;
    }
    if ( gcodes[lStime][cdis[num_cdis]] == -1 ) {
      /* special case: this is not encoded and therefore does not occur
       * in our initial state CNF. thus it can't be implied and we can
       * skip it from the disjunction; we just need to check if there are two
       * contradictory literals that are thrown away, in which case the implication
       * is true trivially; this has been done above already.
       */
      if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
	printf("\nun-encoded ft/0 pair in backchain leafs disjunction: ");
	print_ft_name(cdis[num_cdis]);
	fflush(stdout);
      }
      continue;
    }
    /* we have the code, the literal occurs. write it into the disj. code array.
     */
    cdis[num_cdis] = m * gcodes[lStime][cdis[num_cdis]];
    cdis_to_dis[num_cdis] = i;
    num_cdis++;
  }

  if ( num_cdis == 0 ) {
    printf("\nrp encoded disjunction empty in rp extraction\n\n");
    fflush(stdout);
    exit( 1 );
  }

  if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
    printf("\nrp encoded disjunction: ");
    for ( i = 0; i < num_cdis; i++ ) {
      if ( cdis[i] < 0 ) {
	printf("-"); print_ft_name( gcf[(-1)*cdis[i]] ); 
	printf("(%d)", lStime);
      } else {
	print_ft_name( gcf[cdis[i]] ); 
	printf("(%d)", lStime);
      }
    }
    fflush(stdout);
  }

  /* no pre-check if any ini or is contained in dis:
   * this is bound to fail anyway.
   *
   * extend the S CNF encoding with singleton clauses containing
   * the negations of the elements of the disjunction.
   */
  if ( gcmd_line.R && gcmd_line.debug  >= 2 ) {
    printf("\ndoing full sat test for implication now.");
    fflush(stdout);
  }

  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
    printf("\ndoing debugging sat test");
    gnum_decision_stack = 0;
    for ( i = 0; i < num_cdis; i++ ) {
      gdecision_stack[gnum_decision_stack++] = (-1) * cdis[i];
    }
    sat = dp_CNF();
    if ( sat ) {
      printf("\ndisj in full sat minimisation is not implied\n\n");
      fflush(stdout);
      exit( 1 );
    } else {
      printf(" --- ok.");
    }
  }


  /* and now for the actual code...
   */
  if ( num_cdis > 1 ) {
    /* else, nothing to minimise
     */
    curr_test_index = 0;
    while ( curr_test_index < num_cdis ) {
      if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	printf("\nr now testing ");
	if ( cdis[curr_test_index] < 0 ) {
	  printf("-"); print_ft_name( gcf[(-1)*cdis[curr_test_index]] ); 
	  printf("(%d)", lStime);
	} else {
	  print_ft_name( gcf[cdis[curr_test_index]] ); 
	  printf("(%d)", lStime);
	}
	fflush(stdout);
      }
      /* for the fact ft in question itself, we will select a noop path
       * anyway, see above, so we want to keep it in.
       */
      if ( cdis[curr_test_index] < 0 ) {
	if ( gcf[(-1)*cdis[curr_test_index]] == ft ) {
	  curr_test_index++;
	  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	    printf(" --- target, take NOOPs");
	    fflush(stdout);
	  }
	  continue;
	} 
      } else {
	if ( gcf[cdis[curr_test_index]] == ft ) {
	  curr_test_index++;
	  if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	    printf(" --- target, take NOOPs");
	    fflush(stdout);
	  }
	  continue;
	} 
      }
      gnum_decision_stack = 0;
      for ( i = 0; i < num_cdis; i++ ) {
	if ( i == curr_test_index ) {
	  /* skip this literal
	   */
	  continue;
	}
	gdecision_stack[gnum_decision_stack++] = (-1) * cdis[i];
      }
      /* test if this is still implied
       */
      times( &end );
      TIME( geval_time );
      times( &start );
      grp_sat_calls++;
      sat = dp_CNF();
      times( &end );
      TIME( grp_sat_time );
      times( &start );
      if ( sat ) {
	if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	  printf(" rp sat! no implication, keeping literal");
	  fflush(stdout);
	}
	curr_test_index++;
      } else {
	if ( gcmd_line.R && gcmd_line.debug >= 2 ) {
	  printf(" rp unsat! implication, removing literal");
	  fflush(stdout);
	}
	/* statistics
	 */
	gremoved_lits++;
	for ( i = curr_test_index; i < num_cdis - 1; i++ ) {
	  cdis[i] = cdis[i+1];
	  cdis_to_dis[i] = cdis_to_dis[i+1];/* keep arrays synchronised */
	}
	num_cdis--;
      }
    } /* while curr test index < num cdis */
  } /* if num cdis > 1 */
    

  /* now cdis_to_dis contains exactly the minimal implied disj.
   * that we want to have. copy over.
   */
  for ( i = 0; i < num_cdis; i++ ) {
    (*min_dis)[i] = cdis_to_dis[i];
  }
  (*num_min_dis) = num_cdis;

}


































/**************************************************************
 * "take account" of observation dependencies in relaxed plan...
 * beware: this is very crude and a terrible hack..!!
 * just to have something that somehow takes account of
 * multiple action use
 **************************************************************/

























long **lrpop_obsweight;
long **lrpop_weight;










void account_for_observation_dependencies( void )

{

  static int ***op_depends_on;
  static int **num_op_depends_on;
  static Bool fc = TRUE;
  static int MAXDEP = 10, MAXRPLAN = 1000;
  static long *rplan, *rplanusedfor, **rplandep, *num_rplandep, *rplanweight;

  int i, j, ft, op, k, op2, o2ind, l, r, fttime;
  int lowft, lowfttime, highft, highfttime, newop, newoptime, observedft;
  int num_rplan;
  int relaxed_steps;
  

  if ( fc ) {
    if ( gcmd_line.observe_h == 1 ||
	 gcmd_line.observe_h == 2 ) {
      op_depends_on = ( int *** ) calloc( RELAXED_STEPS, sizeof( int ** ) );
      num_op_depends_on = ( int ** ) calloc( RELAXED_STEPS, sizeof( int * ) );
      for ( i = 0; i < RELAXED_STEPS; i++ ) {
	op_depends_on[i] = ( int ** ) calloc( gnum_op_conn, sizeof( int * ) );
	num_op_depends_on[i] = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
	for ( j = 0; j < gnum_op_conn; j++ ) {
	  op_depends_on[i][j] = ( int * ) calloc( MAXDEP, sizeof( int ) );
	}
      }
      rplan = ( long * ) calloc( MAXRPLAN, sizeof( long ) );
      rplanusedfor = ( long * ) calloc( MAXRPLAN, sizeof( long ) );
      rplanweight = ( long * ) calloc( MAXRPLAN, sizeof( long ) );
      rplandep = ( long ** ) calloc( MAXRPLAN, sizeof( long * ) );
      num_rplandep = ( long * ) calloc( MAXRPLAN, sizeof( long * ) );
      for ( i = 0; i < MAXRPLAN; i++ ) {
	rplandep[i] = ( long * ) calloc( MAXDEP, sizeof( long ) );
      }    
    }
    if ( gcmd_line.observe_h == 3 ||
	 gcmd_line.observe_h == 4 ||
	 gcmd_line.observe_h == 5 ) {
      lrpop_obsweight = ( long ** ) calloc( RELAXED_STEPS, sizeof( long * ) );
      lrpop_weight = ( long ** ) calloc( RELAXED_STEPS, sizeof( long * ) );
      for ( i = 0; i < RELAXED_STEPS; i++ ) {
	lrpop_obsweight[i] = ( long * ) calloc( gnum_op_conn, sizeof( long ) );
	lrpop_weight[i] = ( long * ) calloc( gnum_op_conn, sizeof( long ) );
      }
    }

    fc = FALSE;
  }

  if ( gcmd_line.observe_h == 1 ||
       gcmd_line.observe_h == 2 ) {
    if ( gcmd_line.display_info == 128  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\n\ncollected information as follows:");
      for ( i = 0; i < RELAXED_STEPS; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  op = lused_O_at[i][j];
	  printf("\n\n"); print_op_name( op );
	  printf("\nused at %d for ", i); 
	  ft = gop_conn[op].is_used_for_at[i];
	  fttime = gop_conn[op].is_used_fortime_at[i];
	  if ( ft == -1 ) {
	    printf("\nused op not used for ft in rplan??\n\n");
	    exit( 1 );
	  }
	  print_ft_name( ft );
	  printf(" at %d,\nwhich is goal for ", fttime);
	  op2 = gft_conn[ft].is_goal_for_at[fttime];
	  if ( op2 == -1 ) {
	    printf("-1");
	  } else {
	    print_op_name( op2 );
	  }
	  printf(" at %d,\nand ogoal for ", gft_conn[ft].is_goal_fortime_at[fttime]);
	  op = gft_conn[ft].is_ogoal_for_at[fttime];
	  if ( op == -1 ) {
	    printf("-1");
	  } else {
	    print_op_name( op );
	  }	
	  printf(" at %d", gft_conn[ft].is_ogoal_fortime_at[fttime]);
	}
      }
      printf("\ninformation done\n\n");
      fflush(stdout);
    }
    
    
    /* collect the dependencies.
     */
    for ( i = 0; i < RELAXED_STEPS; i++ ) {
      for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	num_op_depends_on[i][j] = 0;
      }
    }
    /* a "simple" nesting of loops... propagation a bit tricky due to 
     * gaps in the rplan logics...
     */
    for ( i = 0; i < RELAXED_STEPS; i++ ) {
      for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	op = lused_O_at[i][j];
	if ( !gop_conn[op].observation ) continue;
	/* this one is an observation. propagate the dependencies of
	 * the ft it is used for trough the rplan.
	 */
	observedft = gop_conn[op].is_used_for_at[i];
	if ( observedft == -1 ) {
	  printf("\nused observation op not used for ft in rplan??\n\n");
	  exit( 1 );
	}
	lowft = observedft;
	lowfttime = gop_conn[op].is_used_fortime_at[i];
	if ( 0 ) {
	  printf("\nlowft ");
	  print_ft_name( lowft );
	  printf(" at %d", lowfttime);
	}
	while ( TRUE ) {
	  newop = gft_conn[lowft].is_ogoal_for_at[lowfttime];
	  newoptime = gft_conn[lowft].is_ogoal_fortime_at[lowfttime];
	  if ( newop == -1 ) {
	    newop = gft_conn[lowft].is_goal_for_at[lowfttime];
	    newoptime = gft_conn[lowft].is_goal_fortime_at[lowfttime];
	    if ( newop == -1 ) {
	      break;
	    }
	  }
	  if ( 0 ) {
	    printf("\nnewop at %d: ", newoptime);
	    print_op_name( newop );
	  }
	  highft = gop_conn[newop].is_used_for_at[newoptime];
	  highfttime = gop_conn[newop].is_used_fortime_at[newoptime];
	  if ( 0 ) {
	    printf("\nhighft at %d: ", highfttime);
	    print_ft_name( highft );
	  }
	  
	  /* insert observedft into dep set of newop at newoptime
	   * first, find op2 in array
	   */
	  for ( o2ind = 0; o2ind < lnum_used_O_at[newoptime]; o2ind++ ) {
	    if ( lused_O_at[newoptime][o2ind] == newop ) break;
	  }
	  if ( o2ind == lnum_used_O_at[newoptime] ) {
	    printf("\n\ndidn't find newop in used set at newoptime?\n\n");
	    exit( 1 );
	  }
	  /* no duplicates!
	   */
	  for ( l = 0; l < num_op_depends_on[newoptime][o2ind]; l++ ) {
	    if ( op_depends_on[newoptime][o2ind][l] == observedft ) break;
	  }
	  if ( l < num_op_depends_on[newoptime][o2ind] ) {
	    lowft = highft;
	    lowfttime= highfttime;
	    continue;
	  }
	  /* if depends on ft and -ft, remove both
	   */
	  if ( gft_conn[observedft].negation != -1 ) {
	    for ( l = 0; l < num_op_depends_on[newoptime][o2ind]; l++ ) {
	      if ( op_depends_on[newoptime][o2ind][l] == gft_conn[observedft].negation ) break;
	    }
	    if ( l < num_op_depends_on[newoptime][o2ind] ) {
	      /* remove the negation, and continue
	       */
	      for ( r = l; r < num_op_depends_on[newoptime][o2ind] - 1; r++ ) {
		op_depends_on[newoptime][o2ind][r] = op_depends_on[newoptime][o2ind][r+1];
	      }
	      num_op_depends_on[newoptime][o2ind]--;
	      lowft = highft;
	      lowfttime= highfttime;
	      continue;
	    }
	  }
	  /* none of the tests applied, neither ft nor its negation is in here.
	   * insert observedft!
	   */
	  if ( num_op_depends_on[newoptime][o2ind] == MAXDEP ) {
	    printf("\n\nincrease MAXDEP!\n\n");
	    exit( 1 );
	  }
	  op_depends_on[newoptime][o2ind][num_op_depends_on[newoptime][o2ind]++] = observedft;
	  lowft = highft;
	  lowfttime= highfttime;
	} /* lowft, highft, etc. step over dependency chain */
      } /* j used ops at i */
    } /* i relaxed steps */
    
    
    if ( gcmd_line.display_info == 128  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\n\ncollected observation dependencies as follows:");
      for ( i = 0; i < RELAXED_STEPS; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  printf("\n"); print_op_name( lused_O_at[i][j] );
	  printf(" at %d depends on ", i);
	  for ( k = 0; k < num_op_depends_on[i][j]; k++ ) {
	    print_ft_name( op_depends_on[i][j][k] );
	  }
	}
      }
      printf("\n\n");
      fflush(stdout);
    }
    
    
    /* now collect the used ops into a sequence, ordering observations
     * at the end within steps
     */
    num_rplan = 0;
    for ( i = 0; i < RELAXED_STEPS; i++ ) {
      for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	op = lused_O_at[i][j];
	if ( gop_conn[op].observation ) continue;
	if ( num_rplan == MAXRPLAN ) {
	  printf("\n\nincrease MAXRPLAN\n\n");
	  exit( 1 );
	}
	for ( k = 0; k < num_op_depends_on[i][j]; k++ ) {
	  rplandep[num_rplan][k] = op_depends_on[i][j][k];
	}
	num_rplandep[num_rplan] = num_op_depends_on[i][j];
	rplan[num_rplan] = op;
	rplanusedfor[num_rplan] = gop_conn[op].is_used_for_at[i];
	num_rplan++;
      }
      for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	op = lused_O_at[i][j];
	if ( !gop_conn[op].observation ) continue;
	if ( num_rplan == MAXRPLAN ) {
	  printf("\n\nincrease MAXRPLAN\n\n");
	  exit( 1 );
	}
	for ( k = 0; k < num_op_depends_on[i][j]; k++ ) {
	  rplandep[num_rplan][k] = op_depends_on[i][j][k];
	}
	num_rplandep[num_rplan] = num_op_depends_on[i][j];
	rplan[num_rplan] = op;
	rplanusedfor[num_rplan] = gop_conn[op].is_used_for_at[i];
	num_rplan++;
      }
    }
    
    if ( gcmd_line.display_info == 128  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\n\nsequentialised rplan as follows:");
      for ( i = 0; i < num_rplan; i++ ) {
	printf("\n"); print_op_name( rplan[i] );
	printf(" used for "); 
	print_ft_name( rplanusedfor[i] );
	printf(", depends on ");
	for ( j = 0; j < num_rplandep[i]; j++ ) {
	  print_ft_name( rplandep[i][j] );
	  printf(" ");
	}
      }
      printf("\n\n");
      fflush(stdout);
    }
    
    if ( gcmd_line.observe_h == 2 ) {
      /* multiply the weights, and sum up to get overall h value
       * `(to store in lh);
       */
      for ( i = 0; i < num_rplan; i++ ) {
	rplanweight[i] = 1;
      }
      for ( i = num_rplan-1; i >= 0; i-- ) {
	op = rplan[i];
	if ( !gop_conn[op].observation ) continue;
	/* multiply all later weights with 2 where the action at that point does not
	 * depend on the observation which op is used for
	 */
	ft = rplanusedfor[i];
	for ( j = i+1; j < num_rplan; j++ ) {
	  for ( k = 0; k < num_rplandep[j]; k++ ) {
	    if ( rplandep[j][k] == ft ) break;
	  }
	  if ( k < num_rplandep[j] ) continue;
	  rplanweight[j] *= 2;
	}
      }
      lh = 0;
      for ( i = 0; i < num_rplan; i++ ) {
	lh += rplanweight[i];
      }
    }
    if ( gcmd_line.observe_h == 1 ) {
      lh = 0;
      for ( i = 0; i < num_rplan; i++ ) {
	if ( num_rplandep[i] == 0 ) {
	  lh++;
	} else {
	  lh += num_rplandep[i];
	}
      }
    }
    
    
    if ( gcmd_line.display_info == 128  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\n\nweighted rplan as follows:");
      for ( i = 0; i < num_rplan; i++ ) {
	printf("\n"); print_op_name( rplan[i] );
	printf(" weight %ld", rplanweight[i]);
      }
      printf("\nSUM: %ld\n\n", lh);
      fflush(stdout);
      exit( 1 );
    }
  }



  /* different approach: simply take dependencies in resulting parallel rplan,
   * irrespective of if they were intended or no.
   */
  if ( gcmd_line.observe_h == 3 ||
       gcmd_line.observe_h == 4 ||
       gcmd_line.observe_h == 5 ) {
    relaxed_steps = 0;
    for ( i = 0; i < RELAXED_STEPS; i++ ) {
      if ( lnum_used_O_at[i] == 0 ) break;
      relaxed_steps++;
      for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	lrpop_obsweight[i][j] = 0;
	lrpop_weight[i][j] = 0;
      }
    }
    
    for ( i = 0; i < relaxed_steps; i++ ) {
      for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	op = lused_O_at[i][j];
	if ( !gop_conn[op].observation ) continue;
	increment_influenced_region( i, j, relaxed_steps );
      }
    }


    if ( gcmd_line.observe_h == 3 ) {
      lh = 0;
      for ( i = 0; i < relaxed_steps; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  lrpop_weight[i][j] = lrpop_obsweight[i][j];
	  if ( lrpop_obsweight[i][j] == 0 ) {
	    lrpop_weight[i][j]++;
	  }
	  lh += lrpop_weight[i][j];
	}
      }
    }


    if ( gcmd_line.observe_h == 4 ) {
      for ( i = 0; i < relaxed_steps; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  lrpop_weight[i][j] = lrpop_obsweight[i][j];
	  if ( lrpop_weight[i][j] == 0 ) {
	    lrpop_weight[i][j]++;
	  }
	}
      }
      for ( i = relaxed_steps-1; i >= 0; i-- ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  if ( !gop_conn[lused_O_at[i][j]].observation ) continue;
	  for ( l = j+1; l < lnum_used_O_at[i]; l++ ) {
	    if ( !gop_conn[lused_O_at[i][l]].observation ) continue;
	    if ( lrpop_obsweight[i][l] == 0 ) {
	      lrpop_weight[i][l] *= 2;
	    }
	  }
	  for ( k = i+1; k < relaxed_steps; k++ ) {
	    for ( l = 0; l < lnum_used_O_at[k]; l++ ) {
	      if ( lrpop_obsweight[k][l] == 0 ) {
		lrpop_weight[k][l] *= 2;
	      }
	    }
	  }
	}
      }
      lh = 0;
      for ( i = 0; i < relaxed_steps; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  lh += lrpop_weight[i][j];
	}
      }
    }


    if ( gcmd_line.observe_h == 5 ) {
      for ( i = 0; i < relaxed_steps; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  lrpop_weight[i][j] = lrpop_obsweight[i][j];
	  if ( lrpop_weight[i][j] == 0 ) {
	    lrpop_weight[i][j]++;
	  }
	}
      }
      for ( i = relaxed_steps-1; i >= 0; i-- ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  if ( !gop_conn[lused_O_at[i][j]].observation ) continue;
	  for ( l = j+1; l < lnum_used_O_at[i]; l++ ) {
	    if ( !gop_conn[lused_O_at[i][l]].observation ) continue;
	    if ( lrpop_obsweight[i][l] == 0 ) {
	      lrpop_weight[i][l]+=2;
	    }
	  }
	  for ( k = i+1; k < relaxed_steps; k++ ) {
	    for ( l = 0; l < lnum_used_O_at[k]; l++ ) {
	      if ( lrpop_obsweight[k][l] == 0 ) {
		lrpop_weight[k][l]+=2;
	      }
	    }
	  }
	}
      }
      lh = 0;
      for ( i = 0; i < relaxed_steps; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  lh += lrpop_weight[i][j];
	}
      }
    }

    if ( gcmd_line.display_info == 128  || (gcmd_line.R && gcmd_line.debug) ) {
      printf("\n\ncomputed full-graph weights as follows:");
      for ( i = 0; i < relaxed_steps; i++ ) {
	for ( j = 0; j < lnum_used_O_at[i]; j++ ) {
	  printf("\nw %ld, ow %ld: ", lrpop_weight[i][j], lrpop_obsweight[i][j]); 
	  print_op_name( lused_O_at[i][j] );
	}
      }
      printf("\n\n");
      fflush(stdout);
      exit( 1 );
    }

  } /* if option == 3 */

}



void increment_influenced_region( int time, int ind, int relaxed_steps )

{

  static int *lowops, *highops;
  static Bool fc = TRUE;

  int num_lowops, num_highops;
  int t, i, j, k;

  if ( fc ) {
    lowops = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    highops = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    fc = FALSE;
  }

  lowops[0] = lused_O_at[time][ind];
  num_lowops = 1;
  t = time;
  while ( t < relaxed_steps - 1 ) {
    num_highops = 0;
    for ( i = 0; i < num_lowops; i++ ) {
      for ( j = 0; j < lnum_used_O_at[t+1]; j++ ) {
	if ( can_influence( lowops[i], lused_O_at[t+1][j] ) ) {
	  for ( k = 0; k < num_highops; k++ ) {
	    if ( highops[k] == lused_O_at[t+1][j] ) break;
	  }
	  if ( k < num_highops ) continue;
	  highops[num_highops++] = lused_O_at[t+1][j];
	  lrpop_obsweight[t+1][j]++;
	}
      }
    }

    for ( i = 0; i < num_highops; i++ ) {
      for ( j = 0; j < num_lowops; j++ ) {
	if ( lowops[j] == highops[i] ) break;
      }
      if ( j < num_lowops ) continue;
      lowops[num_lowops++] = highops[i];
    }

    t++;
  }

}



Bool can_influence( int o1, int o2 )

{

  int i, j, ft;

  for ( i = 0; i < gop_conn[o1].num_EA; i++ ) {
    ft = gop_conn[o1].EA[i];
    for ( j = 0; j < gop_conn[o2].num_EPC; j++ ) {
      if ( ft == gop_conn[o2].EPC[j] ) return TRUE;
    }
  }

  for ( i = 0; i < gop_conn[o1].num_O; i++ ) {
    ft = gop_conn[o1].O[i];
    for ( j = 0; j < gop_conn[o2].num_EPC; j++ ) {
      if ( ft == gop_conn[o2].EPC[j] ) return TRUE;
    }
    if ( gft_conn[ft].negation != -1 ) {
      for ( j = 0; j < gop_conn[o2].num_EPC; j++ ) {
	if ( gft_conn[ft].negation == gop_conn[o2].EPC[j] ) return TRUE;
      }
    }
  }

  return FALSE;

}





































/**************************************************************
 * SAT RELATED STUFF
 **************************************************************/















































/* top level control fn for (extremely) naive DP implementation...
 */

Bool r_dp_CNF( void )

{

  int i, j, v, sign, c;
  Bool sat;
  MemberList *i_ml, *j_ml;

  gdp_calls++;

  if ( lr_num_decision_stack == 0 ) {
    printf("\nr no decision stack at entering DP\n\n"); 
    exit( 1 );
  }

  if ( gcmd_line.debug ) {
    for ( i = 0; i < gmax_literals + 1; i++ ) {
      for ( j = lr_num_clauses - 1; j >= 0; j-- ) {
	if ( lr_clause_length[j] == i ) gsum_k_clauses[i]++;
	if ( lr_clause_length[j] > gmax_literals ) {
	  printf("\n\nr ?\n\n");
	}
      }
    }
    gsum_clauses += lr_num_clauses;
  }

  if ( gcmd_line.debug ) {
    /* Oh... fuck you all: double-2-2-check that memlists are correct!!!
     */
    for ( i = 1; i <= lr_num_c; i++ ) {
      for ( i_ml = lr_pos_c_in_clause_start[i]->next; 
	    i_ml != lr_pos_c_in_clause_end[i]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	for ( j = 0; j < lr_clause_length[c]; j++ ) {
	  if ( lr_clauses[c][j].literal == i ) break;
	}
	if ( j == lr_clause_length[c] ) {
	  printf("\nr error 1\n\n");
	  exit( 1 );
	}
	for ( j_ml = i_ml->next; 
	      j_ml != lr_pos_c_in_clause_end[i]; j_ml = j_ml->next ) {
	  if ( j_ml->clause == i_ml->clause ) {
	    printf("\nr error 1b\n\n");
	    exit( 1 );
	  }
	}
      }
      for ( i_ml = lr_neg_c_in_clause_start[i]->next; 
	    i_ml != lr_neg_c_in_clause_end[i]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	for ( j = 0; j < lr_clause_length[c]; j++ ) {
	  if ( lr_clauses[c][j].literal == (-1) * i ) break;
	}
	if ( j == lr_clause_length[c] ) {
	  printf("\nr error 2\n\n");
	  exit( 1 );
	}
	for ( j_ml = i_ml->next; 
	      j_ml != lr_neg_c_in_clause_end[i]; j_ml = j_ml->next ) {
	  if ( j_ml->clause == i_ml->clause ) {
	    printf("\nr error 2b\n\n");
	    exit( 1 );
	  }
	}
      }
    }
    for ( i = 0; i < lr_num_clauses; i++ ) {
      for ( j = 0; j < lr_clause_length[i]; j++ ) {
	if ( lr_clauses[i][j].literal > 0 ) {
	  v = lr_clauses[i][j].literal;
	  if ( lr_pos_c_in_clause_start[v] == NULL ) {
	    printf("\nr error 3a\n\n");
	    exit( 1 );
	  }
	  for ( i_ml = lr_pos_c_in_clause_start[v]->next; 
		i_ml != lr_pos_c_in_clause_end[v]; i_ml = i_ml->next ) {
	    if ( i_ml->clause == i ) break;
	  }
	  if ( i_ml == lr_pos_c_in_clause_end[v] ) {
	    printf("\nr error 3\n\n");
	    exit( 1 );
	  }
	} else {
	  v = (-1) * lr_clauses[i][j].literal;
	  if ( lr_neg_c_in_clause_start[v] == NULL ) {
	    printf("\nr error 4a\n\n");
	    exit( 1 );
	  }
	  for ( i_ml = lr_neg_c_in_clause_start[v]->next; 
		i_ml != lr_neg_c_in_clause_end[v]; i_ml = i_ml->next ) {
	    if ( i_ml->clause == i ) break;
	  }
	  if ( i_ml == lr_neg_c_in_clause_end[v] ) {
	    printf("\nr error 4\n\n");
	    exit( 1 );
	  }
	}
      }
    }
  }
    


  /* first, create assignments corresponding to decision stack;
   * check for contradictions.
   */
  i = 0;
  while ( i < lr_num_decision_stack ) {
    if ( lr_decision_stack[i] < 0 ) {
      v = (-1) * lr_decision_stack[i];
      sign = 0;
    } else {
      v = lr_decision_stack[i];
      sign = 1;
    }
    if ( lr_assigned[v] == sign ) {
      if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	printf("\nr skipping pre-insertion %d of var %d val %d, already set so",
	       i, v, sign);
      }
      for ( j = i; j < lr_num_decision_stack-1; j++ ) {
	lr_decision_stack[j] = lr_decision_stack[j+1];
      }
      lr_num_decision_stack--;
      continue;
    }
    if ( lr_assigned[v] != -1 ) {
      if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	printf("\nr contradictory pre-insertion %d of var %d val %d!",
	       i, v, sign);
      }
      /* unset info
       */
      for ( j = 0; j < i; j++ ) {
	if ( lr_decision_stack[j] < 0 ) {
	  v = (-1) * lr_decision_stack[j];
	  sign = 0;
	} else {
	  v = lr_decision_stack[j];
	  sign = 1;
	}
	lr_assigned[v] = -1;
      }
      return FALSE;
    }
    /* insert new assignment
     */
    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
      printf("\nr pre-inserting pos %d: var %d val %d",
	     i, v, sign);
    }
    lr_assigned[v] = sign;
    i++;
  }


  if ( lr_num_decision_stack == 0 ) {
    printf("\nr no decision stack after pre-inserting DP\n\n"); 
    exit( 1 );
  }

  /* now do the corresponding unit props for all the pre-insertions
   * in sequence; is implicit because do_unit_props will move through
   * entire (growing) decision stack, starting from start index
   *
   * no contradiction --> do DP!
   */
  sat = FALSE;
  if ( r_do_unit_props( 0 ) ) {
    sat = r_dp();
  }

  /* undo assignments
   */
  for ( i = 0; i < lr_num_decision_stack; i++ ) {
    if ( lr_decision_stack[i] < 0 ) {
      v = (-1) * lr_decision_stack[i];
    } else {
      v = lr_decision_stack[i];
    }
    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
      printf("\nr top retracting dec stack %d, var %d val %d!",
	     i, v, lr_decision_stack[i] < 0 ? 0 : 1);
    }
    lr_assigned[v] = -1;
  }
  /* not strictly necessary, as full (..) initialisation done above 
   * before DP calls... looks cleaner this way...
   */
  lr_num_decision_stack = 0;

  return sat;

}



/* invariant: lr_num_decision_stack is same on entry and exit; same holds for
 * lr_assigned array.
 */
Bool r_dp( void )

{

  int entry_num_decision_stack;
  int i, v, dec_v;

  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
    printf("\nr entering naive dp");
  }

  /* store the entry nr. of decisions on the stack, for later
   * retraction of the new decisions.
   */
  entry_num_decision_stack = lr_num_decision_stack;


  /* choose split vars from "backside of plan"
   */
  for ( dec_v = lr_num_c; dec_v > 0; dec_v-- ) {
    if ( lr_assigned[dec_v] == -1 ) break;
  }
  if ( dec_v == 0 ) {
    /* all vars assigned and no contradiction --> sat!
     */
    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
      printf("\nr all vars assigned, no contradiction -> sat!");
    }
    return TRUE;
  }

  /* split on v.
   *
   * first, positive.
   */
  if ( lr_num_decision_stack == gnum_ft_conn ) {
    printf("\nr dec stack overflow\n\n");
    exit( 1 );
  }
  lr_decision_stack[lr_num_decision_stack++] = dec_v;
  lr_assigned[dec_v] = 1;
  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
    printf("\nr decide %d true, doing unit props", dec_v);
  } 
  if ( r_do_unit_props( lr_num_decision_stack-1 ) ) {
    if ( r_dp() ) {
      /* found solution.
       */
      return TRUE;
    }
  }
  for ( i = entry_num_decision_stack; i < lr_num_decision_stack; i++ ) {
    if ( lr_decision_stack[i] < 0 ) {
      v = (-1) * lr_decision_stack[i];
    } else {
      v = lr_decision_stack[i];
    }
    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
      printf("\nr true tried retracting dec stack %d, var %d val %d!",
	     i, v, lr_decision_stack[i] < 0 ? 0 : 1);
    }
    lr_assigned[v] = -1;
  }
  lr_num_decision_stack = entry_num_decision_stack;


  /* now, negative.
   */
  lr_decision_stack[lr_num_decision_stack++] = (-1) * dec_v;
  lr_assigned[dec_v] = 0;
  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
    printf("\nr decide %d false, doing unit props", dec_v);
  } 
  if ( r_do_unit_props( lr_num_decision_stack-1 ) ) {
    if ( r_dp() ) {
      /* found solution.
       */
      return TRUE;
    }
  }
  for ( i = entry_num_decision_stack; i < lr_num_decision_stack; i++ ) {
    if ( lr_decision_stack[i] < 0 ) {
      v = (-1) * lr_decision_stack[i];
    } else {
      v = lr_decision_stack[i];
    }
    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
      printf("\nr false tried retracting dec stack %d, var %d val %d!",
	     i, v, lr_decision_stack[i] < 0 ? 0 : 1);
    }
    lr_assigned[v] = -1;
  }
  lr_num_decision_stack = entry_num_decision_stack;

  return FALSE;

}



Bool r_do_unit_props( int startidx )

{

  int idx, i, v, c, v_, numopenlits, lastopen;  
  MemberList *i_ml;

  gup_calls++;

  idx = startidx;
  while ( idx < lr_num_decision_stack ) {
    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
      printf("\nr propagating dec stack %d, %d!",
	     idx, lr_decision_stack[idx]);
      fflush(stdout);
    }
    if ( lr_decision_stack[idx] < 0 ) {
      v = (-1) * lr_decision_stack[idx];
      for ( i_ml = lr_pos_c_in_clause_start[v]->next; 
	    i_ml != lr_pos_c_in_clause_end[v]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	numopenlits = 0;
	lastopen = 0;
	for ( i = 0; i < lr_clause_length[c]; i++ ) {
	  if ( numopenlits > 1 ) break;
	  if ( lr_clauses[c][i].literal > 0 ) {
	    v_ = lr_clauses[c][i].literal;
	    if ( lr_assigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = lr_clauses[c][i].literal;
	    }
	    if ( lr_assigned[v_] == 1 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  } else {
	    v_ = (-1) * lr_clauses[c][i].literal;
	    if ( lr_assigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = lr_clauses[c][i].literal;
	    }
	    if ( lr_assigned[v_] == 0 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  }
	}
	if ( i < lr_clause_length[c] ) {
	  /* clause sat or >1 open lit! ignore.
	   */
	  continue;
	}
	if ( numopenlits == 0 ) {
	  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	    printf("\nr empty clause %d", c);
	  }
	  /* no fulfilled and no open lits --> contradiction!!
	   */
	  return FALSE;
	}
	if ( numopenlits == 1 ) {
	  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	    printf("\nr unit clause %d", c);
	  }
	  if ( lr_num_decision_stack == gnum_ft_conn ) {
	    printf("\nr dec stack overflow\n\n");
	    exit( 1 );
	  }
	  /* this one's unit. constrain the variable.
	   */
	  /* DEBUGGING. REMOVE LATER.
	   */
	  if ( lastopen == 0 ) {
	    printf("\n\nr lastopen 0?\n\n");
	    exit( 1 );
	  }
	  if ( lastopen > 0 ) {
	    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	      printf("\nr UP constr %d true", lastopen);
	    }
	    lr_assigned[lastopen] = 1;
	  } else {
	    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	      printf("\nr UP constr %d false", (-1) * lastopen);
	    }
	    lr_assigned[(-1) * lastopen] = 0;
	  }
	  lr_decision_stack[lr_num_decision_stack++] = lastopen;
	} /* numopenlits == 1 */
      } /* clauses in that negated v participates */
    } else { /* if decision here is positive */
      v = lr_decision_stack[idx];
      fflush(stdout);
      for ( i_ml = lr_neg_c_in_clause_start[v]->next; 
	    i_ml != lr_neg_c_in_clause_end[v]; i_ml = i_ml->next ) {
	c = i_ml->clause;
	numopenlits = 0;
	lastopen = 0;
	for ( i = 0; i < lr_clause_length[c]; i++ ) {
	  if ( numopenlits > 1 ) break;
	  if ( lr_clauses[c][i].literal > 0 ) {
	    v_ = lr_clauses[c][i].literal;
	    if ( lr_assigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = lr_clauses[c][i].literal;
	    }
	    if ( lr_assigned[v_] == 1 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  } else {
	    v_ = (-1) * lr_clauses[c][i].literal;
	    if ( lr_assigned[v_] == -1 ) {
	      numopenlits++;
	      lastopen = lr_clauses[c][i].literal;
	    }
	    if ( lr_assigned[v_] == 0 ) {
	      /* clause satisfied!
	       */
	      break;
	    }
	  }
	}
	if ( i < lr_clause_length[c] ) {
	  /* clause sat or >1 open lit! ignore.
	   */
	  continue;
	}
	if ( numopenlits == 0 ) {
	  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	    printf("\nr empty clause %d", c);
	  }
	  /* no fulfilled and no open lits --> contradiction!!
	   */
	  return FALSE;
	}
	if ( numopenlits == 1 ) {
	  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	    printf("\nr unit clause %d", c);
	  }
	  if ( lr_num_decision_stack == gnum_ft_conn ) {
	    printf("\nr dec stack overflow\n\n");
	    exit( 1 );
	  }
	  /* this one's unit. constrain the variable.
	   */
	  /* DEBUGGING. REMOVE LATER.
	   */
	  if ( lastopen == 0 ) {
	    printf("\n\nr lastopen 0?\n\n");
	    exit( 1 );
	  }
	  if ( lastopen > 0 ) {
	    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	      printf("\nr UP constr %d true", lastopen);
	    }
	    lr_assigned[lastopen] = 1;
	  } else {
	    if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	      printf("\nr UP constr %d false", (-1) * lastopen);
	    }
	    lr_assigned[(-1) * lastopen] = 0;
	  }
	  lr_decision_stack[lr_num_decision_stack++] = lastopen;
	} /* numopenlits == 1 */
      } /* clauses in that positive v participates */
    } /* neg or pos */
    idx++;
  } /* while not all decisions on stack propagated */

  return TRUE;

}



void print_r_clauses( void )

{

  int i, j;
  
  for ( i = 0; i < lr_num_clauses; i++ ) {
    printf("\n%d: ", i);
    for ( j = 0; j < lr_clause_length[i]; j++ ) {
      if ( lr_clauses[i][j].time >= 0 ) {
	if ( lr_clauses[i][j].literal < 0 ) {
	  printf("-");
	  print_ft_name( (-1) * lr_clauses[i][j].literal - 1 );
	} else {
	  print_ft_name( lr_clauses[i][j].literal - 1 );
	}
	printf("(%d) ", lr_clauses[i][j].time);
      } else {
	/* this is a code!
	 */
	if ( lr_clauses[i][j].literal < 0 ) {
	  printf("-");
	  print_ft_name( lr_cf[(-1) * lr_clauses[i][j].literal] );
	} else {
	  print_ft_name( lr_cf[lr_clauses[i][j].literal] );
	}
      }
    }
  }

}



void print_r_clause( int i )

{

  int j;
  
  printf("\n");
  for ( j = 0; j < lr_clause_length[i]; j++ ) {
    if ( lr_clauses[i][j].time >= 0 ) {
      if ( lr_clauses[i][j].literal < 0 ) {
	printf("-");
	print_ft_name( (-1) * lr_clauses[i][j].literal - 1 );
      } else {
	print_ft_name( lr_clauses[i][j].literal - 1 );
      }
      printf("(%d) ", lr_clauses[i][j].time);
    } else {
      /* this is a code!
       */
      if ( lr_clauses[i][j].literal < 0 ) {
	printf("-");
	print_ft_name( lr_cf[(-1) * lr_clauses[i][j].literal] );
      } else {
	print_ft_name( lr_cf[lr_clauses[i][j].literal] );
      }
    }
  }

}



int r_extend_dynamic_clauses_base( BfsNode_pointer *path, int num_path )

{

  static Bool fc = TRUE;
  static Bool *Ft, *Ut, *Ftpp, *Utpp;
  
  int i, j, k, ef, t, time, ft, m, ff;
  
  if ( fc ) {
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
  
  
  
  /* the states leading to S are in 
   * path[num_path + 1] .. path[MAX_PLAN_LENGTH].
   * unknown efs leading to each state are stored in the nodes,
   * so we can produce our CNF based on these.
   */
  if ( gcmd_line.R && gcmd_line.debug ) {
    printf("\nR dynamic CNF");
  }
  if ( gcmd_line.R && gcmd_line.debug ) {
    printf("\n\n-----------------------adding clauses from %d",
	   gnum_clauses);
  }
  if ( gnum_clauses != gnum_fixed_clauses ) {
    printf("\n\nat start R dyn. extend gnum_clauses %d != gnum_fixed_clauses %d\n\n",
	   gnum_clauses, gnum_fixed_clauses);
    fflush(stdout);
    exit( 1 );
  }
  /* insert the t -> t+1 implications for all t on the path.
   *
   * the times here are t-(num_path+1) -> t-(num_path+1)+1 for t in:
   */
  time = -1;
  for ( t = num_path + 1; t < MAX_PLAN_LENGTH; t++ ) {
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
	if ( gcmd_line.R && gcmd_line.debug ) {
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
	if ( gcmd_line.R && gcmd_line.debug ) {
	  printf("\nunknown del eff clause"); print_clause( gnum_clauses-1 );
	}
      }
    } /* i over unknown effs */
    
    /* now, the noop - implications; first, the positives then the negatives
     * QUESTION: do we need the negatives for our derivations to be correct?
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
	 */
	continue;
      }
      if ( !Ft[ft] && !Ut[ft] ) continue;/* clause satisfied */
      if ( r_unconditional_nondet_del_on( ft, Ft, path[t+1]->ingoing_edge->op ) ) {
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
	 */
	continue;
      }
      if ( Ft[ft] ) continue;/* clause satisfied */
      if ( r_unconditional_nondet_add_on( ft, Ft, path[t+1]->ingoing_edge->op ) ) {
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
	if ( gcmd_line.R && gcmd_line.debug ) {
	  printf("\nR observed 'true' clause"); print_clause( gnum_clauses-1 );
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
	if ( gcmd_line.R && gcmd_line.debug ) {
	  printf("\nR observed 'false' clause"); print_clause( gnum_clauses-1 );
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
  } /* end t loop */

  /* debugging: see what that looks like!
   */
  if ( gcmd_line.R && gcmd_line.debug >= 7 ) {
    if ( gcmd_line.R && gcmd_line.debug >= 9 ) {
      printf("\n\n-----------------------------R to execution path\n");
      printf("\n---------state:");
      print_state( path[num_path+1]->S );
      for ( t = num_path + 2; t <= MAX_PLAN_LENGTH; t++ ) {
	printf("\n----------state:");
	print_state( path[t]->S );
      }
    }
    printf("\n\n-----------------------------R I get the CNF\n");
    print_clauses();
  }
  
  if ( time < -1 ) {
    printf("\n\ntime < -1 in R dynamic extend\n\n");
    exit( 1 );
  }
  
  return time + 1;

}



/* these two are exactly the same as in state_transitions.c
 * except for the debugging info...
 */
Bool r_unconditional_nondet_del_on( int ft, Bool *Ft, int op )

{

  int i, j, ef;
  
  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
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
	  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	    printf(" -- yes it does!");
	  }
	  return TRUE;
	} else {
	  printf("\n\nR uncond det del eff in noop check!!\n\n");
	  exit( 1 );
	}
      }
    }
  }

  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
    printf(" -- no it doesn't!");
  }
  return FALSE;

}



Bool r_unconditional_nondet_add_on( int ft, Bool *Ft, int op )

{

  int i, j, ef;
  
  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
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
	  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
	    printf(" -- yes it does!");
	  }
	  return TRUE;
	} else {
	  printf("\n\nR uncond det add eff in noop check!!\n\n");
	  exit( 1 );
	}
      }
    }
  }

  if ( gcmd_line.R && gcmd_line.debug >= 5 ) {
    printf(" -- no it doesn't!");
  }
  return FALSE;

}

