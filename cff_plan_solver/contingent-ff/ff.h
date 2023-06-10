
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
 * File: ff.h
 * Description: Types and structures for the FastForward planner.
 *
 *        --------- ADL  VERSION  v 1.0 --------------
 *
 * Author: Joerg Hoffmann 2000
 * Contact: hoffmann@informatik.uni-freiburg.de
 *
 *********************************************************************/ 








#ifndef __FF_H
#define __FF_H




/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
#define YYINITDEPTH 20000
#endif



#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/times.h>









/*
 *  ------------------------------------ DEFINES ----------------------------
 */











/***********************
 * MEANINGLESS HELPERS *
 ***********************/




/* strcmp returns 0 if two strings are equal, which is not nice */
#define SAME 0









/****************
 * PARSING ETC. *
 ****************/









/* various defines used in parsing
 */
#define HIDDEN_STR "#"
#define AXIOM_STR "AXIOM"
#define NAME_STR "name\0"
#define VARIABLE_STR "variable\0"
#define STANDARD_TYPE "OBJECT\0"
#define EITHER_STR "EITHER"









/***************************
 * SOME ARBITRARY SETTINGS *
 ***************************/







/* maximal string length
 */
#define MAX_LENGTH 256 


/* marks border between connected items 
 */
#define CONNECTOR "~"


/* first size of goals_at array in 1P extraction
 */
#define RELAXED_STEPS_DEFAULT 25
/* definite size of U facts array (temporal implication graph)
 */
#define RELAXED_STEPS 100
/* bound on edges in a single rplan layer
 */
#define MAX_UEDGES 110



/* size of hash table for repeated states checking
 * during EHC breadth first search
 */
#define EHC_HASH_SIZE 65536
#define EHC_HASH_BITS 65535


/* size of hash table for repeated states checking
 * in plan construction
 */
#define PLAN_HASH_SIZE 1024
#define PLAN_HASH_BITS 1023


/* size of hash table for repeated states checking
 * during BFS search
 */
#define BFS_HASH_SIZE 65536
#define BFS_HASH_BITS 65535


/* cut random values of facts off modulo this value,
 * to make state sums fit into a single integer
 */
#define BIG_INT 1500000


/* when rplan is potentially incomplete, infty is replaced with this.
 */
#define BIG_H 100000







/************************
 * INSTANTIATION LIMITS *
 ************************/








#define MAX_CONSTANTS 2000
#define MAX_PREDICATES 10000
#define MAX_TYPES 150
#define MAX_ARITY 10
#define MAX_VARS 15


#define MAX_TYPE 2000


#define MAX_OPERATORS 1000


/* in DNF: AND with OR - sons - collect 'hitting set':
 * one son of each OR node. 
 *
 * this here is initial max number of such son s that can be collected
 * (grows dynamically, if required)
 */
#define MAX_HITTING_SET_DEFAULT 1000


#define MAX_TYPE_INTERSECTIONS 10


#define MAX_RELEVANT_FACTS 20000






/******************************************
 * DOMAIN STRUCTURE AND SEARCHING LIMITS *
 ******************************************/







#define MAX_PLAN_LENGTH 1500







/****************
 * CODE DEFINES *
 ****************/










/* not a real 'code' define; used in relax and search to encode
 * infinite level number / plan length
 */
#ifndef INFINITY
#define INFINITY -1
#endif

#ifndef GIVEUP_ID 
#define GIVEUP_ID -117
#endif






/* define boolean types if not allready defined
 */
#ifndef Bool
typedef unsigned char Bool;
#ifndef TRUE /* we assume that FALSE is also not defined */
#define TRUE 1
#define FALSE 0
#endif /* TRUE */
#endif /* Bool */


/* code a param number into a negative number and vice versa
 */
#define ENCODE_VAR( val ) (val * (-1)) - 1
#define DECODE_VAR( val ) (val + 1) * (-1)

#define GET_CONSTANT( val, pointer ) ( val >= 0 ) ? val : pointer->inst_table[DECODE_VAR( val )]


