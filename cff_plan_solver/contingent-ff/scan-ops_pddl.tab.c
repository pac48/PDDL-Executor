/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         ops_pddlparse
#define yylex           ops_pddllex
#define yyerror         ops_pddlerror
#define yydebug         ops_pddldebug
#define yynerrs         ops_pddlnerrs
#define yylval          ops_pddllval
#define yychar          ops_pddlchar

/* First part of user prologue.  */
#line 1 "scan-ops_pddl.y"

#ifdef YYDEBUG
  extern int yydebug=1;
#endif


#include <stdio.h>
#include <string.h> 
#include "ff.h"
#include "memory.h"
#include "parse.h"


#ifndef SCAN_ERR
#define SCAN_ERR
#define DOMDEF_EXPECTED            0
#define DOMAIN_EXPECTED            1
#define DOMNAME_EXPECTED           2
#define LBRACKET_EXPECTED          3
#define RBRACKET_EXPECTED          4
#define DOMDEFS_EXPECTED           5
#define REQUIREM_EXPECTED          6
#define TYPEDLIST_EXPECTED         7
#define LITERAL_EXPECTED           8
#define PRECONDDEF_UNCORRECT       9
#define TYPEDEF_EXPECTED          10
#define CONSTLIST_EXPECTED        11
#define PREDDEF_EXPECTED          12 
#define NAME_EXPECTED             13
#define VARIABLE_EXPECTED         14
#define ACTIONFUNCTOR_EXPECTED    15
#define ATOM_FORMULA_EXPECTED     16
#define EFFECT_DEF_EXPECTED       17
#define NEG_FORMULA_EXPECTED      18
#define NOT_SUPPORTED             19
#define ACTION                    20
#endif


#define NAME_STR "name\0"
#define VARIABLE_STR "variable\0"
#define STANDARD_TYPE "OBJECT\0"
 

static char *serrmsg[] = {
  "domain definition expected",
  "'domain' expected",
  "domain name expected",
  "'(' expected",
  "')' expected",
  "additional domain definitions expected",
  "requirements (e.g. ':STRIPS') expected",
  "typed list of <%s> expected",
  "literal expected",
  "uncorrect precondition definition",
  "type definition expected",
  "list of constants expected",
  "predicate definition expected",
  "<name> expected",
  "<variable> expected",
  "action functor expected",
  "atomic formula expected",
  "effect definition expected",
  "negated atomic formula expected",
  "requirement %s not supported by this IPP version",  
  "action definition is not correct",
  NULL
};


/* void opserr( int errno, char *par ); */


static int sact_err;
static char *sact_err_par = NULL;
static PlOperator *scur_op = NULL;
static Bool sis_negated = FALSE;


int supported( char *str )

{

  int i;
  char * sup[] = { ":STRIPS", ":NEGATION", ":EQUALITY",":TYPING", 
		   ":CONDITIONAL-EFFECTS", ":DISJUNCTIVE-PRECONDITIONS", 
		   ":EXISTENTIAL-PRECONDITIONS", ":UNIVERSAL-PRECONDITIONS", 
		   ":QUANTIFIED-PRECONDITIONS", ":ADL",
		   NULL };     

  for (i=0; NULL != sup[i]; i++) {
    if ( SAME == strcmp(sup[i], str) ) {
      return TRUE;
    }
  }
  
  return FALSE;

}


