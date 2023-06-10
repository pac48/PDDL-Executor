


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
 * File: inst_final.c
 * Description: final domain representation functions
 *
 *
 * Author: Joerg Hoffmann 2000
 *
 *********************************************************************/ 






#include <stdlib.h>



#include "ff.h"

#include "output.h"
#include "memory.h"

#include "inst_pre.h"
#include "inst_final.h"














/********************************
 * POSSIBLY TRUE FACTS ANALYSIS *
 ********************************/








/* local globals for this part
 */

int_pointer lpos[MAX_PREDICATES];
int_pointer lneg[MAX_PREDICATES];
int_pointer luse[MAX_PREDICATES];
int_pointer lindex[MAX_PREDICATES];

int lp;
int largs[MAX_VARS];







void perform_reachability_analysis( void )

{

  int size, i, j, k, adr, num;
  Bool fixpoint;
  Facts *f;
  NormOperator *no;
  EasyTemplate *t1, *t2;
  NormEffect *ne;
  Action *tmp, *a;
  Bool *had_hard_template;
  PseudoAction *pa;
  PseudoActionEffect *pae;

  gactions = NULL;
  gnum_actions = 0;

  for ( i = 0; i < gnum_predicates; i++ ) {
    size =  1;
    for ( j = 0; j < garity[i]; j++ ) {
      size *= gnum_constants;
    }

    lpos[i] = ( int_pointer ) calloc( size, sizeof( int ) );
    lneg[i] = ( int_pointer ) calloc( size, sizeof( int ) );
    luse[i] = ( int_pointer ) calloc( size, sizeof( int ) );
    lindex[i] = ( int_pointer ) calloc( size, sizeof( int ) );

    for ( j = 0; j < size; j++ ) {
      lpos[i][j] = 0;
      lneg[i][j] = 1;/* all facts but initials are poss. negative */
      luse[i][j] = 0;
      lindex[i][j] = -1;
    }
  }

  had_hard_template = ( Bool * ) calloc( gnum_hard_templates, sizeof( Bool ) );
  for ( i = 0; i < gnum_hard_templates; i++ ) {
    had_hard_template[i] = FALSE;
  }

  /* mark initial facts as possibly positive, not poss. negative
   */
  for ( i = 0; i < gnum_predicates; i++ ) {

    lp = i;
    for ( j = 0; j < gnum_initial_predicate[i]; j++ ) {
      for ( k = 0; k < garity[i]; k++ ) {
	largs[k] = ginitial_predicate[i][j].args[k];
      }
      adr = fact_adress();
      lpos[lp][adr] = 1;
      lneg[lp][adr] = 0;
    }
    for ( j = 0; j < gnum_unknown_initial_predicate[i]; j++ ) {
      for ( k = 0; k < garity[i]; k++ ) {
	largs[k] = gunknown_initial_predicate[i][j].args[k];
      }
      adr = fact_adress();
      lpos[lp][adr] = 1;
      lneg[lp][adr] = 1;
      /* neg is already 1, see above (it is checked in inst-pre that
       * the same fact does not occur both in I and unknown-I)
       * anyway, who cares
       *
       * unknown facts are assumed relevant.
       */
      luse[lp][adr] = 1;
      if ( gnum_relevant_facts == MAX_RELEVANT_FACTS ) {
	printf("\ntoo many relevant facts! increase MAX_RELEVANT_FACTS (currently %d)\n\n",
	       MAX_RELEVANT_FACTS);
	exit( 1 );
      }
      grelevant_facts[gnum_relevant_facts].predicate = lp;
      for ( k = 0; k < garity[lp]; k++ ) {
	grelevant_facts[gnum_relevant_facts].args[k] = largs[k];
      }
      lindex[lp][adr] = gnum_relevant_facts;
      gnum_relevant_facts++;
    }
  }

  /* compute fixpoint
   */
  fixpoint = FALSE;
  while ( !fixpoint ) {
    fixpoint = TRUE;

    /* assign next layer of easy templates to possibly positive fixpoint
     */
    t1 = geasy_templates;
    while ( t1 ) {
      no = t1->op;
      for ( i = 0; i < no->num_preconds; i++ ) {
	lp = no->preconds[i].predicate;

	for ( j = 0; j < garity[lp]; j++ ) {
	  largs[j] = ( no->preconds[i].args[j] >= 0 ) ?
	    no->preconds[i].args[j] : t1->inst_table[DECODE_VAR( no->preconds[i].args[j] )];
	}
	if ( !lpos[lp][fact_adress()] ) {
	  break;
	}
      }

      if ( i < no->num_preconds ) {
	t1 = t1->next;
	continue;
      }

      num = 0;
      for ( ne = no->effects; ne; ne = ne->next ) {
	num++;
	/* currently, simply ignore effect conditions and assume
	 * they will all be made true eventually.
	 */
	for ( i = 0; i < ne->num_adds; i++ ) {
	  lp = ne->adds[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = ( ne->adds[i].args[j] >= 0 ) ?
	      ne->adds[i].args[j] : t1->inst_table[DECODE_VAR( ne->adds[i].args[j] )];
	  }
	  adr = fact_adress();
	  if ( !lpos[lp][adr] ) {
	    /* new relevant fact! (added neg-initial)
	     */
	    lpos[lp][adr] = 1;
	    lneg[lp][adr] = 1;
	    luse[lp][adr] = 1;
	    if ( gnum_relevant_facts == MAX_RELEVANT_FACTS ) {
	      printf("\ntoo many relevant facts! increase MAX_RELEVANT_FACTS (currently %d)\n\n",
		     MAX_RELEVANT_FACTS);
	      exit( 1 );
	    }
	    grelevant_facts[gnum_relevant_facts].predicate = lp;
	    for ( j = 0; j < garity[lp]; j++ ) {
	      grelevant_facts[gnum_relevant_facts].args[j] = largs[j];
	    }
	    lindex[lp][adr] = gnum_relevant_facts;
	    gnum_relevant_facts++;
	    fixpoint = FALSE;
	  }
	}
      }

      tmp = new_Action();
      tmp->norm_operator = no;
      for ( i = 0; i < no->num_vars; i++ ) {
	tmp->inst_table[i] = t1->inst_table[i];
      }
      tmp->name = no->op->name;
      tmp->num_name_vars = no->op->number_of_real_params;
      make_name_inst_table_from_NormOperator( tmp, no, t1 );
      tmp->next = gactions;
      tmp->num_effects = num;
      gactions = tmp;
      gnum_actions++;

      t2 = t1->next;
      if ( t1->next ) {
	t1->next->prev = t1->prev;
      }
      if ( t1->prev ) {
	t1->prev->next = t1->next;
      } else {
	geasy_templates = t1->next;
      }
      free_single_EasyTemplate( t1 );
      t1 = t2;
    }

    /* now assign all hard templates that have not been transformed
     * to actions yet.
     */
    for ( i = 0; i < gnum_hard_templates; i++ ) {
      if ( had_hard_template[i] ) {
	continue;
      }
      pa = ghard_templates[i];

      for ( j = 0; j < pa->num_preconds; j++ ) {
	lp = pa->preconds[j].predicate;

	for ( k = 0; k < garity[lp]; k++ ) {
	  largs[k] = pa->preconds[j].args[k];
	}
	if ( !lpos[lp][fact_adress()] ) {
	  break;
	}
      }

      if ( j < pa->num_preconds ) {
	continue;
      }

      for ( pae = pa->effects; pae; pae = pae->next ) {
	/* currently, simply ignore effect conditions and assume
	 * they will all be made true eventually.
	 */
	for ( j = 0; j < pae->num_adds; j++ ) {
	  lp = pae->adds[j].predicate;
	  for ( k = 0; k < garity[lp]; k++ ) {
	    largs[k] = pae->adds[j].args[k];
	  }
	  adr = fact_adress();
	  if ( !lpos[lp][adr] ) {
	    /* new relevant fact! (added neg-initial)
	     */
	    lpos[lp][adr] = 1;
	    lneg[lp][adr] = 1;
	    luse[lp][adr] = 1;
	    if ( gnum_relevant_facts == MAX_RELEVANT_FACTS ) {
	      printf("\ntoo many relevant facts! increase MAX_RELEVANT_FACTS (currently %d)\n\n",
		     MAX_RELEVANT_FACTS);
	      exit( 1 );
	    }
	    grelevant_facts[gnum_relevant_facts].predicate = lp;
	    for ( k = 0; k < garity[lp]; k++ ) {
	      grelevant_facts[gnum_relevant_facts].args[k] = largs[k];
	    }
	    lindex[lp][adr] = gnum_relevant_facts;
	    gnum_relevant_facts++;
	    fixpoint = FALSE;
	  }
	}
      }

      tmp = new_Action();
      tmp->pseudo_action = pa;
      for ( j = 0; j < pa->op->num_vars; j++ ) {
	tmp->inst_table[j] = pa->inst_table[j];
      }
      tmp->name = pa->op->name;
      tmp->num_name_vars = pa->op->number_of_real_params;
      make_name_inst_table_from_PseudoAction( tmp, pa );
      tmp->next = gactions;
      tmp->num_effects = pa->num_effects;
      gactions = tmp;
      gnum_actions++;

      had_hard_template[i] = TRUE;
    }
  }

  free( had_hard_template );

  gnum_pp_facts = gnum_initial + gnum_relevant_facts;

  if ( gcmd_line.display_info == 118 ) {
    printf("\nreachability analysys came up with:");

    printf("\n\npossibly positive facts:");
    for ( f = ginitial; f; f = f->next ) {
      printf("\n");
      print_Fact( f->fact );
    }
    for ( i = 0; i < gnum_relevant_facts; i++ ) {
      printf("\n");
      print_Fact( &(grelevant_facts[i]) );
    }

    printf("\n\nthis yields these %d action templates:", gnum_actions);
    for ( i = 0; i < gnum_operators; i++ ) {
      printf("\n\noperator %s:", goperators[i]->name);
      for ( a = gactions; a; a = a->next ) {
	if ( ( a->norm_operator && 
	       a->norm_operator->op !=  goperators[i] ) ||
	     ( a->pseudo_action &&
	       a->pseudo_action->op !=  goperators[i] ) ) {
	  continue;
	}
	printf("\ntemplate: ");
	for ( j = 0; j < goperators[i]->number_of_real_params; j++ ) {
	  printf("%s", gconstants[a->name_inst_table[j]]);
	  if ( j < goperators[i]->num_vars-1 ) {
	    printf(" ");
	  }
	}
      }
    }
    printf("\n\n");
  }

}