/* Check allocated memory
 */
#define CHECK_PTR(p) if (NULL == (p)) { \
  fprintf(stdout, "\n\aNO MEMORY in file %s:%d\n\n", __FILE__, __LINE__); \
  exit(1);}


/* add elapsed time from main local time vars to specified val
 */
#define TIME( val ) val += ( float ) ( ( end.tms_utime - start.tms_utime + \
					 end.tms_stime - start.tms_stime  ) / 100.0 )












/*
 *  ------------------------------ DATA STRUCTURES ----------------------------
 */











/*******************
 * GENERAL HELPERS *
 *******************/








/* all command switches
 */
struct _command_line {

  char path[MAX_LENGTH];
  char ops_file_name[MAX_LENGTH];
  char fct_file_name[MAX_LENGTH];
  int display_info;

  int heuristic;
  int observe_h;

  int stagnating;
  Bool dominating;

  Bool manual;
  int debug;
  Bool R;
  Bool A;
  Bool T;
  Bool P;

  int search_mode; /*In the observations, 0: sum, 1: max, 2: avg*/
  int search_alg; /* 0: AO*, 1: greedy AO* */

  int hweight;
  Bool helpful;

  Bool giveup_action;
  int giveup_cost;
};


typedef char *Token;












/***********
 * PARSING *
 ***********/










/* A list of strings
 */
typedef struct _TokenList {

  char *item;
  struct _TokenList *next;

} TokenList;



/* list of string lists
 */
typedef struct _FactList {

  TokenList *item;
  struct _FactList *next;

} FactList;



/* structure to store  typed-list-of <name>/<variable>,
 * as they are declared in PDDL files
 */
typedef struct _TypedList {

  char *name;

  /* each item in this list is the name of a type which
   * our type is the union of (EITHER - types ...)
   *
   * usually, this will default to a single-item TokenList.
   */
  TokenList *type;
  /* after first sweep, this will contain the number in type table
   */
  int n;

  struct _TypedList *next;

} TypedList;



/* only needed to parse in the predicates and their arg
 * definitions
 */
typedef struct _TypedListList {

  char *predicate;

  TypedList *args;

  struct _TypedListList *next;

} TypedListList;



/* This type indicates whether a node in the pddl tree stands for
 * an atomic expression, a junctor or a quantor. 
 */
typedef enum _Connective{TRU,
			   FAL,
			   ATOM, 
			   NOT, 
			   AND, 
			   OR, 
			   ALL, 
			   EX, 
			   WHEN,
                           UNKNOWN,
			   ONEOF} Connective;



/*
 * This is a node in the tree to parse PDDL files
 */
typedef struct _PlNode {

  /* type of the node
   */
  Connective connective;

  /* only for parsing: the var args in quantifiers
   */
  TypedList *parse_vars;

  /* AND, OR, NOT, WHEN => NULL
   * ALL, EX            => the quantified variable with its type
   * ATOM               => the atom as predicate->param1->param2->...
   */
  TokenList *atom;

  /* marks nodes that are non-deterministic (atomic) effects.
   */
  Bool is_nondeteff;

  /* (a) for AND, OR this is the list of sons(a AND b AND c...),
   * (b) for the rest this is the son, e.g. a subtree that is negated
   * (c) for WHEN, the first son is the condition and the next son
   * is the effect
   */
  struct _PlNode *sons;

  /* if you have a list of sons, they are connected by next
   */
  struct _PlNode *next;

} PlNode, *PlNode_pointer;


/*
 * This resembles an uninstantiated PDDL operator
 */
typedef struct _PlOperator {

  char *name;

  /* only important for PDDL where :VARS may be added to the param list
   * which must be hidden when writing the plan to an output file
   */
  int number_of_real_params; 

  /* the params, as they are declared in domain file
   */
  TypedList *parse_params;

  /* params is a list of variable/type pairs, such that:
   * factlist->item = [variable] -> [type]
   */
  FactList *params;
  PlNode *preconds;
  PlNode *observe;
  PlNode *effects;

  struct _PlOperator *next;

} PlOperator;















