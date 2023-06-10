


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
 * File: search.h
 *
 * Description: headers of routines that search the state space
 *
 *              ADL version, Enforced Hill-climbing enhanced with
 *                           Goal-adders deletion heuristic
 *
 * Author: Joerg Hoffmann 2000
 *
 *********************************************************************/ 






#ifndef _SEARCH_H
#define _SEARCH_H



void do_best_first_search( void );
BfsNode *get_open_leaf_of_best_subtree( void );
void add_to_bfs_space( State *S, BfsNode *tmpbfs, BfsEdge *tmpedge, BfsNode *open_leaf, 
		       int observe_mode );
void update_fvalues_and_markers( BfsNode *open_leaf );
/* void backpropagate_fvalue( BfsNode *open_leaf, long newleaff ); */
void print_plan( void );



void source_to_dest( State *dest, State *source );
void print_state( State S );

int avg( int v1, int v2 );

/*Alvaro: Auxiliar function to know the cost of an operator (useful
  for giveup action and possibly if we want to add support for action
  costs*/
int action_cost( int operator );



#endif /* _SEARCH_H */
