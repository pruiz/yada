
/******************************************************************************
 ******************************************************************************/

/** \file yada_pgsql.c
 * yada postgresql module
 *
 * $Id: yada_pgsql.c 182 2007-09-28 15:28:41Z grizz $
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

#include <errno.h>

#include "_yada.h"
#include "common.h"
#include "libpq-fe.h"

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

#define YADA_PG_PREP_STMT 1

#define PREP_ELE_CHUNK_SZ 8
#define GROW_BUF_IF_NEEDED(a, b, c) \
  { \
  void *tmp_ptr; \
  if(c >= b) \
    { \
    b += (c << 1); \
    if( !(tmp_ptr = realloc(a, b)) ) \
      { \
      free(a); \
      return(0); \
      } \
    a = tmp_ptr; \
    } \
  }

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

struct yada_modpriv_t
{
  PGconn *db;
  int last_statement;
};

typedef struct
{
  PGresult *res;
  char *name;
  int sz;
  int eles;
  prep_ele_t ele[1];
} yada_pg_prep_t;

/* A result structure, since libpq returns results as a table, we must track
 * the table row position for 'popping' from the table sequentially.
 */
typedef struct
  {
  PGresult *res;
  unsigned long row_count;
  unsigned long cur_row;
  } _yada_pg_s_result_t;

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** notice handler to stop libpq from writing to stderr
 */

void yada_pgsql_noticer(void *arg, const char *message)
{
}

/******************************************************************************/
/** connect to postgresql database
 * @param dbstr hostname:port:databasename
 * @param yada_user username for db access
 * @param yada_pass password for db access
 */

static int yada_pgsql_connect(yada_t *_yada, char *yada_user, char *yada_pass)
{
  int i;
  char *fsep;
  char *dbargs[] = {0, 0, 0};

  
  fsep = dbargs[0] = strdup(_yada->dbstr);
  for(i = 1; i < 3 && (fsep = strchr(fsep, ':')); i++)
    {
    *fsep++ = 0;
    dbargs[i] = fsep;
    }

  _yada->_mod->db = PQsetdbLogin(dbargs[0], dbargs[1], NULL, NULL, dbargs[2],
   yada_user, yada_pass);

  free(dbargs[0]);

  if(PQstatus(_yada->_mod->db) != CONNECTION_OK)
    {
    _yada_set_err(_yada, errno, PQerrorMessage(_yada->_mod->db));
    return(0);
    }

  PQsetNoticeProcessor(_yada->_mod->db, yada_pgsql_noticer, 0);

  return(1);
}

/******************************************************************************/
/** disconnect from database
 */

static void yada_pgsql_disconnect(yada_t *_yada)
{

  if(!_yada->_mod->db)
    return;

  PQfinish(_yada->_mod->db);
  _yada->_mod->db = 0;
}

/******************************************************************************/
/** escape string
 */

char *yada_pgsql_escstr(char *src, int slen, char *dest, int *dlen)
{
  if(!slen)
    slen = strlen(src);

  /* no dest specified, alloc for it */
  if(!dest)
    if(!(dest = malloc((slen << 1) + 1)))
      return(0);

  *dlen = PQescapeString(dest, src, slen);
  return(dest);
}

/******************************************************************************/

static PGresult* _pg_exec(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  char *pgquery;
  PGresult *pg_res;


  if(!sqlstr_len)
    {
    sqlstr_len = strlen(sqlstr);
    pgquery = sqlstr;
    }
  else
    {
    if(!(pgquery = malloc(sizeof(char) + sqlstr_len)))
      {
      _yada_set_err(_yada, YADA_ENOMEM, "Failed to alloc for query");
      return(NULL);
      }
    pgquery[sqlstr_len] = '\0';
    memcpy(pgquery, sqlstr, sqlstr_len);
    }

  pg_res = PQexec(_yada->_mod->db, pgquery);

  if (pgquery != sqlstr)
    free(pgquery);
  return(pg_res);
}