#line 180 "scan-ops_pddl.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int ops_pddldebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    DEFINE_TOK = 258,              /* DEFINE_TOK  */
    DOMAIN_TOK = 259,              /* DOMAIN_TOK  */
    REQUIREMENTS_TOK = 260,        /* REQUIREMENTS_TOK  */
    TYPES_TOK = 261,               /* TYPES_TOK  */
    EITHER_TOK = 262,              /* EITHER_TOK  */
    CONSTANTS_TOK = 263,           /* CONSTANTS_TOK  */
    PREDICATES_TOK = 264,          /* PREDICATES_TOK  */
    ACTION_TOK = 265,              /* ACTION_TOK  */
    VARS_TOK = 266,                /* VARS_TOK  */
    IMPLIES_TOK = 267,             /* IMPLIES_TOK  */
    PRECONDITION_TOK = 268,        /* PRECONDITION_TOK  */
    PARAMETERS_TOK = 269,          /* PARAMETERS_TOK  */
    OBSERVE_TOK = 270,             /* OBSERVE_TOK  */
    EFFECT_TOK = 271,              /* EFFECT_TOK  */
    EQ_TOK = 272,                  /* EQ_TOK  */
    AND_TOK = 273,                 /* AND_TOK  */
    NOT_TOK = 274,                 /* NOT_TOK  */
    WHEN_TOK = 275,                /* WHEN_TOK  */
    FORALL_TOK = 276,              /* FORALL_TOK  */
    IMPLY_TOK = 277,               /* IMPLY_TOK  */
    OR_TOK = 278,                  /* OR_TOK  */
    EXISTS_TOK = 279,              /* EXISTS_TOK  */
    NONDET_TOK = 280,              /* NONDET_TOK  */
    NAME = 281,                    /* NAME  */
    VARIABLE = 282,                /* VARIABLE  */
    TYPE = 283,                    /* TYPE  */
    OPEN_PAREN = 284,              /* OPEN_PAREN  */
    CLOSE_PAREN = 285              /* CLOSE_PAREN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 107 "scan-ops_pddl.y"


  char string[MAX_LENGTH];
  char *pstring;
  PlNode *pPlNode;
  FactList *pFactList;
  TokenList *pTokenList;
  TypedList *pTypedList;


#line 268 "scan-ops_pddl.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE ops_pddllval;


int ops_pddlparse (void);



/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_DEFINE_TOK = 3,                 /* DEFINE_TOK  */
  YYSYMBOL_DOMAIN_TOK = 4,                 /* DOMAIN_TOK  */
  YYSYMBOL_REQUIREMENTS_TOK = 5,           /* REQUIREMENTS_TOK  */
  YYSYMBOL_TYPES_TOK = 6,                  /* TYPES_TOK  */
  YYSYMBOL_EITHER_TOK = 7,                 /* EITHER_TOK  */
  YYSYMBOL_CONSTANTS_TOK = 8,              /* CONSTANTS_TOK  */
  YYSYMBOL_PREDICATES_TOK = 9,             /* PREDICATES_TOK  */
  YYSYMBOL_ACTION_TOK = 10,                /* ACTION_TOK  */
  YYSYMBOL_VARS_TOK = 11,                  /* VARS_TOK  */
  YYSYMBOL_IMPLIES_TOK = 12,               /* IMPLIES_TOK  */
  YYSYMBOL_PRECONDITION_TOK = 13,          /* PRECONDITION_TOK  */
  YYSYMBOL_PARAMETERS_TOK = 14,            /* PARAMETERS_TOK  */
  YYSYMBOL_OBSERVE_TOK = 15,               /* OBSERVE_TOK  */
  YYSYMBOL_EFFECT_TOK = 16,                /* EFFECT_TOK  */
  YYSYMBOL_EQ_TOK = 17,                    /* EQ_TOK  */
  YYSYMBOL_AND_TOK = 18,                   /* AND_TOK  */
  YYSYMBOL_NOT_TOK = 19,                   /* NOT_TOK  */
  YYSYMBOL_WHEN_TOK = 20,                  /* WHEN_TOK  */
  YYSYMBOL_FORALL_TOK = 21,                /* FORALL_TOK  */
  YYSYMBOL_IMPLY_TOK = 22,                 /* IMPLY_TOK  */
  YYSYMBOL_OR_TOK = 23,                    /* OR_TOK  */
  YYSYMBOL_EXISTS_TOK = 24,                /* EXISTS_TOK  */
  YYSYMBOL_NONDET_TOK = 25,                /* NONDET_TOK  */
  YYSYMBOL_NAME = 26,                      /* NAME  */
  YYSYMBOL_VARIABLE = 27,                  /* VARIABLE  */
  YYSYMBOL_TYPE = 28,                      /* TYPE  */
  YYSYMBOL_OPEN_PAREN = 29,                /* OPEN_PAREN  */
  YYSYMBOL_CLOSE_PAREN = 30,               /* CLOSE_PAREN  */
  YYSYMBOL_YYACCEPT = 31,                  /* $accept  */
  YYSYMBOL_file = 32,                      /* file  */
  YYSYMBOL_33_1 = 33,                      /* $@1  */
  YYSYMBOL_domain_definition = 34,         /* domain_definition  */
  YYSYMBOL_35_2 = 35,                      /* $@2  */
  YYSYMBOL_domain_name = 36,               /* domain_name  */
  YYSYMBOL_optional_domain_defs = 37,      /* optional_domain_defs  */
  YYSYMBOL_predicates_def = 38,            /* predicates_def  */
  YYSYMBOL_39_3 = 39,                      /* $@3  */
  YYSYMBOL_predicates_list = 40,           /* predicates_list  */
  YYSYMBOL_41_4 = 41,                      /* $@4  */
  YYSYMBOL_require_def = 42,               /* require_def  */
  YYSYMBOL_43_5 = 43,                      /* $@5  */
  YYSYMBOL_44_6 = 44,                      /* $@6  */
  YYSYMBOL_require_key_star = 45,          /* require_key_star  */
  YYSYMBOL_46_7 = 46,                      /* $@7  */
  YYSYMBOL_types_def = 47,                 /* types_def  */
  YYSYMBOL_48_8 = 48,                      /* $@8  */
  YYSYMBOL_constants_def = 49,             /* constants_def  */
  YYSYMBOL_50_9 = 50,                      /* $@9  */
  YYSYMBOL_action_def = 51,                /* action_def  */
  YYSYMBOL_52_10 = 52,                     /* $@10  */
  YYSYMBOL_53_11 = 53,                     /* $@11  */
  YYSYMBOL_param_def = 54,                 /* param_def  */
  YYSYMBOL_action_def_body = 55,           /* action_def_body  */
  YYSYMBOL_56_12 = 56,                     /* $@12  */
  YYSYMBOL_57_13 = 57,                     /* $@13  */
  YYSYMBOL_58_14 = 58,                     /* $@14  */
  YYSYMBOL_adl_goal_description = 59,      /* adl_goal_description  */
  YYSYMBOL_adl_goal_description_star = 60, /* adl_goal_description_star  */
  YYSYMBOL_adl_effect = 61,                /* adl_effect  */
  YYSYMBOL_adl_effect_star = 62,           /* adl_effect_star  */
  YYSYMBOL_literal_term = 63,              /* literal_term  */
  YYSYMBOL_atomic_formula_term = 64,       /* atomic_formula_term  */
  YYSYMBOL_term_star = 65,                 /* term_star  */
  YYSYMBOL_term = 66,                      /* term  */
  YYSYMBOL_name_plus = 67,                 /* name_plus  */
  YYSYMBOL_predicate = 68,                 /* predicate  */
  YYSYMBOL_typed_list_name = 69,           /* typed_list_name  */
  YYSYMBOL_typed_list_variable = 70        /* typed_list_variable  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   143

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  31
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  40
/* YYNRULES -- Number of rules.  */
#define YYNRULES  75
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  169

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   285


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   165,   165,   165,   176,   175,   189,   199,   201,   203,
     205,   207,   209,   216,   215,   225,   228,   227,   257,   261,
     256,   272,   276,   275,   289,   288,   302,   301,   317,   321,
     316,   335,   339,   352,   355,   374,   373,   380,   379,   389,
     388,   403,   416,   422,   428,   434,   444,   459,   479,   483,
     495,   508,   523,   529,   544,   563,   567,   579,   585,   594,
     601,   614,   616,   627,   633,   643,   650,   661,   672,   674,
     684,   695,   715,   717,   726,   737
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "DEFINE_TOK",
  "DOMAIN_TOK", "REQUIREMENTS_TOK", "TYPES_TOK", "EITHER_TOK",
  "CONSTANTS_TOK", "PREDICATES_TOK", "ACTION_TOK", "VARS_TOK",
  "IMPLIES_TOK", "PRECONDITION_TOK", "PARAMETERS_TOK", "OBSERVE_TOK",
  "EFFECT_TOK", "EQ_TOK", "AND_TOK", "NOT_TOK", "WHEN_TOK", "FORALL_TOK",
  "IMPLY_TOK", "OR_TOK", "EXISTS_TOK", "NONDET_TOK", "NAME", "VARIABLE",
  "TYPE", "OPEN_PAREN", "CLOSE_PAREN", "$accept", "file", "$@1",
  "domain_definition", "$@2", "domain_name", "optional_domain_defs",
  "predicates_def", "$@3", "predicates_list", "$@4", "require_def", "$@5",
  "$@6", "require_key_star", "$@7", "types_def", "$@8", "constants_def",
  "$@9", "action_def", "$@10", "$@11", "param_def", "action_def_body",
  "$@12", "$@13", "$@14", "adl_goal_description",
  "adl_goal_description_star", "adl_effect", "adl_effect_star",
  "literal_term", "atomic_formula_term", "term_star", "term", "name_plus",
  "predicate", "typed_list_name", "typed_list_variable", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-104)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
    -104,    17,   -18,  -104,    16,  -104,    14,    36,  -104,    39,
      -7,    40,    71,  -104,  -104,    -7,    -7,    -7,    -7,    -7,
    -104,  -104,  -104,  -104,    43,  -104,  -104,  -104,  -104,  -104,
    -104,    45,    48,    48,    56,  -104,    57,  -104,    -2,    54,
      62,    73,    72,  -104,    75,    77,    48,  -104,  -104,  -104,
       1,    74,  -104,    55,  -104,    76,    77,    79,  -104,    77,
      73,  -104,  -104,    81,    78,    75,  -104,  -104,    48,    82,
    -104,    43,    73,    84,    85,    86,    87,    88,  -104,  -104,
      73,  -104,    89,    73,    28,  -104,  -104,  -104,    91,    42,
    -104,  -104,  -104,  -104,  -104,    90,    11,    85,    85,    92,
      85,    85,    93,  -104,    11,    78,    11,    87,    94,    85,
      95,    96,    78,    78,  -104,  -104,    97,    11,    85,    98,
      99,   100,    73,    85,   101,    73,   102,  -104,   103,    87,
     104,    22,   100,    87,    73,    47,   105,  -104,  -104,  -104,
    -104,  -104,  -104,  -104,  -104,   106,   107,  -104,   108,  -104,
    -104,  -104,  -104,   109,   110,  -104,    85,  -104,    85,    78,
    -104,    87,   111,   112,  -104,   113,  -104,  -104,  -104
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     0,     1,     0,     3,     0,     0,     4,     0,
       0,     0,     0,     7,     5,     0,     0,     0,     0,     0,
       6,    18,    24,    26,    15,    28,    12,     8,    10,     9,
      11,     0,    68,    68,     0,    13,     0,    19,    68,     0,
       0,    72,     0,    29,    21,     0,    68,    71,    25,    27,
      72,     0,    14,    31,    22,     0,    65,     0,    70,     0,
      72,    75,    16,     0,    33,    21,    20,    66,    68,     0,
      74,    15,    72,     0,     0,     0,     0,     0,    23,    69,
      72,    17,     0,    72,     0,    35,    41,    58,     0,     0,
      39,    50,    30,    73,    32,     0,    61,    48,     0,     0,
       0,    48,     0,    67,    61,    33,    61,    55,     0,     0,
       0,     0,    33,    33,    63,    64,     0,    61,    48,     0,
       0,     0,    72,     0,     0,    72,     0,    36,     0,    55,
       0,     0,     0,     0,    72,     0,     0,    40,    34,    60,
      62,    49,    42,    44,    57,     0,     0,    43,     0,    59,
      37,    56,    52,     0,     0,    51,     0,    45,     0,    33,
      54,     0,     0,     0,    38,     0,    47,    46,    53
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -104,  -104,  -104,  -104,  -104,  -104,    80,  -104,  -104,    34,
    -104,  -104,  -104,  -104,    46,  -104,  -104,  -104,  -104,  -104,
    -104,  -104,  -104,  -104,  -103,  -104,  -104,  -104,   -68,   -97,
     -75,   -22,   -76,   -64,   -90,  -104,   -41,    20,   -26,   -47
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,     2,     5,    10,     8,    14,    15,    42,    35,
      71,    16,    31,    44,    55,    65,    17,    32,    18,    33,
      19,    36,    53,    64,    77,   105,   159,   112,   118,   119,
     129,   130,    86,    87,   116,   117,    57,   104,    39,    51
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      91,    90,   127,    61,   124,    45,    85,    40,    59,   137,
     138,     4,    47,    70,   126,    67,   128,     3,    69,     6,
      58,   141,    12,    13,    38,    82,    46,   140,    50,    60,
     120,    91,   123,    93,   121,   136,    95,   114,   115,    96,
       9,   133,    79,     7,   132,    96,    97,    98,   103,    99,
     100,   101,   102,    91,   103,   146,   164,    91,   153,    96,
     107,   108,   109,   110,    96,    11,   108,   111,   103,    63,
      20,    37,    34,   103,    38,   145,    21,    22,   148,    23,
      24,    25,    41,    43,    48,    91,   165,   154,   162,    73,
     163,    74,    49,    75,    76,    26,    27,    28,    29,    30,
      50,    54,    52,    56,    62,    81,    66,   151,   106,    68,
      72,    78,    80,    83,    84,    88,    89,   103,    92,    94,
     113,   122,   125,   131,   134,   135,     0,   139,   142,   143,
     144,   147,   149,   150,   152,   155,   156,   157,   158,   160,
     161,   166,   167,   168
};