/***************** 
 * INSTANTIATION *
 *****************/









/* helpers
 */

typedef int TypeArray[MAX_TYPE_INTERSECTIONS];

typedef int *int_pointer;




/* first step structures: parsing & preprocessing
 */

typedef struct _Fact {

  int predicate, args[MAX_ARITY];

} Fact;



typedef struct _Facts {

  Fact *fact;

  struct _Facts *next;

} Facts;



typedef struct _WffNode {

  Connective connective;

  /* in ALL/EX s
   */
  int var, var_type;
  char *var_name;

  /* in AND/OR s
   */
  struct _WffNode *sons;
  /* sons are doubly connected linear list
   */
  struct _WffNode *next;
  struct _WffNode *prev;

  /* in ATOMs
   */
  Fact *fact;
  /* after translation: mark NOT-p s for efficiency
   */
  int NOT_p;

  /* in ALL/EX/NOT
   */
  struct _WffNode *son;

  /* for expansion speedup
   */
  Bool visited;

  /* no WHEN s here... use Pl Connectives anyway for simplicity
   */

} WffNode, *WffNode_pointer;



typedef struct _Literal {

  Bool negated;
  Bool is_nondeteff;

  Fact fact;

  struct _Literal *next;
  struct _Literal *prev;

} Literal;



typedef struct _Effect {

  int num_vars, var_types[MAX_VARS];
  char *var_names[MAX_VARS];

  WffNode *conditions;

  Literal *effects;

  struct _Effect *next;
  struct _Effect *prev;

} Effect;



typedef struct _Operator {

  char *name, *var_names[MAX_VARS];
  int number_of_real_params; 

  int num_vars, var_types[MAX_VARS];
  Bool removed[MAX_VARS];
 
  WffNode *preconds;

  Fact *observe;
  int num_observe;

  Effect *effects;

  Bool hard;

} Operator, *Operator_pointer;




/* second step: structures that keep already normalized
 * operators
 */




typedef struct _NormEffect {

  int num_vars, var_types[MAX_VARS];
  int inst_table[MAX_VARS];

  Fact *conditions;
  int num_conditions;

  Fact *adds;
  Bool *adds_nondet;
  int num_adds;
  Fact *dels;
  Bool *dels_nondet;
  int num_dels;

  struct _NormEffect *prev;
  struct _NormEffect *next;

} NormEffect;



typedef struct _NormOperator {
  
  Operator *op;

  int num_vars, var_types[MAX_VARS];
  int inst_table[MAX_VARS];
  int removed_vars[MAX_VARS], num_removed_vars, type_removed_vars[MAX_VARS];

  Fact *preconds;
  int num_preconds;

  Fact *observe;
  int num_observe;

  NormEffect *effects;

  Bool out;

} NormOperator, *NormOperator_pointer;
  


/* minimal info for a fully instantiated easy operator;
 * yields one action when expanded
 */
typedef struct _EasyTemplate {

  NormOperator *op;
  int inst_table[MAX_VARS];

  struct _EasyTemplate *prev;
  struct _EasyTemplate *next;

} EasyTemplate;






/* structures for hard ops
 */





/* intermediate step: structure for keeping hard ops
 * with normalized precondition, but arbitrary
 * effect conditions
 */
typedef struct _MixedOperator {
  
  Operator *op;

  int inst_table[MAX_VARS];

  Fact *preconds;
  int num_preconds;

  Fact *observe;
  int num_observe;

  Effect *effects;

  struct _MixedOperator *next;

} MixedOperator;





/* last hard step: everything is action - like, except that
 * facts are not yet integer coded
 */  

typedef struct _PseudoActionEffect {

  Fact *conditions;
  int num_conditions;

  Fact *adds;
  Bool *adds_nondet;
  int num_adds;
  Fact *dels;
  Bool *dels_nondet;
  int num_dels;

  struct _PseudoActionEffect *next;

} PseudoActionEffect;



