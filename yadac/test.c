
/******************************************************************************
 ******************************************************************************/

/** \file test.c
 *  yada compiler test
 *
 * $Id: test.c 177 2007-07-05 16:48:27Z grizz $
 */

/******************************************************************************
 * I N C L U D E S ************************************************************
 ******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "test.yada.h"

/******************************************************************************
 * M A I N ********************************************************************
 ******************************************************************************/

int main(void)
{
  char name[256] = {0};
  char *str1 = NULL;
  int err = 0;
  int *var2 = NULL;
  int id, var1;
  double flt;
  yada_t *db;
  testruct_t strct;


  /* connect to database */
  if(!(db = yada_init("mysql:localhost::test", 0)))
    {
    printf("error initializing db connection\n");
    return(-1);
    }
  if(!db->connect(db, "USER", "PASS") && (db->error == -1))
    {
    printf("db connect error: %s\n", db->errmsg);
    return(-1);
    }

  /* init compiled queries */
  if(!test_init(NULL, db))
    {
    printf("query init failed\n");
    return(-1);
    }

  if(test_create_table(NULL, "ttab"))
    {
    printf("table creation failed: %s\n", db->errmsg);
    return(-1);
    }

  /* add some data */
  err |= (test_addrow(NULL, "test1", 100, 2.5, NULL) != 1);
  err |= (test_addrow(NULL, "test2", 100, 2.5, "stuff and things") != 1);
  
  strcpy(strct.name, "test3");
  strct.var1 = 100;
  strct.flt = 9.5;
  strct.str1 = "part 3";
  err |= (test_addrowstruct(NULL, &strct) != 1);

  /* check for errors */
  if(err)
    {
    printf("errors inserting data: %s\n", db->errmsg);
    test_drop_table(NULL);
    return(-1);
    }

  /* output something */
  printf("id of test1: %i\n", test_getid(NULL, 0, "test1"));
  printf("id of test2: %i\n", test_getid(NULL, 0, "test2"));

  if(!test_findrow(NULL, 2, 100, &id, name, &var1, &flt, &var2, &str1))
    printf("row: [%i, %s, %i, %f, %i, %s]\n",
     id, name, var1, flt, (var2 ? *var2 : -1), str1);

  if(!test_findstruct(NULL, 2, 100, &strct))
    printf("struct: [%i, %s, %i, %f, %i, %s]\n", strct.id,
     strct.name, strct.var1, strct.flt, (strct.var2 ? *strct.var2 : -1),
     strct.str1);

  if(test_findmatch(NULL, 100, &id, &str1))
    {
    printf("errors with multirow: %s\n", db->errmsg);
    test_drop_table(NULL);
    return(-1);
    }

  while(test_findmatch_fetch(NULL))
    printf("multi: [%i, %s]\n", id, str1);

  test_free(NULL);

  if(test_matchstruct(NULL, 100, &strct))
    {
    printf("errors with multirow: %s\n", db->errmsg);
    test_drop_table(NULL);
    return(-1);
    }

  while(test_findmatch_fetch(NULL))
    printf("multistruct: [%i, %s, %i, %i, %s]\n", strct.id,
     strct.name, strct.var1, (strct.var2 ? *strct.var2 : -1), strct.str1);


  /* cleanup and exit */
  test_drop_table(NULL);
  test_destroy(NULL);
  db->destroy(db);
  return(0);
}

/******************************************************************************
 ******************************************************************************/

