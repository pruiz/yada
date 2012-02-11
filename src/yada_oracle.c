
/******************************************************************************
 ******************************************************************************/

/** \file yada_oracle.c
 * yada oracle module
 *
 * $Id: yada_oracle.c 182 2007-09-28 15:28:41Z grizz $
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
#include <errno.h>
#include <dlfcn.h>

#include "_yada.h"
#include "common.h"
#include <oci.h>
#include <xa.h>

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

/* size of buffer to allocate for bind pointers */
#define MAXDATA			2048

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

typedef enum
{
  ORAF_INTRX = 0x01
} oraflag_t;

struct yada_modpriv_t
{
  char *dbuser;
  char *dbpass;
  OCIEnv *env;
  OCIError *err;
  OCIServer *srv;
  OCISvcCtx *ctx;
  OCISession *ses;
  oraflag_t flags;
};

/* extended statement info */
typedef struct
{
  OCIStmt *stmt;
  int bind;
  int invalid;
} extinfo;

/******************************************************************************
 * M A C R O S ****************************************************************
 ******************************************************************************/

/* define out this function for older versions of OCI */
#ifdef OBSOLETE_OCI
# define OCIStmtRelease(a,b,c,d,e)
#endif

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** connect to oracle database
 * @param dbstr database name
 * @param yada_user username for db access
 * @param yada_pass password for db access
 * @return 0 on failure
 */

