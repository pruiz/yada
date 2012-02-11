
/****************************************************************************
 ****************************************************************************/

/****************************************************************************
 * L I C E N S E ************************************************************
 ****************************************************************************/

/****************************************************************************
 * I N C L U D E S **********************************************************
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <yada.h>

/****************************************************************************
 * D E F I N E S ************************************************************
 ****************************************************************************/

#define STR_CNT(a) a, sizeof(a) - 1

/****************************************************************************
 * S Q L ********************************************************************
 ****************************************************************************/

#define SQL_PREP_INS \
 "insert into yada_test (id, stdint, longint, str, esc, vnull, v, f) " \
 "values (?d, ?d, ?l, ?v, ?v, ?v, ?v, ?f)"

#define SQL_PREPF_INS \
 "insert into %s (id, stdint, longint, str, esc, vnull, v, f) " \
 "values (?d, ?d, ?l, ?v, ?v, ?v, ?v, ?f)"

#define SQL_SELECT \
 "select id, stdint, longint, str, esc, vnull, v, f from yada_test"

#define SQL_YPREP_INS \
 "insert into yada_test values (?d, ?d, ?l, '?s', '?e', '?b', ?v, ?v, ?f)"

#define SQL_YPREPF_INS \
 "insert into %s values (?d, ?d, ?l, '?s', '?e', '?b', ?v, ?v, ?f)"

#define SQL_EXECF_INS \
 "insert into yada_test values " \
 "(%d, %d, %lld, '%s', NULL, NULL, NULL, NULL, %g)"

/****************************************************************************
 * T Y P E D E F S **********************************************************
 ****************************************************************************/

typedef struct
{
  int i;
  long long l;
  char s[60];
  char e[60];
  char b[60];
  int blen;
  int vnull;
  char v[60];
  double f;
} insval_t;

typedef struct
{
  int *id;
  int *i;
  long long *l;
  char *s;
  char *e;
  char *b;
  int blen;
  char *vnull;
  char *v;
  double *f;
} bindptr_t;

/****************************************************************************
 * M A C R O S **************************************************************
 ****************************************************************************/

#define errmsg(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }
#define errdie(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); \
                      exit(1); \
                    }

#define test_start(...) printf(__VA_ARGS__); printf("..."); ttl_tests++;
#define test_pass() { printf("ok\n"); return(1); }
#define test_passret(a) { printf("ok\n"); return(a); }
#define test_fail(...) { printf("FAILED\n    "); printf(__VA_ARGS__); \
                         printf("\n\n"); return(0); }

#define grade(a) \
    if(a == 100) \
      printf("Success!!\n"); \
    else if(!a) \
      printf("Failure!!\n"); \
    else \
      printf("Limited Success!!\n");

/****************************************************************************
 * G L O B A L S ************************************************************
 ****************************************************************************/
int ttl_tests = 0;
static char spinner[] = "/-\\|";
static int s = 0;

static insval_t insval[] =
{
  {0, 0x100000000LL, "zero", "zer'o", STR_CNT("binary data, wooo eh"), 0,
   "'\\'//\\//\\", 1.5},
  {1, -2, "one", "one/;/;/;\\", STR_CNT("binary data, woo eh"), 0,
   "'''''''''", 2.75},
  {2, 0x100000000LL, "two", "t'\"'''wo", STR_CNT(" woo eh"), 0, "'';;;;;;;'",
   3.25},
  {3, 0x100000000LL, "Nthree", "nu'll", STR_CNT("binary data, woo eh"), 0, ";.;.;.;.;.", 9.5},
  {0},
};
int insval_rows = 0;

static char mysql_create_table[] = "create table yada_test "
 "(id int not null, stdint int, longint bigint, str varchar(255), "
 "esc varchar(255), bin blob, vnull varchar(255), v varchar(255), "
 "f decimal(9,2), "
 "primary key(id)) type=InnoDB";
static char oracle_create_table[] = "create table yada_test "
 "(id number primary key, stdint number, longint number(21,0), "
 "str varchar2(255), esc varchar2(255), bin varchar2(255), "
 "vnull varchar2(255), v varchar2(255), f decimal(9,2)"
 ")";
