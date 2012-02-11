
/******************************************************************************
 ******************************************************************************/

/** \file yada_mysql.c
 * yada mysql module
 *
 * $Id: yada_mysql.c 182 2007-09-28 15:28:41Z grizz $
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
#include <mysql.h>
#include <errmsg.h>

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

struct yada_modpriv_t
{
  MYSQL rdb;
  MYSQL *db;
};

/******************************************************************************
 * G L O B A L S **************************************************************
 ******************************************************************************/

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** connect to mysql database
 * @param dbstr hostname:port:databasename
 * @param yada_user username for db access
 * @param yada_pass password for db access
 */

static int yada_mysql_connect(yada_t *_yada, char *yada_user, char *yada_pass)
{
  int i, port = 0;
  char *fsep;
  char *sock = 0;
  char *dbargs[] = {0, 0, 0};


  fsep = dbargs[0] = strdup(_yada->dbstr);
  for(i = 1; i < 3 && (fsep = strchr(fsep, ':')); i++)
    {
    *fsep++ = 0;
    dbargs[i] = fsep;
    }

  if(dbargs[1] && !(port = atoi(dbargs[1])) && strlen(dbargs[1]))
    sock = dbargs[1];

  _yada->_mod->db = &_yada->_mod->rdb;
  mysql_init(_yada->_mod->db);
  if(!(mysql_real_connect(_yada->_mod->db, dbargs[0], yada_user,
                           yada_pass, dbargs[2], port, sock, 0)))
    {
    _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
    free(dbargs[0]);
    return(0);
    }

  free(dbargs[0]);
  return(1);
}

/******************************************************************************/
/** disconnect from database
 */

static void yada_mysql_disconnect(yada_t *_yada)
{
  if(!_yada->_mod->db)
    return;

  mysql_close(_yada->_mod->db);
  _yada->_mod->db = 0;
}

/******************************************************************************/

static int yada_mysql__exec(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  int rv;


  if(!sqlstr_len)
    sqlstr_len = strlen(sqlstr);

  if((rv = mysql_real_query(_yada->_mod->db, sqlstr, sqlstr_len)))
    {
    /* return if not a connection error */
    if(rv != CR_SERVER_GONE_ERROR && rv != CR_SERVER_LOST)
      {
      _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
      return(-1);
      }

    /* ping and retry */
    mysql_ping(_yada->_mod->db);

    if(mysql_real_query(_yada->_mod->db, sqlstr, sqlstr_len))
      {
      _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
      return(-1);
      }
    }

  return(mysql_affected_rows(_yada->_mod->db));
}

/******************************************************************************/

static yada_rc_t* yada_mysql__query(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  int rv;
  yada_rc_t *_yrc;
  MYSQL_RES *res;


  if(!sqlstr_len)
    sqlstr_len = strlen(sqlstr);

  if((rv = mysql_real_query(_yada->_mod->db, sqlstr, sqlstr_len)))
    {
    /* return if not a connection error */
    if(rv != CR_SERVER_GONE_ERROR && rv != CR_SERVER_LOST)
      {
      _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
      return(0);
      }

    /* ping and retry */
    mysql_ping(_yada->_mod->db);

    if(mysql_real_query(_yada->_mod->db, sqlstr, sqlstr_len))
      {
      _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
      return(0);
      }
    }

  if(!(res = mysql_store_result(_yada->_mod->db)))
    {

    /* no results */
    if(!mysql_field_count(_yada->_mod->db))
      {
      if(!(_yrc = _yada_rc_new(_yada)))
        {
        _yada_set_yadaerr(_yada, YADA_ENOMEM);
        return(0);
        }

      _yrc->t = YADA_RESULT;
      _yrc->data = 0;
      return(_yrc);
      }

    _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
    return(0);
  }

  /* return results */
  if(!(_yrc = _yada_rc_new(_yada)))
    {
    mysql_free_result(res);
    return(0);
    }

  _yrc->t = YADA_RESULT;
  _yrc->data = res;
  return(_yrc);
}

/******************************************************************************/

static void yada_mysql_free_result(yada_t *_yada, yada_rc_t *_yrc)
{
  mysql_free_result(_yrc->data);
}

/******************************************************************************/

static void yada_mysql__destroy(yada_t *_yada)
{
  yada_mysql_disconnect(_yada);
  free(_yada->_mod);
}

/******************************************************************************/
/** fetch result row into bound vars
 * @param yada yada struct
 * @param rrc result rc
 * @param brc bound vars rc
 */

