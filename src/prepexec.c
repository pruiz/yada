
/******************************************************************************
 ******************************************************************************/

/** \file prepexec.c
 * yada compat prepare and execute functions
 *
 * $Id: prepexec.c 182 2007-09-28 15:28:41Z grizz $
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
#include <stdarg.h>
#include <string.h>

#include "_yada.h"

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

#define PREP_ELE_CHUNK_SZ 8

/******************************************************************************/
/** double the size of a buffer if needed
 * @param a buffer
 * @param b current buffer size
 * @param c length needed
 * @return 0 on error
 */

#define GROW_BUF_IF_NEEDED(a, b, c) { \
  void *tmp_ptr; \
  if(c >= b) { \
    b += (c << 1); \
    if( !(tmp_ptr = realloc(a, b)) ) { \
      free(a); \
      return 0; \
    } \
    a = tmp_ptr; \
  } \
}

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

/******************************************************************************
 * G L O B A L S **************************************************************
 ******************************************************************************/

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** create a yada prepare statement struct
 * @return pointer to the struct, or free's passed struct and returns NULL
 */

yada_prep_t* _prep_ele_new(void)
{
  yada_prep_t *yprep;


  if(!(yprep = malloc(sizeof(yada_prep_t) +
                     (sizeof(prep_ele_t) * PREP_ELE_CHUNK_SZ))))
    return(0);

  yprep->sz = PREP_ELE_CHUNK_SZ;
  yprep->len = 0;
  yprep->args = yprep->eles = 0;
  return(yprep);
}

/******************************************************************************/
/** grow a yada prepare statement struct
 * @param yprep struct to grow
 * @return pointer to the struct, or free's passed struct and returns NULL
 */