static char pgsql_create_table[] = "create table yada_test "
 "(id integer primary key, stdint integer, longint bigint, str varchar(255), "
 "esc varchar(255), bin varchar(255), vnull varchar(255), v varchar(255), "
 "f decimal(9,2)"
 ")";
static char sqlite3_create_table[] = "create table yada_test "
 "(id int, stdint int, longint int, str varchar(255), esc varchar(255), "
 "bin blob, vnull varchar(255), v varchar(255), f numeric(9,2), "
 "primary key(id));";

static char *create_table;
static char drop_table[] = "drop table yada_test";

/* database auth info */
static char *dbuser = NULL;
static char *dbpass = NULL;

/****************************************************************************
 * F U N C T I O N S ********************************************************
 ****************************************************************************/

/******************************************************************************/
/* displays a status line with spinner - always returns true
 * function - so it can be put anywhere
 * sleep - what's the point of spinner if you don't get to see it spin
 */

int spin(void)
{
  printf("%c\E[D", spinner[s++ & 3]);
  fflush(stdout);
  usleep(2500);
  return(1);
}

/******************************************************************************/
/* compare values */

int compare_val(insval_t *src, insval_t *dst)
{
  if(src->i != dst->i)
    test_fail("%d: NEQ: %d <=> %d", src->i, src->i, dst->i);
  if(src->l != dst->l)
    test_fail("%d: NEQ: %lld <=> %lld", src->i, dst->l, src->l);
  if(strcmp(src->s, dst->s))
    test_fail("%d: NEQ: %s <=> %s", src->i, dst->s, src->s);
  if(strcmp(src->e, dst->e))
    test_fail("%d: NEQ: %s <=> %s", src->i, dst->e, src->e);
  if(strcmp(src->v, dst->v))
    test_fail("%d: NEQ: %s <=> %s", src->i, dst->v, src->v);
  if(src->f != dst->f)
    test_fail("%d: NEQ: %g <=> %g", src->i, dst->f, src->f);
  return(1);
}

/******************************************************************************/
/* compare values */

int compare_ptr(insval_t *src, bindptr_t *dst)
{
  if(src->i != *dst->i)
    test_fail("%d: NEQ: %d <=> %d", src->i, src->i, *dst->i);
  if(src->l != *dst->l)
    test_fail("%d: NEQ: %lld <=> %lld", src->i, src->l, *dst->l);
  if(strcmp(src->s, dst->s))
    test_fail("%d: NEQ: %s <=> %s", src->i, src->s, dst->s);
  if(strcmp(src->e, dst->e))
    test_fail("%d: NEQ: %s <=> %s", src->i, src->e, dst->e);
  if(strcmp(src->v, dst->v))
    test_fail("%d: NEQ: %s <=> %s", src->i, src->v, dst->v);
  if((char *)src->vnull != dst->vnull)
    test_fail("%d: NEQ: %d <=> %d", src->i, src->vnull, (intptr_t)dst->vnull);
  return(1);
}

/******************************************************************************/
/* init yada */

yada_t* ytest_init(char *type)
{
  int flags = 0;
  char *dbstr;
  yada_t *yada;


  test_start("* initializing yada_%s", type);

  /* find module and set specifics */
  if(spin() && !strcmp("mysql", type))
    {
    dbstr = "mysql:localhost::test";
    dbuser = "test";
    dbpass = "";
    create_table = mysql_create_table;
    }
  else if(spin() && !strcmp("oracle", type))
    {
    dbstr = "oracle:orcl";
    dbuser = "test";
    dbpass = "test";
    create_table = oracle_create_table;
    }
  else if(spin() && !strcmp("pgsql", type))
    {
    dbstr = "pgsql:::test";
    dbuser = "test";
    dbpass = "";
    create_table = pgsql_create_table;
    }
  else if(spin() && !strcmp("sqlite3", type))
    {
    dbstr = "sqlite3:.sqlite3_test";
    create_table = sqlite3_create_table;
    }
  else
    test_fail("Unknown yada module: skipping tests");

  /* init yada */
  if(!(yada = yada_init(dbstr, flags)))
    test_fail("Failed to initialize module: %s: skipping tests",
     strerror(errno));

  /* try to connect */
  if(!yada->connect(yada, dbuser, dbpass))
    test_fail("Failed to connect: %s: skipping tests", yada->errmsg);

  test_passret(yada);
}

/******************************************************************************/
/* tests execute / creating tables */

