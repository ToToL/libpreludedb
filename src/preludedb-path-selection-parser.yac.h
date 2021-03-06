/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_YY_PRELUDEDB_PATH_SELECTION_PARSER_YAC_H_INCLUDED
# define YY_YY_PRELUDEDB_PATH_SELECTION_PARSER_YAC_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 15 "preludedb-path-selection-parser.yac.y" /* yacc.c:1909  */


#ifndef YY_TYPEDEF_YY_SCANNER_T
# define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#line 52 "preludedb-path-selection-parser.yac.h" /* yacc.c:1909  */

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    tSTRING = 258,
    tIDMEF = 259,
    tNUMBER = 260,
    tERROR = 261,
    tLPAREN = 262,
    tRPAREN = 263,
    tCOLON = 264,
    tCOMMA = 265,
    tSLASH = 266,
    tMIN = 267,
    tMAX = 268,
    tSUM = 269,
    tCOUNT = 270,
    tINTERVAL = 271,
    tAVG = 272,
    tEXTRACT = 273,
    tTIMEZONE = 274,
    tYEAR = 275,
    tQUARTER = 276,
    tMONTH = 277,
    tWEEK = 278,
    tYDAY = 279,
    tMDAY = 280,
    tWDAY = 281,
    tDAY = 282,
    tHOUR = 283,
    tSEC = 284,
    tMSEC = 285,
    tUSEC = 286,
    tORDER_ASC = 287,
    tORDER_DESC = 288,
    tGROUP_BY = 289
  };
#endif
/* Tokens.  */
#define tSTRING 258
#define tIDMEF 259
#define tNUMBER 260
#define tERROR 261
#define tLPAREN 262
#define tRPAREN 263
#define tCOLON 264
#define tCOMMA 265
#define tSLASH 266
#define tMIN 267
#define tMAX 268
#define tSUM 269
#define tCOUNT 270
#define tINTERVAL 271
#define tAVG 272
#define tEXTRACT 273
#define tTIMEZONE 274
#define tYEAR 275
#define tQUARTER 276
#define tMONTH 277
#define tWEEK 278
#define tYDAY 279
#define tMDAY 280
#define tWDAY 281
#define tDAY 282
#define tHOUR 283
#define tSEC 284
#define tMSEC 285
#define tUSEC 286
#define tORDER_ASC 287
#define tORDER_DESC 288
#define tGROUP_BY 289

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 23 "preludedb-path-selection-parser.yac.y" /* yacc.c:1909  */

        int val;
        int flags;
        int error;
        preludedb_selected_object_interval_t interval;
        preludedb_selected_object_type_t type;
        preludedb_selected_object_t *object;

#line 141 "preludedb-path-selection-parser.yac.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (yyscan_t scanner, preludedb_selected_path_t *root);

#endif /* !YY_YY_PRELUDEDB_PATH_SELECTION_PARSER_YAC_H_INCLUDED  */