int fact_adress( void )

{

  int r = 0, b = 1, i;

  for ( i = garity[lp] - 1; i > -1; i-- ) {
    r += b * largs[i];
    b *= gnum_constants;
  }

  return r;

}



void make_name_inst_table_from_NormOperator( Action *a, NormOperator *o, EasyTemplate *t )

{

  int i, r = 0, m = 0;

  for ( i = 0; i < o->op->number_of_real_params; i++ ) {
    if ( o->num_removed_vars > r &&
	 o->removed_vars[r] == i ) {
      /* this var has been removed in NormOp;
       * insert type constraint constant
       *
       * at least one there, as empty typed pars ops are removed
       */
      a->name_inst_table[i] = gtype_consts[o->type_removed_vars[r]][0];
      r++;
    } else {
      /* this par corresponds to par m  in NormOp
       */
      a->name_inst_table[i] = t->inst_table[m];
      m++;
    }
  }

}



void make_name_inst_table_from_PseudoAction( Action *a, PseudoAction *pa )

{

  int i;

  for ( i = 0; i < pa->op->number_of_real_params; i++ ) {
    a->name_inst_table[i] = pa->inst_table[i];
  }

}


















/***********************************************************
 * RELEVANCE ANALYSIS AND FINAL DOMAIN AND PROBLEM CLEANUP *
 ***********************************************************/









/* counts effects for later allocation
 */
int lnum_effects;









void collect_relevant_facts( void )

{

  Action *a;
  NormOperator *no;
  NormEffect *ne;
  int i, j, adr;
  PseudoAction *pa;
  PseudoActionEffect *pae;

  /* mark all deleted facts; such facts, that are also pos, are relevant.
   */
  for ( a = gactions; a; a = a->next ) {
    if ( a->norm_operator ) {
      no = a->norm_operator;

      for ( ne = no->effects; ne; ne = ne->next ) {
	for ( i = 0; i < ne->num_dels; i++ ) {
	  lp = ne->dels[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = ( ne->dels[i].args[j] >= 0 ) ?
	      ne->dels[i].args[j] : a->inst_table[DECODE_VAR( ne->dels[i].args[j] )];
	  }
	  adr = fact_adress();

	  lneg[lp][adr] = 1;
	  if ( lpos[lp][adr] &&
	       !luse[lp][adr] ) {
	    luse[lp][adr] = 1;
	    lindex[lp][adr] = gnum_relevant_facts;
	    if ( gnum_relevant_facts == MAX_RELEVANT_FACTS ) {
	      printf("\nincrease MAX_RELEVANT_FACTS! (current value: %d)\n\n",
		     MAX_RELEVANT_FACTS);
	      exit( 1 );
	    }
	    grelevant_facts[gnum_relevant_facts].predicate = lp;
	    for ( j = 0; j < garity[lp]; j++ ) {
	      grelevant_facts[gnum_relevant_facts].args[j] = largs[j];
	    }
	    lindex[lp][adr] = gnum_relevant_facts;
	    gnum_relevant_facts++;
	  }
	}
      }
    } else {
      pa = a->pseudo_action;

      for ( pae = pa->effects; pae; pae = pae->next ) {
	for ( i = 0; i < pae->num_dels; i++ ) {
	  lp = pae->dels[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = pae->dels[i].args[j];
	  }
	  adr = fact_adress();

	  lneg[lp][adr] = 1;
	  if ( lpos[lp][adr] &&
	       !luse[lp][adr] ) {
	    luse[lp][adr] = 1;
	    lindex[lp][adr] = gnum_relevant_facts;
	    if ( gnum_relevant_facts == MAX_RELEVANT_FACTS ) {
	      printf("\nincrease MAX_RELEVANT_FACTS! (current value: %d)\n\n",
		     MAX_RELEVANT_FACTS);
	      exit( 1 );
	    }
	    grelevant_facts[gnum_relevant_facts].predicate = lp;
	    for ( j = 0; j < garity[lp]; j++ ) {
	      grelevant_facts[gnum_relevant_facts].args[j] = largs[j];
	    }
	    lindex[lp][adr] = gnum_relevant_facts;
	    gnum_relevant_facts++;
	  }
	}
      }
    }
  }

  if ( gcmd_line.display_info == 119 ) {
    printf("\n\nfacts selected as relevant:\n\n");
    for ( i = 0; i < gnum_relevant_facts; i++ ) {
      printf("\n%d: ", i);
      print_Fact( &(grelevant_facts[i]) );
    }
  }

  lnum_effects = 0;

  /* first make place for initial and goal states.
   * (one artificial fact might still be added here)
   *
   * no Us in goal, no UEs in initial or goal.
   */
  ggoal_state.F = ( int * ) calloc( gnum_relevant_facts + 1, sizeof( int ) );
  ginitial_state.F = ( int * ) calloc( gnum_relevant_facts + 1, sizeof( int ) );
  ginitial_state.U = ( int * ) calloc( gnum_relevant_facts + 1, sizeof( int ) );

  create_final_goal_state();
  create_final_initial_state();
  create_final_actions();

  if ( gcmd_line.display_info == 120 ) {
    printf("\n\nfinal domain representation is:\n\n");  
    for ( i = 0; i < gnum_operators; i++ ) {
      printf("\n\n------------------operator %s-----------\n\n", goperators[i]->name);
      for ( a = gactions; a; a = a->next ) {
	if ( ( !a->norm_operator &&
	       !a->pseudo_action ) ||
	     ( a->norm_operator && 
	       a->norm_operator->op != goperators[i] ) ||
	     ( a->pseudo_action &&
	       a->pseudo_action->op != goperators[i] ) ) {
	  continue;
	}
	print_Action( a );
      }
    }
    printf("\n\n--------------------GOAL REACHED ops-----------\n\n");
    for ( a = gactions; a; a = a->next ) {
      if ( !a->norm_operator &&
	   !a->pseudo_action ) {
	print_Action( a );
      }
    }
   
    printf("\n\nfinal initial state is, known:\n\n");
    for ( i = 0; i < ginitial_state.num_F; i++ ) {
      print_ft_name( ginitial_state.F[i] );
      printf("\n");
    }
    printf("\n\nfinal initial state is, unknown:\n\n");
    for ( i = 0; i < ginitial_state.num_U; i++ ) {
      print_ft_name( ginitial_state.U[i] );
      printf("\n");
    }
    printf("\n\nfinal goal state is:\n\n");
    for ( i = 0; i < ggoal_state.num_F; i++ ) {
      print_ft_name( ggoal_state.F[i] );
      printf("\n");
    }
  }

}



void create_final_goal_state( void )

{

  WffNode *w, *ww;
  int m, i, adr;
  Action *tmp;

  set_relevants_in_wff( &ggoal );
  cleanup_wff( &ggoal );
  if ( ggoal->connective == TRU ) {
    printf("\nff: goal can be simplified to TRUE. The empty plan solves it\n\n");
    exit( 1 );
  }
  if ( ggoal->connective == FAL ) {
    printf("\nff: goal can be simplified to FALSE. No plan will solve it\n\n");
    exit( 1 );
  }

  switch ( ggoal->connective ) {
  case OR:
    printf("\nsorry -- this version of cff can't handle disjunctive goals.\n\n");
    exit( 1 );
    if ( gnum_relevant_facts == MAX_RELEVANT_FACTS ) {
      printf("\nincrease MAX_RELEVANT_FACTS! (current value: %d)\n\n",
	     MAX_RELEVANT_FACTS);
      exit( 1 );
    }
    grelevant_facts[gnum_relevant_facts].predicate = -3;
    gnum_relevant_facts++;
    for ( w = ggoal->sons; w; w = w->next ) {
      tmp = new_Action();
      if ( w->connective == AND ) {
	m = 0;
	for ( ww = w->sons; ww; ww = ww->next ) m++;
	tmp->preconds = ( int * ) calloc( m, sizeof( int ) );
	tmp->num_preconds = 0;
	for ( ww = w->sons; ww; ww = ww->next ) {
	  lp = ww->fact->predicate;
	  for ( i = 0; i < garity[lp]; i++ ) {
	    largs[i] = ww->fact->args[i];
	  }
	  adr = fact_adress();
	  tmp->preconds[tmp->num_preconds++] = lindex[lp][adr];
	}
      } else {
	tmp->preconds = ( int * ) calloc( 1, sizeof( int ) );
	tmp->num_preconds = 1;
	lp = w->fact->predicate;
	for ( i = 0; i < garity[lp]; i++ ) {
	  largs[i] = w->fact->args[i];
	}
	adr = fact_adress();
	tmp->preconds[0] = lindex[lp][adr];
      }
      tmp->effects = ( ActionEffect * ) calloc( 1, sizeof( ActionEffect ) );
      tmp->num_effects = 1;
      tmp->effects[0].conditions = NULL;
      tmp->effects[0].num_conditions = 0;
      tmp->effects[0].dels = NULL;
      tmp->effects[0].num_dels = 0;
      tmp->effects[0].adds = ( int * ) calloc( 1, sizeof( int ) );
      tmp->effects[0].adds[0] = gnum_relevant_facts - 1;
      tmp->effects[0].num_adds = 1;
      tmp->next = gactions;
      gactions = tmp;
      gnum_actions++;
      lnum_effects++;
    }
    ggoal_state.F[0] = gnum_relevant_facts - 1;
    ggoal_state.num_F = 1;
    break;
  case AND:
    for ( w = ggoal->sons; w; w = w->next ) {
      lp = w->fact->predicate;
      for ( i = 0; i < garity[lp]; i++ ) {
	largs[i] = w->fact->args[i];
      }
      adr = fact_adress();
      ggoal_state.F[ggoal_state.num_F++] = lindex[lp][adr];
    }
    break;
  case ATOM:
    ggoal_state.num_F = 1;
    lp = ggoal->fact->predicate;
    for ( i = 0; i < garity[lp]; i++ ) {
      largs[i] = ggoal->fact->args[i];
    }
    adr = fact_adress();
    ggoal_state.F[0] = lindex[lp][adr];
    break;
  default:
    printf("\n\nwon't get here: non ATOM,AND,OR in fully simplified goal\n\n");
    exit( 1 );
  }

}