int ytest_create(yada_t *yada)
{
  test_start("execute / create table");
  spin();

  yada->execute(yada, drop_table, 0);

  spin();

  if(yada->execute(yada, create_table, 0) == -1)
    test_fail("error creating test table: %s", yada->errmsg);

  yada->freeall(yada, -1);

  test_pass();
}

/******************************************************************************/
/* tests xexecute */

int ytest_xexecute(yada_t *yada)
{
  test_start("xexecute / create table");

  spin();
  yada->xexecute(yada, 0, drop_table, strlen(drop_table));

  spin();
  if(yada->xexecute(yada, 0, create_table, strlen(create_table)) == -1)
    test_fail("failed inserting from xexecute: %s", yada->errmsg);

  test_pass();
}

/******************************************************************************/
/* tests xexecute with format flag */

int ytest_xexecutef(yada_t *yada)
{
  int i;


  test_start("xexecute with format flag");
  spin();

  for(i = 0; i < insval_rows; i++)
    {
    spin();
 
    if(yada->xexecute(yada, YADA_FORMAT, SQL_EXECF_INS, i, insval[i].i,
     insval[i].l, insval[i].s, insval[i].f) == -1)
      test_fail("failed inserting from xexecute: %s", yada->errmsg);
    }

  test_pass();
}

/******************************************************************************/
/* tests native prepared statements and inserting */

int ytest_prepare(yada_t *yada)
{
  int i;
  yada_rc_t *stmt;


  test_start("native prepare / insert");
  spin();

  if(!(stmt = yada->prepare(yada, SQL_PREP_INS, 0)))
    test_fail("failed to prepare statement: %s", yada->errmsg);

  for(i = 0; i < insval_rows; i++)
    {
    spin();
 
    if(yada->execute(yada, stmt, i, insval[i].i, insval[i].l, insval[i].s,
     insval[i].e, insval[i].vnull, insval[i].v, insval[i].f) == -1)
      test_fail("failed inserting from prepare: %s", yada->errmsg);
    }

  yada->free(yada, stmt);
  test_pass();
}

/******************************************************************************/
/* tests dumpexec */

int ytest_dumpexec(yada_t *yada)
{
  int i, len;
  char *qstr;
  yada_rc_t *stmt;


  test_start("dumpexec / execute");
  spin();

  if(!(stmt = yada->yprepare(yada, SQL_YPREP_INS, 0)))
    test_fail("failed to prepare statement: %s", yada->errmsg);

  for(i = 0; i < insval_rows; i++)
    {
    spin();
 
    if(!(qstr = yada->dumpexec(yada, &len, stmt, i, insval[i].i, insval[i].l,
     insval[i].s, insval[i].e, insval[i].b, insval[i].blen, insval[i].vnull,
     insval[i].v, insval[i].f)))
      test_fail("failed to dumpexec: %s", yada->errmsg);

    if(!yada->execute(yada, qstr, len))
      {
      free(qstr);
      test_fail("failed inserting from dumpexec: %s", yada->errmsg);
      }
    free(qstr);
    }

  yada->free(yada, stmt);
  test_pass();
}

/******************************************************************************/
/* tests yada prepared statements and inserting */

int ytest_yprepare(yada_t *yada)
{
  int i;
  yada_rc_t *stmt;


  test_start("prepare / insert");
  spin();

  if(!(stmt = yada->yprepare(yada, SQL_YPREP_INS, 0)))
    test_fail("failed to prepare statement: %s", yada->errmsg);

  for(i = 0; i < insval_rows; i++)
    {
    spin();
 
    if(yada->execute(yada, stmt, i, insval[i].i, insval[i].l, insval[i].s,
     insval[i].e, insval[i].b, insval[i].blen, insval[i].vnull, insval[i].v,
     insval[i].f) == -1)
      test_fail("failed inserting from prepare: %s", yada->errmsg);
    }

  yada->free(yada, stmt);
  test_pass();
}

/******************************************************************************/
/* tests formatted prepared statements */