static int yada_oracle_connect(yada_t *_yada, char *yada_user, char *yada_pass)
{
  char errbuf[1024];
  sb4 errcode;


  /* check cached user/pass */
  if(_yada->_mod->dbuser && yada_user && strcmp(_yada->_mod->dbuser, yada_user))
    {
    free(_yada->_mod->dbuser);
    _yada->_mod->dbuser = NULL;
    }
  if(_yada->_mod->dbpass && yada_pass && strcmp(_yada->_mod->dbpass, yada_pass))
    {
    free(_yada->_mod->dbpass);
    _yada->_mod->dbpass = NULL;
    }

  /* cache user/pass */
  if(!_yada->_mod->dbuser)
    {
    if(!yada_user)
      {
      _yada_set_err(_yada, -1, "invalid username");
      return(0);
      }
    if(!(_yada->_mod->dbuser = strdup(yada_user)))
      {
      _yada_set_err(_yada, -1, strerror(errno));
      return(0);
      }
    }

  if(!_yada->_mod->dbpass)
    {
    if(!yada_user)
      {
      _yada_set_err(_yada, -1, "invalid username");
      return(0);
      }
    if(!(_yada->_mod->dbpass = strdup(yada_pass)))
      {
      _yada_set_err(_yada, -1, strerror(errno));
      return(0);
      }
    }

  /* create environment */
  if(OCIEnvCreate(&_yada->_mod->env, OCI_DEFAULT,
   NULL, NULL, NULL, NULL, 0, NULL) != OCI_SUCCESS)
    {
    _yada_set_err(_yada, -1, "OCIEnvCreate failed");
    return(0);
    }

  /* allocate error handle */
  if(OCIHandleAlloc(_yada->_mod->env, (dvoid**)&_yada->_mod->err,
   OCI_HTYPE_ERROR, 0, NULL) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->env, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ENV);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* allocate server handle */
  if(OCIHandleAlloc(_yada->_mod->env, (dvoid**)&_yada->_mod->srv,
   OCI_HTYPE_SERVER, 0, NULL) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->env, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ENV);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* attach to server */
  if(OCIServerAttach(_yada->_mod->srv, _yada->_mod->err, (text*)_yada->dbstr,
   strlen(_yada->dbstr), OCI_DEFAULT) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* allocate context handle */
  if(OCIHandleAlloc(_yada->_mod->env, (dvoid**)&_yada->_mod->ctx,
   OCI_HTYPE_SVCCTX, 0, NULL) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->env, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ENV);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* set server attribute of context handle */
  if(OCIAttrSet(_yada->_mod->ctx, OCI_HTYPE_SVCCTX, _yada->_mod->srv, 0,
   OCI_ATTR_SERVER, _yada->_mod->err) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* allocate session handle */
  if(OCIHandleAlloc(_yada->_mod->env, (dvoid**)&_yada->_mod->ses,
   OCI_HTYPE_SESSION, 0, NULL) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->env, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ENV);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* set username/password attributes of session handle */
  if(OCIAttrSet(_yada->_mod->ses, OCI_HTYPE_SESSION,
   (dvoid*)_yada->_mod->dbuser, strlen(_yada->_mod->dbuser), OCI_ATTR_USERNAME,
   _yada->_mod->err) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }
  if(OCIAttrSet(_yada->_mod->ses, OCI_HTYPE_SESSION,
   (dvoid*)_yada->_mod->dbpass, strlen(_yada->_mod->dbpass), OCI_ATTR_PASSWORD,
   _yada->_mod->err) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* start the session */
  if(OCISessionBegin(_yada->_mod->ctx, _yada->_mod->err, _yada->_mod->ses,
   OCI_CRED_RDBMS, OCI_DEFAULT) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  /* set session attribute of context handle */
  if(OCIAttrSet(_yada->_mod->ctx, OCI_HTYPE_SVCCTX, _yada->_mod->ses, 0,
   OCI_ATTR_SESSION, _yada->_mod->err) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
    }

  return(1);
}

/******************************************************************************/
/** disconnect from database
 */

static void yada_oracle_disconnect(yada_t *_yada)
{
  yada_rc_t *yrc;

  
  /* invalidate all result sets associated with this connection */
  for(yrc=_yada->_priv->rc_head; yrc; yrc=yrc->next)
    {
    if(yrc->t != YADA_RESULT)
      continue;

    ((extinfo*)yrc->data)->invalid = 1;
    OCIStmtRelease(((extinfo*)yrc->data)->stmt, _yada->_mod->err,
     NULL, 0, OCI_DEFAULT);
    OCIHandleFree(((extinfo*)yrc->data)->stmt, OCI_HTYPE_STMT);
    }

  /* cleanup OCI */
  if(_yada->_mod->ses)
    OCISessionEnd(_yada->_mod->ctx, _yada->_mod->err, _yada->_mod->ses, OCI_DEFAULT);
  if(_yada->_mod->srv)
    OCIServerDetach(_yada->_mod->srv, _yada->_mod->err, OCI_DEFAULT);
  if(_yada->_mod->env)
    OCIHandleFree(_yada->_mod->env, OCI_HTYPE_ENV);

  /* re-initialize extended structure */
  _yada->_mod->env = NULL;
  _yada->_mod->err = NULL;
  _yada->_mod->srv = NULL;
  _yada->_mod->ctx = NULL;
  _yada->_mod->ses = NULL;
  _yada->errmsg = _yada->_priv->errbuf;
}

/******************************************************************************/
/** escape strings for oracle
 * @return escaped string on success
 */

char* yada_oracle_escstr(char *src, int slen, char *dest, int *dlen)
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

    *dlen--;

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
/** start a transaction
 * @return 0 on success
 */

static int yada_oracle_trx(yada_t *_yada, int flags)
{
  /* check database connection */
  if(!_yada->_mod->env && !_yada->connect(_yada, NULL, NULL))
    return(-1);

  if(_yada->_mod->flags & ORAF_INTRX)
    {
    _yada_set_yadaerr(_yada, YADA_EINTRX);
    return(-1);
    }

  _yada->_mod->flags |= ORAF_INTRX;
  return(0);
}

/******************************************************************************/
/** commit a transaction
 * @return 0 on success
 */

static int yada_oracle_commit(yada_t *_yada)
{
  char errbuf[1024];
  sb4 errcode;


  /* check database connection */
  if(!_yada->_mod->env)
    {
    _yada_set_err(_yada, 0, "database unavailable");
    return(-1);
    }

  if(OCITransCommit(_yada->_mod->ctx, _yada->_mod->err, OCI_DEFAULT)
   != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(-1);
    }

  if(_yada->_mod->flags & ORAF_INTRX)
    _yada->_mod->flags ^= ORAF_INTRX;
  return(0);
}

/******************************************************************************/
/** abort a transaction
 * @return 0 on success
 */

static int yada_oracle_rollback(yada_t *_yada, int flags)
{
  char errbuf[1024];
  sb4 errcode;


  /* check database connection */
  if(!_yada->_mod->env)
    {
    _yada_set_err(_yada, 0, "database unavailable");
    return(-1);
    }

  if(OCITransRollback(_yada->_mod->ctx, _yada->_mod->err, OCI_DEFAULT)
    != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(-1);
    }

  if(_yada->_mod->flags & ORAF_INTRX)
    _yada->_mod->flags ^= ORAF_INTRX;
  return(0);
}

/******************************************************************************/
/** execute a non-select query
 * @return number of affected rows on success, -1 on error
 */

static int yada_oracle__exec(yada_t *_yada, char *sqlstr, int sqlstr_len)
{
  char errbuf[1024];
  sb4 errcode;
  ub4 rows;
  OCIStmt *stmt = NULL;


  /* check database connection */
  if(!_yada->_mod->env && !_yada->connect(_yada, NULL, NULL))
    return(0);

  if(!sqlstr_len)
    sqlstr_len = strlen(sqlstr);

  /* prepare the statement */
#ifdef OBSOLETE_OCI
  /* allocate error handle */
  if(OCIHandleAlloc(_yada->_mod->env, (void *)&stmt,
   OCI_HTYPE_STMT, 0, NULL) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->env, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ENV);
    _yada_set_err(_yada, errcode, errbuf);
    return(0);
    }

  if(OCIStmtPrepare(stmt, _yada->_mod->err, (text*)sqlstr,
   (ub4)sqlstr_len, OCI_NTV_SYNTAX, OCI_DEFAULT) != OCI_SUCCESS)
    {
#else
  if(OCIStmtPrepare2(_yada->_mod->ctx, &stmt, _yada->_mod->err, (text*)sqlstr,
   (ub4)sqlstr_len, NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT) != OCI_SUCCESS)
    {
#endif
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
    return(-1);
    }

  /* execute the statement */
  if(OCIStmtExecute(_yada->_mod->ctx, stmt, _yada->_mod->err,
   (ub4)1, (ub4)0, NULL, NULL, OCI_DEFAULT) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    OCIStmtRelease(stmt, _yada->_mod->err, NULL, 0, OCI_DEFAULT);
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
    _yada->disconnect(_yada);
    return(-1);
    }

  /* determine number of affected rows */
  if(OCIAttrGet(stmt, OCI_HTYPE_STMT, &rows, NULL,
   OCI_ATTR_ROW_COUNT, _yada->_mod->err) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    OCIStmtRelease(stmt, _yada->_mod->err, NULL, 0, OCI_DEFAULT);
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
    _yada->disconnect(_yada);
    return(-1);
    }

  /* commit if not in a transaction */
  if(!(_yada->_mod->flags & ORAF_INTRX))
    yada_oracle_commit(_yada);

  /* release prepared statement */
  OCIStmtRelease(stmt, _yada->_mod->err, NULL, 0, OCI_DEFAULT);
  OCIHandleFree(stmt, OCI_HTYPE_STMT);
  return(rows);
}

/******************************************************************************/
/** prepare a statement
 * @return YADA_RESULT resource on success
 */

static yada_rc_t* yada_oracle__query(yada_t *_yada, char *sqlstr,
 int sqlstr_len)
{
  char errbuf[1024];
  sb4 errcode;
  yada_rc_t *_yrc;
  OCIStmt *stmt = NULL;


  /* check database connection */
  if(!_yada->_mod->env && !_yada->connect(_yada, NULL, NULL))
    return(0);

  if(!sqlstr_len)
    sqlstr_len = strlen(sqlstr);

  /* prepare the statement */
#ifdef OBSOLETE_OCI
  /* allocate error handle */
  if(OCIHandleAlloc(_yada->_mod->env, (void*)&stmt,
   OCI_HTYPE_STMT, 0, NULL) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->env, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ENV);
    _yada_set_err(_yada, errcode, errbuf);
    return(NULL);
    }

  if(OCIStmtPrepare(stmt, _yada->_mod->err, (text*)sqlstr,
   (ub4)sqlstr_len, OCI_NTV_SYNTAX, OCI_DEFAULT) != OCI_SUCCESS)
    {
#else
  if(OCIStmtPrepare2(_yada->_mod->ctx, &stmt, _yada->_mod->err, (text*)sqlstr,
   (ub4)sqlstr_len, NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT) != OCI_SUCCESS)
    {
#endif
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
    return(NULL);
    }

  /* execute the statement */
  if(OCIStmtExecute(_yada->_mod->ctx, stmt, _yada->_mod->err,
   (ub4)0, (ub4)0, NULL, NULL, OCI_DEFAULT) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    OCIStmtRelease(stmt, _yada->_mod->err, NULL, 0, OCI_DEFAULT);
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
    _yada->disconnect(_yada);
    return(NULL);
    }

  /* return statement handle */
  if(!(_yrc = _yada_rc_new(_yada)))
    {
    OCIStmtRelease(stmt, _yada->_mod->err, NULL, 0, OCI_DEFAULT);
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    return(NULL);
    }
  _yrc->t = YADA_RESULT;

  /* alocate space for extended info */
  if(!(_yrc->data = malloc(sizeof(extinfo))))
    {
    _yada->free(_yada, _yrc);
    OCIStmtRelease(stmt, _yada->_mod->err, NULL, 0, OCI_DEFAULT);
    OCIHandleFree(stmt, OCI_HTYPE_STMT);
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    return(NULL);
    }
  ((extinfo*)_yrc->data)->stmt = stmt;
  ((extinfo*)_yrc->data)->bind = 1;
  ((extinfo*)_yrc->data)->invalid = 0;
  return(_yrc);
}

/******************************************************************************/
/** free prepared statement
 */

static void yada_oracle_free_result(yada_t *_yada, yada_rc_t *_yrc)
{
  if(!((extinfo*)_yrc->data)->invalid)
    {
    OCIStmtRelease(((extinfo*)_yrc->data)->stmt, _yada->_mod->err,
     NULL, 0, OCI_DEFAULT);
    OCIHandleFree(((extinfo*)_yrc->data)->stmt, OCI_HTYPE_STMT);
    }

  free(_yrc->data);
}

/******************************************************************************/
/** free bindset
 */

static void yada_oracle_free_bindset(yada_t *_yada, yada_rc_t *_yrc)
{
  int i;
  yada_bindset_t *bs;
  bindset_ele_t *bse;


  bs = _yrc->data;

  /* free any allocated buffers */
  for(i=0, bse=bs->ele; i<bs->eles; bse++, i++)
    {
    switch(bse->t)
      {
    /************************************************/
    case -'b':
    /************************************************/
      /* free allocated buffer */
      free(bse->var.buf);
    /* fallthrough */
    /************************************************/
    case 'b':
    /************************************************/
      /* skip length element */
      bse++;
      break;
    /************************************************/
    case 'l':
    case -'l':
    /************************************************/
      free(bse->tmp);
      break;
    /************************************************/
    case -'s':
    case -'e':
    case -'v':
    /************************************************/
      /* free allocated buffer */
      free(bse->var.buf);
      break;
    /************************************************/
      }
    } /* foreach(binding) */

  /* call generic free function */
  yada_free_bindset(_yada, _yrc);
}

/******************************************************************************/
/** destroy module specific data
 */

static void yada_oracle__destroy(yada_t *_yada)
{
  yada_oracle_disconnect(_yada);
  free(_yada->_mod->dbuser);
  free(_yada->_mod->dbpass);
  free(_yada->_mod);
}

/******************************************************************************/
/** define output bindings to oracle statement
 * @return 0 on success
 */

static int yada_oracle_bind(yada_t *_yada, OCIStmt *stmt, yada_bindset_t *bs)
{
  int i, ctr;
  ub2 type;
  sb4 errcode;
  char errbuf[1024];
  OCIParam *param;
  OCIDefine *def;
  bindset_ele_t *bse;


  /* determine number of actual bindings */
  for(i=ctr=0, bse=bs->ele; i<bs->eles; i++, bse++)
    {
    ctr++;
    if((bse->t == 'b') || (bse->t == -'b'))
      {
      bse++;
      i++;
      }
    }

  /* check number of parameters */
  if(OCIAttrGet(stmt, OCI_HTYPE_STMT, &i, NULL,
   OCI_ATTR_PARAM_COUNT, _yada->_mod->err) != OCI_SUCCESS)
    {
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    return(-1);
    }
  if(i != ctr)
    {
    _yada_set_err(_yada, EBADE, "parameter count mismatch");
    return(-1);
    }


  /* prepare each select-list item */
  for(i=1, bse=bs->ele; i<=ctr; i++, bse++)
    {
    def = NULL;

    if((OCIParamGet(stmt, OCI_HTYPE_STMT,
     _yada->_mod->err, (dvoid*)&param, i)) != OCI_SUCCESS)
      {
      OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
       OCI_HTYPE_ERROR);
      _yada_set_err(_yada, errcode, errbuf);
      return(-1);
      }
    if((OCIAttrGet(param, OCI_DTYPE_PARAM, &type, 0,
     OCI_ATTR_DATA_TYPE, _yada->_mod->err)) != OCI_SUCCESS)
      {
      OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
       OCI_HTYPE_ERROR);
      _yada_set_err(_yada, errcode, errbuf);
      return(-1);
      }


    switch(bse->t)
      {
    /************************************************/
    case 'b':
    /************************************************/
      /* user allocated binary string */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->ptr,
       MAXDATA, SQLT_STR, &bse->ind, (ub2*)&bse[1].var.i, 0, OCI_DEFAULT);
      bse++;
      break;
    /************************************************/
    case 'd':
    /************************************************/
      /* user allocated int */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->ptr,
       sizeof(int), SQLT_INT, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
    case 'f':
    /************************************************/
      /* user allocated int */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->ptr,
       sizeof(double), SQLT_FLT, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
    case 'l':
    /************************************************/
      /* user allocated int 64
       * alloc for string and then convert after fetch
       */
      if(!(bse->tmp = (unsigned char*)malloc(STRLEN_INT64)))
        {
        _yada_set_yadaerr(_yada, YADA_ENOMEM);
        return(-1);
        }

      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->tmp,
       STRLEN_INT64, SQLT_STR, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
    case 's':
    case 'e':
    case 'v':
    /************************************************/
      /* user allocated string */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->ptr,
       MAXDATA, SQLT_STR, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
    case -'b':
    /************************************************/
      /* allocate space for data */
      if(!(bse->var.buf = (unsigned char*)malloc(MAXDATA)))
        {
        _yada_set_yadaerr(_yada, YADA_ENOMEM);
        return(-1);
        }

      /* locally allocated binary string */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->var.buf,
       MAXDATA, SQLT_STR, &bse->ind, (ub2*)&bse[1].var.i, 0, OCI_DEFAULT);
      bse++;
      break;
    /************************************************/
    case -'d':
    /************************************************/
      /* locally provided int */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, &bse->var.i,
       sizeof(int), SQLT_INT, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
    case -'f':
    /************************************************/
      /* locally provided int */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, &bse->var.f,
       sizeof(double), SQLT_FLT, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
    case -'l':
    /************************************************/
      /* locally provided int 64
       * alloc for string and then copy after fetch
       */
      if(!(bse->tmp = (unsigned char*)malloc(STRLEN_INT64)))
        {
        _yada_set_yadaerr(_yada, YADA_ENOMEM);
        return(-1);
        }

      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->tmp,
       STRLEN_INT64, SQLT_STR, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
    case -'s':
    case -'e':
    case -'v':
    /************************************************/
      /* allocate space for string */
      if(!(bse->var.buf = (unsigned char*)malloc(MAXDATA)))
        {
        _yada_set_yadaerr(_yada, YADA_ENOMEM);
        return(-1);
        }

      /* localy allocated string */
      OCIDefineByPos(stmt, &def, _yada->_mod->err, i, bse->var.buf,
       MAXDATA, SQLT_STR, &bse->ind, 0, 0, OCI_DEFAULT);
      break;
    /************************************************/
      } /* switch(bind_type) */
    } /* foreach(binding) */

  return(0);
}

