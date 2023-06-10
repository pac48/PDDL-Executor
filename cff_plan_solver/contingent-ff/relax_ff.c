/*********************************************************************
 * (C) Copyright 2001 Albert Ludwigs University Freiburg
 *     Institute of Computer Science
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 *********************************************************************/


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
 * File: relax.c
 * Description: this file handles the relaxed planning problem, i.e.,
 *              the code is responsible for the heuristic evaluation
 *              of states during search.
 *
 *              --- THE HEART PIECE OF THE FF PLANNER ! ---
 *
 *              - fast (time critical) computation of the relaxed fixpoint
 *              - extraction of as short as possible plans, without search
 *
 *
 *
 *  ----- UPDATED VERSION TO HANDLE NORMALIZED ADL OPERATORS -----
 *
 *
 *
 * Author: Joerg Hoffmann 2000
 *
 *********************************************************************/ 


#include "ff.h"

#include "output.h"
#include "memory.h"

#include "relax_ff.h"
#include "search.h"



/* local globals
 */


/*Alvaro: For plan extraction */
int *gin_plan_E;
int gnum_in_plan_E;





/* in agenda driven algorithm, the current set of goals is this
 */
State lff_current_goals;



/* fixpoint
 */
int *lff_F;
int lff_num_F;
int *lff_E;
int lff_num_E;

int *lff_ch_E;
int lff_num_ch_E;

int *lff_0P_E;
int lff_num_0P_E;





/* 1P extraction
 */
int **lff_goals_at;
int *lff_num_goals_at;

int *lff_ch_F;
int lff_num_ch_F;

int *lff_used_O;
int lff_num_used_O;

int lff_h;










/*************************************
 * helper, for -1 == INFINITY method *
 *************************************/












Bool ff_LESS( int a, int b )

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











void ff_initialize_relax( void )

{

  make_state( &lff_current_goals, gnum_ft_conn );
  lff_current_goals.max_F = gnum_ft_conn;

}



int ff_get_1P_and_H( State *S, State *current_goals )

{

  int h, max;

  source_to_dest( &lff_current_goals, current_goals );  

  gevaluated_states++;

  max = ff_build_fixpoint( S );
  h = ff_extract_1P( max, TRUE, S);

  if ( gcmd_line.display_info == 122 ) {
    ff_print_fixpoint_result();
  }

  ff_reset_fixpoint();

  return h;

}



int ff_get_1P( State *S, State *current_goals )

{

  int h, max;

  source_to_dest( &lff_current_goals, current_goals );  

  gevaluated_states++;

  max = ff_build_fixpoint( S );
  h = ff_extract_1P( max, FALSE, S );

  if ( gcmd_line.display_info == 122 ) {
    ff_print_fixpoint_result();
  }

  ff_reset_fixpoint();

  return h;

}




void ff_collect_A_info( void )

{

  static Bool first_call = TRUE;

  int i;

  if ( first_call ) {
    gA = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    gnum_A = 0;
    first_call = FALSE;
  }

  for ( i = 0; i < gnum_A; i++ ) {
    gop_conn[gA[i]].is_in_A = FALSE;
  }
  gnum_A = 0;

  for ( i = 0; i < lff_num_E; i++ ) {
    if ( gop_conn[gef_conn[lff_E[i]].op].is_in_A ) {
      continue;
    }
    gop_conn[gef_conn[lff_E[i]].op].is_in_A = TRUE;
    gA[gnum_A++] = gef_conn[lff_E[i]].op;
  }

}















/*******************************
 * RELAXED FIXPOINT ON A STATE *
 *******************************/
















int ff_build_fixpoint( State *S )