typedef struct _PseudoAction {

  Operator *op;

  int inst_table[MAX_VARS];

  Fact *preconds;
  int num_preconds;

  Fact *observe;
  int num_observe;

  PseudoActionEffect *effects;
  int num_effects;

} PseudoAction, *PseudoAction_pointer;




/* final domain representation structure
 */




typedef struct _ActionEffect {

  int *conditions;
  int num_conditions;

  int *adds;
  Bool *adds_nondet;
  int num_adds;
  int *dels;
  Bool *dels_nondet;
  int num_dels;

} ActionEffect;



typedef struct _Action {

  NormOperator *norm_operator;
  PseudoAction *pseudo_action;

  char *name;
  int num_name_vars;
  int name_inst_table[MAX_VARS];

  int inst_table[MAX_VARS];

  int *preconds;
  int num_preconds;

  int *observe;
  int num_observe;

  ActionEffect *effects;
  int num_effects;

  struct _Action *next;

} Action;











/*****************************************************
 * BASIC OP AND FT STRUCTURES FOR CONNECTIVITY GRAPH *
 *****************************************************/











typedef struct _OpConn {

  /* to get name
   */
  Action *action;

  /* preconds
   */
  int *P;
  int num_P;

  /* observations; only those mentioned in definition, NOT negation translations!!
   *               in order to know true/false branches, ie branch names
   */
  int *O;
  int num_O;

  /* effects
   */
  int *E;
  int num_E;


  /* currently we allow actually only a single nondet eff literal per action.
   * it is stored in here, encoding: 0 -- none, add: (ftid +1), del: (ftid +1) * (-1)
   */
  int nondeteff;
  /* these are the corresponding effconds
   */
  int *nondeteff_C;
  int num_nondeteff_C;



  /* for heuristic computation reg. observation-rplan-dependencies
   */
  /* the union of all E adds
   */
  int *EA, num_EA;
  /* the union of all E cons, + pre
   */
  int *EPC, num_EPC;

  /* does it observe and branch on the value of a fact?
   */
  Bool observation;

  /* member for applicable actions extraction
   */
  int level;
  Bool is_in_A;
  int num_active_Ps;
  Bool ch;

  /* members for 1Ph - H(S) extraction
   *
   * simple solution: static array size
   */
  Bool is_used_at[RELAXED_STEPS];
  /* we inserted this to achieve/observe what fact?
   */
  int is_used_for_at[RELAXED_STEPS];
  int is_used_fortime_at[RELAXED_STEPS];
  int is_used;
  Bool is_in_H;

} OpConn;



typedef struct _EfConn {

  int op;

  /* op preconds + conds
   *
   * redundant with info below; keep that in from original FF
   * for the time being as don't know which info is more useful
   * later.
   */
  int *PC;
  int num_PC;

  /* conditions separately as we will be reasoning about these
   */
  int *C;
  int num_C;

  int *A;
  Bool *A_nondet;
  int num_A;

  int *D;
  Bool *D_nondet;
  int num_D;

  /* implied effects
   */
  int *I;
  int num_I;

  Bool removed;

  /* members for relaxed fixpoint computation
   */
  int level;
  Bool in_E;
  int num_active_PCs;
  Bool ch;

  Bool in_plan; /*Alvaro: For relax_ff*/

} EfConn;



