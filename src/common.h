
/******************************************************************************
 ******************************************************************************/

#ifndef __UTIL_H__
#define __UTIL_H__

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
 * P R O T O T Y P E S ********************************************************
 ******************************************************************************/

char *_yada_escstr(char *, int, char *, int *);

yada_rc_t* _yada_prepare(yada_t *, char *, int);
yada_rc_t* _yada_preparef(yada_t *, char *, ...);
yada_rc_t* _yada_npreparef(yada_t *, char *, ...);
yada_rc_t* _yada_xprepare(yada_t *, int, char *, ...);

int _yada_vexecute(yada_t *, void *, va_list);
int _yada_execute(yada_t *, void *, ...);
int _yada_xexecute(yada_t *, int, void *, ...);

yada_rc_t* _yada_vquery(yada_t *, void *, va_list);
yada_rc_t* _yada_query(yada_t *, void *, ...);
yada_rc_t* _yada_xquery(yada_t *, int, void *, ...);

char* _yada_dumpexec(yada_t *, int *, yada_rc_t *, ...);
void _yada_free_stmt(yada_t *, yada_rc_t *);

yada_rc_t* _yada_vbind(yada_t *, char *, va_list);
yada_rc_t* _yada_bind(yada_t *, char *, ...);

void yada_free_bindset(yada_t *, yada_rc_t *);

char* _yada_parse_exec(yada_t *_yada, yada_prep_t *_ypr, int *rlen, va_list ap);

/******************************************************************************/

#endif /* end __YADA_H__ */

/******************************************************************************
 ******************************************************************************/

