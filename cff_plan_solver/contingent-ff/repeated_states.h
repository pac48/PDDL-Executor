


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
 * File: repeated_states.h
 *
 * Description: headers for conformant hashing routines
 *
 * Author: Joerg Hoffmann 2002
 *
 *********************************************************************/ 






#ifndef _REPEATED_STATES_H
#define _REPEATED_STATES_H



void initialize_repeated_states( void );



Bool stagnates( State *dest, BfsNode *dest_node, int endtime );
Bool do_nondet_stagnation_check( State *dest, BfsNode *dest_node, int endtime );
Bool single_stagnates( State *dest, State *source, int endtime, int ancestornr );
Bool rec_do_nondet_stagnation_check( int nondets_set_on_spath, 
				     State *dest, BfsNode *dest_node, BfsNode *source_node, 
				     int endtime, int anc );
Bool nondet_single_stagnates( Bool eff_appears, 
			      State *dest, BfsNode *dest_node, BfsNode *source_node, int endtime, int ancestornr );




#endif /* _REPEATED_STATES_H */