typedef struct _FtConn {

  /* its negation; the resp ft number if it is translated,
   * otherwise -1
   */
  int negation;

  /* ops it is precond of -> get_A
   */
  int *P;
  int num_P;

  /* ops it is observed by; ALSO set for negations of observed facts!!!
   */
  int *O;
  int num_O;

  /* effects it is union conds, pres element of
   */
  int *PC;
  int num_PC;

  /* efs it is cond of -> figure will need this for relaxed plans
   */
  int *C;
  int num_C;

  /* efs that add or del it
   */
  int *A;
  Bool *A_nondet;/* is this effect nondet? */
  int num_A;

  int *D;
  Bool *D_nondet;/* is this effect nondet? */
  int num_D;
    
    /* 
     * Indirect Observations: observe a fact that is related to this
     * fact with some clause.
     */
  int * Oind;
  int num_Oind;

  /* members for orderings preprocessing
   */
  int *False;
  int num_False;

  /* members for relaxed fixpoint computation
   */
  int level;
  Bool in_F;
  /* when is it observed for the first time?
   */
  int observed_level;

  /* members for 1Ph extraction
   *
   * NOTE: by standard as inhgerited from classical FF, 
   *       a fact only ever becomes a subgoal at its 1st
   *       level in the RPG. for this we don't need an array
   *       here but only a  single Bool. keep full array structure
   *       for more flexibility, eg. making facts goals
   *       at those points in the RPG where needed
   *       (this can make a difference when, eg. some op1 just below
   *       the op2 in need of precond p, but above the 1st level of p,
   *       adds p)
   */
  Bool is_goal_at[RELAXED_STEPS];
  Bool is_ogoal_at[RELAXED_STEPS];
  /* for what operator?
   */
  int is_goal_for_at[RELAXED_STEPS];
  int is_ogoal_for_at[RELAXED_STEPS];
  int is_goal_fortime_at[RELAXED_STEPS];
  int is_ogoal_fortime_at[RELAXED_STEPS];
  int is_goal; /*Alvaro: Added for relax_ff*/
  int is_true;
  int is_observed;
  Bool ch;
  Bool ch1;

  /* search
   */
  int rand;/* for hashing */
  Bool is_global_goal;/* fast goal add finding */

  /* in goal or in eff PC
   */
  Bool srelevant;

  /* ini U or eff (add o. del) of a c eff
   */
  Bool poss_U;

  Bool CNF;

} FtConn;













/****************************
 * in RELAXATION            *
 ****************************/











typedef struct _UftNode UftNode, *UftNode_pointer;

struct _UftNode {

  /* well, who's in here?
   */
  int ft;
  
  /* this will be false except at the layer where the fact became
   * true --> flag for recognizing such facts fast later
   * in rplan extraction.
   */
  Bool became_F;

  /* this says if there is an action below observing this fact
   */
  Bool observed;

  /* two corresponding arrays, one for
   * the respective Uft nodes one layer earlier,
   * one for the effects inducing the edges.
   */
  UftNode_pointer in_edges[MAX_UEDGES];
  int in_efs[MAX_UEDGES];
  int num_in;

};



























/****************************
 * STRUCTURES FOR SEARCHING *
 ****************************/









typedef struct _State {
  
  int *F;
  int num_F;

  int max_F;

  /* the unknown facts!
   *
   * these are, in difference to the F facts above, allocated
   * statically, ie always with the max possible number
   * -- assuming that the nr of unknown facts will not be critically 
   *    high in itself.
   */
  int *U;
  int num_U;

} State, *State_pointer;



/* search nodes and edges defined interleavingly that's why we
 * need to declare them first.
 */
typedef struct _BfsEdge BfsEdge, *BfsEdge_pointer;
typedef struct _BfsNode BfsNode, *BfsNode_pointer;



struct _BfsEdge {

  /* op associated with this edge
   */
  int op;

  /* the cond effects (ie their indices in gef_conn) with unknown antecedents 
   * direct store convenient for CNF generations
   */
  int *unknown_E;
  int num_unknown_E;

  /* where do we come from
   */
  BfsNode *in_node;

  /* it's not an observation -> just one result state
   */
  BfsNode *out_node;

  /* it's an observation -> two result states
   */
  BfsNode *true_out_node;
  BfsNode *false_out_node;

  /* for making linked lists
   */
  struct _BfsEdge *next;

};



struct _BfsNode {

  /* the known and unknown facts in the considered state
   */
  State S;