{

  int start_ft, stop_ft, start_ef, stop_ef, i, time = 0;

  static Bool first_call = TRUE;

  if ( first_call ) {
    /* get memory for lff_ocal globals
     */
    lff_F = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    lff_E = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
    lff_ch_E = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
    lff_0P_E = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
    
    /* initialize connectivity graph members for
     * relaxed planning
     */
    lff_num_0P_E = 0;
    for ( i = 0; i < gnum_ef_conn; i++ ) {      
      gef_conn[i].level = INFINITY;    
      gef_conn[i].in_E = FALSE;
      gef_conn[i].num_active_PCs = 0;
      gef_conn[i].ch = FALSE;
      
      if ( gef_conn[i].num_PC == 0 ) {
	lff_0P_E[lff_num_0P_E++] = i;
      }
    }
    for ( i = 0; i < gnum_op_conn; i++ ) {    
	if(!gcmd_line.helpful){
	    gop_conn[i].is_in_A = FALSE;
	}
      gop_conn[i].is_in_H = FALSE;
    }
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      gft_conn[i].level = INFINITY;
      gft_conn[i].in_F = FALSE;
    }
    first_call = FALSE;
  }

  ff_initialize_fixpoint( S );
  /* Alvaro: add unknown facts */ 
  for ( i = 0; i < S->num_U; i++ ) {
    if ( gft_conn[S->U[i]].in_F ) {
      continue;
    }
    ff_new_fact( S->U[i] );
  }


  start_ft = 0;
  start_ef = 0;
  while ( TRUE ) {
    if ( ff_all_goals_activated( time ) ) {
      break;
    }

    stop_ft = lff_num_F;
    for ( i = start_ft; i < stop_ft; i++ ) {
      ff_activate_ft( lff_F[i], time );
    }

    if ( time == 0 ) {
      for ( i = 0; i < lff_num_0P_E; i++ ) {
	if ( gef_conn[lff_0P_E[i]].in_E ) {
	  continue;
	}
	ff_new_ef( lff_0P_E[i] );
      }
    }

    stop_ef = lff_num_E;
    for ( i = start_ef; i < stop_ef; i++ ) {
      ff_activate_ef( lff_E[i], time );
    }

    if ( stop_ft == lff_num_F ) {
      break;
    }

    start_ft = stop_ft;
    start_ef = stop_ef;
    time++;
  }

  return time;

}    



void ff_initialize_fixpoint( State *S )

{

  int i;

  lff_num_E = 0;
  lff_num_ch_E = 0;

  lff_num_F = 0;
  for ( i = 0; i < S->num_F; i++ ) {
    if ( gft_conn[S->F[i]].in_F ) {
      continue;
    }
    ff_new_fact( S->F[i] );
  }

}
   


void ff_activate_ft( int index, int time )

{

  int i;

  gft_conn[index].level = time;

  for ( i = 0; i < gft_conn[index].num_PC; i++ ) {
    gef_conn[gft_conn[index].PC[i]].num_active_PCs++;
    if ( !gef_conn[gft_conn[index].PC[i]].ch ) {
      gef_conn[gft_conn[index].PC[i]].ch = TRUE;
      lff_ch_E[lff_num_ch_E++] = gft_conn[index].PC[i];
    }
    if ( gef_conn[gft_conn[index].PC[i]].num_active_PCs ==
	 gef_conn[gft_conn[index].PC[i]].num_PC ) {
      ff_new_ef( gft_conn[index].PC[i] );
    }
  }

}



void ff_activate_ef( int index, int time )

{

  int i;

  gef_conn[index].level = time;

  for ( i = 0; i < gef_conn[index].num_A; i++ ) {
    if ( gft_conn[gef_conn[index].A[i]].in_F ) {
      continue;
    }
    ff_new_fact( gef_conn[index].A[i] );
  }

}



void ff_new_fact( int index )

{

  lff_F[lff_num_F++] = index;
  gft_conn[index].in_F = TRUE;

}



void ff_new_ef( int index )

{

  lff_E[lff_num_E++] = index;
  gef_conn[index].in_E = TRUE;

}



void ff_reset_fixpoint( void )

{

  int i;

  for ( i = 0; i < lff_num_F; i++ ) {
    gft_conn[lff_F[i]].level = INFINITY;
    gft_conn[lff_F[i]].in_F = FALSE;
  }

  for ( i = 0; i < lff_num_E; i++ ) {
    gef_conn[lff_E[i]].level = INFINITY;
    gef_conn[lff_E[i]].in_E = FALSE;
  }

  for ( i = 0; i < lff_num_ch_E; i++ ) {
    gef_conn[lff_ch_E[i]].num_active_PCs = 0;
    gef_conn[lff_ch_E[i]].ch = FALSE;
  }

}



Bool ff_all_goals_activated( int time ) 

{

  int i;

  for ( i = 0; i < lff_current_goals.num_F; i++ ) {
    if ( !gft_conn[lff_current_goals.F[i]].in_F ) {
      return FALSE;
    }
  }

  for ( i = 0; i < lff_current_goals.num_F; i++ ) {
    if ( gft_conn[lff_current_goals.F[i]].level == INFINITY ) {
      gft_conn[lff_current_goals.F[i]].level = time;
    }
  }

  return TRUE;

}



void ff_print_fixpoint_result( void )