/******************************************************************************/
/** fetch result row into bound vars
 * @param yada yada struct
 * @param rrc result rc
 * @param brc bound vars rc
 */

static int yada_oracle_fetch(yada_t *_yada, yada_rc_t *rrc, yada_rc_t *brc)
{
  char errbuf[1024];
  sb4 errcode;
  int i;
  yada_bindset_t *bs;
  bindset_ele_t *bse;
  extinfo *info;


  bs = brc->data;
  info = (extinfo*)rrc->data;

  /* check if database connection disappeared */
  if(info->invalid)
    {
    _yada_set_err(_yada, -1, "database unavailable");
    return(0);
    }

  /* check database connection */
  if(!_yada->_mod->env && !_yada->connect(_yada, NULL, NULL))
    return(0);

  /* create oracle bindings on the fly */
  if(info->bind)
    {
    if(yada_oracle_bind(_yada, info->stmt, bs))
      return(0);
    info->bind = 0;
    }


  /* fetch rows into buffers */
#ifdef OBSOLETE_OCI
  switch(OCIStmtFetch(info->stmt, _yada->_mod->err, 1,
   OCI_FETCH_NEXT, OCI_DEFAULT))
    {
#else
  switch(OCIStmtFetch2(info->stmt, _yada->_mod->err, 1,
   OCI_FETCH_NEXT, 0, OCI_DEFAULT))
    {
#endif
  /************************************************/
  case OCI_SUCCESS:
  /************************************************/
    /* fetch succeeded */
    break;
  /************************************************/
  case OCI_SUCCESS_WITH_INFO:
  /************************************************/
    /* fetch succeeded -- store warning info */
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    break;
  /************************************************/
  case OCI_NO_DATA:
  /************************************************/
    /* no more data to fetch */
    _yada->error = 0;
    return(0);
  /************************************************/
  default:
  /************************************************/
    OCIErrorGet(_yada->_mod->err, 1, NULL, &errcode, errbuf, sizeof(errbuf),
     OCI_HTYPE_ERROR);
    _yada_set_err(_yada, errcode, errbuf);
    _yada->disconnect(_yada);
    return(0);
  /************************************************/
    } /* switch(status) */


  /* tweek bound data */
  for(i=0, bse=bs->ele; i<bs->eles; i++, bse++)
    {
    if(bse->t <= 0)
      {
      switch(*(sb2*)&bse->ind)
        {
      /************************************************/
      case 0:
      /************************************************/
        /* value contained a result */
        *(void**)bse->ptr = bse->var.buf;
        break;
      /************************************************/
      case -1:
      /************************************************/
        /* value was NULL */
        *(void**)bse->ptr = NULL;
        break;
      /************************************************/
      default:
      /************************************************/
        /* value didn't fit in buffer */
        _yada_set_err(_yada, ENAMETOOLONG, "value too large for buffer");
        return(0);
        /************************************************/
        } /* switch(indicator) */
      } /* if(bind_ptr) */

    switch(bse->t)
      {
    /************************************************/
    case -'d':
    /************************************************/
      /* update pointer if value was successful */
      *(void**)bse->ptr = (*(sb2*)&bse->ind ? NULL : &bse->var.i);
      break;
    /************************************************/
    case 'l':
    /************************************************/
      /* update user-supplied value */
      *(long long*)bse->ptr = (*(sb2*)&bse->ind ? 0 : atoll(bse->tmp));
      break;
    /************************************************/
    case -'l':
    /************************************************/
      /* update pointer if value was successful */
      if(*(sb2*)&bse->ind)
        *(void**)bse->ptr = NULL;
      else
        {
        bse->var.l = atoll(bse->tmp);
        *(void**)bse->ptr = &bse->var.l;
        }
      break;
    /************************************************/
    case 'b':
    case -'b':
    /************************************************/
      /* convert size to int */
      bse++;
      i++;
      *(int*)bse->ptr = (int)*(sb2*)&bse->var.i;
      break;
    /************************************************/
      } /* switch(bind_type) */
    } /* foreach(binding) */

  return(1);
}

/******************************************************************************/
/** get the last insert id
 * @returns last insert id, or 0 on failure
 */

static uint64_t yada_oracle_insert_id(yada_t *_yada, char *table, char *col)
{
  /* this needs to be user set */
  return(0);
}

/******************************************************************************/
/** create and init yada struct
 * @returns non zero on success, 0 on error
 */

int yada_mod_init(yada_t *_yada)
{
  if(!(_yada->_mod = calloc(1, sizeof(yada_modpriv_t))))
    return(0);

  _yada_set_err(_yada, 0, "no error reported");

  /* yada module base functions */
  _yada->type_id = YADA_ORACLE;
  _yada->connect = yada_oracle_connect;
  _yada->disconnect = yada_oracle_disconnect;

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
  _yada->escstr = yada_oracle_escstr;
  _yada->dumpexec = _yada_dumpexec;

  /* yada compat bind functions */
  _yada->bind = _yada_bind;
  _yada->fetch = yada_oracle_fetch;

  /* var args functions */
  _yada->vquery = _yada_vquery;
  _yada->vbind = _yada_vbind;
  _yada->vexecute = _yada_vexecute;

  /* transaction functions */
  _yada->trx = yada_oracle_trx;
  _yada->commit = yada_oracle_commit;
  _yada->rollback = yada_oracle_rollback;

  _yada->insert_id = yada_oracle_insert_id;

  /* private interfaces */
  _yada->_priv->exec = yada_oracle__exec;
  _yada->_priv->query = yada_oracle__query;
  _yada->_priv->destroy = yada_oracle__destroy;

  _yada->_priv->free_rc[YADA_STATEMENT] = _yada_free_stmt;
  _yada->_priv->free_rc[YADA_RESULT] = yada_oracle_free_result;
  _yada->_priv->free_rc[YADA_BINDSET] = yada_oracle_free_bindset;

  return(1);
}

/******************************************************************************
 ******************************************************************************/