yada_prep_t* _prep_ele_grow(yada_prep_t *yprep)
{
  size_t sz = yprep->sz + PREP_ELE_CHUNK_SZ;
  yada_prep_t *tmp_ptr;


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
/** free a yada prepare statement struct
 * @param yprep struct to free
 */

void _yada_free_stmt(yada_t *_yada, yada_rc_t *_yrc)
{
  prep_ele_t *elep, *elelen;


  elep = ((yada_prep_t *)_yrc->data)->ele;
  elelen = elep + ((yada_prep_t *)_yrc->data)->eles;

  /* free all static bufs */
  for(; elep < elelen; elep++)
    if(!elep->t)
      free(elep->buf);

  free(_yrc->data);
}

/******************************************************************************/
/** alloc and create a string from fmt
 * shouldn't be called directly
 */

static inline char* _fmtdup(yada_t* _yada, size_t *rlen, const char *fmt,
 va_list ap)
{
  size_t len;
  char *str;


  if((len = vsnprintf(NULL, 0, fmt, ap)) < 1)
    {
    _yada_set_yadaerr(_yada, YADA_EINVAL);
    return(NULL);
    } 

  /* account for terminating NULL */
  len++;

  if(!(str = malloc(len)))
    {
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    return(NULL);
    }

  if(vsnprintf(str, len, fmt, ap) != len - 1)
    {
    _yada_set_yadaerr(_yada, YADA_EINVAL);
    return(NULL);
    }

  *rlen = len;
  return(str);
}

/******************************************************************************/
/** yada compat function to prepare an sql statement from a string
 * shouldn't be called directly
 */

static inline yada_rc_t* _yada_str_prepare(yada_t *_yada, char *sqlstr)
{
  char *buf, *bufp;
  yada_prep_t *yprep;
  yada_rc_t *_yrc;


  /* create prep struct */
  if(!(yprep = _prep_ele_new()))
    return(0);

  buf = sqlstr;
  while((bufp = strchr(buf, '?')))
    {
    /* grow if needed */
    if(yprep->eles == yprep->sz)
      if( !(yprep = _prep_ele_grow(yprep)) )
        return(0);

    /* char '?', break string and start new one */
    if(*++bufp == '?')
      {
      yprep->ele[yprep->eles].t = 0;
      yprep->ele[yprep->eles].len = bufp - buf;
      yprep->ele[yprep->eles].buf = strndup(buf, yprep->ele[yprep->eles].len);
      yprep->len += yprep->ele[yprep->eles].len;
      buf = bufp + 1;
      yprep->eles++;

      }
    /* new arg */
    else
      {
      /* add static string */
      yprep->ele[yprep->eles].t = 0;
      yprep->ele[yprep->eles].len = bufp - buf - 1;
      yprep->ele[yprep->eles].buf = strndup(buf, yprep->ele[yprep->eles].len);
      yprep->len += yprep->ele[yprep->eles].len;

      /* grow if needed */
      if(++yprep->eles == yprep->sz)
        if( !(yprep = _prep_ele_grow(yprep)) )
          return(0);

      /* add arg */
      yprep->args++;
      yprep->ele[yprep->eles].t = *bufp;
      yprep->ele[yprep->eles].buf = 0;
      yprep->ele[yprep->eles].len = 0;
      buf = bufp + 1;
      yprep->eles++;
      }
    }

  /* get last static part if there is one */
  if((yprep->ele[yprep->eles].len = strlen(buf)))
    {
    yprep->ele[yprep->eles].t = 0;
    yprep->ele[yprep->eles].buf = strndup(buf, yprep->ele[yprep->eles].len);
    yprep->len += yprep->ele[yprep->eles].len;
    yprep->eles++;
    }

  if(!(_yrc = _yada_rc_new(_yada)))
    {
    free(yprep);
    return(0);
    }

  _yrc->t = YADA_STATEMENT;
  _yrc->data = yprep;
  return(_yrc);
}

/******************************************************************************/
/** yada compat function to prepare an sql statement with length
 * shouldn't be called directly
 */

static inline yada_rc_t* _yada_len_prepare(yada_t *_yada, char *sqlstr,
                                           size_t sqlstr_len)
{
  char *buf, *bufp;
  yada_prep_t *yprep;
  yada_rc_t *_yrc;


  /* create prep struct */
  if(!(yprep = _prep_ele_new()))
    return(0);

  buf = sqlstr;
  while((bufp = strchr(buf, '?')))
    {

    /* grow if needed */
    if(yprep->eles == yprep->sz)
      if(!(yprep = _prep_ele_grow(yprep)))
        return(0);

    /* char '?', break string and start new one */
    if(*++bufp == '?')
      {
      yprep->ele[yprep->eles].t = 0;
      yprep->ele[yprep->eles].len = bufp - buf;
      yprep->ele[yprep->eles].buf = strndup(buf, yprep->ele[yprep->eles].len);
      yprep->len += yprep->ele[yprep->eles].len;
      buf = bufp + 1;
      yprep->eles++;

      }
    /* new arg */
    else
      {
      /* add static string */
      yprep->ele[yprep->eles].t = 0;
      yprep->ele[yprep->eles].len = bufp - buf - 1;
      yprep->ele[yprep->eles].buf = strndup(buf, yprep->ele[yprep->eles].len);
      yprep->len += yprep->ele[yprep->eles].len;

      /* grow if needed */
      if(++yprep->eles == yprep->sz)
        if(!(yprep = _prep_ele_grow(yprep)))
          return(0);

      /* add arg */
      yprep->args++;
      yprep->ele[yprep->eles].t = *bufp;
      yprep->ele[yprep->eles].buf = 0;
      yprep->ele[yprep->eles].len = 0;
      buf = bufp + 1;
      yprep->eles++;
      }
    }

  /* get last static part if there is one */
  if((yprep->ele[yprep->eles].len = strlen(buf)))
    {
    yprep->ele[yprep->eles].t = 0;
    yprep->ele[yprep->eles].buf = strndup(buf, yprep->ele[yprep->eles].len);
    yprep->len += yprep->ele[yprep->eles].len;
    yprep->eles++;
    }

  if(!(_yrc = _yada_rc_new(_yada)))
    {
    free(yprep);
    return(0);
    }

  _yrc->t = YADA_STATEMENT;
  _yrc->data = yprep;
  return(_yrc);
}

/******************************************************************************/
/** yada compat function to prepare an sql statement
 */

yada_rc_t* _yada_prepare(yada_t *_yada, char *sqlstr, size_t sqlstr_len)
{
  if(sqlstr_len)
    return(_yada_len_prepare(_yada, sqlstr, sqlstr_len));

  return(_yada_str_prepare(_yada, sqlstr));
}

/******************************************************************************/
/** yada compat function to prepare a formated sql statement
 */

yada_rc_t* _yada_preparef(yada_t *_yada, char *fmt, ...)
{
  size_t len;
  char *sqlstr;
  va_list ap;
  yada_rc_t *_yrc;

  va_start(ap, fmt);
  sqlstr = _fmtdup(_yada, &len, fmt, ap);
  va_end(ap);

  if(!sqlstr)
    return(NULL);

  _yrc = _yada->yprepare(_yada, sqlstr, len);
  free(sqlstr);
  return(_yrc);
}

/******************************************************************************/
/** sprintf then native prepare
 */

yada_rc_t* _yada_npreparef(yada_t *_yada, char *fmt, ...)
{
  size_t len;
  char *sqlstr;
  va_list ap;
  yada_rc_t *_yrc;


  va_start(ap, fmt);
  sqlstr = _fmtdup(_yada, &len, fmt, ap);
  va_end(ap);

  if(!sqlstr)
    return(NULL);

  _yrc = _yada->prepare(_yada, sqlstr, len);
  free(sqlstr);
  return(_yrc);
}

/******************************************************************************/
/** extended prepare
 */

yada_rc_t* _yada_xprepare(yada_t *_yada, int flags, char *fmt, ...)
{ 
  size_t len;
  char *sqlstr;
  va_list ap;
  yada_rc_t *_yrc;

  va_start(ap, fmt);

  if(flags & YADA_FORMAT)
    sqlstr = _fmtdup(_yada, &len, fmt, ap);
  else
    {
    sqlstr = fmt;
    len = va_arg(ap, int);
    }

  va_end(ap);

  if(!sqlstr)
    return(NULL);

  _yrc = _yada_len_prepare(_yada, sqlstr, len);
  if(sqlstr != fmt)
    free(sqlstr);
  return(_yrc);
}

/******************************************************************************/
/** checks a buffer for correct size, growing if needed
 *  @param buf buffer
 *  @param sz current buffer size
 *  @param len length needed
 *  @return 0 on error, frees buf
 */

static inline char* _yada_grow_buf(char *buf, size_t *sz, size_t len)
{
  char *tmp_ptr;


  if(len < *sz)
    return buf;

  /* double size */
  *sz += (len << 1);
  if(!(tmp_ptr = realloc(buf, *sz)))
    {
    free(buf);
    return 0;
    }
  return tmp_ptr;
}

/******************************************************************************/
/** insert an escaped string or binary value onto the query string
 * should not be called directly
 */

static inline int _yada_ins_esc(yada_t *_yada, char **qstr, size_t *sz, size_t *len,
                                char *arg, size_t arglen)
{
  int dlen = (arglen <<1) + 1;


  if(!(*qstr = _yada_grow_buf(*qstr, sz, (*len + dlen))))
    return 0;

  if(!_yada->escstr(arg, arglen, *qstr + *len, &dlen))
    {
    free(*qstr);
    return 0;
    }

  /* set len to correct end of qstr, taking off NULL termination */
  *len += dlen;
  return 1;
}

/******************************************************************************/
/** insert an escaped string or binary value onto the query string
 * should not be called directly
 */

static inline int _yada_ins_var(yada_t *_yada, char **qstr, size_t *sz, size_t *len,
                                char *arg, size_t arglen)
{
  size_t dlen = (arglen <<1) + 3;
  char *str;


  if(!(*qstr = _yada_grow_buf(*qstr, sz, (*len + dlen))))
    return(0);

  str = *qstr + *len;
  *str++ = '\'';
  if(!_yada->escstr(arg, arglen, str, &dlen))
    {
    free(*qstr);
    return(0);
    }
  *(str + dlen) = '\'';

  /* set len to correct end of qstr, taking off NULL termination */
  *len += dlen + 2;
  return(1);
}

/******************************************************************************/
/** insert a double onto the query string
 *  should not be called directly
 */

static inline int _yada_ins_double(yada_t *_yada, char **qstr, size_t *sz,
 size_t *len, double arg)
{
  int dlen, alen;


  /* try to print to string, grow if there isn't enough space */
  while((dlen = snprintf(*qstr + *len, (alen = *sz - *len), "%g", arg)) >= alen)
    if(!(*qstr = _yada_grow_buf(*qstr, sz, (*len + dlen))))
      return(0);

  /* check for snprintf error */
  if(dlen < 0)
    return(0);

  /* set len to correct end of qstr */
  *len += dlen;
  return(1);
}

/******************************************************************************/

inline char* _yada_parse_exec(yada_t *_yada, yada_prep_t *_ypr,
                                     size_t *rlen, va_list ap)
{
  size_t len = 0, i, arglen, nlen, sz;
  char *qstr, *bufp, *arg;
  prep_ele_t *elep, *elelen;


  elep = _ypr->ele;
  elelen = elep + _ypr->eles;
  sz = _ypr->len << 1;

  if(!(qstr = calloc(1, sz)))
    {
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    return(0);
    }

  /* set now incase first token expects it */
  bufp = qstr;

  /* make qstr */
  for(; elep < elelen; elep++)
    {
    switch(elep->t)
      {
    case 0:
      nlen = len + elep->len;
      GROW_BUF_IF_NEEDED(qstr, sz, nlen);
      bufp = qstr + len;
      memcpy(bufp, elep->buf, elep->len);
      len = nlen;
      break;

    /* escaped binary (string with length) */
    case 'a':
      arg = va_arg(ap, char *);
      arglen = strlen(arg);

      if(!_yada_ins_esc(_yada, &qstr, &sz, &len, arg, arglen))
        return(0);
      break;

    /* binary (string with length) */
    case 'b':
      arg = va_arg(ap, char *);
      arglen = va_arg(ap, int);
      nlen = len + arglen;
      GROW_BUF_IF_NEEDED(qstr, sz, nlen);
      bufp = qstr + len;
      memcpy(bufp, arg, arglen);
      len = nlen;
      break;

    /* number */
    case 'd':
      {
      int next;
      char dest[STRLEN_INT];
      char *destp, *endp;

      i = va_arg(ap, int);

      /* always have at least 1 spare byte, no need to check overflow */
      if(i < 0)
        {
        *(qstr + len++) = '-';
        i = -i;
        }

      destp = endp = &dest[sizeof(dest) - 1];

      next = i / 10;
      *destp = '0' + (i - ((next << 3) + (next << 1)));
      i = next;

      for(; i; i = next)
        {
        next = i / 10;
        *--destp = '0' + (i - ((next << 3) + (next << 1)));
        }

      nlen = len + (arglen = endp - destp + 1);
      GROW_BUF_IF_NEEDED(qstr, sz, nlen);
      bufp = qstr + len;
      memcpy(bufp, destp, arglen);
      len = nlen;
      }
      break;

    /* escaped string */
    case 'e':
      arg = va_arg(ap, char *);
      arglen = strlen(arg);

      if(!_yada_ins_esc(_yada, &qstr, &sz, &len, arg, arglen))
        return(0);
      break;

    /* 64 bit number */
    case 'l':
      {
      int next;
      int64_t l;
      char dest[STRLEN_INT64];
      char *destp, *endp;

      l = va_arg(ap, int64_t);

      /* always have at least 1 spare byte, no need to check overflow */
      if(l < 0)
        {
        *(qstr + len++) = '-';
        l = -l;
        }

      destp = endp = &dest[sizeof(dest) - 1];

      next = l / 10;
      *destp = '0' + (l - ((next << 3) + (next << 1)));
      l = next;

      for(; l; l = next)
        {
        next = l / 10;
        *--destp = '0' + (l - ((next << 3) + (next << 1)));
        }

      nlen = len + (arglen = endp - destp + 1);
      GROW_BUF_IF_NEEDED(qstr, sz, nlen);
      bufp = qstr + len;
      memcpy(bufp, destp, arglen);
      len = nlen;
      }
      break;

    /* string */
    case 's':
      arg = va_arg(ap, char *);
      nlen = len + (arglen = strlen(arg));
      GROW_BUF_IF_NEEDED(qstr, sz, nlen);
      bufp = qstr + len;
      memcpy(bufp, arg, arglen);
      len = nlen;
      break;

    /* escaped string variable (NULLable) */
    case 'v':

      /* null */
      if(!(arg = va_arg(ap, char *)))
        {
        nlen = len + 4;
        GROW_BUF_IF_NEEDED(qstr, sz, nlen);
        bufp = qstr + len;
        memcpy(bufp, "NULL", 4);
        len = nlen;
        break;
        }

      arglen = strlen(arg);

      if(!_yada_ins_var(_yada, &qstr, &sz, &len, arg, arglen))
        return 0;
      break;

    /* double */
    case 'f':
      if(!_yada_ins_double(_yada, &qstr, &sz, &len, va_arg(ap, double)))
        return(0);
      break;
    }
  }

  qstr[len] = 0;
  if(rlen)
    *rlen = len;
  return(qstr);
}

/******************************************************************************/
/** yada compat function to execute a prepared statment
 * @returns number of rows affected
 */

int _yada_vexecute(yada_t *_yada, void *magic, va_list ap)
{
  size_t len;
  int rv;
  char *qstr;
  yada_rc_t *rc;
  
  /* check for string */
  if(((yada_rc_t *)magic)->magic)
    {
    len = va_arg(ap, int);
    return(_yada->_priv->exec(_yada, magic, len));
    }

  rc = (yada_rc_t *)magic;

  switch(rc->t)
    {
  case YADA_PREPARED:
    rv = _yada->_priv->execprep(_yada, rc->data, &len, ap);
    return(rv);
  case YADA_STATEMENT:
    /* create query string */
    qstr = _yada_parse_exec(_yada, rc->data, &len, ap);
    if(!qstr)
      return(0);

    /* exec qstr */
    DEBUGMSG("EXEC: %d [%s]", len, qstr);
    rv = _yada->_priv->exec(_yada, qstr, len);
    free(qstr);
    return(rv);
    }

  /* invalid type if not caught by switch */
  _yada_set_yadaerr(_yada, YADA_EINVAL);
  return(-1);
}

/******************************************************************************/
/** yada compat function to execute a prepared statment
 * @returns number of rows affected
 */

int _yada_execute(yada_t *_yada, void *magic, ...)
{
  size_t rv;
  va_list ap;

  va_start(ap, magic);
  rv = _yada_vexecute(_yada, magic, ap);
  va_end(ap);
  return(rv);
}

/******************************************************************************/
/** yada compat function to execute a prepared statment
 * @returns number of rows affected
 */

int _yada_xexecute(yada_t *_yada, int flags, void *magic, ...)
{
  size_t len;
  int rv;
  char *qstr;
  va_list ap;

  va_start(ap, magic);

  /* check for string */
  if(((yada_rc_t *)magic)->magic)
    {

    if(flags & YADA_FORMAT)
      {
      qstr = _fmtdup(_yada, &len, magic, ap);
      va_end(ap);

      if(!qstr)
        return(-1);

      rv = _yada->_priv->exec(_yada, qstr, len);
      free(qstr);
      return(rv);
      }

    len = va_arg(ap, int);
    va_end(ap);

    return(_yada->_priv->exec(_yada, magic, len));
    }

  /* prepared arg */
  if(!(qstr = _yada_parse_exec(_yada, ((yada_rc_t *)magic)->data, &len, ap)))
    goto Err;

  va_end(ap);

  /* exec qstr */
  DEBUGMSG("EXEC: %d [%s]", len, qstr);
  rv = _yada->_priv->exec(_yada, qstr, len);
  free(qstr);
  return(rv);

Err:
  va_end(ap);
  free(qstr);
  return(-1);
}

/******************************************************************************/
/** yada compat function to execute a prepared statment and return results
 */

yada_rc_t* _yada_vquery(yada_t *_yada, void *magic, va_list ap)
{
  size_t len;
  char *qstr;
  yada_rc_t *qrc;
  yada_rc_t *rc;


  /* check for string */
  if(((yada_rc_t *)magic)->magic)
    {
    len = va_arg(ap, int);
    return(_yada->_priv->query(_yada, magic, len));
    }

  rc = (yada_rc_t *)magic;

  switch(rc->t)
    {
  case YADA_PREPARED:
    qrc = _yada->_priv->queryprep(_yada, ((yada_rc_t *)magic)->data, &len, ap);
    return(qrc);
  case YADA_STATEMENT:
    /* create query string */
    qstr = _yada_parse_exec(_yada, ((yada_rc_t *)magic)->data, &len, ap);
    if(!qstr)
      return(0);

    /* exec qstr */
    DEBUGMSG("EXEC: %d [%s]", len, qstr);
    qrc = _yada->_priv->query(_yada, qstr, len);
    free(qstr);
    return(qrc);
    }

  /* invalid type if not caught by switch */
  _yada_set_yadaerr(_yada, YADA_EINVAL);
  return(NULL);
}

/******************************************************************************/
/** yada compat function to execute a prepared statment and return results
 */

yada_rc_t* _yada_query(yada_t *_yada, void *magic, ...)
{
  va_list ap;
  yada_rc_t *rv;


  va_start(ap, magic);
  rv = _yada_vquery(_yada, magic, ap);
  va_end(ap);
  return(rv);
}

/******************************************************************************/
/** yada compat function to execute a prepared statment and return results
 */

yada_rc_t* _yada_xquery(yada_t *_yada, int flags, void *magic, ...)
{
  size_t len;
  char *qstr;
  va_list ap;
  yada_rc_t *qrc;


  va_start(ap, magic);

  /* check for string */
  if(((yada_rc_t *)magic)->magic)
    {

    if(flags & YADA_FORMAT)
      {

      len = vsnprintf(0, 0, magic, ap);
      len++;

      /* FIXME - no need for null once len_prep is done */
      if(!(qstr = malloc(len)))
        {
        _yada_set_yadaerr(_yada, YADA_ENOMEM);
        return(0);
        }

      vsnprintf(qstr, len, magic, ap);

      qrc = _yada->_priv->query(_yada, qstr, len);
      free(qstr);
      va_end(ap);

      return(qrc);
      }

    len = va_arg(ap, int);
    va_end(ap);

    return(_yada->_priv->query(_yada, magic, len));
    }

  if(!(qstr = _yada_parse_exec(_yada, ((yada_rc_t *)magic)->data, &len, ap)))
    {
    va_end(ap);
    return(0);
    }

  va_end(ap);

  /* exec qstr */
  DEBUGMSG("EXEC: %d [%s]", len, qstr);
  qrc = _yada->_priv->query(_yada, qstr, len);
  free(qstr);
  return(qrc);
}

/******************************************************************************/
/** return a string of prepared statement ready to be executed
 */

char* _yada_dumpexec(yada_t *_yada, size_t *retlen, yada_rc_t *_yrc, ...)
{
  char *qstr;
  va_list ap;


  va_start(ap, _yrc);
  if(!(qstr = _yada_parse_exec(_yada, _yrc->data, retlen, ap)))
    {
    va_end(ap);
    return(0);
    }

  va_end(ap);
  return(qstr);
}

/******************************************************************************
 ******************************************************************************/

