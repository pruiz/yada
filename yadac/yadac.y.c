/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INCLUDE = 258,
     PREPARE = 259,
     YPREPARE = 260,
     RESULT = 261,
     STATUS = 262,
     SIMPLE = 263,
     SINGLE = 264,
     STRUCT = 265,
     MULTIROW = 266,
     MULTISTRUCT = 267,
     INT = 268,
     FLOAT = 269,
     LONG = 270,
     BINARY = 271,
     VARCHAR = 272,
     INTP = 273,
     FLOATP = 274,
     LONGP = 275,
     BINARYP = 276,
     VARCHARP = 277,
     TOKEN = 278,
     PARAM = 279,
     SQL = 280,
     STRING = 281,
     CVAR = 282,
     INDENT = 283
   };
#endif
/* Tokens.  */
#define INCLUDE 258
#define PREPARE 259
#define YPREPARE 260
#define RESULT 261
#define STATUS 262
#define SIMPLE 263
#define SINGLE 264
#define STRUCT 265
#define MULTIROW 266
#define MULTISTRUCT 267
#define INT 268
#define FLOAT 269
#define LONG 270
#define BINARY 271
#define VARCHAR 272
#define INTP 273
#define FLOATP 274
#define LONGP 275
#define BINARYP 276
#define VARCHARP 277
#define TOKEN 278
#define PARAM 279
#define SQL 280
#define STRING 281
#define CVAR 282
#define INDENT 283




/* Copy the first part of user declarations.  */
#line 12 "yadac.y"

/******************************************************************************
 * I N C L U D E S ************************************************************
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "yadac.h"
#include "yadac.y.h"
#include "yadac.lex.h"

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

/* autonumber enumeration */
#define FLAG_AUTONUM		1

/******************************************************************************
 * M A C R O S ****************************************************************
 ******************************************************************************/

/* trace for debugging */
#ifdef DEBUG
# define TRACE(m...)	printf(m);
#else
# define TRACE(m...)
#endif

/******************************************************************************
 * I N T E R N A L   F U N C T I O N S ****************************************
 ******************************************************************************/

/******************************************************************************/
/** error handler
 */

static int yyerror(YYLTYPE *loc, yyscan_t scanner, yadac_t *tree, char *msg)
{
  printf("yadac_parse: %s\n", msg);
  printf("line: %i\n", yyget_lineno(scanner) + 1);
  return(-1);
}

/******************************************************************************/
/** free list of SQL chunks
 */

static void free_sql(yadac_line_t *line)
{
  int loop;


  if(!line)
    return;

  /* free all chunks */
  for(loop=0; line[loop].text; loop++)
    free(line[loop].text);

  free(line);
}

/******************************************************************************/
/** add parameter to parameter list
 * @return parameter list pointer on success
 */

static yadac_param_t* add_param(yadac_param_t *list, yadac_param_t param)
{
  int ctr = 0;
  yadac_param_t *new;


  TRACE("param: %u\n", (param - (INT - YADAC_INT)));

  /* count current parameters in list */
  while(list && list[ctr])
    ctr++;

  /* allocate space for new parameter */
  if(!(new = realloc(list, (sizeof(yadac_param_t) * (ctr + 2)))))
    {
    free(list);
    return(NULL);
    }

  /* append parameter */
  new[ctr++] = param;
  new[ctr] = 0;
  return(new);
}

/******************************************************************************/
/** add a token to token list
 * @return token list pointer on success
 */

static yadac_tlist_t* add_token(yadac_tlist_t *list, char *token)
{
  int ctr = list ? list->tokens : 0;
  yadac_tlist_t *new;


  TRACE("token: %s\n", token);

  /* allocate space for new token */
  if(!(new = realloc(list, sizeof(yadac_tlist_t) + sizeof(char *) * ctr)))
    {
    free(list);
    return(NULL);
    }

  /* append parameter */
  new->token[ctr] = token;
  new->tokens = ctr + 1;
  return(new);
}

/******************************************************************************/
/** add a struct definition
 * @return strct pointer on success
 */

static yadac_struct_t* add_struct(char *type, yadac_tlist_t *list)
{
  yadac_struct_t *new;


  TRACE("struct: %s\n", type);

  /* allocate space for new token */
  if(!(new = malloc(sizeof(yadac_struct_t))))
    return(NULL);

  /* append parameter */
  new->type = type;
  new->list = list;
  return(new);
}

/******************************************************************************/
/** add SQL text to list
 * @return SQL pointer on success
 */