/******************************************************************************/

static PGresult* _pg_execprep(yada_t *_yada, yada_pg_prep_t *_ypr,
    int *rlen, va_list ap)
{
  int i, arglen;
  int count = 0;
  char *arg;
  char *args[_ypr->eles];
  prep_ele_t *elep, *elelen;
  PGresult *pg_res;


  elep = _ypr->ele;
  elelen = elep + _ypr->eles;
  
  /* loop through preparing passed variables */
  for(; elep < elelen; elep++)
    {
    switch(elep->t)
      {
    /* escaped binary (string with length) */
    case 'a':
    /* binary (string with length) */
    case 'b':
      arg = va_arg(ap, char *);
      arglen = va_arg(ap, int);
      args[count++] = strdup(arg);
      break;

    /* number */
    case 'd':
      {
      char dest[STRLEN_INT];

      i = va_arg(ap, int);

      snprintf(dest, STRLEN_INT, "%d", i);

      args[count++] = strdup(dest);
      }
      break;

    /* 64 bit number */
    case 'l':
      {
      long long l;
      char dest[STRLEN_INT64];

      l = va_arg(ap, long long);

      snprintf(dest, STRLEN_INT64, "%lld", l);
      args[count++] = strdup(dest);
      }
      break;

     /* escaped string */
    case 'e':
    /* string */
    case 's':
      /* value */
    case 'v':
      if((arg = va_arg(ap, char *)) == NULL)
        args[count++] = NULL;
      else
        args[count++] = strdup(arg);
      break;

    /* boolean */
    case 'B':
      i = va_arg(ap, int);
      if(i)
        args[count++] = strdup("true");
      else
        args[count++] = strdup("false");
      break;

    /* double */
    case 'f':
      {
      char dest[STRLEN_INT64*2];

      snprintf(dest, STRLEN_INT64*2, "%f", va_arg(ap, double));
      args[count++] = strdup(dest);
      break;
      }
      }
    }

  /* return results */
  pg_res = PQexecPrepared(_yada->_mod->db, _ypr->name, _ypr->eles,
      (const char**)args, NULL, NULL, 0);

  for(i = 0; i < count; i++)
    free(args[i]);

  return(pg_res);
}

/******************************************************************************/

static int _pg_retexec(yada_t *_yada, PGresult *pg_res)
{
  int pgtouples;


  if(!pg_res)
    {
    if(PQstatus(_yada->_mod->db) == CONNECTION_BAD)
      _yada_set_err(_yada, YADA_ENOTCONN, PQerrorMessage(_yada->_mod->db))
    else
      _yada_set_err(_yada, PQstatus(_yada->_mod->db),
       PQerrorMessage(_yada->_mod->db))
    return(-1);
    }

  if(PQresultStatus(pg_res) == PGRES_TUPLES_OK)
    pgtouples = PQntuples(pg_res);
  else if(PQresultStatus(pg_res) == PGRES_COMMAND_OK)
    pgtouples = atoi(PQcmdTuples(pg_res));
  else
    {
    _yada_set_err(_yada, PQresultStatus(pg_res),
     PQerrorMessage(_yada->_mod->db));

    PQclear(pg_res);
    return(-1);
    }

  /* no results */
  if(pgtouples < 1 && PQresultStatus(pg_res) == PGRES_TUPLES_OK)
    return(0);

  return(pgtouples);
}

/******************************************************************************/

