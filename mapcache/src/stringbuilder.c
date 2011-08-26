/**
 * Stringbuilder - a library for working with C strings that can grow dynamically as they are appended
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "platform.h"
#include "stringbuilder.h"


/**
 * Creates a new stringbuilder with the default chunk size
 * 
 */
stringbuilder* sb_new() {
    return sb_new_with_size(1024);      // TODO: Is there a heurisitic for this?
}

/**
 * Creates a new stringbuilder with initial size at least the given size
 */
stringbuilder* sb_new_with_size(int size)   {
    stringbuilder* sb;
    
    sb = (stringbuilder*)malloc(sizeof(stringbuilder));
    sb->size = size;
    sb->cstr = (char*)malloc(size);
    sb->pos = 0;
    sb->reallocs = 0;
    
    return sb;
}

/**
 * Destroys the given stringbuilder
 */
void sb_destroy(stringbuilder* sb, int free_string) {
    if (free_string)    {
        free(sb->cstr);
    }
    
    free(sb);
}

/**
 * Appends at most length of the given src string to the string buffer
 */
void sb_append_strn(stringbuilder* sb, const char* src, int length) {
    int sdiff;
    
    sdiff = length - (sb->size - sb->pos);
    if (sdiff > 0)  {
        sb->size = sb->size + sdiff + (sdiff >> 2) + 1;
        sb->cstr = (char*)realloc(sb->cstr, sb->size);
        sb->reallocs++;
    }
    
    memcpy(sb->cstr + sb->pos, src, length);
    sb->pos += length;
}

/**
 * Appends the given src string to the string builder
 */
void sb_append_str(stringbuilder* sb, const char* src)  {
    sb_append_strn(sb, src, strlen(src));
}

/**
 * Appends the formatted string to the given string builder
 */
void sb_append_strf(stringbuilder* sb, const char* fmt, ...)    {
    char *str;
    va_list arglist;

    va_start(arglist, fmt);
    xp_vasprintf(&str, fmt, arglist);
    va_end(arglist);
    
    if (!str)   {
        return;
    }
    
    sb_append_str(sb, str);
    free(str);
}

/**
 * Allocates and copies a new cstring based on the current stringbuilder contents 
 */
char* sb_make_cstring(stringbuilder* sb)    {
    char* out;
    
    if (!sb->pos)   {
        return 0;
    }
    
    out = (char*)malloc(sb->pos + 1);
    strcpy(out, sb_cstring(sb));
    
    return out;
}

