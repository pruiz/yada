/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     INCLUDE = 258,
     PREPARE = 259,
     YPREPARE = 260,
     RESULT = 261,
     STATUS = 262,
     SIMPLE = 263,
     SINGLE = 264,
     STRUCT = 265,
     MULTIROW = 266,
     MULTISTRUCT = 267,
     INT = 268,
     FLOAT = 269,
     LONG = 270,
     BINARY = 271,
     VARCHAR = 272,
     INTP = 273,
     FLOATP = 274,
     LONGP = 275,
     BINARYP = 276,
     VARCHARP = 277,
     TOKEN = 278,
     PARAM = 279,
     SQL = 280,
     STRING = 281,
     CVAR = 282,
     INDENT = 283
   };
#endif
/* Tokens.  */
#define INCLUDE 258
#define PREPARE 259
#define YPREPARE 260
#define RESULT 261
#define STATUS 262
#define SIMPLE 263
#define SINGLE 264
#define STRUCT 265
#define MULTIROW 266
#define MULTISTRUCT 267
#define INT 268
#define FLOAT 269
#define LONG 270
#define BINARY 271
#define VARCHAR 272
#define INTP 273
#define FLOATP 274
#define LONGP 275
#define BINARYP 276
#define VARCHARP 277
#define TOKEN 278
#define PARAM 279
#define SQL 280
#define STRING 281
#define CVAR 282
#define INDENT 283




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 315 "yadac.y"
{
  char *str;
  int type;
  int num;
  yadac_param_t param;
  yadac_param_t *params;
  yadac_tlist_t *tlist;
  yadac_struct_t *strct;
  yadac_sql_t *sql;
}
/* Line 1489 of yacc.c.  */
#line 116 "yadac.y.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


