#include <ctype.h>

#include "map.h"
#include "maphash.h"

static unsigned hash(char *key)
{
  unsigned hashval;
  
  for(hashval=0; *key!='\0'; key++)
    hashval = tolower(*key) + 31 * hashval;

  return(hashval % MS_HASHSIZE);
}

hashTableObj msCreateHashTable()
{
  int i;
  hashTableObj table;

  table = (hashTableObj) malloc(sizeof(struct hashObj *)*MS_HASHSIZE);

  for (i=0;i<MS_HASHSIZE;i++)
    table[i] = NULL;

  return(table);
}

struct hashObj *msInsertHashTable(hashTableObj table, char *string, char *data)
{
  struct hashObj *tp;
  unsigned hashval;

  if(!table || !string || !data)
    return(NULL);

  for(tp=table[hash(string)]; tp!=NULL; tp=tp->next)
    if(strcasecmp(string, tp->key) == 0)
      break;

  if(tp == NULL) { /* not found */
    tp = (struct hashObj *) malloc(sizeof(*tp));
    if((tp == NULL) || (tp->key = strdup(string)) == NULL)
      return(NULL);
    hashval = hash(string);
    tp->next = table[hashval];
    table[hashval] = tp;
  } else {
    free(tp->data);
  }

  if((tp->data = strdup(data)) == NULL)
    return(NULL);

  return(tp);
}

char *msLookupHashTable(hashTableObj table, char *string)
{
  struct hashObj *tp;

  if(!table || !string)
    return(NULL);

  for(tp=table[hash(string)]; tp!=NULL; tp=tp->next)
    if(strcasecmp(string, tp->key) == 0)
      return(tp->data);

  return(NULL);
}

void msFreeHashTable(hashTableObj table)
{
  int i;
  struct hashObj *tp=NULL;
  struct hashObj *prev_tp=NULL;

  if(!table) return;

  for (i=0;i<MS_HASHSIZE; i++) {
    if (table[i] != NULL) {
      for (tp=table[i]; tp!=NULL; prev_tp=tp,tp=tp->next,free(prev_tp)) {
	free(tp->key);
	free(tp->data);
      }
    }
    if(tp) free(tp);
  }
  free(table);
  table = NULL;
}


int msRemoveHashTable(hashTableObj table, char *string)
{ 
  struct hashObj *tp;
  struct hashObj *prev_tp=NULL;
  int success = 0;

  if(!table || !string)
    return MS_FAILURE;

  tp=table[hash(string)];
  if (!tp)
    return MS_FAILURE;
  
  prev_tp = NULL;
  while(tp != NULL)
  {
      if (strcasecmp(string, tp->key) == 0)
      {
          if (prev_tp)
          {     
              prev_tp->next = tp->next;
              free(tp);
              break;
          }
          else
          {
              table[hash(string)] = NULL;
              free(tp);
              break;
          }
          success =1;
      }
      prev_tp = tp;
      tp = tp->next;
  }

  if (success)
    return MS_SUCCESS;

  return  MS_FAILURE;
}
      