static yadac_sql_t* add_sql(yadac_sql_t *sql, int indent,
 char *text, yadac_param_t *param)
{
  int ctr = 0;
  void *new;


  TRACE("sql: (%u)%s\n", indent, text);

  /* allocate main structure if needed */
  if(!sql && !(sql = calloc(1, sizeof(yadac_sql_t))))
    return(NULL);

  /* allocate space for new SQL line */
  if(!(new = realloc(sql->line, (sizeof(yadac_line_t) * (sql->lines + 2)))))
    {
    free_sql(sql->line);
    free(sql->param);
    free(sql);
    return(NULL);
    }
  sql->line = new;

  /* append SQL line */
  sql->line[sql->lines].indent = indent;
  sql->line[sql->lines++].text = text;
  sql->line[sql->lines].text = NULL;


  /* attach params */
  if(param)
    {
    /* count parameters and fix offsets */
    while(param[ctr])
      {
      param[ctr] -= (INT - YADAC_INT);
      ctr++;
      }

    if(sql->param)
      {
      /* allocate additional space for params */
      if(!(new = realloc(sql->param, (sizeof(int) * (sql->params + ctr)))))
        {
        free_sql(sql->line);
        free(sql->param);
        free(sql);
        return(NULL);
        }
      sql->param = new;
      memcpy((sql->param + sql->params), param, (sizeof(int) * ctr));
      free(param);
      sql->params += ctr;
      }
    else
      {
      sql->param = param;
      sql->params = ctr;
      }
    } /* if(params) */

  return(sql);
}

/******************************************************************************/
/** append binding to parse tree
 * @return 0 on success
 */

static int binding(yadac_t *tree, int ptype, char *name, yadac_sql_t *sql,
 int rtype, yadac_param_t *param, yadac_struct_t *istruct,
 yadac_struct_t *ostruct)
{
  void *new;
  char *upper, *ptr;
  yadac_binding_t *yb;


  TRACE("binding: %s\n", name);

  /* make copy of name for uppercase version */
  if(!(upper = strdup(name)))
    return(-1);
  for(ptr=upper; *ptr; ptr++)
    *ptr = toupper(*ptr);

  /* allocate space for new binding */
  if(!(new = realloc(tree->bind,
   (sizeof(yadac_binding_t) * (tree->binds + 1)))))
    {
    free(upper);
    return(-1);
    }

  tree->bind = new;
  yb = (tree->bind + tree->binds++);

  /* init new binding */
  yb->ptype = ptype;
  yb->rtype = rtype;
  yb->name = name;
  yb->upper = upper;
  yb->line = sql->line;
  yb->lines = sql->lines;
  yb->iparam = sql->param;
  yb->iparams = sql->params;
  yb->oparam = param;
  yb->istruct = istruct;
  yb->ostruct = ostruct;

  /* count output parameters and fix offsets */
  for(yb->oparams=0; (param && param[yb->oparams]); )
    {
    param[yb->oparams] -= (INT - YADAC_INT);
    yb->oparams++;
    }

  /* make sure struct count matches oparam count */
// FIXME - add check for multi
  if(yb->rtype == YADAC_STRUCT && yb->ostruct->list->tokens != yb->oparams)
    printf(
     "WARNING: struct var count does not match output param count for %s\n",
     yb->name);

  free(sql);
  return(0);
}

/******************************************************************************/
/** add include to tree
 * @return 0 on success
 */

static int new_inc(yadac_t *tree, char *str)
{
  void *new;


  /* allocate pointer */
  if(!(new = realloc(tree->inc, (sizeof(char*) * (tree->incs + 1)))))
    return(-1);
  tree->inc = new;
  tree->inc[tree->incs++] = str;

  return(0);
}

