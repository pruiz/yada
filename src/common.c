
/******************************************************************************
 ******************************************************************************/

/** \file common.c
 * common yada utility and glue functions
 *
 * $Id: common.c 183 2007-10-17 00:20:32Z grizz $
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
      return(0); \
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

/******************************************************************************/
/** strings for yada errors
 */

const char *_yada_errstrs[] = {
  "Success",
  "Cannot connect to database",
  "Out of memory",
  "Already in a transaction",
  "Invalid argument",
};

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** generic minimal string escaper
 *  outputs null terminated strings
 */

char *_yada_escstr(char *src, size_t slen, char *dest, size_t *dlen)
{
  size_t i, len;
  char *destp;

  printf("====> _yada_escstr\n");

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
      {
      switch(src[i])
        {
      case '\'':
      case '"':
      case '\\':
        *destp++ = '\\';
        }
      *destp++ = src[i];
      }

    *destp++ = 0;

    if(!dlen)
      dlen = &len;

    /* alloc down to only what was used */
    *dlen = destp - dest;
    if((destp = realloc(dest, *dlen)))
      dest = destp;

    /* don't count NULL termination */
    (*dlen)--;

    return(dest);
    }


  destp = dest;

  for(i = 0; i < slen; i++)
    {
    switch(src[i])
      {
    case '\'':
    case '"':
    case '\\':
      *destp++ = '\\';
      }
    *destp++ = src[i];
    }

  *destp++ = 0;
  /* set length, not counting NULL termination */
  if(dlen)
    *dlen = destp - dest - 1;

  DEBUGMSG("[%s] %d", src, slen);
  DEBUGMSG("%d = %u - %u", destp - dest, destp, dest);
  DEBUGMSG("esc'd str: [%s] %d", dest, *dlen);
  return(dest);
}

/******************************************************************************/
/** create a yada bindset struct
 * @returns pointer to the struct
 */

yada_bindset_t* _bindset_new(void)
{
  yada_bindset_t *ybind;


  if(!(ybind = malloc(sizeof(yada_bindset_t) +
                     (sizeof(bindset_ele_t) * PREP_ELE_CHUNK_SZ))))
    return(0);

  ybind->sz = PREP_ELE_CHUNK_SZ;
  ybind->eles = 0;
  return ybind;
}

/******************************************************************************/
/** grow a yada bindset struct
 * @param bindset struct to grow
 * @return pointer to the struct, or free's passed struct and returns NULL
 */

yada_bindset_t* _bindset_ele_grow(yada_bindset_t *ybind)
{
  int sz = ybind->sz + PREP_ELE_CHUNK_SZ;
  yada_bindset_t *tmp_ptr;


  if( !(tmp_ptr = realloc(ybind, sizeof(yada_bindset_t) +
                                (sizeof(bindset_ele_t) * sz))) )
    {
    free(ybind);
    return(0);
    }

  ybind = tmp_ptr;
  ybind->sz = sz;
  return(ybind);
}

/******************************************************************************/

void yada_free_bindset(yada_t *_yada, yada_rc_t *_yrc)
{
  free(_yrc->data);
}

/******************************************************************************/
/** create a new yada resource struct
 * @param _yada yada struct
 */

yada_rc_t* _yada_rc_new(yada_t *_yada)
{
  yada_rc_t *_yrc;


  if(!(_yrc = calloc(1, sizeof(yada_rc_t))))
    {
    _yada_set_yadaerr(_yada, YADA_ENOMEM);
    return(0);
    }

  /* first rc */
  if(!_yada->_priv->rc_head)
    {
    _yada->_priv->rc_head = _yada->_priv->rc_tail = _yrc;
    return(_yrc);
    }

  _yada->_priv->rc_tail->next = _yrc;
  _yrc->prev = _yada->_priv->rc_tail;
  _yada->_priv->rc_tail = _yrc;
  return(_yrc);
}

/******************************************************************************/
/** create bind set from bind vars
 * @param yada yada struct pointer
 * @param fmt string containing the types of the following vars
 * @param ap list of pointers to variables to bind
 */

yada_rc_t* _yada_vbind(yada_t *_yada, char *fmt, va_list ap)
{
  yada_bindset_t *ybind;
  yada_rc_t *_yrc;


  /* create prep struct */
  if(!(ybind = _bindset_new()))
    return(0);

  while((fmt = strchr(fmt, '?')))
    {
    /* grow if needed */
    if(ybind->eles == ybind->sz)
      if(!(ybind = _bindset_ele_grow(ybind)))
        return(0);

    if(*++fmt == 'p')
      ybind->ele[ybind->eles].t = -*++fmt;
    else
      ybind->ele[ybind->eles].t = *fmt;

    ybind->ele[ybind->eles].ptr = va_arg(ap, void *);

    /* get next var for binary types */
    if(*fmt == 'b')
      {

      /* grow if needed */
      if(++ybind->eles == ybind->sz)
        if( !(ybind = _bindset_ele_grow(ybind)) )
          return(0);

      ybind->ele[ybind->eles].ptr = va_arg(ap, void *);
      }

    ybind->eles++;
    }

  if(!(_yrc = _yada_rc_new(_yada)))
    {
    free(ybind);
    return(0);
    }

  _yrc->t = YADA_BINDSET;
  _yrc->data = ybind;
  return(_yrc);
}

/******************************************************************************/
/** create bind set from bind vars
 * @param yada yada struct pointer
 * @param fmt string containing the types of the following vars
 * @param ... list of pointers to variables to bind
 */

yada_rc_t* _yada_bind(yada_t *_yada, char *fmt, ...)
{
  va_list ap;
  yada_rc_t *rv;


  va_start(ap, fmt);
  rv = _yada_vbind(_yada, fmt, ap);
  va_end(ap);
  return(rv);
}

/******************************************************************************/
/** memdup
 */

void* _yada_memdup(void *src, size_t len)
{
  void *buf;


  if(!(buf = malloc(len)))
    return(0);

  return(memcpy(buf, src, len));
}

/******************************************************************************/
/** strndup compat for non gnu libc's
 */

char* _yada_strndup(const char *s, size_t n)
{
  size_t len;
  char *buf;


  for(len=0; len < n; len++) 
    if(!s[len]) 
      break;

  if(!(buf = malloc(len + 1)))
    return(0);

  buf[len] = '\0';
  return((char *)memcpy(buf, s, len));
}

/******************************************************************************/
/** dummy function for unimplemented methods
 */

void *_yada_return_null(yada_t *_yada, ...)
{
  return(NULL);
}

/******************************************************************************/
/** dummy function for unimplemented methods
 */

int _yada_return_zero(yada_t *_yada, ...)
{
  return(0);
}

/******************************************************************************/
/** dummy function for unimplemented methods
 */

void _yada_return(yada_t *_yada, ...)
{
  return;
}

/******************************************************************************
 ******************************************************************************/


