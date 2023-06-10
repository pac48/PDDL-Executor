


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
 * File: state_transitions.h
 *
 * Description: headers for transition routines
 *
 * Author: Joerg Hoffmann 2000
 *
 *********************************************************************/ 






#ifndef _STATE_TRANSITIONS_H
#define _STATE_TRANSITIONS_H



void initialize_state_transitions( void );



Bool result_to_dest( State *dest, BfsNode *dest_node, int observe_mode );



Bool can_contradict_in( int ft, BfsNode *dest_node, int endtime );



void handle_inferred_literals( State *dest, 
			       int *check_pos, int num_cp,
			       int *check_neg, int num_cn,
			       int *check_both, int num_cb,
			       Bool *Npp, int endtime );
void create_fixed_initial_clauses( void );
int extend_dynamic_clauses_base( State *dest, 
				 BfsNode *dest_node, 
				 Bool *Npp,
				 Bool firstpart );
Bool unconditional_nondet_del_on( int ft, Bool *Ft, int op );
Bool unconditional_nondet_add_on( int ft, Bool *Ft, int op );
void print_clauses( void );
void print_clause( int i );
void insert_posnoop_hitting_set_clauses( int ft, int time, 
					 BfsEdge *dest_edge,
					 Bool *Ft, Bool *Ut );
void next_posnoop_hitting_set_step( int Uindex,
				    int ft, int time,
				    BfsEdge *dest_edge,
				    Bool *Ft, Bool *Ut );
void insert_negnoop_hitting_set_clauses( int ft, int time, 
					 BfsEdge *dest_edge,
					 Bool *Ft, Bool *Ut );
void next_negnoop_hitting_set_step( int Uindex,
				    int ft, int time,
				    BfsEdge *dest_edge,
				    Bool *Ft, Bool *Ut );
void create_fixed_initial_clauses_encoding( void );
void extend_dynamic_clauses_base_encoding( int startclauses, int startcodes );
void insert_fixed_membership( int v, Bool neg, int c );
void insert_dynamic_membership( int v, Bool neg, int c );
void print_encoded_clauses( void );



Bool dp_CNF( void );
Bool dp( void );
Bool do_unit_props( int startidx );



#endif /* _STATE_TRANSITIONS_H */