static yada_rc_t* _pg_retquery(yada_t *_yada, PGresult *pg_res)
{
  int pgtouples;
  yada_rc_t *_yrc;
  _yada_pg_s_result_t *y_res;



  if(!pg_res)
    { 
    if(PQstatus(_yada->_mod->db) == CONNECTION_BAD)
      _yada_set_err(_yada, YADA_ENOTCONN, PQerrorMessage(_yada->_mod->db))
    else
      _yada_set_err(_yada, PQstatus(_yada->_mod->db),
       PQerrorMessage(_yada->_mod->db))
    return(NULL);
    }

  if(PQresultStatus(pg_res) == PGRES_TUPLES_OK)
    pgtouples = PQntuples(pg_res);
  else if(PQresultStatus(pg_res) == PGRES_COMMAND_OK)
    pgtouples = atoi(PQcmdTuples(pg_res));
  else
    {
    _yada_set_err(_yada, PQresultStatus(pg_res),
     PQerrorMessage(_yada->_mod->db));

    PQclear(pg_res);
    return(NULL);
    }

  /* no results */
  if(pgtouples < 1 && PQresultStatus(pg_res) == PGRES_TUPLES_OK)
    {
    if(!(_yrc = _yada_rc_new(_yada)))
      {
      _yada_set_yadaerr(_yada, YADA_ENOMEM);
      PQclear(pg_res);
      return(NULL);
      }

    _yrc->t = YADA_RESULT;
    _yrc->data = NULL;

    PQclear(pg_res);
    return(_yrc);
    }

  /* return results */
  if(!(_yrc = _yada_rc_new(_yada)))
    {
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    PQclear(pg_res);
    return(NULL);
    }

  if(!(y_res = malloc(sizeof(_yada_pg_s_result_t))))
    {
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    _yada->free(_yada, _yrc);
    PQclear(pg_res);
    return(NULL);
    }

  y_res->res = pg_res;
  y_res->row_count = pgtouples;
  y_res->cur_row = 0;

  _yrc->t = YADA_RESULT;
  _yrc->data = y_res;
  return(_yrc);
}

/******************************************************************************/

static int yada_pgsql__exec(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  PGresult *pg_res;


  pg_res = _pg_exec(_yada, sqlstr, sqlstr_len);
  return(_pg_retexec(_yada, pg_res));
}

/******************************************************************************/

static yada_rc_t* yada_pgsql__query(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  PGresult *pg_res;


  pg_res = _pg_exec(_yada, sqlstr, sqlstr_len);
  return(_pg_retquery(_yada, pg_res));
}

/******************************************************************************/

static void yada_pgsql_free_result(yada_t *_yada, yada_rc_t *_yrc)
{
  _yada_pg_s_result_t *y_res;


  if(!(y_res = _yrc->data))
    return;

  PQclear(y_res->res);
  free(y_res);
}

/******************************************************************************/

static void yada_pgsql_free_stmt(yada_t *_yada, yada_rc_t *_yrc)
{
  yada_pg_prep_t *prep;


  if(!(prep = _yrc->data))
    return;

  PQclear(prep->res);
  free(prep);
}

/******************************************************************************/

static void yada_pgsql__destroy(yada_t *_yada)
{
  yada_pgsql_disconnect(_yada);
  free(_yada->_mod);
}

/******************************************************************************/
/** fetch result row into bound vars
 * @param yada yada struct
 * @param rrc result rc
 * @param brc bound vars rc
 */

