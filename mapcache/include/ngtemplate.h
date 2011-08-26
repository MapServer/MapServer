/**
 * Public interface to the ngtemplate engine
 */
#ifndef NGTEMPLATE_H
#define NGTEMPLATE_H

#include <stdio.h>
#include "hashtable.h"
#include "stringbuilder.h"

#define MAXMARKERLENGTH     64
#define MAXMODIFIERLENGTH   128

#define NGT_SECTION_VISIBLE 1
#define NGT_SECTION_HIDDEN  0

/* Pointer to a function that will return a pointer to a template string given its name */ 
typedef char* (*get_template_fn)(const char* name);

/* Pointer to a function that will clean up the template data given the data and its name */ 
typedef void (*cleanup_template_fn)(const char* name, char* template);

/** 
 * Pointer to a function that will be called to modify the output of a template process for a marker
 *  name - The name of the modifier being called
 *  marker - The name of the marker being evaluated
 *  value - The value that the template processor will be using if nothing is done
 *  out_sb - The stringbuilder to which to append changes
 *  args - The arguments, if any
 */
typedef void (*modifier_fn)(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb);

/**
 * Pointer to a function that will be called to get the value of the given variable marker
 *   marker - The marker name
 *
 * Return an allocated string containing the value, or 0
 */
typedef char* (*get_variable_fn)(const char* marker);

typedef struct ngt_dictionary_tag   {
    hashtable dictionary;
    int should_expand;                          /* Determines whether the section represented by
                                                    this dictionary should be shown */
    struct ngt_dictionary_tag* parent;
} ngt_dictionary;

// Represents a start or stop marker delimiter
#define MAX_DELIMITER_LENGTH    8               /* I really don't know why someone would want a 
                                                    delimiter this long, but just in case */
typedef struct delimiter_tag    {
    int length;
    char literal[MAX_DELIMITER_LENGTH];         /* Keep the literal as the last record in the 
                                                    struct so people can do the "overallocated 
                                                    struct" trick if they need more than 
                                                    MAX_DELIMITER_LENGTH */
} delimiter;

typedef struct ngt_template_tag {
    ngt_dictionary* dictionary;                 
    
    hashtable modifiers;
    
    char*   template;
    
    delimiter start_delimiter;                  /* Characters that signify start of a marker */
    delimiter end_delimiter;                    /* Characters that signify end of a marker */
        
    modifier_fn         modifier_missing;
    get_variable_fn     variable_missing;
} ngt_template;

/**
 * Creates a new ngt_template
 *
 * Returns the ngt_template created, or NULL if this could not be done.  It is up
 * to the caller to manage this dictionary
 */
ngt_template* ngt_new();

/**
 * Creates a new ngt Template Dictionary, ready to be filled with values
 */
ngt_dictionary* ngt_dictionary_new();

/** 
 * Destroys the given template.  Does NOT destroy the dictionary associated with the template
 */
void ngt_destroy(ngt_template* tpl);

/**
 * Destroys the given template dictionary and any sub-dictionaries
 */
void ngt_dictionary_destroy(ngt_dictionary* dict);

/**
 * Sets the dictionary for the given template.  The dictionary can be NULL
 */
void ngt_set_dictionary(ngt_template* tpl, ngt_dictionary* dict);

/**
 * Loads the template string from the given file pointer.  Does NOT close the pointer
 *
 * Returns 0 if successful, -1 otherwise
 */
int ngt_load_from_file(ngt_template* tpl, FILE* fp);

/**
 * Loads the template string from the given file name
 *
 * Returns 0 if successful, -1 otherwise
 */
int ngt_load_from_filename(ngt_template* tpl, const char* filename);

/**
 * Sets the default start and end delimiters for the given template
 *
 * NOTES:
 *  - Each delimiter must have at least one character
 *  - Delimiters with more than 8 characters will be truncated
 *  - Delimiters can still be overridden inside the template 
 */
void ngt_set_delimiters(ngt_template* tpl, const char* start_delimiter, const char* end_delimiter);

