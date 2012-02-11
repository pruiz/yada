
/******************************************************************************
 ******************************************************************************/

/** \file yada.h
 * yada header file
 *
 * $Id: yada.h 191 2007-11-21 00:13:09Z grizz $
 */

#ifndef __YADA_H__
#define __YADA_H__

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

#include <stdint.h>
#include <stdarg.h>

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

#undef BEGIN_C_DECLS
#undef END_C_DECLS
#ifdef __cplusplus
# define BEGIN_C_DECLS extern "C" {
# define END_C_DECLS }
#else
# define BEGIN_C_DECLS
# define END_C_DECLS
#endif

BEGIN_C_DECLS


/******************************************************************************/
/** version info
 */

#define YADA_TO_STRING_(s)            #s
#define YADA_TO_STRING(s)             TO_STRING_(s)

#define YADAV_MAJ 1
#define YADAV_MIN 0
#define YADAV_BUG 0

#define YADA_VERSION_STR  YADA_TO_STRING(YADAV_MAJ.YADAV_MIN.YADAV_BUG)
#define YADA_VERSION_CODE YADA_VERSION(YADAV_MAJ, YADAV_MIN, YADAV_BUG)

#define YADA_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

/******************************************************************************/
/** rc types
 */

#define YADA_STATEMENT    0x01
#define YADA_RESULT       0x02
#define YADA_BINDSET      0x04
#define YADA_PREPARED     0x08

/******************************************************************************/
/** errors
 */

#define YADA_ESUCCESS    0x00
#define YADA_ECANTCONN   0x01
#define YADA_ENOMEM      0x02
#define YADA_EINTRX      0x03
#define YADA_EINVAL      0x04
#define YADA_ENOTCONN    0x05

#define YADA_ERRORS      0x06

/******************************************************************************/
/** db types
 */

#define YADA_MYSQL       0x01
#define YADA_SQLITE3     0x02
#define YADA_PGSQL       0x03
#define YADA_ORACLE      0x04

/******************************************************************************/
/** command flags
 */

#define YADA_FORMAT      0x01
#define YADA_NATIVE      0x02

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

typedef struct yada_rc_t yada_rc_t;
typedef struct yada_priv_t yada_priv_t;
typedef struct yada_modpriv_t yada_modpriv_t;
typedef struct yada_t yada_t;

/******************************************************************************/
/** yada struct
 */

struct yada_t
{ 
  /* private internal */
  yada_priv_t *_priv;
  yada_modpriv_t *_mod;

  /* public */
  int type_id;
  char *dbtype;
  char *dbstr;
  char* (*escstr)(char *, int, char *, int *);
  int (*connect)(yada_t *, char *, char *);
  void (*disconnect)(yada_t *);
  yada_rc_t* (*prepare)(yada_t *, char *, int);
  yada_rc_t* (*preparef)(yada_t *, char *, ...);
  yada_rc_t* (*yprepare)(yada_t *, char *, int);
  yada_rc_t* (*ypreparef)(yada_t *, char *, ...);
  yada_rc_t* (*xprepare)(yada_t *, int, char *, ...);
  int (*execute)(yada_t *, void *, ...);
  int (*xexecute)(yada_t *, int, void *, ...);
  yada_rc_t* (*query)(yada_t *, void *, ...);
  yada_rc_t* (*xquery)(yada_t *, int, void *, ...);
  char* (*dumpexec)(yada_t *, int *, yada_rc_t *, ...);
  yada_rc_t* (*bind)(yada_t *, char *, ...);
  int (*fetch)(yada_t *, yada_rc_t *, yada_rc_t *);
  int (*trx)(yada_t *, int);
  int (*commit)(yada_t *);
  int (*rollback)(yada_t *, int);
  void (*free)(yada_t *, yada_rc_t *);
  void (*freeall)(yada_t *, int);
  void (*destroy)(yada_t *);
  int error;
  char *errmsg;
  uint64_t (*insert_id)(yada_t *, char *table, char *col);
  int (*vexecute)(yada_t *, void *, va_list ap);
  yada_rc_t* (*vquery)(yada_t *, void *, va_list ap);
  yada_rc_t* (*vbind)(yada_t *, char *, va_list ap);
};

/******************************************************************************
 * G L O B A L S **************************************************************
 ******************************************************************************/

/******************************************************************************
 * P R O T O T Y P E S ********************************************************
 ******************************************************************************/

yada_t *yada_init(const char *, unsigned int);

/******************************************************************************/
END_C_DECLS

#endif /* end __YADA_H__ */

/******************************************************************************
 ******************************************************************************/