int yada_pgsql_fetch(yada_t *_yada, yada_rc_t *rrc, yada_rc_t *brc)
{
  int i, di = 0;
  long pg_fcount;
  _yada_pg_s_result_t *y_res;
  yada_bindset_t *bindset;


  if(!(y_res = rrc->data))
    return(0);

  /* last result */
  if(y_res->row_count <= y_res->cur_row)
    return(0);

  bindset = brc->data;

  pg_fcount = PQnfields(y_res->res);
  
  for(i = 0; i < bindset->eles; i++, di++)
    {

    /************************************************/
    /* bound variables */
    if(bindset->ele[i].t > 0)
      {
      if( di >= pg_fcount )
        {
        *(char *)bindset->ele[i].ptr = 0;

        if(bindset->ele[i].t == 'b')
          i++;
        continue;
        }

      switch(bindset->ele[i].t)
        {
      case 'b':
        memcpy(bindset->ele[i].ptr, PQgetvalue(y_res->res, y_res->cur_row, di),
         PQgetlength(y_res->res, y_res->cur_row, di));
        /* set len var */
        *(unsigned long *)bindset->ele[++i].ptr =
         PQgetlength(y_res->res, y_res->cur_row, di);
        break;
      case 'd':
        *(int *)bindset->ele[i].ptr =
         atoi(PQgetvalue(y_res->res, y_res->cur_row, di));
        break;
      case 'l':
        *(long long *)bindset->ele[i].ptr =
         atoll(PQgetvalue(y_res->res, y_res->cur_row, di));
        break;
      case 's':
      case 'e':
      case 'v':
        /* +1 for terminating null */
        memcpy(bindset->ele[i].ptr, PQgetvalue(y_res->res, y_res->cur_row, di),
         PQgetlength(y_res->res, y_res->cur_row, di) + 1);
        break;
      case 'f':
        *(double *)bindset->ele[i].ptr =
         atof(PQgetvalue(y_res->res, y_res->cur_row, di));
        break;
        }
      continue;
      }

    /************************************************/
    /* bound pointers */
    if(di >= pg_fcount)
      {
      *(char **)bindset->ele[i].ptr = 0;

      if(bindset->ele[i].t == -'b')
        i++;
      continue;
      }

    switch(-bindset->ele[i].t)
      {
    case 'd':
      bindset->ele[i].var.i = atoi(PQgetvalue(y_res->res, y_res->cur_row, di));
      *(int **)bindset->ele[i].ptr = &(bindset->ele[i].var.i);
      break;
    case 'l':
      bindset->ele[i].var.l = atoll(PQgetvalue(y_res->res, y_res->cur_row, di));
      *(long long **)bindset->ele[i].ptr = &(bindset->ele[i].var.l);
      break;
    case 's':
    case 'e':
    case 'v':
      if(PQgetisnull(y_res->res, y_res->cur_row, di))
        {
        *(char **)bindset->ele[i].ptr = 0;
        break;
        }

      *(char **)bindset->ele[i].ptr =
       PQgetvalue(y_res->res, y_res->cur_row, di);
      break;
    case 'b':
      if(PQgetisnull(y_res->res, y_res->cur_row, di))
        {
        *(char **)bindset->ele[i].ptr = 0;
        *(unsigned long *)bindset->ele[++i].ptr = 0;
        break;
        }

      *(char **)bindset->ele[i].ptr =
       PQgetvalue(y_res->res, y_res->cur_row, di);
      /* set len var */
      *(unsigned long *)bindset->ele[++i].ptr =
       PQgetlength(y_res->res, y_res->cur_row, di);
      break;
    case 'f':
      bindset->ele[i].var.f = atof(PQgetvalue(y_res->res, y_res->cur_row, di));
      *(double **)bindset->ele[i].ptr = &(bindset->ele[i].var.f);
      break;
      }
    } /* for(eles) */

  y_res->cur_row++;
  return(1);
}

/******************************************************************************/
/** starts a transaction
 */

static int yada_pgsql_trx(yada_t *_yada, int flags)
{
  int pg_stat;
  PGresult *pg_res;


  pg_res = PQexec(_yada->_mod->db, "BEGIN");
  pg_stat = PQresultStatus(pg_res);
  PQclear(pg_res);

  /* return 0 if no error */
  return(pg_stat != PGRES_COMMAND_OK);
}

/******************************************************************************/
/** commits a transaction
 */

static int yada_pgsql_commit(yada_t *_yada)
{
  int pg_stat;
  PGresult *pg_res;


  pg_res = PQexec(_yada->_mod->db, "COMMIT");
  pg_stat = PQresultStatus(pg_res);
  PQclear(pg_res);

  /* return 0 if no error */
  return(pg_stat != PGRES_COMMAND_OK && pg_stat != PGRES_TUPLES_OK &&
      pg_stat != PGRES_NONFATAL_ERROR);
}