int ytest_preparef(yada_t *yada)
{
  int i;
  yada_rc_t *stmt;


  test_start("preparef");
  spin();

  if(!(stmt = yada->preparef(yada, SQL_PREPF_INS, "yada_test")))
    test_fail("failed to prepare statement: %s", yada->errmsg);

  for(i = 0; i < insval_rows; i++)
    {
    spin();
 
    if(yada->execute(yada, stmt, i, insval[i].i, insval[i].l, insval[i].s,
     insval[i].e, insval[i].vnull, insval[i].v, insval[i].f) == -1)
      test_fail("failed inserting from prepare: %s", yada->errmsg);
    }

  yada->free(yada, stmt);
  test_pass();
}

/******************************************************************************/
/* tests formatted prepared statements */

int ytest_ypreparef(yada_t *yada)
{
  int i;
  yada_rc_t *stmt;


  test_start("ypreparef");
  spin();

  if(!(stmt = yada->ypreparef(yada, SQL_YPREPF_INS, "yada_test")))
    test_fail("failed to prepare statement: %s", yada->errmsg);

  for(i = 0; i < insval_rows; i++)
    {
    spin();
 
    if(yada->execute(yada, stmt, i, insval[i].i, insval[i].l, insval[i].s,
     insval[i].e, insval[i].b, insval[i].blen, insval[i].vnull, insval[i].v,
     insval[i].f) == -1)
      test_fail("failed inserting from prepare: %s", yada->errmsg);
    }

  yada->free(yada, stmt);
  test_pass();
}

/******************************************************************************/
/* tests binding pointers as query output */

int ytest_bindvar(yada_t *yada)
{
  int id, rows = 0;
  yada_rc_t *yrc, *brc;
  insval_t bindval;


  test_start("bind / fetch - prepared variables");
  spin();

  if(!(brc = yada->bind(yada, "?d?d?l?v?v?v?v?f", &id, &bindval.i, &bindval.l,
   &bindval.s, bindval.e, &bindval.vnull, &bindval.v, &bindval.f)))
    test_fail("error binding output vars: %s", yada->errmsg);

  spin();

  if(!(yrc = yada->query(yada, STR_CNT(SQL_SELECT))))
    test_fail("error exec'ing string: %s", yada->errmsg);

  spin();

  memset(&bindval, 0, sizeof(insval_t));
  while(yada->fetch(yada, yrc, brc))
    {
    spin();
    rows++;

    if(!compare_val(&bindval, &insval[id]))
      return(0);

    memset(&bindval, 0, sizeof(insval_t));
    }

  if(yada->error)
    test_fail("error fetching row %d: %s", rows, yada->errmsg);

  if(rows != insval_rows)
    test_fail("Invalid row count; inserted %d, fetched %d", insval_rows, rows);

  test_pass();
}

/******************************************************************************/
/* tests binding pointers as query output */

int ytest_bindptr(yada_t *yada) 
{
  int id, rows = 0;
  bindptr_t bindptr;
  yada_rc_t *yrc, *brc;


  test_start("bind / fetch - pointers");
  spin();

  if(!(brc = yada->bind(yada, "?pd?pd?pl?ps?ps?pv?pv?pf",
   &bindptr.id, &bindptr.i, &bindptr.l, &bindptr.s, &bindptr.e,
   &bindptr.vnull, &bindptr.v, &bindptr.f)))
    test_fail("error binding output vars: %s", yada->errmsg);

  spin();

  if(!(yrc = yada->query(yada, STR_CNT(SQL_SELECT))))
    test_fail("error exec'ing string: %s", yada->errmsg);

  spin();

  while(yada->fetch(yada, yrc, brc))
    {
    spin();
    rows++;
    id = *bindptr.i;

    if(!compare_ptr(&insval[id], &bindptr))
      return(0);
    }

  if(rows != insval_rows)
    test_fail("Invalid row count; inserted %d, fetched %d", insval_rows, rows);

  yada->free(yada, yrc);
  test_pass();
}

/******************************************************************************/
/* tests binding pointers as query output */

