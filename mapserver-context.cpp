#include "mapserver.h"
#include "mapfile.h" // format shares a couple of tokens
#include <string>

extern "C" int msyylex(void);
extern "C" void msyyrestart(FILE *);

extern FILE *msyyin;
extern int msyystate;
extern int msyylineno;
extern char *msyystring_buffer;

static int initContext(contextObj *context)
{
  if (context == NULL) {
    msSetError(MS_MEMERR, "Context object is NULL.", "initContext()");
    return MS_FAILURE;
  }

  if(initHashTable(&(context->env)) != MS_SUCCESS) return MS_FAILURE;
  if(initHashTable(&(context->maps)) != MS_SUCCESS) return MS_FAILURE;
  if(initHashTable(&(context->plugins)) != MS_SUCCESS) return MS_FAILURE;

  return MS_SUCCESS;
}

void msFreeContext(contextObj *context)
{
  if(context == NULL) return;
  if(&(context->env)) msFreeHashItems(&(context->env));
  if(&(context->maps)) msFreeHashItems(&(context->maps));
  if(&(context->plugins)) msFreeHashItems(&(context->plugins));
}

static int loadContext(contextObj *context)
{
  int token;

  if(context == NULL) return MS_FAILURE;

  token = msyylex();
  if(token != MS_CONTEXT_SECTION) {
    msSetError(MS_IDENTERR, "First token must be CONTEXT, this doesn't look like a mapserver context file.", "msLoadContext()");
    return MS_FAILURE;
  }

  for(;;) {
    switch(msyylex()) {
    case(MS_CONTEXT_SECTION_ENV):
      if(loadHashTable(&(context->env)) != MS_SUCCESS) return MS_FAILURE;
      break;
    case(MS_CONTEXT_SECTION_MAPS):
      if(loadHashTable(&(context->maps)) != MS_SUCCESS) return MS_FAILURE;
      break;
    case(MS_CONTEXT_SECTION_PLUGINS):
      if(loadHashTable(&(context->plugins)) != MS_SUCCESS) return MS_FAILURE;
      break;
    case(EOF):
      msSetError(MS_EOFERR, NULL, "msLoadContext()");
      return MS_FAILURE;
    case(END):
      if(msyyin) {
	fclose(msyyin);
	msyyin = NULL;
      }
      return MS_SUCCESS;
      break;
    default:
      msSetError(MS_IDENTERR, "Parsing error near (%s):(line %d)", "msLoadContext()", msyystring_buffer, msyylineno);
      return MS_FAILURE;
    }
  }
}

contextObj *msLoadContext()
{
  contextObj *context = NULL;

  context = (contextObj *)calloc(sizeof(contextObj), 1);
  MS_CHECK_ALLOC(context, sizeof(contextObj), NULL);

  if(initContext(context) != MS_SUCCESS) {
    msFree(context);
    return NULL;
  }

  msAcquireLock(TLOCK_PARSER);

  std::string filename = MS_CONTEXT_PATH;
  filename.append(MS_CONTEXT_FILENAME);

  if((msyyin = fopen(filename.c_str(), "r")) == NULL) {    
    msSetError(MS_IOERR, "(%s)", "msLoadContext()", filename.c_str());
    msReleaseLock(TLOCK_PARSER);
    msFreeContext(context);
    msFree(context);
    return NULL;
  }

  msyystate = MS_TOKENIZE_CONTEXT;
  msyylex(); // sets things up, but doesn't process any tokens

  msyyrestart(msyyin); // start at line 1
  msyylineno = 1;

  if(loadContext(context) != MS_SUCCESS) {
    msFreeContext(context);
    msFree(context);
    msReleaseLock(TLOCK_PARSER);
    if(msyyin) {
      fclose(msyyin);
      msyyin = NULL;
    }
    return NULL;
  }

  return context;  
}

/*
** Couple of helper functions that check environment variables first, then the context object.
*/
const char *msContextGetEnv(contextObj *context, const char *key) 
{
  const char *value;

  if(context == NULL) return NULL;

  value = getenv(key); // check environment vars first
  if(value == NULL) {
    value = msLookupHashTable(&context->env, key);
  }

  return value;
}

const char *msContextGetMap(contextObj *context, const char *key)
{
  const char *value;

  if(context ==NULL) return NULL;

  value = getenv(key); // check environment vars first
  if(value == NULL) {
    value = msLookupHashTable(&context->maps, key);
  }

  return value;
}