/******************************************************************************/


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 315 "yadac.y"
{
  char *str;
  int type;
  int num;
  yadac_param_t param;
  yadac_param_t *params;
  yadac_tlist_t *tlist;
  yadac_struct_t *strct;
  yadac_sql_t *sql;
}
/* Line 187 of yacc.c.  */
#line 462 "yadac.y.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 487 "yadac.y.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
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
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  8
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   201

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  35
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  13
/* YYNRULES -- Number of rules.  */
#define YYNRULES  55
/* YYNRULES -- Number of states.  */
#define YYNSTATES  98

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   283

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      33,    34,     2,     2,    32,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    29,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    30,     2,    31,     2,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     6,     9,    13,    18,    27,    37,
      47,    58,    68,    79,    90,   102,   104,   106,   108,   110,
     112,   114,   116,   118,   120,   122,   124,   126,   128,   130,
     132,   134,   136,   138,   140,   142,   144,   146,   148,   150,
     152,   156,   158,   161,   163,   165,   169,   173,   178,   180,
     183,   186,   190,   193,   197,   201
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      36,     0,    -1,    -1,    37,    -1,    36,    37,    -1,     3,
      26,    29,    -1,    36,     3,    26,    29,    -1,    38,    23,
      30,    47,    31,     6,     7,    29,    -1,    38,    23,    46,
      30,    47,    31,     6,     7,    29,    -1,    38,    23,    30,
      47,    31,     6,     8,    41,    29,    -1,    38,    23,    46,
      30,    47,    31,     6,     8,    41,    29,    -1,    38,    23,
      30,    47,    31,     6,    39,    43,    29,    -1,    38,    23,
      46,    30,    47,    31,     6,    39,    43,    29,    -1,    38,
      23,    30,    47,    31,     6,    40,    46,    43,    29,    -1,
      38,    23,    46,    30,    47,    31,     6,    40,    46,    43,
      29,    -1,     4,    -1,     5,    -1,     9,    -1,    11,    -1,
      10,    -1,    12,    -1,    13,    -1,    18,    -1,    14,    -1,
      19,    -1,    15,    -1,    20,    -1,    21,    -1,    22,    -1,
      13,    -1,    18,    -1,    14,    -1,    19,    -1,    15,    -1,
      20,    -1,    16,    -1,    21,    -1,    17,    -1,    22,    -1,
      42,    -1,    43,    32,    42,    -1,    42,    -1,    44,    42,
      -1,    27,    -1,    23,    -1,    45,    32,    27,    -1,    45,
      32,    23,    -1,    23,    33,    45,    34,    -1,    25,    -1,
      44,    25,    -1,    28,    25,    -1,    28,    44,    25,    -1,
      47,    25,    -1,    47,    44,    25,    -1,    47,    28,    25,
      -1,    47,    28,    44,    25,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   353,   353,   354,   355,   356,   357,   360,   363,   366,
     370,   374,   377,   380,   385,   393,   394,   398,   399,   403,
     404,   408,   409,   410,   411,   412,   413,   414,   415,   419,
     420,   421,   422,   423,   424,   425,   426,   427,   428,   432,
     433,   437,   438,   442,   443,   444,   445,   447,   450,   451,
     452,   453,   454,   455,   456,   457
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INCLUDE", "PREPARE", "YPREPARE",
  "RESULT", "STATUS", "SIMPLE", "SINGLE", "STRUCT", "MULTIROW",
  "MULTISTRUCT", "INT", "FLOAT", "LONG", "BINARY", "VARCHAR", "INTP",
  "FLOATP", "LONGP", "BINARYP", "VARCHARP", "TOKEN", "PARAM", "SQL",
  "STRING", "CVAR", "INDENT", "';'", "'['", "']'", "','", "'('", "')'",
  "$accept", "all", "binding", "ptype", "rtype", "rstype", "simple",
  "param", "oparams", "iparams", "tlist", "strct", "sql", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,    59,
      91,    93,    44,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    35,    36,    36,    36,    36,    36,    37,    37,    37,
      37,    37,    37,    37,    37,    38,    38,    39,    39,    40,
      40,    41,    41,    41,    41,    41,    41,    41,    41,    42,
      42,    42,    42,    42,    42,    42,    42,    42,    42,    43,
      43,    44,    44,    45,    45,    45,    45,    46,    47,    47,
      47,    47,    47,    47,    47,    47
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     2,     3,     4,     8,     9,     9,
      10,     9,    10,    10,    11,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       3,     1,     2,     1,     1,     3,     3,     4,     1,     2,
       2,     3,     2,     3,     3,     4
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,    15,    16,     0,     3,     0,     0,     1,     0,
       4,     0,     5,     0,     0,     0,     0,     6,     0,    29,
      31,    33,    35,    37,    30,    32,    34,    36,    38,    48,
       0,    41,     0,     0,     0,    44,    43,     0,    50,     0,
      49,    42,    52,     0,     0,     0,     0,     0,    47,    51,
      54,     0,     0,    53,     0,    46,    45,    55,     0,     0,
      17,    19,    18,    20,     0,     0,     0,     7,    21,    23,
      25,    22,    24,    26,    27,    28,     0,    39,     0,     0,
       0,     0,     0,     0,     9,    11,     0,     0,     8,     0,
       0,     0,    40,    13,    10,    12,     0,    14
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     4,     5,     6,    64,    65,    76,    31,    78,    32,
      37,    16,    33
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -77
static const yytype_int16 yypact[] =
{
     196,   -17,   -77,   -77,    48,   -77,   -13,     2,   -77,     9,
     -77,   -18,   -77,    27,     4,    66,    40,   -77,    11,   -77,
     -77,   -77,   -77,   -77,   -77,   -77,   -77,   -77,   -77,   -77,
      82,   -77,    95,     8,    66,   -77,   -77,   -16,   -77,   108,
     -77,   -77,   -77,   121,    51,   134,    47,    50,   -77,   -77,
     -77,   147,    33,   -77,    70,   -77,   -77,   -77,    60,   170,
     -77,   -77,   -77,   -77,   160,    67,   186,   -77,   -77,   -77,
     -77,   -77,   -77,   -77,   -77,   -77,    63,   -77,   -21,   160,
      64,   170,   160,    67,   -77,   -77,   160,    17,   -77,    76,
      26,   160,   -77,   -77,   -77,   -77,    42,   -77
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -77,   -77,   102,   -77,    52,    53,    77,   -32,   -76,   -29,
     -77,   -63,    97
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      41,    39,    79,    87,    45,    14,    90,    41,    85,     7,
      11,    86,    15,    41,    51,    96,    47,    45,    48,    41,
      91,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    12,    77,    42,    35,    13,    43,    18,    36,    44,
      58,    59,    60,    61,    62,    63,    93,    77,     8,    86,
      77,     9,     2,     3,    92,    95,    17,    52,    86,    77,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      34,    97,    42,    55,    86,    43,    66,    56,    54,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    67,
      14,    29,    84,    88,    30,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    94,    10,    38,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    82,    83,
      40,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    46,     0,    49,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,     0,     0,    50,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,     0,    89,    53,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
       0,     0,    57,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    68,    69,    70,     0,     0,    71,    72,
      73,    74,    75,    80,    81,    60,    61,    62,    63,     1,
       2,     3
};

