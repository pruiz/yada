
/******************************************************************************
 ******************************************************************************/

/** \file yada.c
 * base yada code, contructor
 *
 * $Id: yada.c 155 2007-05-07 21:02:30Z grizz $
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
#include <errno.h>
#include <ltdl.h>

#include "_yada.h"

/******************************************************************************
 * D E F I N E S **************************************************************
 ******************************************************************************/

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

void _yada_free(yada_t *_yada, yada_rc_t *_yrc)
{

  if(!_yrc)
    return;

  /* call module specific free */
  if(_yada->_priv->free_rc[_yrc->t])
    _yada->_priv->free_rc[_yrc->t](_yada, _yrc);

  /* remove from chain */
  if(_yrc->next)
    _yrc->next->prev = _yrc->prev;

  if(_yrc->prev)
    _yrc->prev->next = _yrc->next;

  if(_yada->_priv->rc_head == _yrc)
    _yada->_priv->rc_head = _yrc->next;
    
  if(_yada->_priv->rc_tail == _yrc)
    _yada->_priv->rc_tail = _yrc->prev;

  free(_yrc);
}

/******************************************************************************/
/** frees all the resources on a yada object
 * @param _yada yada struct
 * @param type rc t to free (-1 for all)
 */

void _yada_freeall(yada_t *_yada, int type)
{
  yada_rc_t *_yrc;


  /* take care of all rc's */
  if(type == -1)
    {
    for(_yrc = _yada->_priv->rc_head; _yada->_priv->rc_head;
        _yrc = _yada->_priv->rc_head)
      _yada->free(_yada, _yrc);

    _yada->_priv->rc_head = _yada->_priv->rc_tail = 0;
    return;
    }

  for(_yrc = _yada->_priv->rc_head; _yada->_priv->rc_head;
      _yrc = _yada->_priv->rc_head)
    if(_yrc->t & type)
      _yada->free(_yada, _yrc);
}

/******************************************************************************/
/** destroys a yada obj, freeing all the resources associated with it
 * @param _yada yada struct
 */

void _yada_destroy(yada_t *_yada)
{
  yada_rc_t *_yrc;

  if (!_yada)
    return;

  /* take care of all rc's */
  for(_yrc = _yada->_priv->rc_head; _yada->_priv->rc_head;
      _yrc = _yada->_priv->rc_head)
    _yada->free(_yada, _yrc);

  /* module specific destroy
   * might not be installed if we failed in yada_init()
   */
  if (_yada->_priv->destroy)
    _yada->_priv->destroy(_yada);

  if(_yada->_priv->dlh)
    lt_dlexit();
  free(_yada->dbtype);
  free(_yada);
  return;
}

/******************************************************************************/
/** initialize yada module
 * @param yada_type db type string
 * @param init flags
 */

yada_t* yada_init(const char *dbstr, unsigned int flags)
{
  int save_errno;
  size_t len;
  char *dbtype, *fsep, *dlname = NULL;
  yada_t *_yada = NULL;
  yada_mod_init_t mod_init;


  /* alloc base and private structs */
  if (!(_yada = calloc(1, sizeof(yada_t) + sizeof(yada_priv_t))))
    return(NULL);
  _yada->_priv = ((void *)_yada) + sizeof(yada_t);

  if(!(_yada->dbtype = dbtype = strdup(dbstr)))
    goto Err;

  /* find db type */
  if(!(fsep = strchr(dbtype, ':')))
    {
    /* empty db string */
    if(!strlen(dbstr))
      goto ErrInval;
    _yada->dbstr = NULL;
    len = strlen(dbstr);
    }
  else
    {
    *fsep = 0;
    len = fsep - dbtype;
    _yada->dbstr = fsep + 1;
    }

  /* alloc dlname: libyada_ + type len + \0 */
  if(!(dlname = malloc(len + 9)))
    {
    DEBUGMSG("malloc failed for module name");
    goto Err;
    }

  /* make yada mod lib name and dlopen it */
  sprintf(dlname, "libyada_%s", dbtype);

  if(lt_dlinit())
    {
    DEBUGMSG("lt_dlinit failed");
    goto Err;
    }

  /* open lib, if fails clean up libtool */
  if(!(_yada->_priv->dlh = lt_dlopenext(dlname)))
    {
    save_errno = errno;
    lt_dlexit();
    DEBUGMSG("dlopen for '%s' failed: %s", dbtype, lt_dlerror());
    errno = save_errno;
    goto Err;
    }

  if(!(mod_init = lt_dlsym(_yada->_priv->dlh, "yada_mod_init")))
    {
    DEBUGMSG("yada_%s dlsym failed on yada_mod_init: %s", dbtype, lt_dlerror());
    goto Err;
    }

  /* init rest of yada struct */
  _yada->free = _yada_free;
  _yada->freeall = _yada_freeall;
  _yada->destroy = _yada_destroy;
  _yada->errmsg = _yada->_priv->errbuf;

  /* initialize module specific struct */
  if(!mod_init(_yada))
    {
    DEBUGMSG("yada_%s yada_mod_init failed", dbtype);
    goto Err;
    }

  free(dlname);
  return(_yada);

ErrInval:
  errno = EINVAL;

Err:
  save_errno = errno;
  _yada_destroy(_yada);
  free(dlname);
  errno = save_errno;
  return(NULL);
}

/******************************************************************************
 ******************************************************************************/