/******************************************************************************/
/** aborts a transaction
 */

static int yada_pgsql_rollback(yada_t *_yada, int flags)
{
  int pg_stat;
  PGresult *pg_res;


  pg_res = PQexec(_yada->_mod->db, "ROLLBACK");
  pg_stat = PQresultStatus(pg_res);
  PQclear(pg_res);

  /* return 0 if no error */
  return(pg_stat != PGRES_COMMAND_OK && pg_stat != PGRES_TUPLES_OK);
}

/******************************************************************************/
/** get the last insert id
 * @returns last insert id, or 0 on failure
 */

static uint64_t yada_pgsql_insert_id(yada_t *_yada, char *table, char *col)
{
  char *pgquery;
  PGresult *pg_res;
  int id, len;

  len = strlen("SELECT currval(pg_get_serial_sequence('',''))") + 
    strlen(table) + strlen(col) + 1;
  pgquery = malloc(len);
  snprintf(pgquery, len, "SELECT currval(pg_get_serial_sequence('%s','%s'))", 
      table, col);

  pg_res = PQexec(_yada->_mod->db, pgquery);
  id = atoi(PQgetvalue(pg_res, 0, 0));

  PQclear(pg_res);
  free(pgquery);
  return(id);
}

/******************************************************************************/

yada_pg_prep_t* pgsql_prep_ele_new(void)
{
  yada_pg_prep_t *yprep;


  if(!(yprep = malloc(sizeof(yada_pg_prep_t) +
                     (sizeof(prep_ele_t) * PREP_ELE_CHUNK_SZ))))
    return(0);

  yprep->sz = PREP_ELE_CHUNK_SZ;
  yprep->eles = 0;
  return(yprep);
}

/******************************************************************************/

yada_pg_prep_t* pgsql_prep_ele_grow(yada_pg_prep_t *yprep)
{
  int sz = yprep->sz + PREP_ELE_CHUNK_SZ;
  yada_pg_prep_t *tmp_ptr;


  if(!(tmp_ptr = realloc(yprep, sizeof(yada_prep_t) +
   (sizeof(prep_ele_t) * sz))))
    {
    free(yprep);
    return(0);
    }

  yprep = tmp_ptr;
  yprep->sz = sz;
  return(yprep);
}

/******************************************************************************/

static yada_rc_t* yada_pgsql_str_prepare(yada_t *_yada, char *sqlstr)
{
  char *str;
  char tmp[32];
  yada_pg_prep_t *yprep;
  yada_rc_t *_yrc;
  int count = 1, len, index = 0;
  char c;

  /* create prep struct */
  if(!(yprep = pgsql_prep_ele_new()))
    return NULL;

  str = strdup(sqlstr);
  len = strlen(sqlstr);
  while(str[index] != '\0')
  {
    c = str[index];
    
    /* char '?', replace with $count */
    if(c == '?')
    {
      int newlen, i;

      if(yprep->eles == yprep->sz)
        if(!(yprep = pgsql_prep_ele_grow(yprep)))
          return NULL;

      yprep->ele[yprep->eles].t = str[index+1];
      yprep->ele[yprep->eles].buf = 0;
      yprep->ele[yprep->eles].len = 0;
      yprep->eles++;

      str[index++] = '$';

      snprintf(tmp, 31, "%d", count);

      newlen = strlen(tmp);
      len += newlen-1;
      if(newlen > 1)
      {
        str = (char*)realloc(str, len+newlen+1);
        memmove(&str[index+newlen], &str[index+1], len-index);
      }

      for(i = 0; i < newlen; i++)
       str[index++] = tmp[i];

      count++;
    }
    else
      index++;
  }

  snprintf(tmp, 31, "STMT %d", _yada->_mod->last_statement++); 
  yprep->name = strdup(tmp);

  yprep->res = PQprepare(_yada->_mod->db, yprep->name, str, 0, NULL);

  DEBUGMSG("PGPREP: (%s)[%s]", yprep->name, str);
  free(str);

  if(PQresultStatus(yprep->res) != PGRES_COMMAND_OK)
    {
    _yada_set_err(_yada, PQresultStatus(yprep->res),
      PQerrorMessage(_yada->_mod->db));
    free(yprep);
    return(NULL);
    }

  if(!(_yrc = _yada_rc_new(_yada)))
    {
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    free(yprep);
    return(NULL);
    }

  _yrc->t = YADA_PREPARED;
  _yrc->data = yprep;
  return _yrc;
}

