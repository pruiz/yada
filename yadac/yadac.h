
/******************************************************************************
 ******************************************************************************/

/** \file yadac.h
 *  yada binding compiler
 *
 * $Id: yadac.h 177 2007-07-05 16:48:27Z grizz $
 */

#ifndef __YADAC_H__
#define __YADAC_H__

/******************************************************************************
 * I N C L U D E S ************************************************************
 ******************************************************************************/

#include <stdlib.h>

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

#define YY_EXTRA_TYPE		int*

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

/* prepare types */
typedef enum
{
  YADAC_PREPARE,
  YADAC_YPREPARE
} yadac_ptype_t;

/* result types */
typedef enum
{
  YADAC_STATUS,
  YADAC_SIMPLE,
  YADAC_SINGLE,
  YADAC_STRUCT,
  YADAC_MULTIROW,
  YADAC_MULTISTRUCT
} yadac_rtype_t;

/* parameter types */
typedef enum
{
  YADAC_INT = 1,
  YADAC_FLOAT,
  YADAC_LONG,
  YADAC_BINARY,
  YADAC_STRING,
  YADAC_INTP,
  YADAC_FLOATP,
  YADAC_LONGP,
  YADAC_BINARYP,
  YADAC_STRINGP
} yadac_param_t;

/* token list */
typedef struct
{
  int tokens;
  char *token[1];
} yadac_tlist_t;

typedef struct
{
  char *type;
  yadac_tlist_t *list;
} yadac_struct_t;

/* SQL line */
typedef struct
{
  int indent;
  char *text;
} yadac_line_t;

/* SQL chunk */
typedef struct
{ 
  int params;
  yadac_param_t *param;
  int lines;
  yadac_line_t *line;
} yadac_sql_t;

/* query binding */
typedef struct
{
  yadac_ptype_t ptype;
  yadac_rtype_t rtype;
  char *name;
  char *upper;
  int lines;
  yadac_line_t *line;
  int iparams;
  yadac_param_t *iparam;
  int oparams;
  yadac_param_t *oparam;
  yadac_struct_t *istruct;
  yadac_struct_t *ostruct;
} yadac_binding_t;

/* parse tree */
typedef struct
{
  int binds;
  yadac_binding_t *bind;
  int incs;
  char **inc;
} yadac_t;

/******************************************************************************
 * P R O T O T Y P E S ********************************************************
 ******************************************************************************/

yadac_t* yadac_parse(char *buf, int siz);
void yadac_free(yadac_t *tree);

/******************************************************************************/

#endif /* end __YADAC_H__ */

/******************************************************************************
 ******************************************************************************/