int yada_mysql_fetch(yada_t *_yada, yada_rc_t *rrc, yada_rc_t *brc)
{
  int i, di = 0;
  unsigned long *rlen;
  char **rrow;
  yada_bindset_t *bindset;


  if(!(rrow = mysql_fetch_row(rrc->data)))
    {
    _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
    return(0);
    }

  if(!(rlen = mysql_fetch_lengths(rrc->data)))
    {
    _yada_set_err(_yada, mysql_errno(_yada->_mod->db), mysql_error(_yada->_mod->db));
    return(0);
    }

  bindset = brc->data;

  for(i = 0; i < bindset->eles; i++, di++)
    {

    /************************************************/
    /* bound variables */
    if(bindset->ele[i].t > 0)
      {
      if(!rrow[di])
        {
        *(char *)bindset->ele[i].ptr = 0;

        if(bindset->ele[i].t == 'b')
          i++;
        continue;
        }

      switch(bindset->ele[i].t)
        {
      case 'b':
        memcpy(bindset->ele[i].ptr, rrow[di], rlen[di]);
        /* set len var */
        *(unsigned long *)bindset->ele[++i].ptr = rlen[di];
        break;
      case 'd':
        *(int *)bindset->ele[i].ptr = atoi(rrow[di]);
        break;
      case 'l':
        *(long long *)bindset->ele[i].ptr = atoll(rrow[di]);
        break;
      case 's':
      case 'e':
      case 'v':
        /* +1 for terminating null */
        memcpy(bindset->ele[i].ptr, rrow[di], rlen[di] + 1);
        break;
      case 'f':
        *(double *)bindset->ele[i].ptr = atof(rrow[di]);
        break;
        }
      continue;
      }

    /************************************************/
    /* bound pointers */
    if(!rrow[di])
      {
      *(char **)bindset->ele[i].ptr = 0;

      if(bindset->ele[i].t == -'b')
        i++;
      continue;
      }

    switch(-bindset->ele[i].t)
      {
    case 'd':
      bindset->ele[i].var.i = atoi(rrow[di]);
      *(int **)bindset->ele[i].ptr = &(bindset->ele[i].var.i);
      break;
    case 'l':
      bindset->ele[i].var.l = atoll(rrow[di]);
      *(long long **)bindset->ele[i].ptr = &(bindset->ele[i].var.l);
      break;
    case 's':
    case 'e':
    case 'v':
      *(char **)bindset->ele[i].ptr = rrow[di];
      break;
    case 'b':
      *(char **)bindset->ele[i].ptr = rrow[di];
      /* set len var */
      *(unsigned long *)bindset->ele[++i].ptr = rlen[di];
      break;
    case 'f':
      bindset->ele[i].var.f = atof(rrow[di]);
      *(double **)bindset->ele[i].ptr = &(bindset->ele[i].var.f);
      break;
      }
    } /* for(eles) */

  return(1);
}

/******************************************************************************/
/** starts a transaction
 */

static int yada_mysql_trx(yada_t *_yada, int flags)
{
  return(yada_mysql__exec(_yada, "START TRANSACTION", 17) == -1 ? 1 : 0);
}

/******************************************************************************/
/** commits a transaction
 */

static int yada_mysql_commit(yada_t *_yada)
{
  return(yada_mysql__exec(_yada, "COMMIT", 6) == -1 ? 1 : 0);
}

/******************************************************************************/
/** aborts a transaction
 */

static int yada_mysql_rollback(yada_t *_yada, int flags)
{
  return(yada_mysql__exec(_yada, "ROLLBACK", 8) == -1 ? 1 : 0);
}

/******************************************************************************/
/** get the last insert id
 * @returns last insert id, or 0 on failure
 */

static uint64_t yada_mysql_insert_id(yada_t *_yada, char *table, char *col)
{
  return(mysql_insert_id(_yada->_mod->db));
}

/******************************************************************************/
/** create and init yada struct
 * @returns non zero on success, 0 on error
 */

int yada_mod_init(yada_t *_yada)
{
  if(!(_yada->_mod = calloc(1, sizeof(yada_modpriv_t))))
    return(0);

  /* yada module base functions */
  _yada->type_id = YADA_MYSQL;
  _yada->connect = yada_mysql_connect;
  _yada->disconnect = yada_mysql_disconnect;

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
  _yada->escstr = _yada_escstr;
  _yada->dumpexec = _yada_dumpexec;

  /* yada compat bind functions */
  _yada->bind = _yada_bind;
  _yada->fetch = yada_mysql_fetch;

  /* var args functions */
  _yada->vquery = _yada_vquery;
  _yada->vbind = _yada_vbind;
  _yada->vexecute = _yada_vexecute;

  /* transaction functions */
  _yada->trx = yada_mysql_trx;
  _yada->commit = yada_mysql_commit;
  _yada->rollback = yada_mysql_rollback;

  _yada->insert_id = yada_mysql_insert_id;

  /* private interfaces */
  _yada->_priv->exec = yada_mysql__exec;
  _yada->_priv->query = yada_mysql__query;
  _yada->_priv->destroy = yada_mysql__destroy;

  _yada->_priv->free_rc[YADA_STATEMENT] = _yada_free_stmt;
  _yada->_priv->free_rc[YADA_RESULT] = yada_mysql_free_result;
  _yada->_priv->free_rc[YADA_BINDSET] = yada_free_bindset;
  return(1);
}

/******************************************************************************
 ******************************************************************************/