/******************************************************************************/

static yada_rc_t* yada_pgsql__queryprep(yada_t *_yada, void *ps, int *rlen, va_list ap)
{
  PGresult *pg_res;


  pg_res = _pg_execprep(_yada, ps, rlen, ap);
  return(_pg_retquery(_yada, pg_res));
}

/******************************************************************************/

static int yada_pgsql__execprep(yada_t *_yada, void *ps, int *rlen, va_list ap)
{
  PGresult *pg_res;


  pg_res = _pg_execprep(_yada, ps, rlen, ap);
  return(_pg_retexec(_yada, pg_res));
}

/******************************************************************************/

static yada_rc_t* yada_pgsql_len_prepare(yada_t *_yada, char *sqlstr,
 int sqlstr_len)
{
  return(yada_pgsql_str_prepare(_yada, sqlstr));
  return(NULL);
}

/******************************************************************************/

yada_rc_t* yada_pgsql_prepare(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  if(sqlstr_len)
    return(yada_pgsql_len_prepare(_yada, sqlstr, sqlstr_len));

  return(yada_pgsql_str_prepare(_yada, sqlstr));
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
  _yada->type_id = YADA_PGSQL;
  _yada->connect = yada_pgsql_connect;
  _yada->disconnect = yada_pgsql_disconnect;

  /* native prepare funtions */
  _yada->prepare = yada_pgsql_prepare;
  _yada->preparef = _yada_npreparef;

  /* yada compat prepare funtions */
  _yada->yprepare = _yada_prepare;
  _yada->ypreparef = _yada_preparef;

  _yada->xprepare = _yada_xprepare;

  _yada->execute = _yada_execute;
  _yada->xexecute = _yada_xexecute;

  _yada->query = _yada_query;
  _yada->xquery = _yada_xquery;

  /* yada util functions */
  _yada->escstr = yada_pgsql_escstr;
  _yada->dumpexec = _yada_dumpexec;

  /* yada compat bind functions */
  _yada->bind = _yada_bind;
  _yada->fetch = yada_pgsql_fetch;

  /* var args functions */
  _yada->vquery = _yada_vquery;
  _yada->vbind = _yada_vbind;
  _yada->vexecute = _yada_vexecute;

  /* transaction functions */
  _yada->trx = yada_pgsql_trx;
  _yada->commit = yada_pgsql_commit;
  _yada->rollback = yada_pgsql_rollback;

  _yada->insert_id = yada_pgsql_insert_id;

  /* private interfaces */
  _yada->_priv->exec = yada_pgsql__exec;
  _yada->_priv->query = yada_pgsql__query;
  _yada->_priv->execprep = yada_pgsql__execprep;
  _yada->_priv->queryprep = yada_pgsql__queryprep;
  _yada->_priv->destroy = yada_pgsql__destroy;

  _yada->_priv->free_rc[YADA_STATEMENT] = _yada_free_stmt;
  _yada->_priv->free_rc[YADA_RESULT] = yada_pgsql_free_result;
  _yada->_priv->free_rc[YADA_BINDSET] = yada_free_bindset;
  _yada->_priv->free_rc[YADA_PREPARED] = yada_pgsql_free_stmt;

  return(1);
}

/******************************************************************************
 ******************************************************************************/

