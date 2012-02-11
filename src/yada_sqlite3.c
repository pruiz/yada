
/******************************************************************************
 ******************************************************************************/

/** \file yada_sqlite.c
 * yada sqlite3 module
 *
 * $Id: yada_sqlite3.c 182 2007-09-28 15:28:41Z grizz $
 */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "_yada.h"
#include "common.h"
#include <sqlite3.h>

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

struct yada_modpriv_t
{
  struct sqlite3 *db;
};

/******************************************************************************
 * G L O B A L S **************************************************************
 ******************************************************************************/

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** connect to sqlite database
 * @param dbstr filename:mode:databasename (defaults to writable)
 * @param yada_user username for db access
 * @param yada_pass password for db access
 */

static int yada_sqlite3_connect(yada_t *_yada, char *yada_user, char *yada_pass)
{
  if(sqlite3_open(_yada->dbstr, &_yada->_mod->db))
    {
    _yada_set_err(_yada, sqlite3_errcode(_yada->_mod->db),
     sqlite3_errmsg(_yada->_mod->db));
    return(0);
    }
  return(1);
}

/******************************************************************************/
/** disconnect from sqlite database
 */

static void yada_sqlite3_disconnect(yada_t *_yada)
{
  if(!_yada->_mod->db)
    return;

  sqlite3_close(_yada->_mod->db);
  _yada->_mod->db = 0;
}

/******************************************************************************/

char *yada_sqlite3_escstr(char *src, int slen, char *dest, int *dlen)
{
  int i, len;
  char *destp, *tmp_ptr;


  if(!slen)
    slen = strlen(src);

  /* no dest specified, alloc for it */
  if(!dest)
    {
    /* alloc for dest */
    if(!(dest = malloc((slen << 1) + 1)))
      return(0);

    destp = dest;

    for(i = 0; i < slen; i++)
      if((*destp++ = src[i]) == '\'')
        *destp++ = '\'';

    *destp++ = 0;

    if(!dlen)
      dlen = &len;

    /* alloc down to only the used bit */
    if((tmp_ptr = realloc(dest, (*dlen = destp - dest))))
      dest = tmp_ptr;

    (*dlen)--;

    return(dest);
    }

  destp = dest;

  for(i = 0; i < slen; i++)
    if((*destp++ = src[i]) == '\'')
      *destp++ = '\'';

  *destp++ = 0;

  if(dlen)
    *dlen = destp - dest - 1;

  return(dest);
}

/******************************************************************************/

static int yada_sqlite3__exec(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  if(sqlite3_exec(_yada->_mod->db, sqlstr, 0, 0, 0))
    {
    _yada_set_err(_yada, sqlite3_errcode(_yada->_mod->db),
     sqlite3_errmsg(_yada->_mod->db));
    return(-1);
    }
  return(sqlite3_changes(_yada->_mod->db));
}

/******************************************************************************/

static yada_rc_t* yada_sqlite3__query(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  sqlite3_stmt *sql_stmt;
  yada_rc_t *_yrc;


  if(sqlite3_prepare(_yada->_mod->db, sqlstr, sqlstr_len, &sql_stmt, 0))
    {
    _yada_set_err(_yada, sqlite3_errcode(_yada->_mod->db),
     sqlite3_errmsg(_yada->_mod->db));
    return(0);
    }

  switch(sqlite3_step(sql_stmt))
    {
  case SQLITE_ROW:
    /* reset so fetch() can step results */
    sqlite3_reset(sql_stmt);
    break;
  case SQLITE_DONE:
    sqlite3_reset(sql_stmt);
    break;
  default:
    /* FIXME - SQLITE_MISUSE and SQLITE_BUSY don't set errcode */
    _yada_set_err(_yada, sqlite3_errcode(_yada->_mod->db), sqlite3_errmsg(_yada->_mod->db));
    sqlite3_finalize(sql_stmt);
    return(0);
    break;
    }

  if(!(_yrc = _yada_rc_new(_yada)))
    return(0);

  _yrc->t = YADA_RESULT;
  _yrc->data = sql_stmt;
  return(_yrc);
}

/******************************************************************************/
/** fetch result row into bound vars
 * @param yada yada struct
 * @param rrc result rc
 * @param brc bound vars rc
 */

