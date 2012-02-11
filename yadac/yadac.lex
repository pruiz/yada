
/******************************************************************************
 ******************************************************************************/

/** \file yadac.lex
 *  scanner for yada binding compiler
 *
 * $Id$
 */

/******************************************************************************/
%{
/******************************************************************************
 * I N C L U D E S ************************************************************
 ******************************************************************************/

#include <string.h>

#include "yadac.h"
#include "yadac.y.h"

/******************************************************************************/
%}
/******************************************************************************
 * D E F I N I T I O N S ******************************************************
 ******************************************************************************/

%option reentrant
%option bison-bridge
%option bison-locations
%option noyywrap
%option yylineno

%x comment sql

SWS		[[:blank:]]+
LWS		[[:space:]]+
COMMENT		(#|(\/\/)).*$
TOKEN		[[:alpha:]_][[:alnum:]_]*
CVAR		(&|\*){TOKEN}
CVAR2		(&|\*)?{TOKEN}((\.|->){TOKEN})+
SQLDATA		[^[:space:]?\]]([^?\]\n]*[^[:space:]?\]])?

/******************************************************************************
 * R U L E S ******************************************************************
 ******************************************************************************/
%%
 /****************************************************************************/
 /** block comments **/

<INITIAL>\/\*		BEGIN(comment);
<comment>([^*]|\*[^/])*	/* ignore */
<comment>\*\/		BEGIN(INITIAL);

 /****************************************************************************/
 /** SQL sections */

^{LWS}?\[{LWS}?\n	{ BEGIN(sql); return('['); }
<sql>^{LWS}?\]{LWS}?\n	{ BEGIN(INITIAL); return(']'); }
<sql>^\ +		{ yylval->num = ((yyleng + 1) >> 1); return(INDENT); }
<sql>\?b		{ yymore(); return(yylval->param = BINARY); }
<sql>\?d		{ yymore(); return(yylval->param = INT); }
<sql>\?f		{ yymore(); return(yylval->param = FLOAT); }
<sql>\?l		{ yymore(); return(yylval->param = LONG); }
<sql>\?(s|e|v)		{ yymore(); return(yylval->param = VARCHAR); }
<sql>\?pb		{ yymore(); return(yylval->param = BINARYP); }
<sql>\?pd		{ yymore(); return(yylval->param = INTP); }
<sql>\?pf		{ yymore(); return(yylval->param = FLOATP); }
<sql>\?pl		{ yymore(); return(yylval->param = LONGP); }
<sql>\?p(s|e|v)		{ yymore(); return(yylval->param = VARCHARP); }
<sql>{SQLDATA}		yymore();
<sql>\n			{ yylval->str = strdup(yytext);
			 yylval->str[yyleng - 1] = 0; return(SQL); }
<sql>.			yymore();

 /****************************************************************************/
 /** keywords **/

include			return(INCLUDE);
prepare			return(PREPARE);
yprepare		return(YPREPARE);
result			return(RESULT);
simple			return(SIMPLE);
single			return(SINGLE);
struct			return(STRUCT);
multirow		return(MULTIROW);
multistruct		return(MULTISTRUCT);
status			return(STATUS);

 /****************************************************************************/
 /** tokens / symbols **/

{CVAR}			{ yylval->str = strdup(yytext); return(CVAR); }
{CVAR2}			{ yylval->str = strdup(yytext); return(CVAR); }
{TOKEN}			{ yylval->str = strdup(yytext); return(TOKEN); }
\?b			return(yylval->param = BINARY);
\?d			return(yylval->param = INT);
\?f			return(yylval->param = FLOAT);
\?l			return(yylval->param = LONG);
\?(s|e|v)		return(yylval->param = VARCHAR);
\?pb			return(yylval->param = BINARYP);
\?pd			return(yylval->param = INTP);
\?pf			return(yylval->param = FLOATP);
\?pl			return(yylval->param = LONGP);
\?p(s|e|v)		return(yylval->param = VARCHARP);
\".+\"			{ yylval->str = strdup(yytext + 1);\
			  yylval->str[yyleng - 2] = 0; return(STRING); }

 /****************************************************************************/

{LWS}			/* ignore */
{COMMENT}		/* ignore */
.			{ return(*yytext); }

 /****************************************************************************/
%%
/******************************************************************************
 ******************************************************************************/