  /* overall f value -- computed from children values,
   * or as h fn if no children in current search tree/graph
   *
   * keep "greedy" which is just sum of leave hs, and astar, which
   * is the latter plus nr actions needed to get to these leaves.
   *
   * astar is used as secondary heuristic in greedy search, to keep
   * the search from being to depth-oriented.
   */
  float f; /* this is the one used in search (cmd line option) */
  float astar_f;

  /* distance from ini state; currently, just to keep
   * track of "plan length" ie max length of any branch
   */
  int depth;

  /* helpful actions, if that option is set.
   */
  int *H;
  int num_H;

  /* the edges to other BfsNodes: linked lists
   *
   * ingoing edge there will be exactly one unless using
   * repeated states, ie. "graph" techniques
   */
  BfsEdge *ingoing_edge;
  BfsEdge *outgoing_edges;

  /* the outgoing edge that's currently considered best
   */
  BfsEdge *best_outgoing_edge;

  /* we have a tree from here to goal-leaves
   */
  Bool solved;

  /* all paths from here go into dead ends
   */
  Bool failed;

};



/* currently disabled
 */
typedef struct _BfsHashEntry {

  int Fsum, Usum;

  BfsNode *bfs_node;

  struct _BfsHashEntry *next;

} BfsHashEntry, *BfsHashEntry_pointer;




/* for state transition
 */
typedef struct _TimedLiteral {

    int literal;
    int time;

} TimedLiteral;



/* for dynamic literal-clause membership lists
 */
typedef struct _MemberList {

  int clause;

  struct _MemberList *next;

} MemberList, *MemberList_pointer;








/*
 *  -------------------------------- MAIN FN HEADERS ----------------------------
 */







void output_planner_info( void );
void ff_usage( void );
Bool process_command_line( int argc, char *argv[] );

