/**********************************************************************
 * $Id$
 *
 * mapows.c - OGC Web Services (WMS, WFS) support functions
 *
 **********************************************************************
 * $Log$
 * Revision 1.1  2002/09/03 03:19:51  dan
 * Set the bases for WFS Server support + moved some WMS/WFS stuff to mapows.c
 *
 **********************************************************************/

#include "map.h"

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR)

/*
** msOWSDispatch() is the entry point for any OWS request (WMS, WFS, ...)
** - If this is a valid request then it is processed and MS_SUCCESS is returned
**   on success, or MS_FAILURE on failure.
** - If this does not appear to be a valid OWS request then MS_DONE
**   is returned and MapServer is expected to process this as a regular
**   MapServer request.
*/
int msOWSDispatch(mapObj *map, char **names, char **values, int numentries)
{
    int status = MS_DONE;

#ifdef USE_WMS_SVR
    if ((status = msWMSDispatch(map, names, values, numentries)) != MS_DONE )
        return status;
#endif
#ifdef USE_WFS_SVR
    if ((status = msWFSDispatch(map, names, values, numentries)) != MS_DONE )
        return status;
#endif

    return MS_DONE;  /* Not a WMS/WFS request... let MapServer handle it */
}


/*
** msRenameLayer()
*/
static int msRenameLayer(layerObj *lp, int count)
{
    char *newname;
    newname = (char*)malloc((strlen(lp->name)+5)*sizeof(char));
    if (!newname) 
    {
        msSetError(MS_MEMERR, NULL, "msRenameLayer()");
        return MS_FAILURE;
    }
    sprintf(newname, "%s_%2.2d", lp->name, count);
    free(lp->name);
    lp->name = newname;
    
    return MS_SUCCESS;
}

/*
** msOWSMakeAllLayersUnique()
*/
int msOWSMakeAllLayersUnique(mapObj *map)
{
  int i, j;

  // Make sure all layers in the map file have valid and unique names
  for(i=0; i<map->numlayers; i++)
  {
      int count=1;
      for(j=i+1; j<map->numlayers; j++)
      {
          if (map->layers[i].name == NULL || map->layers[j].name == NULL)
          {
              msSetError(MS_MISCERR, 
                         "At least one layer is missing a name in map file.", 
                         "msOWSMakeAllLayersUnique()");
              return MS_FAILURE;
          }
          if (strcasecmp(map->layers[i].name, map->layers[j].name) == 0 &&
              msRenameLayer(&(map->layers[j]), ++count) != MS_SUCCESS)
          {
              return MS_FAILURE;
          }
      }

      // Don't forget to rename the first layer if duplicates were found
      if (count > 1 && msRenameLayer(&(map->layers[i]), 1) != MS_SUCCESS)
      {
          return MS_FAILURE;
      }
  }
  return MS_SUCCESS;
}


/*
** msOWSGetOnlineResource()
**
** Return the online resource for this service.  First try to lookup 
** specified metadata, and if not found then try to build the URL ourselves.
**
** Returns a newly allocated string that should be freed by the caller or
** NULL in case of error.
*/
char * msOWSGetOnlineResource(mapObj *map, const char *metadata_name)
{
    const char *value;
    char *online_resource = NULL;

    // We need this script's URL, including hostname.
    // Default to use the value of the "onlineresource" metadata, and if not
    // set then build it: "http://$(SERVER_NAME):$(SERVER_PORT)$(SCRIPT_NAME)?"
    if ((value = msLookupHashTable(map->web.metadata, (char*)metadata_name))) 
    {
        online_resource = strdup(value);
    }
    else 
    {
        const char *hostname, *port, *script, *protocol="http";
        hostname = getenv("SERVER_NAME");
        port = getenv("SERVER_PORT");
        script = getenv("SCRIPT_NAME");

        // HTTPS is set by Apache to "on" in an HTTPS server ... if not set
        // then check SERVER_PORT: 443 is the default https port.
        if ( ((value=getenv("HTTPS")) && strcasecmp(value, "on") == 0) ||
             ((value=getenv("SERVER_PORT")) && atoi(value) == 443) )
        {
            protocol = "https";
        }

        if (hostname && port && script) {
            online_resource = (char*)malloc(sizeof(char)*(strlen(hostname)+strlen(port)+strlen(script)+10));
            if (online_resource) 
                sprintf(online_resource, "%s://%s:%s%s?", protocol, hostname, port, script);
        }
        else 
        {
            msSetError(MS_CGIERR, "Impossible to establish server URL.  Please set \"%s\" metadata.", "msWMSCapabilities()", metadata_name);
            return NULL;
        }
    }

    if (online_resource == NULL) 
    {
        msSetError(MS_MEMERR, NULL, "msWMSCapabilities");
        return NULL;
    }

    return online_resource;
}






#endif /* USE_WMS_SVR || USE_WFS_SVR */