static const yytype_int8 yycheck[] =
{
      32,    30,    65,    79,    33,    23,    82,    39,    29,    26,
      23,    32,    30,    45,    43,    91,    32,    46,    34,    51,
      83,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    29,    64,    25,    23,    26,    28,    33,    27,    31,
       7,     8,     9,    10,    11,    12,    29,    79,     0,    32,
      82,     3,     4,     5,    86,    29,    29,     6,    32,    91,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      30,    29,    25,    23,    32,    28,     6,    27,    31,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    29,
      23,    25,    29,    29,    28,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    29,     4,    25,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    66,    66,
      25,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    34,    -1,    25,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    -1,    -1,    25,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    81,    25,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    -1,    25,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    13,    14,    15,    -1,    -1,    18,    19,
      20,    21,    22,     7,     8,     9,    10,    11,    12,     3,
       4,     5
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,    36,    37,    38,    26,     0,     3,
      37,    23,    29,    26,    23,    30,    46,    29,    33,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    25,
      28,    42,    44,    47,    30,    23,    27,    45,    25,    44,
      25,    42,    25,    28,    31,    44,    47,    32,    34,    25,
      25,    44,     6,    25,    31,    23,    27,    25,     7,     8,
       9,    10,    11,    12,    39,    40,     6,    29,    13,    14,
      15,    18,    19,    20,    21,    22,    41,    42,    43,    46,
       7,     8,    39,    40,    29,    29,    32,    43,    29,    41,
      43,    46,    42,    29,    29,    29,    43,    29
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (&yylloc, scanner, tree, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, &yylloc, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, &yylloc, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, scanner, tree); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner, yadac_t *tree)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, tree)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    yyscan_t scanner;
    yadac_t *tree;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (tree);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, yyscan_t scanner, yadac_t *tree)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, tree)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    yyscan_t scanner;
    yadac_t *tree;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, scanner, tree);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, yyscan_t scanner, yadac_t *tree)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, scanner, tree)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    yyscan_t scanner;
    yadac_t *tree;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , scanner, tree);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, scanner, tree); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
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



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, yyscan_t scanner, yadac_t *tree)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, scanner, tree)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    yyscan_t scanner;
    yadac_t *tree;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (scanner);
  YYUSE (tree);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (yyscan_t scanner, yadac_t *tree);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (yyscan_t scanner, yadac_t *tree)