#ifdef __cplusplus
extern "C" {
#endif

void run(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif




/*
 *  ----------------------------- GLOBAL VARIABLES ----------------------------
 */












/*******************
 * GENERAL HELPERS *
 *******************/










extern struct tms start, end;

/* used to time the different stages of the planner
 */
extern float gtempl_time, greach_time, grelev_time, gconn_time, gmem_time;
extern float gsearch_time, geval_time, gcnf_time, genc_time, gsat_time;
extern float grs_time, grs_sat_time, gsc_time, gss_time;
extern float gr_sat_time, grp_sat_time, gr_cnf_time, gr_enc_time, gmembership_time;
extern int gsat_calls, gcnfs, grs_sat_calls, gss_sat_calls;
extern int gr_sat_calls, grp_sat_calls, gsc_sat_calls, grs_comps;
extern int grs_hits, gss_hits, grs_conf_comps, gdp_calls, gup_calls;

/* the command line inputs
 */
extern struct _command_line gcmd_line;

/* number of states that got heuristically evaluated
 */
extern int gevaluated_states;

/* maximal depth of breadth first search
 */
extern int gmax_search_depth;


/* CNF statistic
 */
extern float *gsum_k_clauses, gsum_clauses;






/***********
 * PARSING *
 ***********/











/* used for pddl parsing, flex only allows global variables
 */
extern int gbracket_count;
extern char *gproblem_name;

/* The current input line number
 */
extern int lineno;

/* The current input filename
 */
extern char *gact_filename;

/* The pddl domain name
 */
extern char *gdomain_name;

/* loaded, uninstantiated operators
 */
extern PlOperator *gloaded_ops;

/* stores initials as fact_list 
 */
extern PlNode *gorig_initial_facts;

/* stores initial ors as an array of OR lists
 */
extern PlNode_pointer *gorig_initial_ors;
extern int gnum_orig_initial_ors;

/* stores initial oneofs as an array of ONEOF lists
 */
extern PlNode_pointer *gorig_initial_oneofs;
extern int gnum_orig_initial_oneofs;

/* not yet preprocessed goal facts
 */
extern PlNode *gorig_goal_facts;

/* the types, as defined in the domain file
 */
extern TypedList *gparse_types;

/* the constants, as defined in domain file
 */
extern TypedList *gparse_constants;

/* the predicates and their arg types, as defined in the domain file
 */
extern TypedListList *gparse_predicates;

/* the objects, declared in the problem file
 */
extern TypedList *gparse_objects;



/* connection to instantiation ( except ops, goal, initial )
 */

/* all typed objects 
 */
extern FactList *gorig_constant_list;

/* the predicates and their types
 */
extern FactList *gpredicates_and_types;














/*****************
 * INSTANTIATING *
 *****************/










/* global arrays of constant names,
 *               type names (with their constants),
 *               predicate names,
 *               predicate aritys,
 *               defined types of predicate args
 */
extern Token gconstants[MAX_CONSTANTS];
extern int gnum_constants;
extern Token gtype_names[MAX_TYPES];
extern int gtype_consts[MAX_TYPES][MAX_TYPE];
extern Bool gis_member[MAX_CONSTANTS][MAX_TYPES];
extern int gtype_size[MAX_TYPES];
extern int gnum_types;
extern Token gpredicates[MAX_PREDICATES];
extern int garity[MAX_PREDICATES];
extern int gpredicates_args_type[MAX_PREDICATES][MAX_ARITY];
extern int gnum_predicates;




/* the domain in first step integer representation
 */
extern Operator_pointer goperators[MAX_OPERATORS];
extern int gnum_operators;
extern Fact *gfull_initial;
extern int gnum_full_initial;
extern Fact *gfull_unknown_initial;
extern int gnum_full_unknown_initial;
extern WffNode_pointer *gfull_or_initial;
extern int gnum_full_or_initial;
extern WffNode_pointer *gfull_oneof_initial;
extern int gnum_full_oneof_initial;
extern WffNode *ggoal;



/* stores inertia - information: is any occurence of the predicate
 * added / deleted in the uninstantiated ops?
 * is any occurence of the predicate unknown?
 */
extern Bool gis_added[MAX_PREDICATES];
extern Bool gis_deleted[MAX_PREDICATES];
extern Bool gis_unknown[MAX_PREDICATES];



/* splitted initial state:
 * initial non static facts,
 * initial static facts, divided into predicates
 * (will be two dimensional array, allocated directly before need)
 *
 * the same mirrored for unknown facts -- "known negatives" is transferred
 * here to "known positives and unknowns"; seems more adequate for later 
 * purposes, giving access to unknowns directly. note that unknowns are
 * not assumed static.
 */
extern Facts *ginitial;
extern int gnum_initial;
extern Facts *gunknown_initial;
extern int gnum_unknown_initial;
extern Fact **ginitial_predicate;
extern int *gnum_initial_predicate;
extern Fact **gunknown_initial_predicate;
extern int *gnum_unknown_initial_predicate;

/* this here stores dependencies between initial variables:
 * when translating negations of an unkwown literal we need
 * to remember that the translation, while unkown, will
 * always have the respective inverse value.
 * we remember the fts for which this holds.
 */
extern Facts *ginitial_ft_equivalence_A;
extern Facts *ginitial_ft_equivalence_notA;
extern int gnum_initial_ft_equivalence;



/* the type numbers corresponding to any unary inertia
 */
extern int gtype_to_predicate[MAX_PREDICATES];
extern int gpredicate_to_type[MAX_TYPES];

/* (ordered) numbers of types that new type is intersection of
 */
extern TypeArray gintersected_types[MAX_TYPES];
extern int gnum_intersected_types[MAX_TYPES];


/* stores which predicate is a translation of which other one.
 */
extern int gtranslated_predicate_to[MAX_PREDICATES];


/* splitted domain: hard n easy ops
 */
extern Operator_pointer *ghard_operators;
extern int gnum_hard_operators;
extern NormOperator_pointer *geasy_operators;
extern int gnum_easy_operators;



/* so called Templates for easy ops: possible inertia constrained
 * instantiation constants
 */
extern EasyTemplate *geasy_templates;
extern int gnum_easy_templates;



/* first step for hard ops: create mixed operators, with conjunctive
 * precondition and arbitrary effects
 */
extern MixedOperator *ghard_mixed_operators;
extern int gnum_hard_mixed_operators;



/* hard ''templates'' : pseudo actions
 */
extern PseudoAction_pointer *ghard_templates;
extern int gnum_hard_templates;



/* store the final "relevant facts"
 */
extern Fact grelevant_facts[MAX_RELEVANT_FACTS];
extern int gnum_relevant_facts;
extern int gnum_pp_facts;



/* the final actions and problem representation
 */
extern Action *gactions;
extern int gnum_actions;
extern State ginitial_state;
extern State ggoal_state;
/* to access initially valid implications: the translated facts.
 */
extern int *ginitial_equivalence_A;
extern int *ginitial_equivalence_notA;
extern int gnum_initial_equivalence;
/* to know how much space we need for unknown conds in states
 */
extern int gmax_E;
/* the initial OR constraints in final coding
 */
extern int **ginitial_or;
extern int *ginitial_or_length;
extern int gnum_initial_or;









/**********************
 * CONNECTIVITY GRAPH *
 **********************/





/* one ops (actions) array ...
 */
extern OpConn *gop_conn;
extern int gnum_op_conn;



/* one effects array ...
 */
extern EfConn *gef_conn;
extern int gnum_ef_conn;



/* one facts array.
 */
extern FtConn *gft_conn;
extern int gnum_ft_conn;



/* max #conds. for max clauses computation.
 */
extern int gmax_C;



/* max U: all initial Us plus facts that are 
 * added / deleted by a conditional effect with poss_U conds.
 * (important for various memory allocations)
 */
extern int gmax_U;
extern int gmax_CNFU;

/* we get these max #s of clauses and lits.
 */
extern int gmax_clauses;
extern int gmax_rs_clauses;
extern int gmax_literals;








/*******************
 * SEARCHING NEEDS *
 *******************/








/* applicable actions
 */
extern int *gA;
extern int gnum_A;



/* communication from extract 1.P. to search engine:
 * 1P action choice
 */
extern int *gH;
extern int gnum_H;




/* the clauses to be communicated to the SAT solver for
 * determining inferred literals.
 */
extern TimedLiteral **gclauses;
extern int *gclause_length;
extern int gnum_fixed_clauses;
extern int gnum_clauses;

/* array; maps ft / time pair to its number in CNF encoding.
 */
extern int **gcodes;
extern int gnum_fixed_c;


/* inverse mapping, to undo changes in table.
 */
extern int *gcf, *gct, gnum_c;

 

/* statistics: count nr. of times the "disjunction minimisation" actually
 * minimised something
 */
extern int gremoved_lits;



/* stores the current DP decisions including unit propagations.
 *
 * is for DP given in state_transitions.c!!!!!
 *
 * have to make this global as it's also accessed from repeated states --
 * when checking stagnation. I know it's ugly...
 */
extern int *gdecision_stack;
extern int gnum_decision_stack;



/* for each possible ft code, a pointer to connected dynamic list
 * of member elements, ie the clauses in which it participates,
 * positive and negative.
 *
 * used in state_transitions DP solver. global as accessed from search.c
 * in reset between ehc and bfs switch.
 */
extern MemberList_pointer *gpos_c_in_clause_start;
extern MemberList_pointer *gpos_c_in_clause_fixed;/* before here, list corresp. to fixed CNF */
extern MemberList_pointer *gpos_c_in_clause_end;/* before here, members of current list */
extern MemberList_pointer *gneg_c_in_clause_start;
extern MemberList_pointer *gneg_c_in_clause_fixed;
extern MemberList_pointer *gneg_c_in_clause_end;



/* automatically checked Bool saying if sufficient criteria for
 * "more facts are always better" holds.
 */
extern Bool gdomination_valid;







/* search space in bfs
 */
extern BfsNode *gbfs_initial_state;



#endif
