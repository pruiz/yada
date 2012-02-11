
/******************************************************************************
 ******************************************************************************/

#ifndef ___YADA_H__
#define ___YADA_H__

/******************************************************************************
 * L I C E N S E **************************************************************
 ******************************************************************************/

/*
 * Copyright (c) 2003, 2004 dev/IT - http://www.devit.com
 *
 * This file is part of yada.
 *
 * Yada is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Yada is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with yada; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/******************************************************************************
 * I N C L U D E S ************************************************************
 ******************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ltdl.h>

#define YADA_INTERNAL
#include "yada.h"

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

/******************************************************************************/
/** code defines
 */

#define STRLEN_INT     11
#define STRLEN_INT64   21

/******************************************************************************/
/** debug macros
 */

#ifdef DEBUGMODE
#  define DEBUGMSG(m...) { fprintf(stderr, m); fprintf(stderr, "\n"); }
#  ifdef DEBUGCOLOR
#    define DBG_RED(a) "\033[01;31m" a "\033[00m"
#    define DBG_GREEN(a) "\033[01;32m" a "\033[00m"
#    define DBG_YELLOW(a) "\033[01;33m" a "\033[00m"
#    define DBG_BLUE(a) "\033[01;34m" a "\033[00m"
#    define DBG_MAGENTA(a) "\033[01;35m" a "\033[00m"
#    define DBG_CYAN(a) "\033[01;36m" a "\033[00m"
#  else
#    define DBG_RED(a) a
#    define DBG_GREEN(a) a
#    define DBG_YELLOW(a) a
#    define DBG_BLUE(a) a
#    define DBG_MAGENTA(a) a
#    define DBG_CYAN(a) a
#  endif
#else
#  define DEBUGMSG(...)
#endif

/******************************************************************************/
/** macro to set arbitrary error info to the yada struct
 * @param y yada struct pointer
 * @param e error code (int)
 * @param m error string (char *, null if none)
 */

#define _yada_set_err(y, e, m) \
  { \
  y->error = e; \
  strncpy(y->errmsg, m, sizeof(y->_priv->errbuf) - 1); \
  }

/******************************************************************************/
/** macro to set yada-specific error info to the yada struct
 * @param y yada struct pointer
 * @param e error code (int)
 */

#define _yada_set_yadaerr(y, e)  _yada_set_err(y, e, _yada_errstrs[e])

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

struct yada_rc_t
{
  char magic;
  unsigned int t;
  unsigned int len;
  void *data;
  yada_rc_t *prev, *next;
};

/******************************************************************************/
/* prepare structs */

typedef struct
{
  /** type of element, 0 for static string, (char) type of arg */
  int t;
  /** pointer to the static string */
  char *buf;
  /** len of static string */
  int len;
} prep_ele_t;

typedef struct
{
  /** number of elements allocated */
  int sz;
  /** number of elements used */
  int eles;
  /** total length of all static strings */
  int len;
  /** number of arguments */
  int args;
  /** element array */
  prep_ele_t ele[1];
} yada_prep_t;

/******************************************************************************/
/* bindset structs */

typedef struct
{
  /** type of element */
  int t;
  /** pointer to the storage space */
  void *ptr;
  /** indicator variable */
  int ind;
  /** tmp var if needed (oracle) */
  void *tmp;
  /** vars for yada stored variables */
  union
    {
    int i;
    long long l;
    unsigned char *buf;
    double f;
    } var;
} bindset_ele_t;

typedef struct
{
  /** number of elements allocated */
  int sz;
  /** number of elements used */
  int eles;
  /** element array */
  bindset_ele_t ele[1];
} yada_bindset_t;

/******************************************************************************/
/* main yada struct */

typedef void (*yada_rc_free_t)(yada_t *, yada_rc_t *);

/* main private struct */
struct yada_priv_t
{
  void (*destroy)(yada_t *);
  int (*exec)(yada_t *, char *, int);
  yada_rc_t* (*query)(yada_t *, char *, int);
  unsigned int flags;
  lt_dlhandle dlh;
  yada_rc_t *rc_head;
  yada_rc_t *rc_tail;
  yada_rc_free_t free_rc[10];
  char errbuf[1024];
  int (*execprep)(yada_t *, void *, int *, va_list);
  yada_rc_t* (*queryprep)(yada_t *, void *, int *, va_list);
};

/******************************************************************************
 * G L O B A L S **************************************************************
 ******************************************************************************/

extern const char *_yada_errstrs[];

/******************************************************************************
 * P R O T O T Y P E S ********************************************************
 ******************************************************************************/

typedef int (*yada_mod_init_t)(yada_t *);

yada_rc_t* _yada_rc_new(yada_t *);
void _yada_rc_destroy(yada_t *, yada_rc_t *);
void _yada_destroy(yada_t *);

void *_yada_return_null(yada_t *, ...);
int _yada_return_zero(yada_t *, ...);
void _yada_return(yada_t *, ...);

#ifdef DEBUGMODE
  int errmsg(char *, ...);
#endif

#ifndef HAVE_STRNDUP
  char* _yada_strndup(const char *, size_t);
#endif

/******************************************************************************/

#endif /* end ___YADA_H__ */

/******************************************************************************
 ******************************************************************************/