int ytest_ybindvar(yada_t *yada)
{
  int id, rows = 0;
  yada_rc_t *yrc, *brc;
  insval_t bindval;


  test_start("bind / fetch - variables");
  spin();

  if(!(brc = yada->bind(yada, "?d?d?l?s?s?b?v?v?f", &id, &bindval.i, &bindval.l,
   &bindval.s, bindval.e, bindval.b, &bindval.blen, &bindval.vnull,
   &bindval.v, &bindval.f)))
    test_fail("error binding output vars: %s", yada->errmsg);

  spin();

  if(!(yrc = yada->query(yada, STR_CNT("select * from yada_test"))))
    test_fail("error exec'ing string: %s", yada->errmsg);

  spin();

  memset(&bindval, 0, sizeof(insval_t));
  while(yada->fetch(yada, yrc, brc))
    {
    spin();
    rows++;
    id = bindval.i;

    if(!compare_val(&insval[id], &bindval))
      return(0);

    memset(&bindval, 0, sizeof(insval_t));
    }

  if(yada->error)
    test_fail("error fetching row %d: %s", rows, yada->errmsg);

  if(rows != insval_rows)
    test_fail("Invalid row count; inserted %d, fetched %d", insval_rows, rows);

  test_pass();
}


/******************************************************************************/
/* tests binding pointers as query output */

int ytest_ybindptr(yada_t *yada) 
{
  int id, rows = 0;
  bindptr_t bindptr;
  yada_rc_t *yrc, *brc;


  test_start("bind / fetch - pointers");
  spin();

  if(!(brc = yada->bind(yada, "?pd?pd?pl?ps?ps?pb?pv?pv?pf",
   &bindptr.id, &bindptr.i, &bindptr.l, &bindptr.s, &bindptr.e, &bindptr.b,
   &bindptr.blen, &bindptr.vnull, &bindptr.v, &bindptr.f)))
    test_fail("error binding output vars: %s", yada->errmsg);

  spin();

  if(!(yrc = yada->query(yada, STR_CNT("select * from yada_test"))))
    test_fail("error exec'ing string: %s", yada->errmsg);

  spin();

  while(yada->fetch(yada, yrc, brc))
    {
    spin();
    rows++;
    id = *bindptr.i;

    if(!compare_ptr(&insval[id], &bindptr))
      return(0);

    if(insval[id].blen != bindptr.blen)
      test_fail("%d: NEQ: %d <=> %d", id, insval[id].blen, bindptr.blen);
    if((char *)insval[id].vnull != bindptr.vnull)
      test_fail("%d: NEQ: %d <=> %d", id, insval[id].vnull,
       (intptr_t)bindptr.vnull);
    }

  if(rows != insval_rows)
    test_fail("Invalid row count; inserted %d, fetched %d", insval_rows, rows);

  yada->free(yada, yrc);
  test_pass();
}

/******************************************************************************/
/* transaction - rollback */

int ytest_rollback(yada_t *yada) 
{
  int id;
  insval_t bindval;
  yada_rc_t *yrc, *brc;


  test_start("transaction - rollback");
  spin();

  if(yada->trx(yada, 0))
    test_fail("error beginning transaction: %s", yada->errmsg);

  spin();

  if(yada->execute(yada, "delete from yada_test", 0) == -1)
    test_fail("error exec'ing delete string: %s", yada->errmsg);

  spin();

  /* make sure table is empty */
  if(!(brc = yada->bind(yada, "?d?d?s?s?b?v?v?f", &id, &bindval.i, &bindval.s,
   bindval.e, bindval.b, &bindval.blen, &bindval.vnull, &bindval.v,
   &bindval.f)))
    test_fail("error binding output vars: %s", yada->errmsg);

  spin();

  if(!(yrc = yada->query(yada, STR_CNT("select * from yada_test"))))
    test_fail("error query'ing select string: %s", yada->errmsg);

  spin();

  if(yada->fetch(yada, yrc, brc))
    {
    yada->free(yada, yrc);
    test_fail("transactional delete did not delete");
    }

  yada->free(yada, yrc);
  if(yada->rollback(yada, 0))
    test_fail("error rolling back transaction: %s", yada->errmsg);

  test_pass();
}

/******************************************************************************/
/* transaction - commit */