/**
 * Sets a modifier function that can be called when the given modifier name is encountered
 * in the template.  The modifier will have the opportunity to adjust the output of the 
 * marker, and will be passed in any arguments.  If another modifier is already present with
 * the same name, that modifier will be replaced.
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_add_modifier(ngt_template* tpl, const char* name, modifier_fn mod_fn);

/**
 * Sets a modifier function that will be called when a modifier in the template does not
 * resolve to any known modifiers.  The function will have the opportunity to adjust the output
 * of the marker, and will be passed any arguments.
 */
void ngt_set_modifier_missing_cb(ngt_template* tpl, modifier_fn mod_fn);

/**
 * Sets a callback function that will be called when no value for a variable marker can be
 * found.  The function will have the opportunity to give the value of the variable by appending
 * to the out_sb string builder.
 */
void ngt_set_variable_missing_cb(ngt_template* tpl, get_variable_fn get_fn);

/**
 * Returns nonzero if the variable with the given marker name equals str, zero otherwise
 */
int ngt_variable_equals(ngt_dictionary* dict, const char* marker, const char* str);

/**
 * Sets a string value in the template dictionary.  Any instance of "marker" in the template 
 * will be replaced by "value"
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_set_string(ngt_dictionary* dict, const char* marker, const char* value);

/**
 * Sets a string value in the template dictionary using printf-style format specifiers.  Any
 * instance of "marker" in the template will be replaced by "value"
 *
 * Returns 0 if the operations succeeded, -1 otherwise
 */
int ngt_set_stringf(ngt_dictionary* dict, const char* marker, const char* fmt, ...);

/**
 * Sets an integer value in the template dictionary.  Any
 * instance of "marker" in the template will be replaced by "value"
 *
 * Returns 0 if the operations succeeded, -1 otherwise
 */
int ngt_set_int(ngt_dictionary* dict, const char* marker, int value);

/**
 * On an include template dictionary, sets the filename that will be loaded to obtain the template data
 * NOTES: - It is illegal to call this function on a string value marker
 *        - Calling template_set_include_cb is not necessary if you set a filename, as the file will
 *          be looked up for you.  However, if you need to perform custom logic (such as searching
 *          through a list of pre-defined directories for the file), you can do the load logic yourself
 *          by registering a callback function with template_set_include_cb.  When the function is called, 
 *          the "name" argument will be the filename you set instead of the include marker name
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_set_include_filename(ngt_dictionary* dict, const char* marker, const char* filename);

/**
 * On an include template dictionary, sets the callbacks to be called when the system needs the template
 * string for the given include name.  Also, prove a cleanup_template function to call when the
 * system no longer needs the template data.
 * NOTES: - It is illegal to call this function on a string value marker
 *        - It is legal to pass null to cleanup_template
 *        - The first call to the callback for every unique name will be memoized.  The function will 
 *          NOT be called for every expansion of the marker
 * 
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_set_include_cb(ngt_dictionary* dict, const char* marker, get_template_fn get_template, 
                            cleanup_template_fn cleanup_template);

/**
 * Sets the visibility for the section indicated by "section" in the given dictionary
 * 
 * NOTE: visibility can be one of NGT_SECTION_VISIBLE, NGT_SECTION_HIDDEN
 */
void ngt_set_section_visibility(ngt_dictionary* dict, const char* section, int visibility);

/**
 * Adds a child dictionary under the given marker.  If there is an existing dictionary under this marker, 
 * the new dictionary will be ADDED to the end of the list, NOT replace the old one
 *
 * NOTE: Set visible to non-zero if you wish section to be shown automatically, zero if you wish to 
 *      hide it and show later with ngt_show_section();
 *
 * Returns 0 if the operation succeeded, -1 otherwise
 */
int ngt_add_dictionary(ngt_dictionary* dict, const char* marker, ngt_dictionary* child, int visible);

/**
 * Expands the given template according to the dictionary, putting the result in "result" pointer.
 * Sufficient space will be allocated for the result, and it will then be 
 * up to the caller to manage this memory
 *
 * Returns 0 if the template was successfully processed, -1 if there was an error
 */
int ngt_expand(ngt_template* tpl, char** result);

/**
 * Returns that Global Dictionary in which the Standard Values for all templates are defined 
 */
ngt_dictionary* ngt_get_global_dictionary();

/**
 * Pretty-prints the dictionary key value pairs, one per line, with nested dictionaries tabbed
 */
void ngt_print_dictionary(ngt_dictionary* dict, FILE* out);

#endif // NGTEMPLATE_H