int yada_sqlite3_fetch(yada_t *_yada, yada_rc_t *rrc, yada_rc_t *brc)
{
  int i, di = 0;
  int len;
  const void *ptr;
  yada_bindset_t *bindset;
  sqlite3_stmt *res;


  res = rrc->data;
  bindset = brc->data;

  /* step for row */
  switch(sqlite3_step(res))
    {
  case SQLITE_ROW:
    break;

  /* FIXME - SQLITE_MISUSE and SQLITE_BUSY don't set errcode */
  case SQLITE_ERROR:
    _yada_set_err(_yada, sqlite3_errcode(_yada->_mod->db),
     sqlite3_errmsg(_yada->_mod->db));
    return(0);
  default:
  case SQLITE_BUSY:
  case SQLITE_MISUSE:
  case SQLITE_DONE:
    /* FIXME - SQLITE_MISUSE and SQLITE_BUSY don't set errcode */
    _yada->error = 0;
    return(0);
    break;
    }

  /* populate all returned columns */
  for(i = 0; i < bindset->eles; i++, di++)
    {
    /************************************************/
    /* bound variables */
    if(bindset->ele[i].t > 0)
      {

      /* check for null */
      if(!(len = sqlite3_column_bytes(res, di)))
        {
        *(char *)bindset->ele[i].ptr = 0;

        if(bindset->ele[i].t == 'b')
          i++;
        continue;
        }

      switch(bindset->ele[i].t)
        {
      case 'b':
        ptr = sqlite3_column_blob(res, di);
        memcpy(bindset->ele[i].ptr, ptr, len);
        /* set len var */
        *(unsigned long *)bindset->ele[++i].ptr = len;
        break;
      case 'd':
        *(int *)bindset->ele[i].ptr = sqlite3_column_int(res, di);
        break;
      case 'l':
        *(long long *)bindset->ele[i].ptr = sqlite3_column_int64(res, di);
        break;
      case 's':
      case 'e':
      case 'v':
        ptr = sqlite3_column_blob(res, di);
        /* +1 for terminating null */
        memcpy(bindset->ele[i].ptr, ptr, len + 1);
        break;
      case 'f':
        *(double *)bindset->ele[i].ptr = sqlite3_column_double(res, di);
        break;
        }
      continue;
      }

    /************************************************/
    /* bound pointers */

    /* check for null */
    if(!(len = sqlite3_column_bytes(res, di)))
      {
      *(char **)bindset->ele[i].ptr = 0;
      if(bindset->ele[i].t == -'b')
        i++;
      continue;
      }

    switch(-bindset->ele[i].t)
      {
    case 'd':
      bindset->ele[i].var.i = sqlite3_column_int(res, di);
      *(int **)bindset->ele[i].ptr = &bindset->ele[i].var.i;
      break;
    case 'l':
      bindset->ele[i].var.l = sqlite3_column_int64(res, di);
      *(long long **)bindset->ele[i].ptr = &bindset->ele[i].var.l;
      break;
    case 's':
    case 'e':
    case 'v':
      *(const void**)bindset->ele[i].ptr = sqlite3_column_blob(res, di);
      break;
    case 'b':
      *(const void**)bindset->ele[i].ptr = sqlite3_column_blob(res, di);
      /* set len var */
      *(unsigned long *)bindset->ele[++i].ptr = len;
      break;
    case 'f':
      bindset->ele[i].var.f = sqlite3_column_double(res, di);
      *(double **)bindset->ele[i].ptr = &bindset->ele[i].var.f;
      break;
      }
    } /* for(eles) */

  return(1);
}

/******************************************************************************/

void yada_sqlite3_free_result(yada_t *_yada, yada_rc_t *_yrc)
{
  sqlite3_finalize(_yrc->data);
}

/******************************************************************************/

void yada_sqlite3__destroy(yada_t *_yada)
{
  yada_sqlite3_disconnect(_yada);
  free(_yada->_mod);
}

/******************************************************************************/
/** starts a transaction
 */

static int yada_sqlite3_trx(yada_t *_yada, int flags)
{
  return(yada_sqlite3__exec(_yada, "BEGIN", 0) == -1 ? 1 : 0);
}

/******************************************************************************/
/** commits a transaction
 */

static int yada_sqlite3_commit(yada_t *_yada)
{
  return(yada_sqlite3__exec(_yada, "COMMIT", 0) == -1 ? 1 : 0);
}

/******************************************************************************/
/** aborts a transaction
 */

static int yada_sqlite3_rollback(yada_t *_yada, int flags)
{
  return(yada_sqlite3__exec(_yada, "ROLLBACK", 0) == -1 ? 1 : 0);
}

/******************************************************************************/
/** get the last insert id
 * @returns last insert id, or 0 on failure
 */

static uint64_t yada_sqlite3_insert_id(yada_t *_yada, char *table, char *col)
{
  return(sqlite3_last_insert_rowid(_yada->_mod->db));
}

/******************************************************************************/
/** create and init yada struct
 * @returns non zero on success, 0 on error
 */

int yada_mod_init(yada_t *_yada)
{
  if(!(_yada->_mod = calloc(1, sizeof(yada_t))))
    return(0);

  /* yada module base functions */
  _yada->type_id = YADA_SQLITE3;
  _yada->connect = yada_sqlite3_connect;
  _yada->disconnect = yada_sqlite3_disconnect;

  /* native prepare funtions */
  _yada->prepare = _yada_prepare;
  _yada->preparef = _yada_preparef;

  /* yada compat prepare funtions */
  _yada->yprepare = _yada_prepare;
  _yada->ypreparef = _yada_preparef;

  _yada->xprepare = _yada_xprepare;
  
  _yada->execute = _yada_execute;
  _yada->xexecute = _yada_xexecute;
  
  _yada->query = _yada_query;
  _yada->xquery = _yada_xquery;

  /* yada util functions */
  _yada->escstr = yada_sqlite3_escstr;
  _yada->dumpexec = _yada_dumpexec;

  /* yada compat bind functions */
  _yada->bind = _yada_bind;
  _yada->fetch = yada_sqlite3_fetch;

  /* var args functions */
  _yada->vquery = _yada_vquery;
  _yada->vbind = _yada_vbind;
  _yada->vexecute = _yada_vexecute;

  /* transaction functions */
  _yada->trx = yada_sqlite3_trx;
  _yada->commit = yada_sqlite3_commit;
  _yada->rollback = yada_sqlite3_rollback;

  _yada->insert_id = yada_sqlite3_insert_id;

  /* private interfaces */
  _yada->_priv->exec = yada_sqlite3__exec;
  _yada->_priv->query = yada_sqlite3__query;
  _yada->_priv->destroy = yada_sqlite3__destroy;

  _yada->_priv->free_rc[YADA_STATEMENT] = _yada_free_stmt;
  _yada->_priv->free_rc[YADA_RESULT] = yada_sqlite3_free_result;
  _yada->_priv->free_rc[YADA_BINDSET] = yada_free_bindset;

  return(1);
}

/******************************************************************************
 ******************************************************************************/