void set_relevants_in_wff( WffNode **w )

{

  WffNode *i;
  int j, adr;

  switch ( (*w)->connective ) {
  case AND:
  case OR:
    for ( i = (*w)->sons; i; i = i->next ) {
      set_relevants_in_wff( &i );
    }
    break;
  case ATOM:
    /* no equalities, as fully instantiated
     */
    lp = (*w)->fact->predicate;
    for ( j = 0; j < garity[lp]; j++ ) {
      largs[j] = (*w)->fact->args[j];
    }
    adr = fact_adress();

    if ( !lneg[lp][adr] ) {
      (*w)->connective = TRU;
      free( (*w)->fact );
      (*w)->fact = NULL;
      break;
    }
    if ( !lpos[lp][adr] ) {
      (*w)->connective = FAL;
      free( (*w)->fact );
      (*w)->fact = NULL;
      break;
    }
    break;
  default:
    printf("\n\nwon't get here: non NOT,OR,AND in goal set relevants\n\n");
    exit( 1 );
  }

}



void create_final_initial_state( void )

{

  Facts *f;
  int i, adr;

  for ( f = ginitial; f; f = f->next ) {
    lp = f->fact->predicate;
    for ( i = 0; i < garity[lp]; i++ ) {
      largs[i] = f->fact->args[i];
    }
    adr = fact_adress();
    if ( !lneg[lp][adr] ) {/* non deleted ini */
      continue;
    }
    ginitial_state.F[ginitial_state.num_F++] = lindex[lp][adr];
  }

  ginitial_state.num_U = 0;
  for ( f = gunknown_initial; f; f = f->next ) {
    lp = f->fact->predicate;
    for ( i = 0; i < garity[lp]; i++ ) {
      largs[i] = f->fact->args[i];
    }
    adr = fact_adress();
    ginitial_state.U[ginitial_state.num_U++] = lindex[lp][adr];
  }
 
}



void create_final_actions( void )

{

  Action *a, *p, *t;
  NormOperator *no;
  NormEffect *ne;
  int i, j, adr;
  PseudoAction *pa;
  PseudoActionEffect *pae;

  a = gactions; p = NULL;
  while ( a ) {
    if ( a->norm_operator ) {
      /* action comes from an easy template NormOp
       */
      no = a->norm_operator;
      
/*       printf("\nop %s", no->op->name); */
/*       for ( i = 0; i < no->num_vars; i++ ) { */
/* 	printf(" %s", a->inst_table[i] < 0 ? "<o?" : gconstants[a->inst_table[i]]); */
/*       } */

      if ( no->num_preconds > 0 ) {
	a->preconds = ( int * ) calloc( no->num_preconds, sizeof( int ) );
      }
      a->num_preconds = 0;
      for ( i = 0; i < no->num_preconds; i++ ) {
	lp = no->preconds[i].predicate;
	for ( j = 0; j < garity[lp]; j++ ) {
	  largs[j] = ( no->preconds[i].args[j] >= 0 ) ?
	    no->preconds[i].args[j] : a->inst_table[DECODE_VAR( no->preconds[i].args[j] )];
	}
	adr = fact_adress();
	
	/* preconds are lpos in all cases due to reachability analysis
	 */
	if ( !lneg[lp][adr] ) {
	  continue;
	}
	
	a->preconds[a->num_preconds++] = lindex[lp][adr];
      }

      if ( no->num_observe > 0 ) {
	a->observe = ( int * ) calloc( no->num_observe, sizeof( int ) );
      }
      a->num_observe = 0;
      for ( i = 0; i < no->num_observe; i++ ) {
/* 	printf("\nobserve %s", gpredicates[no->observe[i].predicate]); */
	lp = no->observe[i].predicate;
	for ( j = 0; j < garity[lp]; j++ ) {
	  largs[j] = ( no->observe[i].args[j] >= 0 ) ?
	    no->observe[i].args[j] : a->inst_table[DECODE_VAR( no->observe[i].args[j] )];
/* 	  if ( largs[j] < 0 ) { */
/* 	    printf(" ,0?"); */
/* 	  } else { */
/* 	    printf(" %s", gconstants[largs[j]]); */
/* 	  } */
	}
	adr = fact_adress();

	/* facts with only one possible truth value need not be observed.
	 */
	if ( !lneg[lp][adr] ||
	     !lpos[lp][adr] ) {
/* 	  if ( !lneg[lp][adr] ) { */
/* 	    printf(" -- not neg"); */
/* 	  } */
/* 	  if ( !lpos[lp][adr] ) { */
/* 	    printf(" -- not pos"); */
/* 	  } */
	  continue;
	}
	
	a->observe[a->num_observe++] = lindex[lp][adr];
      }



      if ( a->num_effects > 0 ) {
	a->effects = ( ActionEffect * ) calloc( a->num_effects, sizeof( ActionEffect ) );
      }
      a->num_effects = 0;
      for ( ne = no->effects; ne; ne = ne->next ) {
	if ( ne->num_conditions > 0 ) {
	  a->effects[a->num_effects].conditions =
	    ( int * ) calloc( ne->num_conditions, sizeof( int ) );
	}
	a->effects[a->num_effects].num_conditions = 0;

	for ( i = 0; i < ne->num_conditions; i++ ) {
	  lp = ne->conditions[i].predicate;

	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = ( ne->conditions[i].args[j] >= 0 ) ?
	      ne->conditions[i].args[j] : a->inst_table[DECODE_VAR( ne->conditions[i].args[j] )];
	  }

	  adr = fact_adress();
	  if ( !lpos[lp][adr] ) {/* condition not reachable: skip effect */
	    break;
	  }
	  if ( !lneg[lp][adr] ) {/* condition always true: skip it */
	    continue;
	  }
	  a->effects[a->num_effects].conditions[a->effects[a->num_effects].num_conditions++] =
	    lindex[lp][adr];
	}

	if ( i < ne->num_conditions ) {/* found unreachable condition: free condition space */
	  free( a->effects[a->num_effects].conditions );
	  continue;
	}

	/* now create the add and del effects.
	 */
	if ( ne->num_adds > 0 ) {
	  a->effects[a->num_effects].adds = ( int * ) calloc( ne->num_adds, sizeof( int ) );
	  a->effects[a->num_effects].adds_nondet = 
	    ( Bool * ) calloc( ne->num_adds, sizeof( Bool ) );
	}
	a->effects[a->num_effects].num_adds = 0;
	for ( i = 0; i < ne->num_adds; i++ ) {
	  lp = ne->adds[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = ( ne->adds[i].args[j] >= 0 ) ?
	      ne->adds[i].args[j] : a->inst_table[DECODE_VAR( ne->adds[i].args[j] )];
	  }
	  adr = fact_adress();

	  if ( !lneg[lp][adr] ) {/* effect always true: skip it */
	    continue;
	  }
	  
	  a->effects[a->num_effects].adds_nondet[a->effects[a->num_effects].num_adds] = 
	    ne->adds_nondet[i];
	  a->effects[a->num_effects].adds[a->effects[a->num_effects].num_adds++] = lindex[lp][adr];
	}

	if ( ne->num_dels > 0 ) {
	  a->effects[a->num_effects].dels = ( int * ) calloc( ne->num_dels, sizeof( int ) );
	  a->effects[a->num_effects].dels_nondet = 
	    ( Bool * ) calloc( ne->num_dels, sizeof( Bool ) );
	}
	a->effects[a->num_effects].num_dels = 0;
	for ( i = 0; i < ne->num_dels; i++ ) {
	  lp = ne->dels[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = ( ne->dels[i].args[j] >= 0 ) ?
	      ne->dels[i].args[j] : a->inst_table[DECODE_VAR( ne->dels[i].args[j] )];
	  }
	  adr = fact_adress();

	  if ( !lpos[lp][adr] ) {/* effect always false: skip it */
	    continue;
	  }

	  a->effects[a->num_effects].dels_nondet[a->effects[a->num_effects].num_dels] = 
	    ne->dels_nondet[i];
	  a->effects[a->num_effects].dels[a->effects[a->num_effects].num_dels++] = lindex[lp][adr];
	}
	if ( i < ne->num_dels ) break;

	/* this effect is OK. go to next one in NormOp.
	 */
	a->num_effects++;
	lnum_effects++;
      }
      if ( ne ) {
	/* we get here if one effect was faulty
	 */
	if ( p ) {
	  p->next = a->next;
	  t = a;
	  a = a->next;
	  free_single_Action( t );
	} else {
	  gactions = a->next;
	  t = a;
	  a = a->next;
	  free_single_Action( t );
	}
      } else {
	p = a;
	a = a->next;
      }
      continue;
    }
    if ( a->pseudo_action ) {
      /* action is result of a PseudoAction
       */
      pa = a->pseudo_action;

      if ( pa->num_preconds > 0 ) {
	a->preconds = ( int * ) calloc( pa->num_preconds, sizeof( int ) );
      }
      a->num_preconds = 0;
      for ( i = 0; i < pa->num_preconds; i++ ) {
	lp = pa->preconds[i].predicate;
	for ( j = 0; j < garity[lp]; j++ ) {
	  largs[j] = pa->preconds[i].args[j];
	}
	adr = fact_adress();
	
	/* preconds are lpos in all cases due to reachability analysis
	 */
	if ( !lneg[lp][adr] ) {
	  continue;
	}
	
	a->preconds[a->num_preconds++] = lindex[lp][adr];
      }

      if ( pa->num_observe > 0 ) {
	a->observe = ( int * ) calloc( pa->num_observe, sizeof( int ) );
      }
      a->num_observe = 0;
      for ( i = 0; i < pa->num_observe; i++ ) {
	lp = pa->observe[i].predicate;
	for ( j = 0; j < garity[lp]; j++ ) {
	  largs[j] = ( pa->observe[i].args[j] >= 0 ) ?
	    pa->observe[i].args[j] : a->inst_table[DECODE_VAR( pa->observe[i].args[j] )];
	}
	adr = fact_adress();

	/* facts with only one possible truth value need not be observed.
	 */
	if ( !lneg[lp][adr] ||
	     !lpos[lp][adr] ) {
	  continue;
	}
	
	a->observe[a->num_observe++] = lindex[lp][adr];
      }


      if ( a->num_effects > 0 ) {
	a->effects = ( ActionEffect * ) calloc( a->num_effects, sizeof( ActionEffect ) );
      }
      a->num_effects = 0;
      for ( pae = pa->effects; pae; pae = pae->next ) {
	if ( pae->num_conditions > 0 ) {
	  a->effects[a->num_effects].conditions =
	    ( int * ) calloc( pae->num_conditions, sizeof( int ) );
	}
	a->effects[a->num_effects].num_conditions = 0;

	for ( i = 0; i < pae->num_conditions; i++ ) {
	  lp = pae->conditions[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = pae->conditions[i].args[j];
	  }
	  adr = fact_adress();

	  if ( !lpos[lp][adr] ) {/* condition not reachable: skip effect */
	    break;
	  }
	  if ( !lneg[lp][adr] ) {/* condition always true: skip it */
	    continue;
	  }
	  
	  a->effects[a->num_effects].conditions[a->effects[a->num_effects].num_conditions++] =
	    lindex[lp][adr];
	}

	if ( i < pae->num_conditions ) {/* found unreachable condition: free condition space */
	  free( a->effects[a->num_effects].conditions );
	  continue;
	}

	/* now create the add and del effects.
	 */
	if ( pae->num_adds > 0 ) {
	  a->effects[a->num_effects].adds = ( int * ) calloc( pae->num_adds, sizeof( int ) );
	  a->effects[a->num_effects].adds_nondet = 
	    ( Bool * ) calloc( pae->num_adds, sizeof( Bool ) );
	}
	a->effects[a->num_effects].num_adds = 0;
	for ( i = 0; i < pae->num_adds; i++ ) {
	  lp = pae->adds[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = pae->adds[i].args[j];
	  }
	  adr = fact_adress();

	  if ( !lneg[lp][adr] ) {/* effect always true: skip it */
	    continue;
	  }
	  
	  a->effects[a->num_effects].adds_nondet[a->effects[a->num_effects].num_adds] = 
	    pae->adds_nondet[i];
	  a->effects[a->num_effects].adds[a->effects[a->num_effects].num_adds++] = lindex[lp][adr];
	}

	if ( pae->num_dels > 0 ) {
	  a->effects[a->num_effects].dels = ( int * ) calloc( pae->num_dels, sizeof( int ) );
	  a->effects[a->num_effects].dels_nondet = 
	    ( Bool * ) calloc( pae->num_dels, sizeof( Bool ) );
	}
	a->effects[a->num_effects].num_dels = 0;
	for ( i = 0; i < pae->num_dels; i++ ) {
	  lp = pae->dels[i].predicate;
	  for ( j = 0; j < garity[lp]; j++ ) {
	    largs[j] = pae->dels[i].args[j];
	  }
	  adr = fact_adress();

	  if ( !lpos[lp][adr] ) {/* effect always false: skip it */
	    continue;
	  }
	  
	  a->effects[a->num_effects].dels_nondet[a->effects[a->num_effects].num_dels] = 
	    pae->dels_nondet[i];
	  a->effects[a->num_effects].dels[a->effects[a->num_effects].num_dels++] = lindex[lp][adr];
	}
	if ( i < pae->num_dels ) break;

	/* this effect is OK. go to next one in PseudoAction.
	 */
	a->num_effects++;
	lnum_effects++;
      }
      if ( pae ) {
	/* we get here if one effect was faulty
	 */
	if ( p ) {
	  p->next = a->next;
	  t = a;
	  a = a->next;
	  free_single_Action( t );
	} else {
	  gactions = a->next;
	  t = a;
	  a = a->next;
	  free_single_Action( t );
	}
      } else {
	p = a;
	a = a->next;
      }
      continue;
    }/* end of if clause for PseudoAction */
    /* if action was neither normop, nor pseudo action determined,
     * then it is an artificial action due to disjunctive goal
     * conditions.
     *
     * these are already in final form.
     */
    p = a;
    a = a->next;
  }/* endfor all actions ! */

}