static const yytype_int16 yycheck[] =
{
      76,    76,   105,    50,   101,     7,    74,    33,     7,   112,
     113,    29,    38,    60,   104,    56,   106,     0,    59,     3,
      46,   118,    29,    30,    26,    72,    28,   117,    27,    28,
      98,   107,   100,    80,    98,   111,    83,    26,    27,    17,
       4,   109,    68,    29,   108,    17,    18,    19,    26,    21,
      22,    23,    24,   129,    26,   123,   159,   133,   133,    17,
      18,    19,    20,    21,    17,    26,    19,    25,    26,    14,
      30,    26,    29,    26,    26,   122,     5,     6,   125,     8,
       9,    10,    26,    26,    30,   161,   161,   134,   156,    11,
     158,    13,    30,    15,    16,    15,    16,    17,    18,    19,
      27,    26,    30,    26,    30,    71,    30,   129,    88,    30,
      29,    65,    30,    29,    29,    29,    29,    26,    30,    30,
      30,    29,    29,    29,    29,    29,    -1,    30,    30,    30,
      30,    30,    30,    30,    30,    30,    30,    30,    30,    30,
      30,    30,    30,    30
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    32,    33,     0,    29,    34,     3,    29,    36,     4,
      35,    26,    29,    30,    37,    38,    42,    47,    49,    51,
      30,     5,     6,     8,     9,    10,    37,    37,    37,    37,
      37,    43,    48,    50,    29,    40,    52,    26,    26,    69,
      69,    26,    39,    26,    44,     7,    28,    69,    30,    30,
      27,    70,    30,    53,    26,    45,    26,    67,    69,     7,
      28,    70,    30,    14,    54,    46,    30,    67,    30,    67,
      70,    41,    29,    11,    13,    15,    16,    55,    45,    69,
      30,    40,    70,    29,    29,    59,    63,    64,    29,    29,
      61,    63,    30,    70,    30,    70,    17,    18,    19,    21,
      22,    23,    24,    26,    68,    56,    68,    18,    19,    20,
      21,    25,    58,    30,    26,    27,    65,    66,    59,    60,
      59,    64,    29,    59,    60,    29,    65,    55,    65,    61,
      62,    29,    64,    59,    29,    29,    63,    55,    55,    30,
      65,    60,    30,    30,    30,    70,    59,    30,    70,    30,
      30,    62,    30,    61,    70,    30,    30,    30,    30,    57,
      30,    30,    59,    59,    55,    61,    30,    30,    30
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    31,    33,    32,    35,    34,    36,    37,    37,    37,
      37,    37,    37,    39,    38,    40,    41,    40,    43,    44,
      42,    45,    46,    45,    48,    47,    50,    49,    52,    53,
      51,    54,    54,    55,    55,    56,    55,    57,    55,    58,
      55,    59,    59,    59,    59,    59,    59,    59,    60,    60,
      61,    61,    61,    61,    61,    62,    62,    63,    63,    64,
      64,    65,    65,    66,    66,    67,    67,    68,    69,    69,
      69,    69,    70,    70,    70,    70
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     0,     5,     4,     1,     2,     2,
       2,     2,     2,     0,     5,     0,     0,     6,     0,     0,
       7,     0,     0,     3,     0,     5,     0,     5,     0,     0,
       8,     0,     4,     0,     5,     0,     4,     0,     7,     0,
       4,     1,     4,     4,     4,     5,     7,     7,     0,     2,
       1,     4,     4,     7,     5,     0,     2,     4,     1,     4,
       4,     0,     2,     1,     1,     1,     2,     1,     0,     5,
       3,     2,     0,     5,     3,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* $@1: %empty  */
#line 165 "scan-ops_pddl.y"
{ 
  opserr( DOMDEF_EXPECTED, NULL ); 
}
#line 1424 "scan-ops_pddl.tab.c"
    break;

  case 4: /* $@2: %empty  */
#line 176 "scan-ops_pddl.y"
{ 
}
#line 1431 "scan-ops_pddl.tab.c"
    break;

  case 5: /* domain_definition: OPEN_PAREN DEFINE_TOK domain_name $@2 optional_domain_defs  */
#line 179 "scan-ops_pddl.y"
{
  if ( gcmd_line.display_info >= 1 ) {
    printf("\ndomain '%s' defined\n", gdomain_name);
  }
}
#line 1441 "scan-ops_pddl.tab.c"
    break;

  case 6: /* domain_name: OPEN_PAREN DOMAIN_TOK NAME CLOSE_PAREN  */
#line 190 "scan-ops_pddl.y"
{ 
  gdomain_name = new_Token( strlen((yyvsp[-1].string))+1 );
  strcpy( gdomain_name, (yyvsp[-1].string));
}
#line 1450 "scan-ops_pddl.tab.c"
    break;

  case 13: /* $@3: %empty  */
#line 216 "scan-ops_pddl.y"
{
}
#line 1457 "scan-ops_pddl.tab.c"
    break;

  case 14: /* predicates_def: OPEN_PAREN PREDICATES_TOK predicates_list $@3 CLOSE_PAREN  */
#line 219 "scan-ops_pddl.y"
{ 
}
#line 1464 "scan-ops_pddl.tab.c"
    break;

  case 15: /* predicates_list: %empty  */
#line 225 "scan-ops_pddl.y"
{}
#line 1470 "scan-ops_pddl.tab.c"
    break;

  case 16: /* $@4: %empty  */
#line 228 "scan-ops_pddl.y"
{

  TypedListList *tll;

  if ( gparse_predicates ) {
    tll = gparse_predicates;
    while ( tll->next ) {
      tll = tll->next;
    }
    tll->next = new_TypedListList();
    tll = tll->next;
  } else {
    tll = new_TypedListList();
    gparse_predicates = tll;
  }

  tll->predicate = new_Token( strlen( (yyvsp[-2].string) ) + 1);
  strcpy( tll->predicate, (yyvsp[-2].string) );

  tll->args = (yyvsp[-1].pTypedList);

}
#line 1497 "scan-ops_pddl.tab.c"
    break;

  case 18: /* $@5: %empty  */
#line 257 "scan-ops_pddl.y"
{ 
  opserr( REQUIREM_EXPECTED, NULL ); 
}
#line 1505 "scan-ops_pddl.tab.c"
    break;

  case 19: /* $@6: %empty  */
#line 261 "scan-ops_pddl.y"
{ 
  if ( !supported( (yyvsp[0].string) ) ) {
    opserr( NOT_SUPPORTED, (yyvsp[0].string) );
    yyerror();
  }
}
#line 1516 "scan-ops_pddl.tab.c"
    break;

  case 22: /* $@7: %empty  */
#line 276 "scan-ops_pddl.y"
{ 
  if ( !supported( (yyvsp[0].string) ) ) {
    opserr( NOT_SUPPORTED, (yyvsp[0].string) );
    yyerror();
  }
}
#line 1527 "scan-ops_pddl.tab.c"
    break;

  case 24: /* $@8: %empty  */
#line 289 "scan-ops_pddl.y"
{ 
  opserr( TYPEDEF_EXPECTED, NULL ); 
}
#line 1535 "scan-ops_pddl.tab.c"
    break;

  case 25: /* types_def: OPEN_PAREN TYPES_TOK $@8 typed_list_name CLOSE_PAREN  */
#line 293 "scan-ops_pddl.y"
{
  gparse_types = (yyvsp[-1].pTypedList);
}
#line 1543 "scan-ops_pddl.tab.c"
    break;

  case 26: /* $@9: %empty  */
#line 302 "scan-ops_pddl.y"
{ 
  opserr( CONSTLIST_EXPECTED, NULL ); 
}
#line 1551 "scan-ops_pddl.tab.c"
    break;

  case 27: /* constants_def: OPEN_PAREN CONSTANTS_TOK $@9 typed_list_name CLOSE_PAREN  */
#line 306 "scan-ops_pddl.y"
{
  gparse_constants = (yyvsp[-1].pTypedList);
}
#line 1559 "scan-ops_pddl.tab.c"
    break;

  case 28: /* $@10: %empty  */
#line 317 "scan-ops_pddl.y"
{ 
  opserr( ACTION, NULL ); 
}
#line 1567 "scan-ops_pddl.tab.c"
    break;

  case 29: /* $@11: %empty  */
#line 321 "scan-ops_pddl.y"
{ 
  scur_op = new_PlOperator( (yyvsp[0].string) );
}
#line 1575 "scan-ops_pddl.tab.c"
    break;

  case 30: /* action_def: OPEN_PAREN ACTION_TOK $@10 NAME $@11 param_def action_def_body CLOSE_PAREN  */
#line 325 "scan-ops_pddl.y"
{
  scur_op->next = gloaded_ops;
  gloaded_ops = scur_op; 
}
#line 1584 "scan-ops_pddl.tab.c"
    break;

  case 31: /* param_def: %empty  */
#line 335 "scan-ops_pddl.y"
{ 
  scur_op->params = NULL; 
}
#line 1592 "scan-ops_pddl.tab.c"
    break;

  case 32: /* param_def: PARAMETERS_TOK OPEN_PAREN typed_list_variable CLOSE_PAREN  */
#line 340 "scan-ops_pddl.y"
{
  TypedList *tl;
  scur_op->parse_params = (yyvsp[-1].pTypedList);
  for (tl = scur_op->parse_params; tl; tl = tl->next) {
    /* to be able to distinguish params from :VARS 
     */
    scur_op->number_of_real_params++;
  }
}
#line 1606 "scan-ops_pddl.tab.c"
    break;

  case 34: /* action_def_body: VARS_TOK OPEN_PAREN typed_list_variable CLOSE_PAREN action_def_body  */
#line 356 "scan-ops_pddl.y"
{
  TypedList *tl = NULL;

  /* add vars as parameters 
   */
  if ( scur_op->parse_params ) {
    for( tl = scur_op->parse_params; tl->next; tl = tl->next ) {
      /* empty, get to the end of list 
       */
    }
    tl->next = (yyvsp[-2].pTypedList);
    tl = tl->next;
  } else {
    scur_op->parse_params = (yyvsp[-2].pTypedList);
  }
}
#line 1627 "scan-ops_pddl.tab.c"
    break;

  case 35: /* $@12: %empty  */
#line 374 "scan-ops_pddl.y"
{ 
  scur_op->preconds = (yyvsp[0].pPlNode); 
}
#line 1635 "scan-ops_pddl.tab.c"
    break;

  case 37: /* $@13: %empty  */
#line 380 "scan-ops_pddl.y"
{ 
  scur_op->observe = new_PlNode(ATOM);
  scur_op->observe->atom = new_TokenList();
  scur_op->observe->atom->item = (yyvsp[-2].pstring);
  scur_op->observe->atom->next = (yyvsp[-1].pTokenList);
}
#line 1646 "scan-ops_pddl.tab.c"
    break;

  case 39: /* $@14: %empty  */
#line 389 "scan-ops_pddl.y"
{ 
  scur_op->effects = (yyvsp[0].pPlNode); 
}
#line 1654 "scan-ops_pddl.tab.c"
    break;

  case 41: /* adl_goal_description: literal_term  */
#line 404 "scan-ops_pddl.y"
{ 
  if ( sis_negated ) {
    (yyval.pPlNode) = new_PlNode(NOT);
    (yyval.pPlNode)->sons = new_PlNode(ATOM);
    (yyval.pPlNode)->sons->atom = (yyvsp[0].pTokenList);
    sis_negated = FALSE;
  } else {
    (yyval.pPlNode) = new_PlNode(ATOM);
    (yyval.pPlNode)->atom = (yyvsp[0].pTokenList);
  }
}
#line 1670 "scan-ops_pddl.tab.c"
    break;

  case 42: /* adl_goal_description: OPEN_PAREN AND_TOK adl_goal_description_star CLOSE_PAREN  */
#line 417 "scan-ops_pddl.y"
{ 
  (yyval.pPlNode) = new_PlNode(AND);
  (yyval.pPlNode)->sons = (yyvsp[-1].pPlNode);
}
#line 1679 "scan-ops_pddl.tab.c"
    break;

  case 43: /* adl_goal_description: OPEN_PAREN OR_TOK adl_goal_description_star CLOSE_PAREN  */
#line 423 "scan-ops_pddl.y"
{ 
  (yyval.pPlNode) = new_PlNode(OR);
  (yyval.pPlNode)->sons = (yyvsp[-1].pPlNode);
}
#line 1688 "scan-ops_pddl.tab.c"
    break;

  case 44: /* adl_goal_description: OPEN_PAREN NOT_TOK adl_goal_description CLOSE_PAREN  */
#line 429 "scan-ops_pddl.y"
{ 
  (yyval.pPlNode) = new_PlNode(NOT);
  (yyval.pPlNode)->sons = (yyvsp[-1].pPlNode);
}
#line 1697 "scan-ops_pddl.tab.c"
    break;

  case 45: /* adl_goal_description: OPEN_PAREN IMPLY_TOK adl_goal_description adl_goal_description CLOSE_PAREN  */
#line 435 "scan-ops_pddl.y"
{ 
  PlNode *np = new_PlNode(NOT);
  np->sons = (yyvsp[-2].pPlNode);
  np->next = (yyvsp[-1].pPlNode);

  (yyval.pPlNode) = new_PlNode(OR);
  (yyval.pPlNode)->sons = np;
}
#line 1710 "scan-ops_pddl.tab.c"
    break;

  case 46: /* adl_goal_description: OPEN_PAREN EXISTS_TOK OPEN_PAREN typed_list_variable CLOSE_PAREN adl_goal_description CLOSE_PAREN  */
#line 447 "scan-ops_pddl.y"
{ 

  PlNode *pln;

  pln = new_PlNode(EX);
  pln->parse_vars = (yyvsp[-3].pTypedList);

  (yyval.pPlNode) = pln;
  pln->sons = (yyvsp[-1].pPlNode);

}
#line 1726 "scan-ops_pddl.tab.c"
    break;

  case 47: /* adl_goal_description: OPEN_PAREN FORALL_TOK OPEN_PAREN typed_list_variable CLOSE_PAREN adl_goal_description CLOSE_PAREN  */
#line 462 "scan-ops_pddl.y"
{ 

  PlNode *pln;

  pln = new_PlNode(ALL);
  pln->parse_vars = (yyvsp[-3].pTypedList);

  (yyval.pPlNode) = pln;
  pln->sons = (yyvsp[-1].pPlNode);

}
#line 1742 "scan-ops_pddl.tab.c"
    break;

  case 48: /* adl_goal_description_star: %empty  */
#line 479 "scan-ops_pddl.y"
{
  (yyval.pPlNode) = NULL;
}
#line 1750 "scan-ops_pddl.tab.c"
    break;

  case 49: /* adl_goal_description_star: adl_goal_description adl_goal_description_star  */
#line 484 "scan-ops_pddl.y"
{
  (yyvsp[-1].pPlNode)->next = (yyvsp[0].pPlNode);
  (yyval.pPlNode) = (yyvsp[-1].pPlNode);
}
#line 1759 "scan-ops_pddl.tab.c"
    break;

  case 50: /* adl_effect: literal_term  */
#line 496 "scan-ops_pddl.y"
{ 
  if ( sis_negated ) {
    (yyval.pPlNode) = new_PlNode(NOT);
    (yyval.pPlNode)->sons = new_PlNode(ATOM);
    (yyval.pPlNode)->sons->atom = (yyvsp[0].pTokenList);
    sis_negated = FALSE;
  } else {
    (yyval.pPlNode) = new_PlNode(ATOM);
    (yyval.pPlNode)->atom = (yyvsp[0].pTokenList);
  }
}
#line 1775 "scan-ops_pddl.tab.c"
    break;

  case 51: /* adl_effect: OPEN_PAREN NONDET_TOK literal_term CLOSE_PAREN  */
#line 509 "scan-ops_pddl.y"
{ 
  if ( sis_negated ) {
    (yyval.pPlNode) = new_PlNode(NOT);
    (yyval.pPlNode)->sons = new_PlNode(ATOM);
    (yyval.pPlNode)->sons->atom = (yyvsp[-1].pTokenList);
    sis_negated = FALSE;
  } else {
    (yyval.pPlNode) = new_PlNode(ATOM);
    (yyval.pPlNode)->atom = (yyvsp[-1].pTokenList);
  }
  (yyval.pPlNode)->is_nondeteff = TRUE;

}
#line 1793 "scan-ops_pddl.tab.c"
    break;

  case 52: /* adl_effect: OPEN_PAREN AND_TOK adl_effect_star CLOSE_PAREN  */
#line 524 "scan-ops_pddl.y"
{ 
  (yyval.pPlNode) = new_PlNode(AND);
  (yyval.pPlNode)->sons = (yyvsp[-1].pPlNode);
}
#line 1802 "scan-ops_pddl.tab.c"
    break;

  case 53: /* adl_effect: OPEN_PAREN FORALL_TOK OPEN_PAREN typed_list_variable CLOSE_PAREN adl_effect CLOSE_PAREN  */
#line 532 "scan-ops_pddl.y"
{ 

  PlNode *pln;

  pln = new_PlNode(ALL);
  pln->parse_vars = (yyvsp[-3].pTypedList);

  (yyval.pPlNode) = pln;
  pln->sons = (yyvsp[-1].pPlNode);

}
#line 1818 "scan-ops_pddl.tab.c"
    break;

  case 54: /* adl_effect: OPEN_PAREN WHEN_TOK adl_goal_description adl_effect CLOSE_PAREN  */
#line 545 "scan-ops_pddl.y"
{
  /* This will be conditional effects in FF representation, but here
   * a formula like (WHEN p q) will be saved as:
   *  [WHEN]
   *  [sons]
   *   /  \
   * [p]  [q]
   * That means, the first son is p, and the second one is q. 
   */
  (yyval.pPlNode) = new_PlNode(WHEN);
  (yyvsp[-2].pPlNode)->next = (yyvsp[-1].pPlNode);
  (yyval.pPlNode)->sons = (yyvsp[-2].pPlNode);
}
#line 1836 "scan-ops_pddl.tab.c"
    break;

  case 55: /* adl_effect_star: %empty  */
#line 563 "scan-ops_pddl.y"
{ 
  (yyval.pPlNode) = NULL; 
}
#line 1844 "scan-ops_pddl.tab.c"
    break;

  case 56: /* adl_effect_star: adl_effect adl_effect_star  */
#line 568 "scan-ops_pddl.y"
{
  (yyvsp[-1].pPlNode)->next = (yyvsp[0].pPlNode);
  (yyval.pPlNode) = (yyvsp[-1].pPlNode);
}
#line 1853 "scan-ops_pddl.tab.c"
    break;

  case 57: /* literal_term: OPEN_PAREN NOT_TOK atomic_formula_term CLOSE_PAREN  */
#line 580 "scan-ops_pddl.y"
{ 
  (yyval.pTokenList) = (yyvsp[-1].pTokenList);
  sis_negated = TRUE;
}
#line 1862 "scan-ops_pddl.tab.c"
    break;

  case 58: /* literal_term: atomic_formula_term  */
#line 586 "scan-ops_pddl.y"
{
  (yyval.pTokenList) = (yyvsp[0].pTokenList);
}
#line 1870 "scan-ops_pddl.tab.c"
    break;

  case 59: /* atomic_formula_term: OPEN_PAREN predicate term_star CLOSE_PAREN  */
#line 595 "scan-ops_pddl.y"
{ 
  (yyval.pTokenList) = new_TokenList();
  (yyval.pTokenList)->item = (yyvsp[-2].pstring);
  (yyval.pTokenList)->next = (yyvsp[-1].pTokenList);
}
#line 1880 "scan-ops_pddl.tab.c"
    break;

  case 60: /* atomic_formula_term: OPEN_PAREN EQ_TOK term_star CLOSE_PAREN  */
#line 602 "scan-ops_pddl.y"
{
  (yyval.pTokenList) = new_TokenList();
  (yyval.pTokenList)->item = new_Token( 5 );
  (yyval.pTokenList)->item = "=";
  (yyval.pTokenList)->next = (yyvsp[-1].pTokenList);
}
#line 1891 "scan-ops_pddl.tab.c"
    break;

  case 61: /* term_star: %empty  */
#line 614 "scan-ops_pddl.y"
{ (yyval.pTokenList) = NULL; }
#line 1897 "scan-ops_pddl.tab.c"
    break;

  case 62: /* term_star: term term_star  */
#line 617 "scan-ops_pddl.y"
{
  (yyval.pTokenList) = new_TokenList();
  (yyval.pTokenList)->item = (yyvsp[-1].pstring);
  (yyval.pTokenList)->next = (yyvsp[0].pTokenList);
}
#line 1907 "scan-ops_pddl.tab.c"
    break;

  case 63: /* term: NAME  */
#line 628 "scan-ops_pddl.y"
{ 
  (yyval.pstring) = new_Token( strlen((yyvsp[0].string))+1 );
  strcpy( (yyval.pstring), (yyvsp[0].string) );
}
#line 1916 "scan-ops_pddl.tab.c"
    break;

  case 64: /* term: VARIABLE  */
#line 634 "scan-ops_pddl.y"
{ 
  (yyval.pstring) = new_Token( strlen((yyvsp[0].string))+1 );
  strcpy( (yyval.pstring), (yyvsp[0].string) );
}
#line 1925 "scan-ops_pddl.tab.c"
    break;

  case 65: /* name_plus: NAME  */
#line 644 "scan-ops_pddl.y"
{
  (yyval.pTokenList) = new_TokenList();
  (yyval.pTokenList)->item = new_Token( strlen((yyvsp[0].string))+1 );
  strcpy( (yyval.pTokenList)->item, (yyvsp[0].string) );
}
#line 1935 "scan-ops_pddl.tab.c"
    break;

  case 66: /* name_plus: NAME name_plus  */
#line 651 "scan-ops_pddl.y"
{
  (yyval.pTokenList) = new_TokenList();
  (yyval.pTokenList)->item = new_Token( strlen((yyvsp[-1].string))+1 );
  strcpy( (yyval.pTokenList)->item, (yyvsp[-1].string) );
  (yyval.pTokenList)->next = (yyvsp[0].pTokenList);
}
#line 1946 "scan-ops_pddl.tab.c"
    break;

  case 67: /* predicate: NAME  */
#line 662 "scan-ops_pddl.y"
{ 
  (yyval.pstring) = new_Token( strlen((yyvsp[0].string))+1 );
  strcpy( (yyval.pstring), (yyvsp[0].string) );
}
#line 1955 "scan-ops_pddl.tab.c"
    break;

  case 68: /* typed_list_name: %empty  */
#line 672 "scan-ops_pddl.y"
{ (yyval.pTypedList) = NULL; }
#line 1961 "scan-ops_pddl.tab.c"
    break;

  case 69: /* typed_list_name: NAME EITHER_TOK name_plus CLOSE_PAREN typed_list_name  */
#line 675 "scan-ops_pddl.y"
{ 

  (yyval.pTypedList) = new_TypedList();
  (yyval.pTypedList)->name = new_Token( strlen((yyvsp[-4].string))+1 );
  strcpy( (yyval.pTypedList)->name, (yyvsp[-4].string) );
  (yyval.pTypedList)->type = (yyvsp[-2].pTokenList);
  (yyval.pTypedList)->next = (yyvsp[0].pTypedList);
}
#line 1974 "scan-ops_pddl.tab.c"
    break;

  case 70: /* typed_list_name: NAME TYPE typed_list_name  */
#line 685 "scan-ops_pddl.y"
{
  (yyval.pTypedList) = new_TypedList();
  (yyval.pTypedList)->name = new_Token( strlen((yyvsp[-2].string))+1 );
  strcpy( (yyval.pTypedList)->name, (yyvsp[-2].string) );
  (yyval.pTypedList)->type = new_TokenList();
  (yyval.pTypedList)->type->item = new_Token( strlen((yyvsp[-1].string))+1 );
  strcpy( (yyval.pTypedList)->type->item, (yyvsp[-1].string) );
  (yyval.pTypedList)->next = (yyvsp[0].pTypedList);
}
#line 1988 "scan-ops_pddl.tab.c"
    break;

  case 71: /* typed_list_name: NAME typed_list_name  */
#line 696 "scan-ops_pddl.y"
{
  (yyval.pTypedList) = new_TypedList();
  (yyval.pTypedList)->name = new_Token( strlen((yyvsp[-1].string))+1 );
  strcpy( (yyval.pTypedList)->name, (yyvsp[-1].string) );
  if ( (yyvsp[0].pTypedList) ) {/* another element (already typed) is following */
    (yyval.pTypedList)->type = copy_TokenList( (yyvsp[0].pTypedList)->type );
  } else {/* no further element - it must be an untyped list */
    (yyval.pTypedList)->type = new_TokenList();
    (yyval.pTypedList)->type->item = new_Token( strlen(STANDARD_TYPE)+1 );
    strcpy( (yyval.pTypedList)->type->item, STANDARD_TYPE );
  }
  (yyval.pTypedList)->next = (yyvsp[0].pTypedList);
}
#line 2006 "scan-ops_pddl.tab.c"
    break;

  case 72: /* typed_list_variable: %empty  */
#line 715 "scan-ops_pddl.y"
{ (yyval.pTypedList) = NULL; }
#line 2012 "scan-ops_pddl.tab.c"
    break;

  case 73: /* typed_list_variable: VARIABLE EITHER_TOK name_plus CLOSE_PAREN typed_list_variable  */
#line 718 "scan-ops_pddl.y"
{ 
  (yyval.pTypedList) = new_TypedList();
  (yyval.pTypedList)->name = new_Token( strlen((yyvsp[-4].string))+1 );
  strcpy( (yyval.pTypedList)->name, (yyvsp[-4].string) );
  (yyval.pTypedList)->type = (yyvsp[-2].pTokenList);
  (yyval.pTypedList)->next = (yyvsp[0].pTypedList);
}
#line 2024 "scan-ops_pddl.tab.c"
    break;

  case 74: /* typed_list_variable: VARIABLE TYPE typed_list_variable  */
#line 727 "scan-ops_pddl.y"
{
  (yyval.pTypedList) = new_TypedList();
  (yyval.pTypedList)->name = new_Token( strlen((yyvsp[-2].string))+1 );
  strcpy( (yyval.pTypedList)->name, (yyvsp[-2].string) );
  (yyval.pTypedList)->type = new_TokenList();
  (yyval.pTypedList)->type->item = new_Token( strlen((yyvsp[-1].string))+1 );
  strcpy( (yyval.pTypedList)->type->item, (yyvsp[-1].string) );
  (yyval.pTypedList)->next = (yyvsp[0].pTypedList);
}
#line 2038 "scan-ops_pddl.tab.c"
    break;

  case 75: /* typed_list_variable: VARIABLE typed_list_variable  */
#line 738 "scan-ops_pddl.y"
{
  (yyval.pTypedList) = new_TypedList();
  (yyval.pTypedList)->name = new_Token( strlen((yyvsp[-1].string))+1 );
  strcpy( (yyval.pTypedList)->name, (yyvsp[-1].string) );
  if ( (yyvsp[0].pTypedList) ) {/* another element (already typed) is following */
    (yyval.pTypedList)->type = copy_TokenList( (yyvsp[0].pTypedList)->type );
  } else {/* no further element - it must be an untyped list */
    (yyval.pTypedList)->type = new_TokenList();
    (yyval.pTypedList)->type->item = new_Token( strlen(STANDARD_TYPE)+1 );
    strcpy( (yyval.pTypedList)->type->item, STANDARD_TYPE );
  }
  (yyval.pTypedList)->next = (yyvsp[0].pTypedList);
}
#line 2056 "scan-ops_pddl.tab.c"
    break;


#line 2060 "scan-ops_pddl.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 755 "scan-ops_pddl.y"

#include "lex.ops_pddl.c"


/**********************************************************************
 * Functions
 **********************************************************************/

/* 
 * call	bison -pops -bscan-ops scan-ops.y
 */

void opserr( int errno, char *par )

{

/*   sact_err = errno; */

/*   if ( sact_err_par ) { */
/*     free(sact_err_par); */
/*   } */
/*   if ( par ) { */
/*     sact_err_par = new_Token(strlen(par)+1); */
/*     strcpy(sact_err_par, par); */
/*   } else { */
/*     sact_err_par = NULL; */
/*   } */

}
  


int yyerror( char *msg )

{

  fflush(stdout);
  fprintf(stderr, "\n%s: syntax error in line %d, '%s':\n", 
	  gact_filename, lineno, yytext);

  if ( NULL != sact_err_par ) {
    fprintf(stderr, "%s %s\n", serrmsg[sact_err], sact_err_par);
  } else {
    fprintf(stderr, "%s\n", serrmsg[sact_err]);
  }

  exit( 1 );

}



void load_ops_file( char *filename )

{

  FILE * fp;/* pointer to input files */
  char tmp[MAX_LENGTH] = "";

  /* open operator file 
   */
  if( ( fp = fopen( filename, "r" ) ) == NULL ) {
    sprintf(tmp, "\nff: can't find operator file: %s\n\n", filename );
    perror(tmp);
    exit( 1 );
  }

  gact_filename = filename;
  lineno = 1; 
  yyin = fp;

  yyparse();

  fclose( fp );/* and close file again */

}
