#include "map.h"
#include "maperror.h"

// Entry point for WCS requests
int msWCSDispatch(mapObj *map, cgiRequestObj *request)
{
#ifdef USE_WCS_SVR
  return(MS_DONE); // no WCS support committed yet
#else
  msSetError(MS_WCSERR, "WCS server support is not available.", "msWCSDispatch()");
  return(MS_FAILURE);
#endif
}