/**************************************************
 * CONNECTIVITY GRAPH. ULTRA CLEAN REPRESENTATION *
 **************************************************/













void build_connectivity_graph( void )

{

  int i, j, k, l, n_op, n_ef, na, nd, ef, ef_, m, adr, ind1, ind2, ft;
  Action *a;
  int *same_effects, sn;
  Bool *had_effects;
  ActionEffect *e, *e_;
  Facts *ff, *fff;
  WffNode *iwff, *jwff;
  Bool changes;

  int nr_srel, last_srel;

  struct timeb tp;

  ftime( &tp );
  srandom( tp.millitm );

  gnum_ft_conn = gnum_relevant_facts;
  gnum_op_conn = gnum_actions;
  gft_conn = ( FtConn * ) calloc( gnum_ft_conn, sizeof( FtConn ) );
  gop_conn = ( OpConn * ) calloc( gnum_op_conn, sizeof( OpConn ) );
  gef_conn = ( EfConn * ) calloc( lnum_effects, sizeof( EfConn ) );
  gnum_ef_conn = 0;

  same_effects = ( int * ) calloc( lnum_effects, sizeof( int ) );
  had_effects = ( Bool * ) calloc( lnum_effects, sizeof( Bool ) );

  for ( i = 0; i < gnum_ft_conn; i++ ) {
    gft_conn[i].negation = -1;

    gft_conn[i].num_P = 0;
    gft_conn[i].num_O = 0;
    gft_conn[i].num_PC = 0;
    gft_conn[i].num_C = 0;
    gft_conn[i].num_A = 0;
    gft_conn[i].num_D = 0;

    gft_conn[i].srelevant = FALSE;
    gft_conn[i].poss_U = FALSE;

    gft_conn[i].CNF = FALSE;

    gft_conn[i].rand = random() % BIG_INT;
  }

  for ( i = 0; i < gnum_op_conn; i++ ) {
    gop_conn[i].num_P = 0;
    gop_conn[i].num_O = 0;
    gop_conn[i].num_E = 0;

    gop_conn[i].num_EA = 0;
    gop_conn[i].num_EPC = 0;

    gop_conn[i].is_in_A = FALSE;
    gop_conn[i].is_in_H = FALSE;

    gop_conn[i].observation = FALSE;
  }

  for ( i = 0; i < lnum_effects; i++ ) {
    gef_conn[i].num_PC = 0;
    gef_conn[i].num_C = 0;
    gef_conn[i].num_A = 0;
    gef_conn[i].num_D = 0;
    gef_conn[i].num_I = 0;

    gef_conn[i].removed = FALSE;
  }

  /* set up the initial equivalence array(s)
   *
   * early cause we need negations info for observations
   */
  ginitial_equivalence_A = ( int * ) 
    calloc( gnum_initial_ft_equivalence, sizeof( int ) );
  ginitial_equivalence_notA = ( int * ) 
    calloc( gnum_initial_ft_equivalence, sizeof( int ) );
  gnum_initial_equivalence = 0;
  for ( ff = ginitial_ft_equivalence_A, 
	  fff = ginitial_ft_equivalence_notA; 
	ff; ff = ff->next, fff = fff->next ) {
    lp = ff->fact->predicate;

    for ( i = 0; i < garity[lp]; i++ ) {
      largs[i] = ff->fact->args[i];
    }
    adr = fact_adress();
    ind1 = lindex[lp][adr];
    if ( ind1 == -1 ) continue;
    ginitial_equivalence_A[gnum_initial_equivalence] = ind1;
    lp = fff->fact->predicate;
    for ( i = 0; i < garity[lp]; i++ ) {
      largs[i] = fff->fact->args[i];
    }
    adr = fact_adress();
    ind2 = lindex[lp][adr];
    if ( ind2 == -1 ) continue;
    ginitial_equivalence_notA[gnum_initial_equivalence++] = ind2;

    gft_conn[ind1].negation = ind2;
    gft_conn[ind2].negation = ind1;
  }



  n_op = 0;
  n_ef = 0;
  for ( a = gactions; a; a = a->next ) {

    gop_conn[n_op].action = a;

    if ( a->num_preconds > 0 ) {
      gop_conn[n_op].P = ( int * ) calloc( a->num_preconds, sizeof( int ) );
    }
    for ( i = 0; i < a->num_preconds; i++ ) {
      for ( j = 0; j < gop_conn[n_op].num_P; j++ ) {
	if ( gop_conn[n_op].P[j] == a->preconds[i] ) break;
      }
      if ( j < gop_conn[n_op].num_P ) continue;
      gop_conn[n_op].P[gop_conn[n_op].num_P++] = a->preconds[i];
    }

    if ( a->num_observe > 0 ) {
      gop_conn[n_op].O = ( int * ) calloc( a->num_observe, sizeof( int ) );
      gop_conn[n_op].observation = TRUE;
    }
    for ( i = 0; i < a->num_observe; i++ ) {
      for ( j = 0; j < gop_conn[n_op].num_O; j++ ) {
	if ( gop_conn[n_op].O[j] == a->observe[i] ) break;
      }
      if ( j < gop_conn[n_op].num_O ) continue;
      gop_conn[n_op].O[gop_conn[n_op].num_O++] = a->observe[i];
    }

    if ( a->num_effects > 0 ) {
      gop_conn[n_op].E = ( int * ) calloc( a->num_effects, sizeof( int ) );
    }
    for ( i = 0; i < a->num_effects; i++ ) {
      had_effects[i] = FALSE;
    }
    for ( i = 0; i < a->num_effects; i++ ) {
      if ( had_effects[i] ) {
	continue;
      }
      had_effects[i] = TRUE;
      e = &(a->effects[i]);
      gop_conn[n_op].E[gop_conn[n_op].num_E++] = n_ef;
      gef_conn[n_ef].op = n_op;

      gef_conn[n_ef].PC = ( int * ) 
	calloc( e->num_conditions + a->num_preconds, sizeof( int ) );
      gef_conn[n_ef].C = ( int * ) 
	calloc( e->num_conditions, sizeof( int ) );
      for ( j = 0; j < a->num_preconds; j++ ) {
	for ( k = 0; k < gef_conn[n_ef].num_PC; k++ ) {
	  if ( gef_conn[n_ef].PC[k] == a->preconds[j] ) break;
	}
	if ( k < gef_conn[n_ef].num_PC ) continue;
	gef_conn[n_ef].PC[gef_conn[n_ef].num_PC++] = a->preconds[j];
      }
      for ( j = 0; j < e->num_conditions; j++ ) {
	for ( k = 0; k < gef_conn[n_ef].num_PC; k++ ) {
	  if ( gef_conn[n_ef].PC[k] == e->conditions[j] ) break;
	}
	if ( k == gef_conn[n_ef].num_PC ) {
	    gef_conn[n_ef].PC[gef_conn[n_ef].num_PC++] = e->conditions[j];
	}
	/* do not include preconds into the effect conds;
	 *
	 * NOTE: thus we might accept non-unary antecedents, namely
	 *       when they're redundant in that sense anyway; is there
	 *       a generalization to this?
	 */
	for ( k = 0; k < gop_conn[n_op].num_P; k++ ) {
	  if ( gop_conn[n_op].P[k] == e->conditions[j] ) break;
	}
	if ( k < gop_conn[n_op].num_P ) {
	    continue;
	}
	for ( k = 0; k < gef_conn[n_ef].num_C; k++ ) {
	  if ( gef_conn[n_ef].C[k] == e->conditions[j] ) break;
	}
	if ( k == gef_conn[n_ef].num_C ) {
	    gef_conn[n_ef].C[gef_conn[n_ef].num_C++] = e->conditions[j];
	}
      }

      sn = 0;
      for ( j = i + 1; j < a->num_effects; j++ ) {
	if ( had_effects[j] ) {
	  continue;
	}
	e_ = &(a->effects[j]);
	/* check conditions
	 */
	for ( k = 0; k < e_->num_conditions; k++ ) {
	  for ( l = 0; l < e->num_conditions; l++ ) {
	    if ( e_->conditions[k] == e->conditions[l] ) {
	      break;
	    }
	  }
	  if ( l == e->num_conditions ) {
	    break;
	  }
	}
	if ( k < e_->num_conditions ) {
	  continue;
	}
	if ( e->num_conditions == e_->num_conditions ) {
	  same_effects[sn++] = j;
	}
      }

      na = e->num_adds;
      nd = e->num_dels;
      for ( j = 0; j < sn; j++ ) {
	na += a->effects[same_effects[j]].num_adds;
	nd += a->effects[same_effects[j]].num_dels;
      }
      gef_conn[n_ef].A = ( int * ) calloc( na, sizeof( int ) );
      gef_conn[n_ef].A_nondet = ( Bool * ) calloc( na, sizeof( Bool ) );
      gef_conn[n_ef].D = ( int * ) calloc( nd, sizeof( int ) );
      gef_conn[n_ef].D_nondet = ( Bool * ) calloc( nd, sizeof( Bool ) );
      for ( j = 0; j < e->num_adds; j++ ) {
	for ( k = 0; k < gef_conn[n_ef].num_A; k++ ) {
	  if ( gef_conn[n_ef].A[k] == e->adds[j] ) break;
	}
	if ( k < gef_conn[n_ef].num_A ) {
	  if ( !e->adds_nondet[j] ) {
	    gef_conn[n_ef].A_nondet[k] = FALSE;
	  }
	  continue;
	}
	/* adding conds is useless; need this for easier implementation of 
	 * stagnating states, see repeated_states.c at end
	 */
	for ( k = 0; k < gef_conn[n_ef].num_PC; k++ ) {
	  if ( gef_conn[n_ef].PC[k] == e->adds[j] ) break;
	}
	if ( k < gef_conn[n_ef].num_PC ) continue;
	gef_conn[n_ef].A_nondet[gef_conn[n_ef].num_A] = e->adds_nondet[j];
	gef_conn[n_ef].A[gef_conn[n_ef].num_A++] = e->adds[j];
      }
      for ( j = 0; j < e->num_dels; j++ ) {
	for ( k = 0; k < gef_conn[n_ef].num_D; k++ ) {
	  if ( gef_conn[n_ef].D[k] == e->dels[j] ) break;
	}
	if ( k < gef_conn[n_ef].num_D ) {
	  if ( !e->dels_nondet[j] ) {
	    gef_conn[n_ef].D_nondet[k] = FALSE;
	  }
	  continue;
	}
	gef_conn[n_ef].D_nondet[gef_conn[n_ef].num_D] = e->dels_nondet[j];
	gef_conn[n_ef].D[gef_conn[n_ef].num_D++] = e->dels[j];
      }
      for ( j = 0; j < sn; j++ ) {
	e_ = &(a->effects[same_effects[j]]);
	for ( l = 0; l < e_->num_adds; l++ ) {
	  for ( k = 0; k < gef_conn[n_ef].num_A; k++ ) {
	    if ( gef_conn[n_ef].A[k] == e_->adds[l] ) break;
	  }
	  if ( k < gef_conn[n_ef].num_A ) {
	    if ( !e_->adds_nondet[l] ) {
	      gef_conn[n_ef].A_nondet[k] = FALSE;
	    }
	    continue;
	  }
	  /* adding conds is useless; need this for easier implementation of 
	   * stagnating states, see repeated_states.c at end
	   */
	  for ( k = 0; k < gef_conn[n_ef].num_PC; k++ ) {
	    if ( gef_conn[n_ef].PC[k] == e_->adds[l] ) break;
	  }
	  if ( k < gef_conn[n_ef].num_PC ) continue;
	  gef_conn[n_ef].A_nondet[gef_conn[n_ef].num_A] = e_->adds_nondet[l];
	  gef_conn[n_ef].A[gef_conn[n_ef].num_A++] = e_->adds[l];
	}
	for ( l = 0; l < e_->num_dels; l++ ) {
	  for ( k = 0; k < gef_conn[n_ef].num_D; k++ ) {
	    if ( gef_conn[n_ef].D[k] == e_->dels[l] ) break;
	  }
	  if ( k < gef_conn[n_ef].num_D ) {
	    if ( !e_->dels_nondet[l] ) {
	      gef_conn[n_ef].D_nondet[k] = FALSE;
	    }
	    continue;
	  }
	  gef_conn[n_ef].D_nondet[gef_conn[n_ef].num_D] = e_->dels_nondet[l];
	  gef_conn[n_ef].D[gef_conn[n_ef].num_D++] = e_->dels[l];
	}
      }
      for ( j = 0; j < sn; j++ ) {
	had_effects[same_effects[j]] = TRUE;
      }
      
      n_ef++;
      gnum_ef_conn++;
    }/* ende all a->effects */


    if ( gop_conn[n_op].num_E >= 1 ) {
      /* CHECK EMPTY EFFECTS!
       *
       * two step process --- first, remove all effects that are entirely empty.
       *                      second, check if all remaining effects are illegal
       *                      or only delete:
       *                      in that case, and if the op observes nothing, the op will 
       *                      never do any good so we remove all its effects.
       */
      i = 0;
      while ( i < gop_conn[n_op].num_E ) {
	if ( gef_conn[gop_conn[n_op].E[i]].num_A != 0 ||
	     gef_conn[gop_conn[n_op].E[i]].num_D != 0 ) {
	  i++;
	  continue;
	}
	/* we keep it in the gef_conn (seems easier), 
	 * but mark it as removed, which will exclude it from everything.
	 */
	gef_conn[gop_conn[n_op].E[i]].removed = TRUE;
	for ( j = i; j < gop_conn[n_op].num_E - 1; j++ ) {
	  gop_conn[n_op].E[j] = gop_conn[n_op].E[j+1];
	}
	gop_conn[n_op].num_E--;
      }

      if ( gop_conn[n_op].num_O == 0 ) {
	m = 0;
	for ( i = 0; i < gop_conn[n_op].num_E; i++ ) {
	  if ( gef_conn[gop_conn[n_op].E[i]].num_A == 0 ) {
	    m++;
	  }
	}
	if ( m == gop_conn[n_op].num_E ) {
	  /* all remaining effects solely-deleters.
	   */
	  for ( i = 0; i < gop_conn[n_op].num_E; i++ ) {
	    gef_conn[gop_conn[n_op].E[i]].removed = TRUE;
	  }
	  gop_conn[n_op].num_E = 0;
	}
      }
    }

    /* setup implied effects info
     */
    if ( gop_conn[n_op].num_E > 1 ) {
      for ( i = 0; i < gop_conn[n_op].num_E; i++ ) {
	ef = gop_conn[n_op].E[i];
	gef_conn[ef].I = ( int * ) calloc( gop_conn[n_op].num_E, sizeof( int ) );
	gef_conn[ef].num_I = 0;
      }    
      for ( i = 0; i < gop_conn[n_op].num_E - 1; i++ ) {
	ef = gop_conn[n_op].E[i];
	for ( j = i+1; j < gop_conn[n_op].num_E; j++ ) {
	  ef_ = gop_conn[n_op].E[j];
	  /* ef ==> ef_ ? */
	  for ( k = 0; k < gef_conn[ef_].num_PC; k++ ) {
	    for ( l = 0; l < gef_conn[ef].num_PC; l++ ) {
	      if ( gef_conn[ef].PC[l] == gef_conn[ef_].PC[k] ) break;
	    }
	    if ( l == gef_conn[ef].num_PC ) break;
	  }
	  if ( k == gef_conn[ef_].num_PC ) {
	    gef_conn[ef].I[gef_conn[ef].num_I++] = ef_;
	  }
	  /* j ==> i ? */
	  for ( k = 0; k < gef_conn[ef].num_PC; k++ ) {
	    for ( l = 0; l < gef_conn[ef_].num_PC; l++ ) {
	      if ( gef_conn[ef_].PC[l] == gef_conn[ef].PC[k] ) break;
	    }
	    if ( l == gef_conn[ef_].num_PC ) break;
	  }
	  if ( k == gef_conn[ef].num_PC ) {
	    gef_conn[ef_].I[gef_conn[ef_].num_I++] = ef;
	  }
	}
      }
    }

    /* first sweep: only count the space we need for the fact arrays !
     */
    for ( j = 0; j < gop_conn[n_op].num_P; j++ ) {
      gft_conn[gop_conn[n_op].P[j]].num_P++;
    }
    /* put also into negated fts that they are observed by op
     * --> used in relax.c
     */
    for ( j = 0; j < gop_conn[n_op].num_O; j++ ) {
      ft = gop_conn[n_op].O[j];
      gft_conn[ft].num_O++;
      if ( gft_conn[ft].negation != -1 ) {
	gft_conn[gft_conn[ft].negation].num_O++;
      }
    }
    if ( gop_conn[n_op].num_E > 0 ) {
      for ( i = 0; i < gop_conn[n_op].num_E; i++ ) {
	ef = gop_conn[n_op].E[i];
	if ( gef_conn[ef].removed ) continue;
	for ( j = 0; j < gef_conn[ef].num_PC; j++ ) {
	  gft_conn[gef_conn[ef].PC[j]].num_PC++;
	}
	for ( j = 0; j < gef_conn[ef].num_C; j++ ) {
	  gft_conn[gef_conn[ef].C[j]].num_C++;
	}
 	for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
	  gft_conn[gef_conn[ef].A[j]].num_A++;
	}
	for ( j = 0; j < gef_conn[ef].num_D; j++ ) {
	  gft_conn[gef_conn[ef].D[j]].num_D++;
	}
      }
    }

    n_op++;
  }/* actions loop */



  for ( i = 0; i < gnum_ft_conn; i++ ) {
    if ( gft_conn[i].num_P > 0 ) {
      gft_conn[i].P = ( int * ) calloc( gft_conn[i].num_P, sizeof( int ) );
    }
    gft_conn[i].num_P = 0;
    if ( gft_conn[i].num_O > 0 ) {
      gft_conn[i].O = ( int * ) calloc( gft_conn[i].num_O, sizeof( int ) );
    }
    gft_conn[i].num_O = 0;
    if ( gft_conn[i].num_PC > 0 ) {
      gft_conn[i].PC = ( int * ) calloc( gft_conn[i].num_PC, sizeof( int ) );
      gft_conn[i].srelevant = TRUE;
    }
    gft_conn[i].num_PC = 0;
    if ( gft_conn[i].num_C > 0 ) {
      gft_conn[i].C = ( int * ) calloc( gft_conn[i].num_C, sizeof( int ) );
    }
    gft_conn[i].num_C = 0;
    if ( gft_conn[i].num_A > 0 ) {
      gft_conn[i].A = ( int * ) calloc( gft_conn[i].num_A, sizeof( int ) );
      gft_conn[i].A_nondet = ( Bool * ) calloc( gft_conn[i].num_A, sizeof( Bool ) );
    }
    gft_conn[i].num_A = 0;
    if ( gft_conn[i].num_D > 0 ) {
      gft_conn[i].D = ( int * ) calloc( gft_conn[i].num_D, sizeof( int ) );
      gft_conn[i].D_nondet = ( Bool * ) calloc( gft_conn[i].num_D, sizeof( Bool ) );
    }
    gft_conn[i].num_D = 0;

    gft_conn[i].is_global_goal = FALSE;
  }
  for ( i = 0; i < ggoal_state.num_F; i++ ) {
    gft_conn[ggoal_state.F[i]].is_global_goal = TRUE;
    gft_conn[ggoal_state.F[i]].srelevant = TRUE;
   }

  for ( i = 0; i < gnum_op_conn; i++ ) {
	  for ( j = 0; j < gop_conn[i].num_P; j++ ) {
	      gft_conn[gop_conn[i].P[j]].P[gft_conn[gop_conn[i].P[j]].num_P++] = i;
      }
    for ( j = 0; j < gop_conn[i].num_O; j++ ) {
      ft = gop_conn[i].O[j];
      gft_conn[ft].O[gft_conn[ft].num_O++] = i;
      if ( gft_conn[ft].negation != -1 ) {
	gft_conn[gft_conn[ft].negation].O[gft_conn[gft_conn[ft].negation].num_O++] = i;
      }
    }
  }

  gmax_C = -1;
  for ( i = 0; i < gnum_ef_conn; i++ ) {
    if ( gef_conn[i].removed ) continue;
    if ( gmax_C == -1 || gef_conn[i].num_C > gmax_C ) {
      gmax_C = gef_conn[i].num_C;
    }
    for ( j = 0; j < gef_conn[i].num_PC; j++ ) {
      gft_conn[gef_conn[i].PC[j]].PC[gft_conn[gef_conn[i].PC[j]].num_PC++] = i;
    }
    for ( j = 0; j < gef_conn[i].num_C; j++ ) {
      gft_conn[gef_conn[i].C[j]].C[gft_conn[gef_conn[i].C[j]].num_C++] = i;
    }
    for ( j = 0; j < gef_conn[i].num_A; j++ ) {
      gft_conn[gef_conn[i].A[j]].A_nondet[gft_conn[gef_conn[i].A[j]].num_A] =
	gef_conn[i].A_nondet[j];
      gft_conn[gef_conn[i].A[j]].A[gft_conn[gef_conn[i].A[j]].num_A++] = i;
    }
    for ( j = 0; j < gef_conn[i].num_D; j++ ) {
      gft_conn[gef_conn[i].D[j]].D_nondet[gft_conn[gef_conn[i].D[j]].num_D] =
	gef_conn[i].D_nondet[j];
      gft_conn[gef_conn[i].D[j]].D[gft_conn[gef_conn[i].D[j]].num_D++] = i;
    }
  }

  free( same_effects );
  free( had_effects );

  /* finally, do the initial OR stuff etc.
   *
   * first count how many cond effs we maximally need to remember.
   */
  gmax_E = -1;
  for ( i = 0; i < gnum_op_conn; i++ ) {
    if ( gmax_E == -1 || gop_conn[i].num_E > gmax_E ) {
      gmax_E = gop_conn[i].num_E;
    }
  }

  /* and the initial ORs
   *
   * NOTE: HERE, WE TRANSLATE THE ONEOF CONSTRAINTS INTO ORS!!
   */
  /* first, count how many ORs we'll need to introduce.
   */
  j = gnum_full_or_initial;
  for ( i = 0; i < gnum_full_oneof_initial; i++ ) {
    k = 0;
    for ( iwff = gfull_oneof_initial[i]->sons; iwff; iwff = iwff->next ) k++;
    if ( k == 1 ) {
      printf("\n\nONEOF constraint is on a single fact?!\n\n");
      exit( 1 );
    }
    /* this oneof constraint has k members; need 1 non-empty, and
     * k * (k-1) / 2 exclusions ORs.
     */
    j++;
    j += k * (k-1) / 2;
  }
  /* now introduce the ORs given in I, as well as those forced by ONEOF.
   */
  ginitial_or = ( int ** ) calloc( j, sizeof( int * ) );
  ginitial_or_length = ( int * ) calloc( j, sizeof( int ) );
  for ( i = 0; i < gnum_full_or_initial; i++ ) {
    k = 0;
    for ( iwff = gfull_or_initial[i]->sons; iwff; iwff = iwff->next ) k++;
    ginitial_or[i] = ( int * ) calloc( k, sizeof( int ) );

    ginitial_or_length[i] = 0;
    for ( iwff = gfull_or_initial[i]->sons; iwff; iwff = iwff->next ) {
      lp = iwff->fact->predicate;
      for ( j = 0; j < garity[lp]; j++ ) {
	largs[j] = iwff->fact->args[j];
      }
      adr = fact_adress();
      ind1 = lindex[lp][adr];
      if ( ind1 == -1 ) {
	printf("\n\ninitially OR-constr ft not indexed relevant?\n\n");
	exit( 1 );
      }
      ginitial_or[i][ginitial_or_length[i]++] = ind1;
    }
  }
  gnum_initial_or = gnum_full_or_initial;
  /* here come the ONEOFs
   */
  for ( i = 0; i < gnum_full_oneof_initial; i++ ) {
    k = 0;
    for ( iwff = gfull_oneof_initial[i]->sons; iwff; iwff = iwff->next ) k++;
    /* 1., the non-empty constr.
     */
    ginitial_or[gnum_initial_or] = ( int * ) calloc( k, sizeof( int ) );
    for ( iwff = gfull_oneof_initial[i]->sons; iwff; iwff = iwff->next ) {
      lp = iwff->fact->predicate;
      for ( j = 0; j < garity[lp]; j++ ) {
	largs[j] = iwff->fact->args[j];
      }
      adr = fact_adress();
      ind1 = lindex[lp][adr];
      if ( ind1 == -1 ) {
	printf("\n\ninitially ONEOF-constr ft not indexed relevant?\n\n");
	exit( 1 );
      }
      ginitial_or[gnum_initial_or][ginitial_or_length[gnum_initial_or]++] = ind1;
    }
    gnum_initial_or++;
    /* 2., the exclusion constr.
     */
    for ( iwff = gfull_oneof_initial[i]->sons; iwff->next; iwff = iwff->next ) {
      for ( jwff = iwff->next; jwff; jwff = jwff->next ) {
	lp = iwff->fact->predicate;
	for ( j = 0; j < garity[lp]; j++ ) {
	  largs[j] = iwff->fact->args[j];
	}
	adr = fact_adress();
	ind1 = lindex[lp][adr];
	if ( ind1 == -1 ) {
	  printf("\n\ninitially ONEOF-constr ft not indexed relevant?\n\n");
	  exit( 1 );
	}
	lp = jwff->fact->predicate;
	for ( j = 0; j < garity[lp]; j++ ) {
	  largs[j] = jwff->fact->args[j];
	}
	adr = fact_adress();
	ind2 = lindex[lp][adr];
	if ( ind2 == -1 ) {
	  printf("\n\ninitially ONEOF-constr ft not indexed relevant?\n\n");
	  exit( 1 );
	}
	if ( ind1 == ind2 ) {
	  printf("\n\nsame fact appears twice in a ONEOF constraint??\n\n");
	  exit( 1 );
	}
	/* really, what we want is the negated version of these facts.
	 */
	ind1 = gft_conn[ind1].negation;
	if ( ind1 == -1 ) {
	  printf("\n\nfact in ONEOF constraint not neg translated??\n\n");
	  exit( 1 );
	}
	ind2 = gft_conn[ind2].negation;
	if ( ind2 == -1 ) {
	  printf("\n\nfact in ONEOF constraint not neg translated??\n\n");
	  exit( 1 );
	}
	ginitial_or[gnum_initial_or] = ( int * ) calloc( 2, sizeof( int ) );
	ginitial_or[gnum_initial_or][0] = ind1;
	ginitial_or[gnum_initial_or][1] = ind2;
	ginitial_or_length[gnum_initial_or] = 2;
	gnum_initial_or++;
      }/* jwff */
    }/* iwff */
  }/* i */


  /* say which of the equivalent fts to use in the CNFs
   */
  for ( i = 0; i < gnum_ft_conn; i++ ) {
    if ( gft_conn[i].negation == -1 ) {
      gft_conn[i].CNF = TRUE;
      continue;
    }
    if ( gft_conn[gft_conn[i].negation].CNF ) {
      continue;
    }
    gft_conn[i].CNF = TRUE;
  }
  gmax_U = 0;
  gmax_CNFU = 0;
  for ( i = 0; i < ginitial_state.num_U; i++ ) {
    if ( gft_conn[ginitial_state.U[i]].poss_U ) {
      printf("\n\n1 fact is unknown twice in initial state?? - %d, %d\n\n", 
	     i, ginitial_state.U[i]); print_ft_name(ginitial_state.U[i]); 
      exit( 1 );
    }
    gft_conn[ginitial_state.U[i]].poss_U = TRUE;
    gmax_U++;
    if ( !gft_conn[ginitial_state.U[i]].CNF ) continue;
    gmax_CNFU++;
  }
  /* make fts poss_U that are affected by a nondet effect
   */
  for ( i = 0; i < gnum_ef_conn; i++ ) {
    for ( j = 0; j < gef_conn[i].num_A; j++ ) {
      if ( !gef_conn[i].A_nondet[j] ) continue;
      if ( !gft_conn[gef_conn[i].A[j]].poss_U ) {
	gft_conn[gef_conn[i].A[j]].poss_U = TRUE;
	gmax_U++;
	if ( gft_conn[gef_conn[i].A[j]].CNF ) gmax_CNFU++;
      }
    }
    for ( j = 0; j < gef_conn[i].num_D; j++ ) {
      if ( !gef_conn[i].D_nondet[j] ) continue;
      if ( !gft_conn[gef_conn[i].D[j]].poss_U ) {
	gft_conn[gef_conn[i].D[j]].poss_U = TRUE;
	gmax_U++;
	if ( gft_conn[gef_conn[i].D[j]].CNF ) gmax_CNFU++;
      }
    }
  }

  changes = TRUE;
  while ( changes ) {
    changes = FALSE;

    /* make fts poss_U that are affected by a cond effect with
     * (curr) poss_U conds
     */
    for ( i = 0; i < gnum_ef_conn; i++ ) {
      if ( gef_conn[i].removed ) continue;

      for ( j = 0; j < gef_conn[i].num_C; j++ ) {
	if ( gft_conn[gef_conn[i].C[j]].poss_U ) break;
      }
      if ( j == gef_conn[i].num_C ) {
	for ( j = 0; j < gop_conn[gef_conn[i].op].num_P; j++ ) {
	  if ( gft_conn[gop_conn[gef_conn[i].op].P[j]].poss_U &&
	       gft_conn[gop_conn[gef_conn[i].op].P[j]].num_O > 0 ) break;
	}
	if ( j == gop_conn[gef_conn[i].op].num_P ) continue;
      }

      for ( j = 0; j < gef_conn[i].num_A; j++ ) {
	if ( !gft_conn[gef_conn[i].A[j]].poss_U ) {
	  gft_conn[gef_conn[i].A[j]].poss_U = TRUE;
	  gmax_U++;
	  changes = TRUE;
	  if ( gft_conn[gef_conn[i].A[j]].CNF ) gmax_CNFU++;
	}
      }
      for ( j = 0; j < gef_conn[i].num_D; j++ ) {
	if ( !gft_conn[gef_conn[i].D[j]].poss_U ) {
	  gft_conn[gef_conn[i].D[j]].poss_U = TRUE;
	  gmax_U++;
	  changes = TRUE;
	  if ( gft_conn[gef_conn[i].D[j]].CNF ) gmax_CNFU++;
	}
      }
    }
  }


  /* apply simple tests if domination is valid
   */
  gdomination_valid = TRUE;
  for ( i = 0; i < gnum_ef_conn; i++ ) {
    if ( gef_conn[i].num_C == 0 ) continue;
    nr_srel = 0; 
    last_srel = -1;
    for ( j = 0; j < gef_conn[i].num_D; j++ ) {
      if ( gft_conn[gef_conn[i].D[j]].srelevant ) {
	nr_srel++;
	last_srel = j;
      }
    }
    if ( nr_srel == 0 ) continue;
    if ( gef_conn[i].num_C > 1 ||
	 nr_srel > 1 ) {
      break;
    }
    if ( gef_conn[i].D[last_srel] != gef_conn[i].C[0] ) {
      break;
    }
  }
  if ( i < gnum_ef_conn ) {
    gdomination_valid = FALSE;
    if ( gcmd_line.display_info > 0 ) {
      printf("\ndomination test not valid. turning full state equality on.\n\n");
    }
  }








  /* set up the EA and EPC arrays
   */
  for ( i = 0; i < gnum_op_conn; i++ ) {
    gop_conn[i].num_EA = 0;
    gop_conn[i].num_EPC = 0;
    for ( j = 0; j < gop_conn[i].num_E; j++ ) {
      ef = gop_conn[i].E[j];
      gop_conn[i].num_EA += gef_conn[ef].num_A;
      gop_conn[i].num_EPC += gef_conn[ef].num_C;
    }
    gop_conn[i].num_EPC += gop_conn[i].num_P;
    gop_conn[i].EA = ( int * ) calloc( gop_conn[i].num_EA, sizeof( int ) );
    gop_conn[i].EPC = ( int * ) calloc( gop_conn[i].num_EPC, sizeof( int ) );
    gop_conn[i].num_EA = 0;
    gop_conn[i].num_EPC = 0;

    for ( j = 0; j < gop_conn[i].num_E; j++ ) {
      ef = gop_conn[i].E[j];
      for ( k = 0; k < gef_conn[ef].num_A; k++ ) {
	ft = gef_conn[ef].A[k];
	for ( l = 0; l < gop_conn[i].num_EA; l++ ) {
	  if ( gop_conn[i].EA[l] == ft ) break;
	}
	if ( l < gop_conn[i].num_EA ) continue;
	gop_conn[i].EA[gop_conn[i].num_EA++] = ft;
      }
      for ( k = 0; k < gef_conn[ef].num_C; k++ ) {
	ft = gef_conn[ef].C[k];
	for ( l = 0; l < gop_conn[i].num_EPC; l++ ) {
	  if ( gop_conn[i].EPC[l] == ft ) break;
	}
	if ( l < gop_conn[i].num_EPC ) continue;
	gop_conn[i].EPC[gop_conn[i].num_EPC++] = ft;
      }
    }
    for ( j = 0; j < gop_conn[i].num_P; j++ ) {
      ft = gop_conn[i].P[j];
      for ( l = 0; l < gop_conn[i].num_EPC; l++ ) {
	if ( gop_conn[i].EPC[l] == ft ) break;
      }
      if ( l < gop_conn[i].num_EPC ) continue;
      gop_conn[i].EPC[gop_conn[i].num_EPC++] = ft;
    }
  }








  /* set up -- and check -- for single nondeteff literal
   */
  for ( i = 0; i < gnum_op_conn; i++ ) {
    gop_conn[i].nondeteff = 0;
    for ( j = 0; j < gop_conn[i].num_E; j++ ) {
      ef = gop_conn[i].E[j];
      for ( k = 0; k < gef_conn[ef].num_A; k++ ) {
	if ( gef_conn[ef].A_nondet[k] ) {
	  if ( gop_conn[i].nondeteff != 0 ) {
	    ft = gef_conn[ef].A[k];
	    if ( gft_conn[ft].negation != -1 ) {
	      if ( gop_conn[i].nondeteff < 0 &&
		   (gft_conn[ft].negation == ((-1)*gop_conn[i].nondeteff)-1) ) {
		if ( gcmd_line.display_info ) {
		  printf("\nwarning: skipping nondet addeff on negated fact");
		}
		continue;
	      }
	    }
	    printf("\n\nsorry -- this version of cff can only handle a single nondet eff lit per action\n\n");
	    exit( 1 );
	  }
	  gop_conn[i].nondeteff = gef_conn[ef].A[k]+1;
	  if ( gef_conn[ef].num_C > 0 ) {
	    printf("\nsorry, no conditional nondet effs");
	    exit( 1 );
	  }
	  gop_conn[i].nondeteff_C = ( int * ) calloc( gef_conn[ef].num_C, sizeof( int ) );
	  for ( l = 0; l < gef_conn[ef].num_C; l++ ) {
	    gop_conn[i].nondeteff_C[l] = gef_conn[ef].C[l];
	  }
	  gop_conn[i].num_nondeteff_C = gef_conn[ef].num_C;
	} else {/* det eff; just check for interferences */
	  if ( (gop_conn[i].nondeteff < 0 &&
		gef_conn[ef].A[k] == ((-1)*gop_conn[i].nondeteff)-1) ||
	       (gop_conn[i].nondeteff < 0 &&
		gft_conn[gef_conn[ef].A[k]].negation == ((-1)*gop_conn[i].nondeteff)-1) ||
	       (gop_conn[i].nondeteff > 0 &&
		gef_conn[ef].A[k] == gop_conn[i].nondeteff-1) ||
	       (gop_conn[i].nondeteff > 0 &&
		gft_conn[gef_conn[ef].A[k]].negation == gop_conn[i].nondeteff-1) ) {
	    printf("\nsorry, no interferences between nondets and dets effs\n\n");
	    exit( 1 );
	  }
	}
      }
      for ( k = 0; k < gef_conn[ef].num_D; k++ ) {
	if ( gef_conn[ef].D_nondet[k] ) {
	  if ( gop_conn[i].nondeteff != 0 ) {
	    ft = gef_conn[ef].D[k];
	    if ( gft_conn[ft].negation != -1 ) {
	      if ( gop_conn[i].nondeteff > 0 &&
		   (gft_conn[ft].negation == gop_conn[i].nondeteff-1 ) ) {
		if ( gcmd_line.display_info ) {
		  printf("\nwarning: skipping nondet deleff on negated fact");
		}
		continue;
	      }
	    }
	    printf("\n\nsorry -- this version of cff can only handle a single nondet eff lit per action\n\n");
	    exit( 1 );
	  }
	  gop_conn[i].nondeteff = (gef_conn[ef].D[k]+1) * (-1);
	  if ( gef_conn[ef].num_C > 0 ) {
	    printf("\nsorry, no conditional nondet effs");
	    exit( 1 );
	  }
	  gop_conn[i].nondeteff_C = ( int * ) calloc( gef_conn[ef].num_C, sizeof ( int ) );
	  for ( l = 0; l < gef_conn[ef].num_C; l++ ) {
	    gop_conn[i].nondeteff_C[l] = gef_conn[ef].C[l];
	  }
	  gop_conn[i].num_nondeteff_C = gef_conn[ef].num_C;
	} else {/* det eff; just check for interferences */
	  if ( (gop_conn[i].nondeteff < 0 &&
		gef_conn[ef].D[k] == ((-1)*gop_conn[i].nondeteff)-1) ||
	       (gop_conn[i].nondeteff < 0 &&
		gft_conn[gef_conn[ef].D[k]].negation == ((-1)*gop_conn[i].nondeteff)-1) ||
	       (gop_conn[i].nondeteff > 0 &&
		gef_conn[ef].D[k] == gop_conn[i].nondeteff-1) ||
	       (gop_conn[i].nondeteff > 0 &&
		gft_conn[gef_conn[ef].D[k]].negation == gop_conn[i].nondeteff-1) ) {
	    printf("\nsorry, no interferences between nondets and dets effs\n\n");
	    exit( 1 );
	  }
	} /* det/nondet */
      } /* for dels of ef */
    } /* for the effects of the op */
  } /* for the ops */












  if ( gcmd_line.display_info == 121 ) {
    printf("\n\ncreated connectivity graph as follows:");

    printf("\n\n------------------OP ARRAY:-----------------------");
    for ( i = 0; i < gnum_op_conn; i++ ) {
      printf("\n\nOP: ");
      print_op_name( i );
      printf("\n----------PS:");
      for ( j = 0; j < gop_conn[i].num_P; j++ ) {
	printf("\n");
	print_ft_name( gop_conn[i].P[j] );
      }
      printf("\n----------OS:");
      for ( j = 0; j < gop_conn[i].num_O; j++ ) {
	printf("\n");
	print_ft_name( gop_conn[i].O[j] );
      }
      printf("\n----------EFFS:");
      for ( j = 0; j < gop_conn[i].num_E; j++ ) {
	printf("\neffect %d", gop_conn[i].E[j]);
      }
      printf("\n----------NONDETEFF: %d", gop_conn[i].nondeteff );
      printf("\n--that is, ");
      if ( gop_conn[i].nondeteff == 0 ) {
	printf("NONE");
      }
      if ( gop_conn[i].nondeteff > 0 ) {
	print_ft_name( gop_conn[i].nondeteff-1 );
      }
      if ( gop_conn[i].nondeteff < 0 ) {
	printf("NOT ");
	print_ft_name( ((-1)*gop_conn[i].nondeteff)-1 );
      }
      printf("\n----------CS:");
      for ( j = 0; j < gop_conn[i].num_nondeteff_C; j++ ) {
	printf("\n");
	print_ft_name( gop_conn[i].nondeteff_C[j] );
      }
    }
    
    printf("\n\n-------------------EFFECT ARRAY:----------------------");
    for ( i = 0; i < gnum_ef_conn; i++ ) {
      printf("\n\neffect %d of op %d: ", i, gef_conn[i].op);
      print_op_name( gef_conn[i].op );
      if ( gef_conn[i].removed ) {
	printf(" --- REMOVED ");
	continue;
      }
      printf("\n----------PCS:");
      for ( j = 0; j < gef_conn[i].num_PC; j++ ) {
	printf("\n");
	print_ft_name( gef_conn[i].PC[j] );
      }
      printf("\n----------CS:");
      for ( j = 0; j < gef_conn[i].num_C; j++ ) {
	printf("\n");
	print_ft_name( gef_conn[i].C[j] );
      }
      printf("\n----------ADDS:");
      for ( j = 0; j < gef_conn[i].num_A; j++ ) {
	printf("\n");
	if ( gef_conn[i].A_nondet[j] ) {
	  printf("nondet - ");
	}
	print_ft_name( gef_conn[i].A[j] );
      }
      printf("\n----------DELS:");
      for ( j = 0; j < gef_conn[i].num_D; j++ ) {
	printf("\n");
	if ( gef_conn[i].D_nondet[j] ) {
	  printf("nondet - ");
	}
	print_ft_name( gef_conn[i].D[j] );
      }
      printf("\n----------IMPLIEDS:");
      for ( j = 0; j < gef_conn[i].num_I; j++ ) {
	printf("\nimplied effect %d of op %d: ", 
	       gef_conn[i].I[j], gef_conn[gef_conn[i].I[j]].op);
	print_op_name( gef_conn[gef_conn[i].I[j]].op );
      }
    }
    
    printf("\n\n----------------------FT ARRAY:-----------------------------");
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      printf("\n\nFT %d: ", i);
      print_ft_name( i );
      printf(" negation: %d", gft_conn[i].negation);
      printf(" rand: %d, poss U: %d, CNF: %d", 
	     gft_conn[i].rand, gft_conn[i].poss_U, gft_conn[i].CNF);
      printf("\n----------PRECOND OF:");
      for ( j = 0; j < gft_conn[i].num_P; j++ ) {
	printf("\n");
	print_op_name( gft_conn[i].P[j] );
      }
      printf("\n----------OBSERVED BY:");
      for ( j = 0; j < gft_conn[i].num_O; j++ ) {
	printf("\n");
	print_op_name( gft_conn[i].O[j] );
      }
      printf("\n----------PRE or COND OF:");
      for ( j = 0; j < gft_conn[i].num_PC; j++ ) {
	printf("\neffect %d", gft_conn[i].PC[j]);
      }
      printf("\n----------COND OF:");
      for ( j = 0; j < gft_conn[i].num_C; j++ ) {
	printf("\neffect %d", gft_conn[i].C[j]);
      }
      printf("\n----------ADD BY:");
      for ( j = 0; j < gft_conn[i].num_A; j++ ) {
	printf("\neffect %d", gft_conn[i].A[j]);
	if ( gft_conn[i].A_nondet[j] ) {
	  printf(" - nondet");
	}
      }
      printf("\n----------DEL BY:");
      for ( j = 0; j < gft_conn[i].num_D; j++ ) {
	printf("\neffect %d", gft_conn[i].D[j]);
	if ( gft_conn[i].D_nondet[j] ) {
	  printf(" - nondet");
	}
      }
    }
    
    printf("\n\n----------------------INITIAL EQUIVALENCES:-----------------------------");
    for ( i = 0; i < gnum_initial_equivalence; i++ ) {
      printf("\nneg ");
      print_ft_name( ginitial_equivalence_A[i] );
      printf(" <-> ");
      print_ft_name( ginitial_equivalence_notA[i] );
    }
    
    printf("\n\n----------------------INITIAL ORS:-----------------------------");
    for ( i = 0; i < gnum_initial_or; i++ ) {
      printf("\nOR: ");
      for ( j = 0; j < ginitial_or_length[i]; j++ ) {
	print_ft_name( ginitial_or[i][j] );
	printf(" ");
      }
    }
    fflush( stdout );
  }
    
}



