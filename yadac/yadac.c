
/******************************************************************************
 ******************************************************************************/

/** \file yadac.c
 *  yada binding compiler
 *
 * $Id: yadac.c 190 2007-11-21 00:07:23Z grizz $
 */

/******************************************************************************
 * I N C L U D E S ************************************************************
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#include <unistd.h>
#include <sysexits.h>

#include <yada.h>
#include <yadac.h>

/******************************************************************************
 * T Y P E D E F S ************************************************************
 ******************************************************************************/

/* auto-growable buffer */
typedef struct
{
  char *ptr;
  int siz, max;
  int errs;
} autobuf_t;

/******************************************************************************
 * M A C R O S ****************************************************************
 ******************************************************************************/

/* convert data to strings */
#define TO_STRING_(s)		#s
#define TO_STRING(s)		TO_STRING_(s)

/* map type name */
#define map_type(t,n,f)		if(!strcmp(name, n)) { \
				*flgs |= f; \
				return(TO_STRING(atyp_##t));}

/******************************************************************************
 * F U N C T I O N S **********************************************************
 ******************************************************************************/

/******************************************************************************/
/** auto-grow autobuf
 * @return 0 on success
 */

static inline int agrow(autobuf_t *buf, int siz)
{
  char *ptr;
  int max;


  /* check if buffer already has enough space */
  if((buf->siz + siz) < buf->max)
    return(0);

  /* allocate more space for buffer */
  max = (buf->max ? (buf->max << 1) : 4096);
  if((ptr = realloc(buf->ptr, max)))
    {
    buf->ptr = ptr;
    buf->max = max;
    return(0);
    }

  /* allocation failed */
  buf->errs++;
  return(-1);
}

/******************************************************************************/
/** append NUL terminated string to autobuf
 */

static void acopy(autobuf_t *buf, const char *str)
{
  int len;


  /* make sure buffer has enough space */
  if(agrow(buf, (len = strlen(str))))
    return;

  /* append to buffer */
  memcpy((buf->ptr + buf->siz), str, len);
  buf->siz += len;
}

/******************************************************************************/
/** append formatted string to autobuf
 */

static void aprintf(autobuf_t *buf, const char *fmt, ...)
{
  int len;
  va_list ap;


  va_start(ap, fmt);

  /* make sure buffer has enough space */
  if(agrow(buf, ((len = vsnprintf(NULL, 0, fmt, ap) + 1))))
    return;

  /* print to buffer */
  buf->siz += vsnprintf((buf->ptr + buf->siz), len, fmt, ap);
  va_end(ap);
}

/******************************************************************************/
/** append specified level of indenting
 */

static void aindent(autobuf_t *buf, int indent)
{
  /* double space indent */
  indent <<= 1;

  /* make sure buffer has enough space */
  if(agrow(buf, indent))
    return;


  /* append indenting to buffer */
  memset((buf->ptr + buf->siz), ' ', indent);
  buf->siz += indent;
}

/******************************************************************************/
/** append yada type name
 */

static void ayadavar(autobuf_t *buf, yadac_param_t param, char *name)
{
  switch(param)
    {
  case(YADAC_INT):
    if(name)
      aprintf(buf, "int %s", name);
    else
      acopy(buf, "int");
    break;
  case(YADAC_FLOAT):
    if(name)
      aprintf(buf, "double %s", name);
    else
      acopy(buf, "double");
    break;
  case(YADAC_LONG):
    if(name)
      aprintf(buf, "long long %s", name);
    else
      acopy(buf, "long long");
    break;
  case(YADAC_BINARY):
  case(YADAC_STRING):
    if(name)
      aprintf(buf, "char *%s", name);
    else
      acopy(buf, "char*");
    break;
  case(YADAC_INTP):
    if(name)
      aprintf(buf, "int *%s", name);
    else
      acopy(buf, "int*");
    break;
  case(YADAC_FLOATP):
    if(name)
      aprintf(buf, "double *%s", name);
    else
      acopy(buf, "double*");
    break;
  case(YADAC_LONGP):
    if(name)
      aprintf(buf, "long long *%s", name);
    else
      acopy(buf, "long long*");
    break;
  case(YADAC_BINARYP):
  case(YADAC_STRINGP):
    if(name)
      aprintf(buf, "char *%s", name);
    else
      acopy(buf, "char*");
    break;
    }
}

/******************************************************************************/
/** append yada type code
 */

static void ayadatypes(autobuf_t *buf, yadac_param_t *param, int params)
{
  int loop;


  for(loop=0; loop<params; param++, loop++)
    {
    switch(*param)
      {
    case(YADAC_INT):
      acopy(buf, "?d");
      break;
    case(YADAC_FLOAT):
      acopy(buf, "?f");
      break;
    case(YADAC_LONG):
      acopy(buf, "?l");
      break;
    case(YADAC_BINARY):
      acopy(buf, "?b");
      break;
    case(YADAC_STRING):
      acopy(buf, "?s");
      break;
    case(YADAC_INTP):
      acopy(buf, "?pd");
      break;
    case(YADAC_FLOATP):
      acopy(buf, "?pf");
      break;
    case(YADAC_LONGP):
      acopy(buf, "?pl");
      break;
    case(YADAC_BINARYP):
      acopy(buf, "?pb");
      break;
    case(YADAC_STRINGP):
      acopy(buf, "?ps");
      break;
      }
    } /* foreach(param) */
}

/******************************************************************************/
/** append input variable prototypes
 */

static inline void aiproto(autobuf_t *buf, yadac_binding_t *binding)
{
  int idx;


  /* input struct bound, use it */
  if(binding->istruct)
    {
    aprintf(buf, ", %s *in", binding->istruct->type);
    return;
    }

  /* no input variables needed */
  if(!binding->iparams)
    return;

  /* loop through input paramters making variables */
  for(idx=0; idx<binding->iparams; idx++)
    {
    acopy(buf, ", ");
    ayadavar(buf, binding->iparam[idx], "in");
    aprintf(buf, "%u", (idx + 1));
    }
}

/******************************************************************************/
/** append input variables
 */

static inline void aivars(autobuf_t *buf, yadac_binding_t *binding)
{
  int idx;


  /* input struct bound, use it */
  if(binding->istruct)
    {
    for(idx=0; idx<binding->istruct->list->tokens; idx++)
      if(*binding->istruct->list->token[idx] < 0x2b)
        aprintf(buf, ", %cin->%s", binding->istruct->list->token[idx][0],
         &binding->istruct->list->token[idx][1]);
      else
        aprintf(buf, ", in->%s", binding->istruct->list->token[idx]);
    return;
    }

  /* no input variables needed */
  if(!binding->iparams)
    return;

  /* loop through input paramters making variables */
  for(idx=0; idx<binding->iparams; idx++)
    aprintf(buf, ", in%u", (idx + 1));
}


/******************************************************************************/
/** generate source code output
 * @param siz set to buffer length on success when not NULL
 * @return buffer pointer on success
 */

static char* gen_source(yadac_t *tree, int *siz,
 char *src_fname, char *fname, char *hdr_fname, char *prefix)
{
  char tag[32];
  char *ptr, *in, *out, *hdr;
  int loop, idx;
  autobuf_t *buf;
  yadac_binding_t *binding;


  /* convert prefix to caps */
  snprintf(tag, sizeof(tag), "%s", prefix);
  for(ptr=tag; *ptr; ptr++)
    *ptr = toupper(*ptr);

  /* remove path from source filename */
  in = strrchr(src_fname, '/');
  in = (in ? (in + 1) : src_fname);

  /* remove path from output filename */
  out = strrchr(fname, '/');
  out = (out ? (out + 1) : fname);

  /* remove path from header filename */
  hdr = strrchr(hdr_fname, '/');
  hdr = (hdr ? (hdr + 1) : hdr_fname);

  /* allocate new autobuf */
  if(!(buf = calloc(1, sizeof(autobuf_t))))
    return(NULL);


  acopy(buf, "\n/*******************************************************");
  acopy(buf, "***********************\n ********************************");
  acopy(buf, "**********************************************/\n\n");

  aprintf(buf, "/** \\file %s\n", out);
  aprintf(buf, " *  automatically generated from: %s\n */\n\n", in);

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * D E F I N E S ******************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  aprintf(buf, "#define __YADAC_%s_LIBSRC_\n", tag);
  aprintf(buf, "#define YADAC_%s_CHUNK_SZ 8\n\n", tag);

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * I N C L U D E S ****************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  acopy(buf, "#include <stdarg.h>\n\n");
  acopy(buf, "#include <stdlib.h>\n\n");
  acopy(buf, "#include <yada.h>\n\n");
  aprintf(buf, "#include \"%s\"\n\n", hdr);

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * S Q L **************************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  /* output SQL for each query */
  for(loop=0, binding=tree->bind; loop<tree->binds; binding++, loop++)
    {
    aprintf(buf, "#define SQL_%s \\\n", binding->upper);
    for(idx=0; idx<binding->lines; idx++)
      {
      aindent(buf, binding->line[idx].indent);
      if((idx + 1) < binding->lines)
        aprintf(buf, "\"%s \"\\\n", binding->line[idx].text);
      else
        aprintf(buf, "\"%s\"\n", binding->line[idx].text);
      }
    acopy(buf, "\n");
    }

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * G L O B A L S ******************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  acopy(buf, "/* saved database pointer */\n");
  acopy(buf, "static yadac_t *yadac;\n\n");

  /* output statement pointer for each query */
  aprintf(buf, "/* %s statements */\n", prefix);
  for(loop=0, binding=tree->bind; loop<tree->binds; binding++, loop++)
    aprintf(buf, "static yada_rc_t *stmt_%s = NULL;\n", binding->name);
  acopy(buf, "\n");

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * F U N C T I O N S **************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************/\n");
  aprintf(buf, "/** prepare %s statements\n", prefix);
  acopy(buf, " * @param yc yadac_t to use\n");
  acopy(buf, " * @param yada_t* db if yc is null, alloc new and use this db\n");
  acopy(buf, " * @return yadac_t* on success, NULL on error\n */\n");

  aprintf(buf, "\nyadac_t* %s_init(yadac_t *yc, ...)\n{\n", prefix);
  acopy(buf, "  int err = 0;\n  va_list ap;\n  yada_t *db;\n\n\n");

  /* create yadac */
  acopy(buf, "  if(!yc)\n    {\n");
  acopy(buf, "    if(!(yadac = calloc(1, sizeof(yadac_t))) || ");
  aprintf(buf, "!(yadac->rc = malloc(YADAC_%s_CHUNK_SZ * ", tag);
  acopy(buf, "sizeof(yadac_t*))))\n      {\n      free(yadac);\n");
  aprintf(buf, "      return(NULL);\n      }\n", prefix);
  acopy(buf, "    va_start(ap, yc);\n    yadac->db = va_arg(ap, yada_t*);\n");
  acopy(buf, "    va_end(ap);\n");
  aprintf(buf, "    yadac->sz = YADAC_%s_CHUNK_SZ;\n    }\n", tag);
  acopy(buf, "  else\n    yadac = yc;\n\n");
  acopy(buf, "  db = yadac->db;\n\n");

  /* code to prepare statements */
  aprintf(buf, "  /* prepare %s statements */\n", prefix);
  for(loop=0, binding=tree->bind; loop<tree->binds; binding++, loop++)
    aprintf(buf, "  err |= !(stmt_%s = db->%s(db, SQL_%s, 0));\n",
     binding->name, (binding->ptype ? "yprepare" : "prepare"), binding->upper);
  acopy(buf, "\n");

  /* output error check */
  acopy(buf, "  /* check for errors */\n");
  acopy(buf, "  if(err)\n    {\n");
  aprintf(buf, "    %s_destroy(yadac);\n    return(NULL);\n    }\n\n", prefix);

  acopy(buf, "  return(yadac);\n}\n\n");

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************/\n");
  aprintf(buf, "/** free %s queries and bindsets\n */\n", prefix);

  aprintf(buf, "\nvoid %s_free(yadac_t *yc)\n{\n", prefix);

  /* make sure yadac instance was specified */
  acopy(buf, "  if(!yc)\n    yc = yadac;\n\n");
  acopy(buf, "  if(!yc->rcs)\n    return;\n\n");

  acopy(buf, "  while(yc->rcs--)\n");
  acopy(buf, "    yc->db->free(yc->db, yc->rc[yc->rcs]);\n\n");
  acopy(buf, "  yc->rcs = 0;\n}\n\n");

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************/\n");
  aprintf(buf, "/** destroy %s statements\n */\n", prefix);

  aprintf(buf, "\nvoid %s_destroy(yadac_t *yc)\n{\n", prefix);
  acopy(buf, "  yada_t *db;\n\n\n");

  /* make sure yadac instance was specified */
  acopy(buf, "  if(!yc)\n    yc = yadac;\n");
  acopy(buf, "  db = yc->db;\n\n");

  /* cleanup each statement */
  for(loop=0, binding=tree->bind; loop<tree->binds; binding++, loop++)
    {
    aprintf(buf, "  if(stmt_%s)\n", binding->name);
    aprintf(buf, "    db->free(db, stmt_%s);\n", binding->name);
    aprintf(buf, "  stmt_%s = NULL;\n\n", binding->name);
    }

  /* cleanup results and free yadac */
  aprintf(buf, "  %s_free(yc);\n  free(yc->rc);\n  free(yc);\n", prefix);
  acopy(buf, "  if(yc == yadac)\n    yadac = NULL;\n");
  acopy(buf, "}\n\n");

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************/\n");
  aprintf(buf, "/** push rcs onto yadac\n", prefix);
  acopy(buf, " * @return 0 on success, -1 on error\n */\n");

  acopy(buf, "\ninline static int _yadac_push(yadac_t *yc, ");
  acopy(buf, "yada_rc_t *query, yada_rc_t *bs)\n{\n");
  acopy(buf, "  yada_rc_t **tmp;\n\n\n");

  /* grow yadac rc array if needed */
  acopy(buf, "  if(yc->rcs + 1 >= yc->sz)\n    {\n");
  acopy(buf, "    if(!(tmp = realloc(yc->rc, sizeof(yada_rc_t*) * (yc->sz + ");
  aprintf(buf, "YADAC_%s_CHUNK_SZ))))\n      return(-1);\n", tag);
  aprintf(buf, "    yc->rc = tmp;\n    yc->sz += YADAC_%s_CHUNK_SZ;\n    }\n\n",
   tag);

  /* set rcs */
  acopy(buf, "  yc->rc[yc->rcs++] = query;\n  yc->rc[yc->rcs++] = bs;\n");
  acopy(buf, "  return(0);\n}\n\n");

  /* actual statement functions */
  for(loop=0, binding=tree->bind; loop<tree->binds; binding++, loop++)
    {
    acopy(buf, "/*********************************************************");
    acopy(buf, "*********************/\n");

    switch(binding->rtype)
      {
    /************************************************/
    case(YADAC_STATUS):
    /************************************************/
      aprintf(buf, "/** %s (STATUS operation)\n", binding->name);
      acopy(buf, " * @return num rows affected on success, ");
      acopy(buf, "-1 on error\n */\n\n");
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);

      /* add input variables */
      aiproto(buf, binding);
      acopy(buf, ")\n{\n  yada_t *db;\n\n\n");

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n");
      acopy(buf, "  db = yc->db;\n\n");

      acopy(buf, "  /* execute statement */\n");
      aprintf(buf, "  return(db->execute(db, stmt_%s", binding->name);
      aivars(buf, binding);
      acopy(buf, "));\n}\n\n");
      break;
    /************************************************/
    case(YADAC_SIMPLE):
    /************************************************/
      aprintf(buf, "/** %s (SIMPLE operation)\n", binding->name);
      acopy(buf, " * @param errval value to return on error\n");
      acopy(buf, " * @return value on success, errval on error\n */\n\n");
      ayadavar(buf, *binding->oparam, NULL);
      aprintf(buf, " %s_%s(yadac_t *yc, ", prefix, binding->name);
      ayadavar(buf, *binding->oparam, "errval");
      aiproto(buf, binding);
      acopy(buf, ")\n{\n  ");

      ayadavar(buf, *binding->oparam, "out");
      acopy(buf, ";\n  int rval;\n");
      acopy(buf, "  yada_t *db;\n  yada_rc_t *bs, *query;\n\n\n");

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n");
      acopy(buf, "  db = yc->db;\n\n");

      acopy(buf, "  /* bind output variables */\n");
      acopy(buf, "  if(!(bs = db->bind(db, \"");
      ayadatypes(buf, binding->oparam, binding->oparams);
      acopy(buf, "\", &out)))\n    return(errval);\n\n");

      acopy(buf, "  /* perform query */\n");
      aprintf(buf, "  if(!(query = db->query(db, stmt_%s", binding->name);
      aivars(buf, binding);
      acopy(buf, ")))\n    {\n");
      acopy(buf, "    db->free(db, bs);\n");
      acopy(buf, "    return(errval);\n");
      acopy(buf, "    }\n\n");

      acopy(buf, "  /* fetch result */\n");
      acopy(buf, "  rval = db->fetch(db, query, bs);\n");
      acopy(buf, "  if(!rval || db->error || _yadac_push(yc, query, bs))\n");
      acopy(buf, "    {\n    db->free(db, query);\n    db->free(db, bs);\n");
      acopy(buf, "    return(errval);\n    }\n\n");

      acopy(buf, "  return(out);\n}\n\n");
      break;
    /************************************************/
    case(YADAC_SINGLE):
    /************************************************/
      aprintf(buf, "/** %s (SINGLE operation)\n", binding->name);
      acopy(buf, " * @return 0 on success, -1 on error\n */\n\n");
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      for(idx=0; idx<binding->oparams; idx++)
        {
        acopy(buf, ", ");
        if(binding->oparam[idx] == YADAC_STRING)
          ayadavar(buf, binding->oparam[idx], "out");
        else
          ayadavar(buf, binding->oparam[idx], "*out");
        aprintf(buf, "%u", (idx + 1));
        }
      acopy(buf, ")\n{\n");
      acopy(buf, "  int rval;\n");
      acopy(buf, "  yada_t *db;\n  yada_rc_t *bs, *query;\n\n\n");

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n");
      acopy(buf, "  db = yc->db;\n\n");

      acopy(buf, "  /* bind output variables */\n");
      acopy(buf, "  if(!(bs = db->bind(db, \"");
      ayadatypes(buf, binding->oparam, binding->oparams);
      acopy(buf, "\"");
      for(idx=0; idx<binding->oparams; idx++)
        aprintf(buf, ", out%u", (idx + 1));
      acopy(buf, ")))\n    return(-1);\n\n");

      acopy(buf, "  /* perform query */\n");
      aprintf(buf, "  if(!(query = db->query(db, stmt_%s", binding->name);
      aivars(buf, binding);
      acopy(buf, ")))\n    {\n");
      acopy(buf, "    db->free(db, bs);\n");
      acopy(buf, "    return(-1);\n");
      acopy(buf, "    }\n\n");

      acopy(buf, "  /* fetch result */\n");
      acopy(buf, "  rval = db->fetch(db, query, bs);\n");
      acopy(buf, "  if(!rval || db->error || _yadac_push(yc, query, bs))\n");
      acopy(buf, "    {\n    db->free(db, query);\n    db->free(db, bs);\n");
      acopy(buf, "    return(db->error ? -1 : 1);\n    }\n\n");

      acopy(buf, "  return(0);\n}\n\n");
      break;
    /************************************************/
    case(YADAC_STRUCT):
    /************************************************/
      aprintf(buf, "/** %s (%s STRUCT operation)\n", binding->name,
       binding->ostruct->type);
      acopy(buf, " * @return 0 on success, -1 on error, 1 no results\n */\n\n");
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      aprintf(buf, ", %s *out)\n{\n", binding->ostruct->type);
      acopy(buf, "  int rval;\n");
      acopy(buf, "  yada_t *db;\n  yada_rc_t *bs, *query;\n\n\n");

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n");
      acopy(buf, "  db = yc->db;\n\n");

      acopy(buf, "  /* bind output variables */\n");
      acopy(buf, "  if(!(bs = db->bind(db, \"");
      ayadatypes(buf, binding->oparam, binding->oparams);
      acopy(buf, "\"");
      for(idx=0; idx<binding->ostruct->list->tokens; idx++)
        if(*binding->ostruct->list->token[idx] < 0x2b)
          aprintf(buf, ", %cout->%s", binding->ostruct->list->token[idx][0],
           &binding->ostruct->list->token[idx][1]);
        else
          aprintf(buf, ", out->%s", binding->ostruct->list->token[idx]);
      acopy(buf, ")))\n    return(-1);\n\n");

      acopy(buf, "  /* perform query */\n");
      aprintf(buf, "  if(!(query = db->query(db, stmt_%s", binding->name);
      aivars(buf, binding);
      acopy(buf, ")))\n    {\n");
      acopy(buf, "    db->free(db, bs);\n");
      acopy(buf, "    return(-1);\n");
      acopy(buf, "    }\n\n");

      acopy(buf, "  /* fetch result */\n");
      acopy(buf, "  rval = db->fetch(db, query, bs);\n");
      acopy(buf, "  if(!rval || db->error || _yadac_push(yc, query, bs))\n");
      acopy(buf, "    {\n    db->free(db, query);\n    db->free(db, bs);\n");
      acopy(buf, "    return(db->error ? -1 : 1);\n    }\n\n");

      acopy(buf, "  return(0);\n}\n\n");
      break;
    /************************************************/
    case(YADAC_MULTIROW):
    /************************************************/
      aprintf(buf, "/** %s (MULTIROW operation)\n", binding->name);
      acopy(buf, " * @return 0 on success, -1 on error\n */\n\n");
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      for(idx=0; idx<binding->oparams; idx++)
        {
        acopy(buf, ", ");
        if(binding->oparam[idx] == YADAC_STRING)
          ayadavar(buf, binding->oparam[idx], "out");
        else
          ayadavar(buf, binding->oparam[idx], "*out");
        aprintf(buf, "%u", (idx + 1));
        }
      acopy(buf, ")\n{\n");
      acopy(buf, "  yada_t *db;\n  yada_rc_t *bs, *query;\n\n\n");

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n");
      acopy(buf, "  db = yc->db;\n\n");

      acopy(buf, "  /* bind output variables */\n");
      acopy(buf, "  if(!(bs = db->bind(db, \"");
      ayadatypes(buf, binding->oparam, binding->oparams);
      acopy(buf, "\"");
      for(idx=0; idx<binding->oparams; idx++)
        aprintf(buf, ", out%u", (idx + 1));
      acopy(buf, ")))\n    return(-1);\n\n");

      acopy(buf, "  /* perform query */\n");
      aprintf(buf, "  if(!(query = db->query(db, stmt_%s", binding->name);
      aivars(buf, binding);
      acopy(buf, ")))\n    {\n");
      acopy(buf, "    db->free(db, bs);\n");
      acopy(buf, "    return(-1);\n    }\n\n");

      acopy(buf, "  if(_yadac_push(yc, query, bs))\n");
      acopy(buf, "    {\n    db->free(db, query);\n    db->free(db, bs);\n");
      acopy(buf, "    return(-1);\n    }\n\n");

      acopy(buf, "  return(0);\n}\n\n");

      acopy(buf, "/*******************************************************");
      acopy(buf, "***********************/\n");
      aprintf(buf, "/** %s (MULTIROW operation) fetch routine\n",
       binding->name);
      acopy(buf, " * @return 0 on completion, 1 if incomplete, "\
       "-1 on error\n */\n\n");
      aprintf(buf, "int %s_%s_fetch(yadac_t *yc)\n{\n",
       prefix, binding->name);

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n\n");

      acopy(buf, "  /* fetch result */\n");
      acopy(buf, "  if(yc->db->fetch(yc->db, ");
      acopy(buf, "yc->rc[yc->rcs - 2], yc->rc[yc->rcs - 1]))\n");
      acopy(buf, "    return(1);\n\n");

      acopy(buf, "  /* check for errors */\n");
      acopy(buf, "  return(yc->db->error ? -1 : 0);\n}\n\n");
      break;
    /************************************************/
    case(YADAC_MULTISTRUCT):
    /************************************************/
      aprintf(buf, "/** %s (%s MULTISTRUCT operation)\n", binding->name,
       binding->ostruct->type);
      acopy(buf, " * @return 0 on success, -1 on error, 1 no results\n */\n\n");
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      aprintf(buf, ", %s *out)\n{\n", binding->ostruct->type);
      acopy(buf, "  yada_t *db;\n  yada_rc_t *bs, *query;\n\n\n");

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n");
      acopy(buf, "  db = yc->db;\n\n");

      acopy(buf, "  /* bind output variables */\n");
      acopy(buf, "  if(!(bs = db->bind(db, \"");
      ayadatypes(buf, binding->oparam, binding->oparams);
      acopy(buf, "\"");
      for(idx=0; idx<binding->ostruct->list->tokens; idx++)
        if(*binding->ostruct->list->token[idx] < 0x2b)
          aprintf(buf, ", %cout->%s", binding->ostruct->list->token[idx][0],
           &binding->ostruct->list->token[idx][1]);
        else
          aprintf(buf, ", out->%s", binding->ostruct->list->token[idx]);
      acopy(buf, ")))\n    return(-1);\n\n");

      acopy(buf, "  /* perform query */\n");
      aprintf(buf, "  if(!(query = db->query(db, stmt_%s", binding->name);
      aivars(buf, binding);
      acopy(buf, ")))\n    {\n");
      acopy(buf, "    db->free(db, bs);\n");
      acopy(buf, "    return(-1);\n");
      acopy(buf, "    }\n\n");

      acopy(buf, "  if(_yadac_push(yc, query, bs))\n");
      acopy(buf, "    {\n    db->free(db, query);\n    db->free(db, bs);\n");
      acopy(buf, "    return(-1);\n    }\n\n");

      acopy(buf, "  return(0);\n}\n\n");

      acopy(buf, "/*******************************************************");
      acopy(buf, "***********************/\n");
      aprintf(buf, "/** %s (MULTISTRUCT operation) fetch routine\n",
       binding->name);
      acopy(buf, " * @return 0 on completion, 1 if incomplete, "\
       "-1 on error\n */\n\n");
      aprintf(buf, "int %s_%s_fetch(yadac_t *yc)\n{\n", prefix, binding->name);

      /* make sure yadac instance was specified */
      acopy(buf, "  if(!yc)\n    yc = yadac;\n\n");

      acopy(buf, "  /* fetch result */\n");
      acopy(buf, "  if(yc->db->fetch(yc->db, ");
      acopy(buf, "yc->rc[yc->rcs - 2], yc->rc[yc->rcs - 1]))\n");
      acopy(buf, "    return(1);\n\n");

      acopy(buf, "  /* check for errors */\n");
      acopy(buf, "  return(yc->db->error ? -1 : 0);\n}\n\n");

      break;
    /************************************************/
      } /* switch(rtype) */
    } /* foreach(binding) */

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n **********************************");
  acopy(buf, "********************************************/\n\n");

  /* check if any errors occured */
  if(buf->errs)
    {
    free(buf->ptr);
    free(buf);
    return(NULL);
    }

  /* set size if needed */
  if(siz)
    *siz = buf->siz;

  /* shrink and return buffer */
  ptr = realloc(buf->ptr, buf->siz);
  free(buf);
  return(ptr);
}

/******************************************************************************/
/** generate header output
 * @param siz set to buffer length on success when not NULL
 * @return buffer pointer on success
 */

static char* gen_header(yadac_t *tree, int *siz,
 char *src_fname, char *fname, char *prefix)
{
  char tag[32];
  char *ptr, *in, *out;
  int loop, idx;
  yadac_binding_t *binding;
  autobuf_t *buf;


  /* convert prefix to caps */
  snprintf(tag, sizeof(tag), "%s", prefix);
  for(ptr=tag; *ptr; ptr++)
    *ptr = toupper(*ptr);

  /* remove path from source filename */
  in = strrchr(src_fname, '/');
  in = (in ? (in + 1) : src_fname);

  /* remove path from output filename */
  out = strrchr(fname, '/');
  out = (out ? (out + 1) : fname);

  /* allocate new autobuf */
  if(!(buf = calloc(1, sizeof(autobuf_t))))
    return(NULL);

  acopy(buf, "\n/*******************************************************");
  acopy(buf, "***********************\n ********************************");
  acopy(buf, "**********************************************/\n\n");

  aprintf(buf, "/** \\file %s\n", out);
  aprintf(buf, " *  automatically generated from: %s\n */\n\n", in);
  aprintf(buf, "#ifndef __YADAC_%s_H__\n", tag);
  aprintf(buf, "#define __YADAC_%s_H__\n\n", tag);

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * I N C L U D E S ****************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  acopy(buf, "#include <yada.h>\n\n");

  /* list included files */
  if(tree->incs)
    {
    for(loop=0; loop<tree->incs; loop++)
      aprintf(buf, "#include \"%s\"\n", tree->inc[loop]);
    acopy(buf, "\n");
    }

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * T Y P E D E F S ****************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  acopy(buf, "#ifndef __YADAC_T_\n");
  acopy(buf, "#define __YADAC_T_\n\n");

  acopy(buf, "typedef struct\n{\n");
  acopy(buf, "  yada_t *db;\n");
  acopy(buf, "  int sz;\n  int rcs;\n");
  acopy(buf, "  yada_rc_t **rc;\n");
  acopy(buf, "} yadac_t;\n\n");

  acopy(buf, "#endif\n\n");

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n * P R O T O T Y P E S ************");
  acopy(buf, "********************************************\n ***********");
  acopy(buf, "**********************************************************");
  acopy(buf, "*********/\n\n");

  aprintf(buf, "/* init / destroy %s functions */\n", prefix);
  aprintf(buf, "yadac_t* %s_init(yadac_t *db, ...);\n", prefix);
  aprintf(buf, "void %s_free(yadac_t *yc);\n\n", prefix);
  aprintf(buf, "void %s_destroy(yadac_t *yc);\n\n", prefix);

  /* output prototypes */
  acopy(buf, "/* compiled queries */\n");
  for(loop=0, binding=tree->bind; loop<tree->binds; binding++, loop++)
    {
    switch(binding->rtype)
      {
    /************************************************/
    case(YADAC_STATUS):
    /************************************************/
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      acopy(buf, ");\n\n");
      break;
    /************************************************/
    case(YADAC_SIMPLE):
    /************************************************/
      ayadavar(buf, *binding->oparam, NULL);
      aprintf(buf, " %s_%s(yadac_t *yc, ", prefix, binding->name);
      ayadavar(buf, *binding->oparam, "errval");

      aiproto(buf, binding);
      acopy(buf, ");\n\n");
      break;
    /************************************************/
    case(YADAC_SINGLE):
    /************************************************/
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      for(idx=0; idx<binding->oparams; idx++)
        {
        acopy(buf, ", ");
        if(binding->oparam[idx] == YADAC_STRING)
          ayadavar(buf, binding->oparam[idx], "out");
        else
          ayadavar(buf, binding->oparam[idx], "*out");
        aprintf(buf, "%u", (idx + 1));
        }
      acopy(buf, ");\n\n");
      break;
    /************************************************/
    case(YADAC_STRUCT):
    /************************************************/
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      aprintf(buf, ", %s *out);\n\n", binding->ostruct->type);
      break;
    /************************************************/
    case(YADAC_MULTIROW):
    /************************************************/
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      for(idx=0; idx<binding->oparams; idx++)
        {
        acopy(buf, ", ");
        if(binding->oparam[idx] == YADAC_STRING)
          ayadavar(buf, binding->oparam[idx], "out");
        else
          ayadavar(buf, binding->oparam[idx], "*out");
        aprintf(buf, "%u", (idx + 1));
        }
      acopy(buf, ");\n");

      aprintf(buf, "int %s_%s_fetch(yadac_t *yc);\n", prefix, binding->name);
      break;
    /************************************************/
    case(YADAC_MULTISTRUCT):
    /************************************************/
      aprintf(buf, "int %s_%s(yadac_t *yc", prefix, binding->name);
      aiproto(buf, binding);
      aprintf(buf, ", %s *out);\n\n", binding->ostruct->type);
      aprintf(buf, "int %s_%s_fetch(yadac_t *yc);\n", prefix, binding->name);
      break;
    /************************************************/
      } /* switch(rtype) */
    } /* foreach(binding) */

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************/\n\n");


  aprintf(buf, "#endif /* end __YADAC_%s_H__ */\n\n", tag);

  acopy(buf, "/*********************************************************");
  acopy(buf, "*********************\n **********************************");
  acopy(buf, "********************************************/\n\n");

  /* check if any errors occured */
  if(buf->errs)
    {
    free(buf->ptr);
    free(buf);
    return(NULL);
    }

  /* set size if needed */
  if(siz)
    *siz = buf->siz;

  /* shrink and return buffer */
  ptr = realloc(buf->ptr, buf->siz);
  free(buf);
  return(ptr);
}

/******************************************************************************
 * M A I N ********************************************************************
 ******************************************************************************/

int main(int argc, char **argv)
{
  char *proc_name, *buf;
  char *in_fname, *out_fname = NULL, *hdr_fname = NULL;
  char *fname, *prefix = NULL;
  int opt, help = 0;
  int loop, siz;
  FILE *in_file, *out_file, *hdr_file;
  yadac_t *tree;


  /* store name of process */
  proc_name = strrchr(*argv, '/');
  proc_name = (proc_name ? (proc_name + 1) : *argv);

  /* check for explicit usage request */
  if((argc > 1) && !strcmp(argv[1], "--help"))
    help++;
  else
    {
    /* parse arguments */
    opterr = 0;
    while((opt = getopt(argc, argv, "h:o:p:V")) != EOF)
      {
      switch(opt)
        {
      case('h'):
        hdr_fname = optarg;
        break;
      case('o'):
        out_fname = optarg;
        break;
      case('p'):
        prefix = optarg;
        break;
      case('V'):
        printf("yadac, version %s $Rev: 190 $\n", YADA_VERSION_STR);
        return(EX_OK);
      case('?'):
        fprintf(stderr, "invalid option -- %c\n", optopt);
        help++;
        break;
        } /* switch(opt) */
      } /* foreach(option) */
    } /* else(!help) */

  if(help || (argc < 2))
    {
    printf("usage: %s [options] <in_file>\n", proc_name);
    printf("options:\n");
    printf("  -h file : header filename\n");
    printf("  -o file : output filename\n");
    printf("  -p name : prefix for output functions\n");
    printf("  -V display version information\n");
    return(EX_USAGE);
    }


  /* determine input filename */
  in_fname = argv[optind];

  /* input filename without path */
  fname = strrchr(in_fname, '/');
  fname = (fname ? (fname + 1) : in_fname);

  /* determine output filename */
  if(!out_fname)
    {
    /* derive out_fname from fname */
    siz = strlen(fname);
    if(!(out_fname = malloc(siz + 3)))
      {
      fprintf(stderr, "memory allocation error: (%s)\n", strerror(errno));
      return(EX_OSERR);
      }
    strcpy(out_fname, fname);
    strcpy((out_fname + siz), ".c");
    }

  /* determine header filename */
  if(!hdr_fname)
    {
    /* derive hdr_fname from out_fname */
    siz = strlen(out_fname);
    if((siz >= 2) && !strcmp((out_fname + siz - 2), ".c"))
      siz -= 2;
    if(!(hdr_fname = malloc(siz + 3)))
      {
      fprintf(stderr, "memory allocation error: (%s)\n", strerror(errno));
      return(EX_OSERR);
      }
    strcpy(hdr_fname, out_fname);
    strcpy((hdr_fname + siz), ".h");
    }

  /* determine prefix */
  if(!prefix)
    {
    /* derive prefix from input file name */
    if(!(prefix = strdup(fname)))
      {
      fprintf(stderr, "memory allocation error: (%s)\n", strerror(errno));
      return(EX_OSERR);
      }

    /* check for last usable character */
    siz = strlen(prefix);
    for(loop=0; loop<siz; loop++)
      {
      if(!(isalnum(prefix[loop]) || (prefix[loop] == '_')))
        {
        prefix[loop] = 0;
        break;
        }
      }
    } /* if(need_prefix) */


  printf(" infile: %s\n", in_fname);
  printf("outfile: %s\n", out_fname);
  printf(" header: %s\n", hdr_fname);
  printf(" prefix: %s\n", prefix);

  /* open input file */
  if(!(in_file = fopen(in_fname, "r")))
    {
    fprintf(stderr, "error opening %s: (%s)\n", in_fname, strerror(errno));
    return(EX_NOINPUT);
    }

  /* open output and header files */
  if(!(out_file = fopen(out_fname, "w")))
    {
    fprintf(stderr, "error creating %s: (%s)\n", out_fname, strerror(errno));
    return(EX_CANTCREAT);
    }
  if(!(hdr_file = fopen(hdr_fname, "w")))
    {
    fprintf(stderr, "error creating %s: (%s)\n", hdr_fname, strerror(errno));
    return(EX_CANTCREAT);
    }


  /* find size of source file */
  fseek(in_file, 0, SEEK_END);
  siz = ftell(in_file);
  rewind(in_file);

  /* allocate space for file */
  if(!(buf = malloc(siz)))
    {
    fprintf(stderr, "memory allocation error: (%s)\n", strerror(errno));
    return(EX_OSERR);
    }

  /* load file into memory */
  if(!fread(buf, siz, 1, in_file))
    {
    fprintf(stderr, "error reading %s: (%s)\n", in_fname, strerror(errno));
    return(EX_DATAERR);
    }
  fclose(in_file);

  /* parse source file */
  if(!(tree = yadac_parse(buf, siz)))
    {
    fprintf(stderr, "error parsing %s\n", in_fname);
    return(EX_DATAERR);
    }
  free(buf);


  /* generate output */
  if(!(buf = gen_source(tree, &siz, in_fname, out_fname, hdr_fname, prefix)) ||
   (fwrite(buf, 1, siz, out_file) != siz))
    {
    fprintf(stderr, "error generating %s: (%s)\n", out_fname, strerror(errno));
    return(EX_UNAVAILABLE);
    }
  free(buf);
  fclose(out_file);

  /* generate header */
  if(!(buf = gen_header(tree, &siz, in_fname, hdr_fname, prefix)) ||
   (fwrite(buf, 1, siz, hdr_file) != siz))
    {
    fprintf(stderr, "error generating %s: (%s)\n", hdr_fname, strerror(errno));
    return(EX_UNAVAILABLE);
    }
  free(buf);
  fclose(hdr_file);

  yadac_free(tree);
  return(EX_OK);
}

/******************************************************************************
 ******************************************************************************/