{

  int time, i;
  Bool hit, hit_F, hit_E;

  time = 0;
  while ( 1 ) {
    hit = FALSE;
    hit_F = FALSE;
    hit_E = FALSE;
    for ( i = 0; i < gnum_ft_conn; i++ ) {
      if ( gft_conn[i].level == time ) {
	hit = TRUE;
	hit_F = TRUE;
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
  }
  fflush( stdout );

}
    












/**************************************
 * FIRST RELAXED PLAN (1P) EXTRACTION *
 **************************************/














/* Alvaro, this takes one state in order to 
   check helpful actions
*/
int ff_extract_1P( int max, Bool H_info, State * S)

{

  static Bool first_call = TRUE;
  int i, max_goal_level, time;

  if ( first_call ) {

    for ( i = 0; i < gnum_ft_conn; i++ ) {

      gft_conn[i].is_true = INFINITY;
      gft_conn[i].is_goal = FALSE;
      gft_conn[i].ch = FALSE;
    }
    for ( i = 0; i < gnum_op_conn; i++ ) {
      gop_conn[i].is_used = INFINITY;
    }
    for ( i = 0; i < gnum_ef_conn; i++ ) {
      gef_conn[i].in_plan = FALSE;
    }
    lff_ch_F = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    lff_num_ch_F = 0;
    lff_used_O = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    lff_num_used_O = 0;
    gin_plan_E = ( int * ) calloc( gnum_ef_conn, sizeof( int ) );
    gnum_in_plan_E = 0;
    first_call = FALSE;
  }

  ff_reset_search_info();

  if ( (max_goal_level = ff_initialize_goals( max )) == INFINITY ) {
    return INFINITY;
  }

  lff_h = 0;
  for ( time = max_goal_level; time > 0; time-- ) {
    ff_achieve_goals( time );
  }
  if ( H_info ) {
    ff_collect_H_info(S);
  }

  return lff_h;

}



int ff_initialize_goals( int max )

{

  static Bool first_call = TRUE;
  static int highest_seen;

  int i, max_goal_level, ft;

  if ( first_call ) {
    lff_goals_at = ( int ** ) calloc( RELAXED_STEPS_DEFAULT, sizeof( int * ) );
    lff_num_goals_at = ( int * ) calloc( RELAXED_STEPS_DEFAULT, sizeof( int ) );
    for ( i = 0; i < RELAXED_STEPS_DEFAULT; i++ ) {
      lff_goals_at[i] = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    }
    highest_seen = RELAXED_STEPS_DEFAULT;
    first_call = FALSE;
  }

  if ( max + 1 > highest_seen ) {
    for ( i = 0; i < highest_seen; i++ ) {
      free( lff_goals_at[i] );
    }
    free( lff_goals_at );
    free( lff_num_goals_at );
    highest_seen = max + 1;
    lff_goals_at = ( int ** ) calloc( highest_seen, sizeof( int * ) );
    lff_num_goals_at = ( int * ) calloc( highest_seen, sizeof( int ) );
    for ( i = 0; i < highest_seen; i++ ) {
      lff_goals_at[i] = ( int * ) calloc( gnum_ft_conn, sizeof( int ) );
    }
  }

  for ( i = 0; i < max + 1; i++ ) {
    lff_num_goals_at[i] = 0;
  }

  max_goal_level = 0;
  for ( i = 0; i < lff_current_goals.num_F; i++ ) {
    ft = lff_current_goals.F[i];
    if ( gft_conn[ft].level == INFINITY ) {
      return INFINITY;
    }
    if ( gft_conn[ft].level > max_goal_level ) {
      max_goal_level = gft_conn[ft].level;
    }
    lff_goals_at[gft_conn[ft].level][lff_num_goals_at[gft_conn[ft].level]++] = ft;
    gft_conn[ft].is_goal = TRUE;
    if ( !gft_conn[ft].ch ) {
      lff_ch_F[lff_num_ch_F++] = ft;
      gft_conn[ft].ch = TRUE;
    }
  }

  return max_goal_level;

}



void ff_achieve_goals( int time )

{

  int i, j, k, ft, min_p, min_e, ef, p, op;

  if ( gcmd_line.display_info == 123 ) {
    printf("\nselecting at step %3d: ", time-1);
  }

  for ( i = 0; i < lff_num_goals_at[time]; i++ ) {
    ft = lff_goals_at[time][i];
    if ( gcmd_line.display_info == 123 ) {
	printf("\nsubgoal fact: ");
	print_ft_name(ft);
    }
    
    if ( gft_conn[ft].is_true == time ) {
	if ( gcmd_line.display_info == 123 ) {
	    printf("\nis true!");
	}
      /* fact already added by prev now selected op
       */
      continue;
    }

    min_p = INFINITY;
    min_e = -1;
    for ( j = 0; j < gft_conn[ft].num_A; j++ ) {
      ef = gft_conn[ft].A[j];
      if ( gcmd_line.display_info == 123 ) {
	  printf("\nadder: ");
	  print_op_name(gef_conn[ef].op);
      }
      if ( gef_conn[ef].level != time - 1 ) {
	  if ( gcmd_line.display_info == 123 ) {
	      printf("\nadder has wrong lff_evel %d: ", gef_conn[ef].level);
	  }
	  continue;
      } 
      p = 0;
      for ( k = 0; k < gef_conn[ef].num_PC; k++ ) {
	p += gft_conn[gef_conn[ef].PC[k]].level;
      }
      if ( ff_LESS( p, min_p ) ) {
	min_p = p;
	min_e = ef;
      }
    }
    ef = min_e;

    if ( !gef_conn[ef].in_plan ) {
      gef_conn[ef].in_plan = TRUE;
      gin_plan_E[gnum_in_plan_E++] = ef;
    }
    if ( gcmd_line.display_info == 123 ) {
	printf("\nadder selected: ");
	print_op_name(gef_conn[ef].op);
    }
    op = gef_conn[ef].op;
    if ( gop_conn[op].is_used != time ) {

      if ( gop_conn[op].is_used == INFINITY ) {
	lff_used_O[lff_num_used_O++] = op;


      }
      gop_conn[op].is_used = time;
      lff_h++;
      if ( gcmd_line.display_info == 123 ) {
	print_op_name( op );
	printf("\n                       ");
      }
    } else {
	if ( gcmd_line.display_info == 123 ) {
	    printf("\nadder already used at time %d: ", time);
	}
    }

    for ( j = 0; j < gef_conn[ef].num_PC; j++ ) {
      ft = gef_conn[ef].PC[j];
      if ( gft_conn[ft].is_true == time ) {
	/* a prev at this lff_evel selected op accidently adds this precond, 
	 * so we can order that op before this one and get the precond added for free.
	 */
	continue;
      }
      if ( gft_conn[ft].is_goal ) {
	/* this fact already is a goal
	 */
	continue;
      }
      lff_goals_at[gft_conn[ft].level][lff_num_goals_at[gft_conn[ft].level]++] = ft;
      gft_conn[ft].is_goal = TRUE;
      if ( !gft_conn[ft].ch ) {
	lff_ch_F[lff_num_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
    }

    for ( j = 0; j < gef_conn[ef].num_A; j++ ) {
      ft = gef_conn[ef].A[j];
      gft_conn[ft].is_true = time;
      /* NOTE: one lff_evel below a goal will only be skipped if it's true value is time-1,
       * so subgoals introduced by prev selected ops are not excluded here.
       *
       * --- neither those of the at this lff_evel prev selected oned - which we want -
       * nor those of at prev lff_evels selected ops - which we would want to be skipped.
       *
       * --- so the ordering consraints assumed are valid but don't explore
       * the full potential.
       */
      if ( !gft_conn[ft].ch ) {
	lff_ch_F[lff_num_ch_F++] = ft;
	gft_conn[ft].ch = TRUE;
      }
    }
    for ( j = 0; j < gef_conn[ef].num_I; j++ ) {
      for ( k = 0; k < gef_conn[gef_conn[ef].I[j]].num_A; k++ ) {
	ft = gef_conn[gef_conn[ef].I[j]].A[k];
	gft_conn[ft].is_true = time;
	if ( !gft_conn[ft].ch ) {
	  lff_ch_F[lff_num_ch_F++] = ft;
	  gft_conn[ft].ch = TRUE;
	}
      }
    }
  }

}


/*
 * Alvaro: Important: this requires applying relax::get_A first in
 * order to discern which actions are really applicable. 
 * 
 */
void ff_collect_H_info( State *S )

{


  static Bool first_call = TRUE;
  static int *H, num_H, *D;
  int i, j, k, ft, ef, op, d;
  int opobs, ftpre, opobsi, ftprei;

  if ( first_call ) {
    gH = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    H = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    D = ( int * ) calloc( gnum_op_conn, sizeof( int ) );
    gnum_H = 0;
    num_H = 0;
    first_call = FALSE;
  }

  for ( i = 0; i < gnum_H; i++ ) {
    gop_conn[gH[i]].is_in_H = FALSE;
  }

  num_H = 0;
  for ( i = 0; i < lff_num_goals_at[1]; i++ ) {
    ft = lff_goals_at[1][i];

    if ( gcmd_line.R && gcmd_line.debug ) {
	printf ("\nFact: ");
	print_ft_name(ft);
    }

    for ( j = 0; j < gft_conn[ft].num_A; j++ ) {
      ef = gft_conn[ft].A[j];
      if ( gcmd_line.R && gcmd_line.debug ) {	  
	  printf ("\n is achieved by : ");
	  print_op_name(gef_conn[ef].op);
	  printf( " in level %d\n", gef_conn[ef].level);
      }
      if ( gef_conn[ef].level != 0 ) {
	continue;
      }
      op = gef_conn[ef].op;

      if ( gop_conn[op].is_in_H ) {
	/*  printf(" ... continue\n "); */
	continue;
      }

      if(gop_conn[op].is_in_A){
	  if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("Activated: ");
	      print_op_name(op);
	      printf("\n");
	  }
	  gop_conn[op].is_in_H = TRUE;
	  H[num_H++] = op;
      } else {
	  if ( gcmd_line.R && gcmd_line.debug ) {
	      printf("This op: ");
	      print_op_name(op);
	  }
	  /* 
	   * Activate observation actions related with the preconditions
	   */ 
	  for ( ftprei = 0; ftprei < gop_conn[op].num_EPC; ftprei++ ) {
	      ftpre = gop_conn[op].EPC[ftprei];
	      if ( gcmd_line.R && gcmd_line.debug ) {	  
		  printf ("\n   has as precondition ");
		  print_ft_name(ftpre); 
	      }
	      for ( opobsi = 0; opobsi < gft_conn[ftpre].num_Oind ; opobsi ++) {
		  opobs = gft_conn[ftpre].Oind[opobsi];
		  if(!gop_conn[opobs].is_in_H){
		      if ( gcmd_line.R && gcmd_line.debug ) {	  
			  printf(" causes activation of ");
			  print_op_name(opobs);
			  printf("\n");
		      }
		      gop_conn[opobs].is_in_H = TRUE;
		      H[num_H++] = opobs;
		  }
	      }
	  }
      }
    }
  }
		

  for ( i = 0; i < ggoal_state.num_F; i++ ) {
      ft = ggoal_state.F[i];
      for ( j = 0; j < S->num_U; j++ ) {
	  if ( S->U[j] == ft ) {
	      if ( gcmd_line.R && gcmd_line.debug ) {	  
		  printf("\n\n\nUnknown goal fct activates: %d operators!!!\n\n\n", gft_conn[ft].num_Oind);
		  print_ft_name(ft);
	      }
	      /* There is a goal fact which is unknown */
	      for ( opobsi = 0; opobsi < gft_conn[ft].num_Oind ; opobsi ++) {
		  opobs = gft_conn[ft].Oind[opobsi];
		  if(!gop_conn[opobs].is_in_H){
		      if ( gcmd_line.R && gcmd_line.debug ) {	  
			  printf(" causes activation of ");
			  print_op_name(opobs);
			  printf("\n");
		      }
		      gop_conn[opobs].is_in_H = TRUE;
		      H[num_H++] = opobs;
		  }
	      }
	      break;
	  }
      }
  }
      
		      

  /* H collected; now order it
   * here: count number of goal- and subgoal facts that
   *       op deletes (with lff_evel 0 effects). order lff_ess deletes
   *       before more deletes.
   *       start from back of H, to prefer down under
   *       goals to upper goals.
   */
  gnum_H = 0;
  for ( i = num_H - 1; i > -1; i-- ) {
    d = 0;
    for ( j = 0; j < gop_conn[H[i]].num_E; j++ ) {
      ef = gop_conn[H[i]].E[j];
      if ( gef_conn[ef].level != 0 ) continue;
      for ( k = 0; k < gef_conn[ef].num_D; k++ ) {
	if ( gft_conn[gef_conn[ef].D[k]].is_goal ) d++;
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
  if ( gcmd_line.display_info == 124 ) {
    printf("\ncollected H: ");
    for ( i = 0; i < gnum_H; i++ ) {
      print_op_name( gH[i] );
      printf("\n              ");
    }
  }

}



void ff_reset_search_info( void )

{

  int i;

  for ( i = 0; i < lff_num_ch_F; i++ ) {
    gft_conn[lff_ch_F[i]].is_true = INFINITY;
    gft_conn[lff_ch_F[i]].is_goal = FALSE;
    gft_conn[lff_ch_F[i]].ch = FALSE;
  }
  lff_num_ch_F = 0;

  /* printf("RESETING used: %d\n", lff_num_used_O); */
  for ( i = 0; i < lff_num_used_O; i++ ) {
    gop_conn[lff_used_O[i]].is_used = INFINITY;
  }
  lff_num_used_O = 0;

  for ( i = 0; i < gnum_in_plan_E; i++ ) {
    gef_conn[gin_plan_E[i]].in_plan = FALSE;
  }
  gnum_in_plan_E = 0;
  
}

