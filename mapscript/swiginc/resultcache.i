/* $Id$ */

%extend resultCacheObj
{

    resultCacheMemberObj *getResult(int i)
    {
        if (i >= 0 && i < self->numresults) {
            return &self->results[i];
        }
        return NULL;
    }

}