#else
int
yyparse (scanner, tree)
    yyscan_t scanner;
    yadac_t *tree;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
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
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
#line 356 "yadac.y"
    { new_inc(tree, (yyvsp[(2) - (3)].str)); }
    break;

  case 6:
#line 357 "yadac.y"
    { new_inc(tree, (yyvsp[(3) - (4)].str)); }
    break;

  case 7:
#line 361 "yadac.y"
    { binding(tree, (yyvsp[(1) - (8)].type), (yyvsp[(2) - (8)].str), (yyvsp[(4) - (8)].sql),
					 YADAC_STATUS, NULL, NULL, NULL); }
    break;

  case 8:
#line 364 "yadac.y"
    { binding(tree, (yyvsp[(1) - (9)].type), (yyvsp[(2) - (9)].str), (yyvsp[(5) - (9)].sql),
					 YADAC_STATUS, NULL, (yyvsp[(3) - (9)].strct), NULL); }
    break;

  case 9:
#line 367 "yadac.y"
    { binding(tree, (yyvsp[(1) - (9)].type), (yyvsp[(2) - (9)].str), (yyvsp[(4) - (9)].sql),
					 YADAC_SIMPLE, add_param(NULL, (yyvsp[(8) - (9)].param)),
					 NULL, NULL); }
    break;

  case 10:
#line 371 "yadac.y"
    { binding(tree, (yyvsp[(1) - (10)].type), (yyvsp[(2) - (10)].str), (yyvsp[(5) - (10)].sql),
					 YADAC_SIMPLE, add_param(NULL, (yyvsp[(9) - (10)].param)),
					 (yyvsp[(3) - (10)].strct), NULL); }
    break;

  case 11:
#line 375 "yadac.y"
    { binding(tree, (yyvsp[(1) - (9)].type), (yyvsp[(2) - (9)].str), (yyvsp[(4) - (9)].sql), (yyvsp[(7) - (9)].type), (yyvsp[(8) - (9)].params),
					 NULL, NULL); }
    break;

  case 12:
#line 378 "yadac.y"
    { binding(tree, (yyvsp[(1) - (10)].type), (yyvsp[(2) - (10)].str), (yyvsp[(5) - (10)].sql), (yyvsp[(8) - (10)].type), (yyvsp[(9) - (10)].params),
					 (yyvsp[(3) - (10)].strct), NULL); }
    break;

  case 13:
#line 383 "yadac.y"
    { binding(tree, (yyvsp[(1) - (10)].type), (yyvsp[(2) - (10)].str), (yyvsp[(4) - (10)].sql), (yyvsp[(7) - (10)].type), (yyvsp[(9) - (10)].params),
					 NULL, (yyvsp[(8) - (10)].strct)); }
    break;

  case 14:
#line 388 "yadac.y"
    { binding(tree, (yyvsp[(1) - (11)].type), (yyvsp[(2) - (11)].str), (yyvsp[(5) - (11)].sql), (yyvsp[(8) - (11)].type), (yyvsp[(10) - (11)].params),
					 (yyvsp[(3) - (11)].strct), (yyvsp[(9) - (11)].strct)); }
    break;

  case 15:
#line 393 "yadac.y"
    { (yyval.type) = YADAC_PREPARE; }
    break;

  case 16:
#line 394 "yadac.y"
    { (yyval.type) = YADAC_YPREPARE; }
    break;

  case 17:
#line 398 "yadac.y"
    { (yyval.type) = YADAC_SINGLE; }
    break;

  case 18:
#line 399 "yadac.y"
    { (yyval.type) = YADAC_MULTIROW; }
    break;

  case 19:
#line 403 "yadac.y"
    { (yyval.type) = YADAC_STRUCT; }
    break;

  case 20:
#line 404 "yadac.y"
    { (yyval.type) = YADAC_MULTISTRUCT; }
    break;

  case 39:
#line 432 "yadac.y"
    { (yyval.params) = add_param(NULL, (yyvsp[(1) - (1)].param)); }
    break;

  case 40:
#line 433 "yadac.y"
    { (yyval.params) = add_param((yyvsp[(1) - (3)].params), (yyvsp[(3) - (3)].param)); }
    break;

  case 41:
#line 437 "yadac.y"
    { (yyval.params) = add_param(NULL, (yyvsp[(1) - (1)].param)); }
    break;

  case 42:
