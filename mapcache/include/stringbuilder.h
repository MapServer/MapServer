#ifndef STRINGBUILDER_H
#define STRINGBUILDER_H

typedef struct stringbuilder_tag    {
    char* cstr;             /* Must be first member in the struct! */
    int   pos;
    int   size;
    int   reallocs;         /* Performance metric to record the number of string reallocations */
} stringbuilder;

/**
 * Creates a new stringbuilder with the default chunk size
 * 
 */
stringbuilder* sb_new();

/**
 * Destroys the given stringbuilder.  Pass 1 to free_string if the underlying c string should also be freed
 */
void sb_destroy(stringbuilder* sb, int free_string);

/**
 * Creates a new stringbuilder with initial size at least the given size
 */
stringbuilder* sb_new_with_size(int size);

/**
 * Appends at most length of the given src string to the string buffer
 */
void sb_append_strn(stringbuilder* sb, const char* src, int length);

/**
 * Appends the given src string to the string builder
 */
void sb_append_str(stringbuilder* sb, const char* src);

/**
 * Appends the formatted string to the given string builder
 */
void sb_append_strf(stringbuilder* sb, const char* fmt, ...);

/**
 * Allocates and copies a new cstring based on the current stringbuilder contents 
 */
char* sb_make_cstring(stringbuilder* sb);

/**
 * Returns the stringbuilder as a regular C String
 */
#define sb_cstring(sb) ((sb)->cstr)

/**
 * Resets the stringbuilder to empty
 */
#define sb_reset(sb) ((sb)->pos = 0)

#define sb_append_ch(sb, ch)    {                                                       \
        if ((sb)->pos == (sb)->size - 1 )   {                                           \
            (sb)->reallocs++;                                                           \
            (sb)->size = (sb)->size + ((sb)->size >> 2)+ 1;                             \
            (sb)->cstr = (char*)realloc((sb)->cstr, (sb)->size );                       \
        }                                                                               \
        (sb)->cstr[(sb)->pos++] = ch;                                                   \
        (sb)->cstr[(sb)->pos] = '\0';                                                   \
    }                                                                           
                                                            
#endif // STRINGBUILDER_H