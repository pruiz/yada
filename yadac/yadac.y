
/******************************************************************************
 ******************************************************************************/

/** \file yadac.y
 *  yada binding parser
 *
 * $Id$
 */

/******************************************************************************/
%{
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
%}
/******************************************************************************
 * D E C L A R A T I O N S ****************************************************
 ******************************************************************************/

%union
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

%locations
%pure-parser
%parse-param {yyscan_t scanner}
%parse-param {yadac_t *tree}
%lex-param {yyscan_t scanner}

%token INCLUDE
%token PREPARE YPREPARE RESULT
%token STATUS SIMPLE SINGLE STRUCT MULTIROW MULTISTRUCT
%token <param> INT FLOAT LONG BINARY VARCHAR
%token <param> INTP FLOATP LONGP BINARYP VARCHARP
%token <str> TOKEN PARAM SQL STRING CVAR
%token <num> INDENT

%type <type> ptype rtype rstype
%type <param> param simple
%type <params> iparams oparams
%type <tlist> tlist
%type <strct> strct
%type <sql> sql

/******************************************************************************/
%%
/******************************************************************************
 * R U L E S ******************************************************************
 ******************************************************************************/

all	: /* empty */
	| binding
	| all binding
	| INCLUDE STRING ';'            { new_inc(tree, $2); }
	| all INCLUDE STRING ';'        { new_inc(tree, $3); }
;

binding	: ptype TOKEN '[' sql ']'
	   RESULT STATUS ';'		{ binding(tree, $1, $2, $4,
					 YADAC_STATUS, NULL, NULL, NULL); }
	| ptype TOKEN strct '[' sql ']'
	  RESULT STATUS ';'		{ binding(tree, $1, $2, $5,
					 YADAC_STATUS, NULL, $3, NULL); }
	| ptype TOKEN '[' sql ']'
	   RESULT SIMPLE simple ';'	{ binding(tree, $1, $2, $4,
					 YADAC_SIMPLE, add_param(NULL, $8),
					 NULL, NULL); }
	| ptype TOKEN strct '[' sql ']'
	   RESULT SIMPLE simple ';'	{ binding(tree, $1, $2, $5,
					 YADAC_SIMPLE, add_param(NULL, $9),
					 $3, NULL); }
	| ptype TOKEN '[' sql ']'
	   RESULT rtype oparams ';'	{ binding(tree, $1, $2, $4, $7, $8,
					 NULL, NULL); }
	| ptype TOKEN strct '[' sql ']'
	   RESULT rtype oparams ';'	{ binding(tree, $1, $2, $5, $8, $9,
					 $3, NULL); }
	| ptype TOKEN '[' sql ']'
	   RESULT rstype
	   strct
	   oparams ';'			{ binding(tree, $1, $2, $4, $7, $9,
					 NULL, $8); }
	| ptype TOKEN strct '[' sql ']'
	   RESULT rstype
	   strct
	   oparams ';'			{ binding(tree, $1, $2, $5, $8, $10,
					 $3, $9); }
;

/* query type */
ptype	: PREPARE			{ $$ = YADAC_PREPARE; }
	| YPREPARE			{ $$ = YADAC_YPREPARE; }
;

/* result type */
rtype	: SINGLE			{ $$ = YADAC_SINGLE; }
	| MULTIROW			{ $$ = YADAC_MULTIROW; }
;

/* result structure type */
rstype	: STRUCT			{ $$ = YADAC_STRUCT; }
	| MULTISTRUCT			{ $$ = YADAC_MULTISTRUCT; }
;

/* simple params */
simple	: INT
	| INTP
	| FLOAT
	| FLOATP
	| LONG
	| LONGP
	| BINARYP
	| VARCHARP
;

/* parameter */
param	: INT
	| INTP
	| FLOAT
	| FLOATP
	| LONG
	| LONGP
	| BINARY
	| BINARYP
	| VARCHAR
	| VARCHARP
;

/* output parameter list */
oparams	: param				{ $$ = add_param(NULL, $1); }
	| oparams ',' param		{ $$ = add_param($1, $3); }
;

/* input parameters */
iparams	: param				{ $$ = add_param(NULL, $1); }
	| iparams param			{ $$ = add_param($1, $2); }
;

/* token list */
tlist	: CVAR				{ $$ = add_token(NULL, $1); }
	| TOKEN				{ $$ = add_token(NULL, $1); }
	| tlist ',' CVAR		{ $$ = add_token($1, $3); }
	| tlist ',' TOKEN		{ $$ = add_token($1, $3); }

strct	: TOKEN '(' tlist ')'		{ $$ = add_struct($1, $3); }

/* SQL section */
sql	: SQL				{ $$ = add_sql(NULL, 0, $1, NULL); }
	| iparams SQL			{ $$ = add_sql(NULL, 0, $2, $1); }
	| INDENT SQL			{ $$ = add_sql(NULL, $1, $2, NULL); }
	| INDENT iparams SQL		{ $$ = add_sql(NULL, $1, $3, $2); }
	| sql SQL			{ $$ = add_sql($1, 0, $2, NULL); }
	| sql iparams SQL		{ $$ = add_sql($1, 0, $3, $2); }
	| sql INDENT SQL		{ $$ = add_sql($1, $2, $3, NULL); }
	| sql INDENT iparams SQL	{ $$ = add_sql($1, $2, $4, $3); }
;

/******************************************************************************/
%%
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