#line 438 "yadac.y"
    { (yyval.params) = add_param((yyvsp[(1) - (2)].params), (yyvsp[(2) - (2)].param)); }
    break;

  case 43:
#line 442 "yadac.y"
    { (yyval.tlist) = add_token(NULL, (yyvsp[(1) - (1)].str)); }
    break;

  case 44:
#line 443 "yadac.y"
    { (yyval.tlist) = add_token(NULL, (yyvsp[(1) - (1)].str)); }
    break;

  case 45:
#line 444 "yadac.y"
    { (yyval.tlist) = add_token((yyvsp[(1) - (3)].tlist), (yyvsp[(3) - (3)].str)); }
    break;

  case 46:
#line 445 "yadac.y"
    { (yyval.tlist) = add_token((yyvsp[(1) - (3)].tlist), (yyvsp[(3) - (3)].str)); }
    break;

  case 47:
#line 447 "yadac.y"
    { (yyval.strct) = add_struct((yyvsp[(1) - (4)].str), (yyvsp[(3) - (4)].tlist)); }
    break;

  case 48:
#line 450 "yadac.y"
    { (yyval.sql) = add_sql(NULL, 0, (yyvsp[(1) - (1)].str), NULL); }
    break;

  case 49:
#line 451 "yadac.y"
    { (yyval.sql) = add_sql(NULL, 0, (yyvsp[(2) - (2)].str), (yyvsp[(1) - (2)].params)); }
    break;

  case 50:
#line 452 "yadac.y"
    { (yyval.sql) = add_sql(NULL, (yyvsp[(1) - (2)].num), (yyvsp[(2) - (2)].str), NULL); }
    break;

  case 51:
#line 453 "yadac.y"
    { (yyval.sql) = add_sql(NULL, (yyvsp[(1) - (3)].num), (yyvsp[(3) - (3)].str), (yyvsp[(2) - (3)].params)); }
    break;

  case 52:
#line 454 "yadac.y"
    { (yyval.sql) = add_sql((yyvsp[(1) - (2)].sql), 0, (yyvsp[(2) - (2)].str), NULL); }
    break;

  case 53:
#line 455 "yadac.y"
    { (yyval.sql) = add_sql((yyvsp[(1) - (3)].sql), 0, (yyvsp[(3) - (3)].str), (yyvsp[(2) - (3)].params)); }
    break;

  case 54:
#line 456 "yadac.y"
    { (yyval.sql) = add_sql((yyvsp[(1) - (3)].sql), (yyvsp[(2) - (3)].num), (yyvsp[(3) - (3)].str), NULL); }
    break;

  case 55:
#line 457 "yadac.y"
    { (yyval.sql) = add_sql((yyvsp[(1) - (4)].sql), (yyvsp[(2) - (4)].num), (yyvsp[(4) - (4)].str), (yyvsp[(3) - (4)].params)); }
    break;


/* Line 1267 of yacc.c.  */
#line 1998 "yadac.y.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (&yylloc, scanner, tree, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (&yylloc, scanner, tree, yymsg);
	  }
	else
	  {
	    yyerror (&yylloc, scanner, tree, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
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
		      yytoken, &yylval, &yylloc, scanner, tree);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
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
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, scanner, tree);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, scanner, tree, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, scanner, tree);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, scanner, tree);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 461 "yadac.y"

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** convert buffer to parse tree
 * @return yada bindings pointer on success
 */

yadac_t* yadac_parse(char *buf, int siz)
{
  yadac_t *tree;
  yyscan_t scanner;


  /* allocate structure */
  if(!(tree = calloc(1, sizeof(yadac_t))))
    return(NULL);

  /* initialize the scanner */
  yylex_init(&scanner);
  yy_scan_bytes(buf, siz, scanner);

  if(yyparse(scanner, tree))
    {
    yylex_destroy(scanner);
    yadac_free(tree);
    return(NULL);
    }

  yylex_destroy(scanner);
  return(tree);
}

/******************************************************************************/
/** free yada bindings
 */

void yadac_free(yadac_t *tree)
{
  int loop;


  if(!tree)
    return;

  /* free each binding */
  for(loop=0; loop<tree->binds; loop++)
    {
    free_sql(tree->bind[loop].line);
    free(tree->bind[loop].iparam);
    free(tree->bind[loop].oparam);
    free(tree->bind[loop].name);
    }

  free(tree->bind);
  free(tree);
}

/******************************************************************************
 ******************************************************************************/