int ytest_commit(yada_t *yada) 
{
  int id;
  insval_t bindval;
  yada_rc_t *yrc, *brc;


  test_start("transaction - commit");
  spin();

  if(yada->trx(yada, 0))
    test_fail("error beginning transaction: %s", yada->errmsg);

  spin();

  if(yada->execute(yada, "delete from yada_test", 0) == -1)
    test_fail("error exec'ing string: %s", yada->errmsg);

  spin();

  if(yada->commit(yada))
    test_fail("error committing transaction: %s", yada->errmsg);

  spin();

  /* make sure table is empty */
  if(!(brc = yada->bind(yada, "?d?d?s?s?b?v?v?f", &id, &bindval.i, &bindval.s,
   bindval.e, bindval.b, &bindval.blen, &bindval.vnull, &bindval.v,
   &bindval.f)))
    test_fail("error binding output vars: %s", yada->errmsg);

  spin();

  if(!(yrc = yada->query(yada, STR_CNT("select * from yada_test"))))
    test_fail("error exec'ing string: %s", yada->errmsg);

  spin();

  if(yada->fetch(yada, yrc, brc))
    test_fail("transactional delete did not delete");

  test_pass();
}

/******************************************************************************/
/* connect / discconect */

int ytest_disco(yada_t *yada)
{

  test_start("connect / disconnect");
  spin();

  yada->disconnect(yada);

  spin();

  if(!yada->connect(yada, dbuser, dbpass))
    test_fail("couldn't connect to database: %s", yada->errmsg);

  spin();
  test_pass();
}

/****************************************************************************
 * M A I N ******************************************************************
 ****************************************************************************/

int main(int argc, char **argv)
{
  int passed = 0;
  int allpassed = 0, allttl_tests = 0;
  float percent;
  char *buf, *modp, *module;
  insval_t *insvalp = insval;
  yada_t *yada;


  printf("\n");
  printf("============================================================\n");
  printf("== yada version %s\n\n", PACKAGE_VERSION);

#ifndef YADA_BUILD_MODULES
  printf("no modules built, nothing to test...\n");
  exit(0);
#endif

  /* FIXME - count rows */
  while(*insvalp->s)
    {
    insval_rows++;
    insvalp++;
    }

  buf = modp = strdup(YADA_BUILD_MODULES);

  /* loop through all built modules */
  while((module = modp))
    {
    passed = ttl_tests = 0;

    if((modp = strchr(module, ' ')))
      *modp++ = 0;

    if(!(yada = ytest_init(module)))
      continue;

    passed++;

    /* create test table */
    passed += ytest_create(yada);

    /* native prepare / insert */
    passed += ytest_prepare(yada);
    passed += ytest_bindvar(yada);
    passed += ytest_bindptr(yada);

    /* yada prepare / insert */
    passed += ytest_create(yada);
    passed += ytest_yprepare(yada);
    passed += ytest_ybindvar(yada);
    passed += ytest_ybindptr(yada);

    /* xexecute */
    passed += ytest_xexecute(yada);
    passed += ytest_xexecutef(yada);
    /* FIXME - should test for proper insert */

    /* preparef */
    passed += ytest_create(yada);
    passed += ytest_preparef(yada);
    passed += ytest_bindvar(yada);
    passed += ytest_bindptr(yada);

    /* ypreparef */
    passed += ytest_create(yada);
    passed += ytest_ypreparef(yada);
    passed += ytest_ybindvar(yada);
    passed += ytest_ybindptr(yada);

    /* transactions */
    yada->freeall(yada, -1);
    passed += ytest_rollback(yada);
    passed += ytest_bindvar(yada);
    passed += ytest_commit(yada);

    /* dumpexec / execute */
    passed += ytest_create(yada);
    passed += ytest_dumpexec(yada);
    passed += ytest_ybindvar(yada);
    passed += ytest_ybindptr(yada);

    /* connect / disconnect */
    passed += ytest_disco(yada);

    yada->destroy(yada);

    if(isnan(percent = (float)passed / ttl_tests * 100))
      percent = 0;
    printf("  * Passed %.1f%% (%d/%d) of %s tests: ", percent, passed,
     ttl_tests, module);
    grade(percent);
    printf("\n");

    /* reset values */
    allpassed += passed;
    allttl_tests += ttl_tests;
    passed = ttl_tests = 0;
  }

  free(buf);

  if(isnan(percent = (float)allpassed / allttl_tests * 100))
    percent = 0;
  printf("\n");
  printf("============================================================\n");
  printf("Passed %.1f%% (%d/%d) of all the tests: ", percent, allpassed,
   allttl_tests);
  if(percent == 100)
    printf("Success!!\n");
  else if(!percent)
    printf("Failure!!\n");
  else
    printf("Limited Success!!\n");
  printf("\n");

  return 0;
}

/****************************************************************************
 ****************************************************************************/

