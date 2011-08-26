/**
 * Internal functions for the ngtemplate engine
 */

#include <stdlib.h>
#include <string.h>
#include "ngtemplate.h"
#include "internal.h"

/**
 * The hash function we will use for the template dictionary
 */
int _dictionary_item_hash(const void* key)  {
    // Hash by the marker value
    return ht_hashpjw(((_dictionary_item*)key)->marker);
}

/**
 * The function we will use to determine if two dictionary_items match
 */
int _dictionary_item_match(const void* key1, const void* key2)  {
    return !strcmp(((_dictionary_item*)key1)->marker, ((_dictionary_item*)key2)->marker);
}

/**
 * The function that will be called when a dictionary_item must be destroyed
 */
void _dictionary_item_destroy(void *data)   {
    _dictionary_item* d = (_dictionary_item*)data;
    
    if (d->type == ITEM_STRING) {
        free(d->val.string_value);
    } else if (d->type & ITEM_D_LIST && d->val.d_list_value != 0) {
        list_destroy(d->val.d_list_value);
    } 
    
    if (d->type == ITEM_INCLUDE)    {
        if (d->val.include_value.cleanup_template)  {
            d->val.include_value.cleanup_template(d->marker, d->val.include_value.template);
        }
        
        if (d->val.include_value.filename)  {
            free(d->val.include_value.filename);
        }
    }
    
    if (d->marker)  {
        free(d->marker);
    }
    
    free(d);
}

/**
 * The function that will be called when an ngt_dictionary must be destroyed
 */ 
void _dictionary_destroy(void* data)    {
    ngt_dictionary* dict = (ngt_dictionary*)data;
    
    ht_destroy((hashtable*)dict);
    free(dict);
}

/**
 * The hash function we will use for the modifier dictionary
 */
int _modifier_hash(const void* key) {
    // Hash by the marker value
    return ht_hashpjw(((_modifier*)key)->name);
}

/**
 * The function we will use to determine if two modifiers match
 */
int _modifier_match(const void* key1, const void* key2) {
    return !strcmp(((_modifier*)key1)->name, ((_modifier*)key2)->name);
}

/**
 * The function that will be called when a modifier must be destroyed
 */
void _modifier_destroy(void *data)  {
    _modifier* m = (_modifier*)data;
    
    free(m->name);
    free(m);
}

/**
 * Destroys the given template
 * Signature comforms to hashtable and list function pointer signature
 */
void _destroy_template(void* data)  {
    ngt_template* tpl = (ngt_template*)data;
    
    ht_destroy(&tpl->modifiers);
    
    free(tpl);
}

/**
 * Helper function to retrieve a template string from a file
 */
char* _get_template_from_file(FILE* fd) {
    int length;
    int ch, i;
    char* contents;
    
    fseek(fd, 0, SEEK_END);
    length = ftell(fd);
    if (!length)    {
        close(fd);
        return 0;
    }
    rewind(fd);
    
    // TODO: Do this with block reads so it's faster
    i = 0;
    contents = (char*)malloc(length+1);
    while ((ch = fgetc(fd)) != EOF) {
        contents[i++] = ch;
    }
    close(fd);
    
    return contents;
}

/** 
 * Callback function to retrieve a template string from a file name
 */
char* _get_template_from_filename(const char* filename) {
    FILE* fd;
    char* template;
    
    fd = fopen(filename, "rb");
    if (!fd)    {
        return 0;
    }
    
    template = _get_template_from_file(fd);
    close(fd);
    
    return template;
}

/**
 * Helper function - allocates and initializes a new _dictionary_item
 */
_dictionary_item* _new_dictionary_item()    {
    _dictionary_item* item;
    
    item = (_dictionary_item*)malloc(sizeof(_dictionary_item));
    memset(item, 0, sizeof(_dictionary_item));
    
    return item;
}

/**
 * Helper function for the template_set_* functions.  Does NOT make a copy of the given value
 * string, but uses the pointer directly.
 */
int _set_string(ngt_dictionary* dict, const char* marker, char* value)  {
    _dictionary_item* item, *prev_item;
    
    item = _new_dictionary_item();
    item->type = ITEM_STRING;
    item->marker = (char*)malloc(strlen(marker) + 1);
    item->val.string_value = value;
    
    strcpy(item->marker, marker);
        
    if (ht_insert((hashtable*)dict, item) == 1) {
        // Already in the table, replace
        prev_item = item;
        ht_remove((hashtable*)dict, (void *)&prev_item);
        _dictionary_destroy((void*)prev_item);
        
        ht_insert((hashtable*)dict, item);
    }
    
    return 0;
}

/**
 * Callback function for cleanup of default file template-includes
 */
void _cleanup_template(const char* filename, char* template)    {
    free(template);
}

/** 
 * Helper Function - Queries the dictionary for the item under the given marker and returns the 
 * item if it exists
 * 
 * Returns a pointer to the item if it exists, zero if not
 */
_dictionary_item* _query_item(ngt_dictionary* dict, const char* marker) {
    _dictionary_item* query_item, *item;
    
    if (!dict)  {
        return 0;
    }
    
    query_item = (_dictionary_item*)malloc(sizeof(_dictionary_item));
    memset(query_item, 0, sizeof(_dictionary_item));
    query_item->marker = (char*)marker;
    query_item->val.string_value = 0;
    
    item = query_item;
    
    if (ht_lookup((hashtable*)dict, (void*)&item) != 0) {
        // No value for this key
        free(query_item);
        return 0;
    }
    
    free(query_item);
    
    return item;
}

/**
 * Helper function - Gets the modifier by the given name if it exists anywhere in the
 * template modifier list
 */
_modifier* _query_modifier(ngt_template* tpl, const char* name) {
    _modifier* query_mod, *mod;
    if (!tpl)   {
        return 0;
    }
    
    query_mod = (_modifier*)malloc(sizeof(_modifier));
    query_mod->name = (char*)name;
    
    mod = query_mod;
    
    if (ht_lookup(&tpl->modifiers, (void*)&mod) != 0)   {
        mod = 0;
    }
    
    free(query_mod);
    
    return mod; 
}

/**
 * Gets the string value in the dictionary for the given marker
 * NOTE: The string returned is managed by the dictionary.  Do NOT retain a reference to
 *      it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found
 */
const char* _get_string_value_ref(ngt_dictionary* dict, const char* marker) {
    _dictionary_item* item;
    if (!dict)  {
        return 0;
    }
    
    item = _query_item(dict, marker);
    if (!item)  {
        // Look in the parent dictionary
        item = _query_item(dict->parent, marker);
    }
    
    if (!item && dict != ngt_get_global_dictionary())   {
        // Last chance, look up in global dictionary
        item = _query_item(ngt_get_global_dictionary(), marker);
    }
    
    if (!item || item->type != ITEM_STRING) {
        // Getting a non-string item as a string is not defined
        return 0;
    }
    
    return item->val.string_value;
}

/**
 * Gets the dictionary list value in the dictionary for the given marker
 * NOTE: The dictionary list returned is managed by the dictionary.  Do NOT retain a reference to
 *      it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found or if the given node is not
 * a D_LIST
 */
const list* _get_dictionary_list_ref(ngt_dictionary* dict, const char* marker)  {
    _dictionary_item* item;
    if (!dict)  {
        return 0;
    }
    
    item = _query_item(dict, marker);
    if (!item)  {
        // Look in the parent dictionary
        item = _query_item(dict->parent, marker);
    }
    
    if (!item && dict != ngt_get_global_dictionary())   {
        // Last chance, look up in global dictionary
        item = _query_item(ngt_get_global_dictionary(), marker);
    }
    
    if (!item || !(item->type & ITEM_D_LIST))   {
        // Getting a non-d-list item as a d-list is not defined
        return 0;
    }
    
    return item->val.d_list_value;
}

/**
 * Gets the include params struct in the dictionary for the given marker
 * NOTE: The include params struct returned is managed by the dictionary.  Do NOT retain a reference
 *       to it or destroy it
 *
 * Returns the pointer to the value of the marker, or 0 if not found or if the given node is not
 * an INCLUDE
 */
struct _include_params_tag* _get_include_params_ref(ngt_dictionary* dict, const char* marker)   {
    _dictionary_item* item;
    if (!dict)  {
        return 0;
    }
    
    item = _query_item(dict, marker);
    if (!item)  {
        // Look in the parent dictionary
        item = _query_item(dict->parent, marker);
    }
    
    if (!item && dict != ngt_get_global_dictionary())   {
        // Last chance, look up in global dictionary
        item = _query_item(ngt_get_global_dictionary(), marker);
    }
    
    if (!item || item->type != ITEM_INCLUDE)    {
        // Getting a non-include item as an include is not defined
        return 0;
    }
    
    return &item->val.include_value;
}

/**
 * Helper function - returns nonzero if the portion of the input string starting at p matches the given
 * marker, 0 otherwise
 */
int _match_marker(const char* p, delimiter* delim)  {
    int i;
    
    i = 0;
    while (i < delim->length && *p && *p == delim->literal[i])  {
        p++, i++;
    }
    
    return i == delim->length;
}

/**
 * Helper function. Copies the data from the source delimiter into dest
 */
void _copy_delimiter(delimiter* dest, const delimiter* src) {
    if (!dest || !src)  {
        return;
    }
    
    strncpy(dest->literal, src->literal, src->length);
    dest->length = src->length;
}

/**
 * Helper function.  Returns a freshly allocated copy of the given delimiter
 */
delimiter* _duplicate_delimiter(const delimiter* delim) {
    delimiter* new_delim;
    
    new_delim = (delimiter*)malloc(sizeof(delimiter));
    _copy_delimiter(new_delim, delim);
    
    return new_delim;
}

/**
 * Helper function - assuming that p points to the character after '<start delim>=', reads the string up 
 *                  until a space or '=<end delim>', then modifies the delimiter to be the new delimiter
 *
 * Returns the adjusted character pointer, pointing to one after the last position processed
 */
char* _extract_delimiter(const char* p, delimiter* delim)   {
    int length;
    
    length = 0;
    while (*p && *p != '=' && *p != ' ' && *p != '\t')  {
        delim->literal[length++] = *p++;
        
        if (length >= MAX_DELIMITER_LENGTH) {
            fprintf(stderr, "FATAL: Delimiter cannot be greater than %d characters\n", MAX_DELIMITER_LENGTH);
            exit(-1);
        }
    }
    
    delim->length = length;
        
    return (char*)p;
}

/**
 * Helper function - creates a new context and duplicates the contents of the given context
 */ 
_parse_context* _duplicate_context(_parse_context* ctx) {
    _parse_context* new_ctx;
    
    new_ctx = (_parse_context*)malloc(sizeof(_parse_context));
    memcpy(new_ctx, ctx, sizeof(_parse_context));
    
    return new_ctx;
}

/**
 * Helper function - appends the marked whitespace at the current position in the output
 */
void _append_line_whitespace(_parse_context* line_ctx, _parse_context* out_ctx) {
    char* ptr;
    
    if (line_ctx->parent)   {
        _append_line_whitespace(line_ctx->parent, out_ctx);
    } else {
        return;
    }
    
    if (!line_ctx->expanding_include)   {
        return;
    }
    
    if (line_ctx->parent->line_ws_range == 0)   {
        return;
    }
    
    ptr = line_ctx->parent->line_ws_start;
    while(ptr)  {
        
        sb_append_ch(out_ctx->out_sb, *ptr++);
        
        if ((ptr - line_ctx->parent->line_ws_start) == line_ctx->parent->line_ws_range) {
            // We're done
            ptr = 0;
        }
    }
}

/**
 * Helper function - Processes a set delimiter {{= =}} sequence in the template
 */
void _process_set_delimiter(_parse_context* ctx)    {
    ctx->in_ptr = _extract_delimiter(ctx->in_ptr, &(ctx->active_start_delimiter));
    EAT_SPACES(ctx->in_ptr);
    
    // OK now we should either be at the new end delimiter or an =
    if (*ctx->in_ptr == '=')    {
        ctx->in_ptr++;
        if (!_match_marker(ctx->in_ptr, &(ctx->active_end_delimiter)))  {
            fprintf(stderr, "Unexpected '=' in middle of Set Delimiter\n");
            exit(-1);
        }
        
        ctx->in_ptr += ctx->active_end_delimiter.length;
        
        // In these cases, the start and end delimiters are the same
        _copy_delimiter(&(ctx->active_end_delimiter), &(ctx->active_start_delimiter));
    } else {
        delimiter* old_end_delimiter = _duplicate_delimiter(&ctx->active_end_delimiter);
        ctx->in_ptr = _extract_delimiter(ctx->in_ptr, &(ctx->active_end_delimiter));
        EAT_SPACES(ctx->in_ptr);
        
        if (*ctx->in_ptr != '=')    {
            fprintf(stderr, "Unexpected '%c' after end delimiter in Set Delimiter\n", *ctx->in_ptr);
            exit(-1);
        }
        ctx->in_ptr++;
        
        if (!_match_marker(ctx->in_ptr, old_end_delimiter)) {
            fprintf(stderr, "Unexpected characters after '=' in Set Delimiter\n");
            exit(-1);
        }
        
        ctx->in_ptr += old_end_delimiter->length;
        free(old_end_delimiter);
    }
}

/*
 * Helper function - Given a marker with modifiers and the original value, parses the modifiers and 
 *                   runs them
 */
void _process_modifiers(const char* marker, const char* modifiers, const char* value, _parse_context* ctx)  {
    int applied_modifier;
    stringbuilder* sb;
    
    applied_modifier = 0;   
    if (ctx->mode & MODE_MARKER_MODIFIER)   {
        char *p;
        char* cur_value;
        
        p = (char*)modifiers;
        sb = sb_new();
        cur_value = (char*)value;
                
        while (1)   {
            char modifier[MAXMODIFIERLENGTH];
            char args[MAXMODIFIERLENGTH];
            int m;
            char arg_separator;
            
            _modifier* mod;
            sb_reset(sb);
                                    
            // Modifier
            m = 0;
            while (m < MAXMODIFIERLENGTH && *p && *p != ':' && *p != '=')   {
                modifier[m++] = *p++;
            }
            modifier[m] = '\0';
            
            // HACK: In CTemplate, custom modifiers MUST start with x-.  When they do, the following
            // "modifier" is not actually a modifier at all, but the ARGUMENTS to the modifier!
            //
            // This is silly (why didn't they just follow the '=' convention like with built-ins?!), 
            // but we treat it here for backward-compatibility
            arg_separator = '=';
            if (modifier[0] == 'x' && modifier[1] == '-' && *p == ':')  {
                // CTemplate-style custom modifier, change the arg separator
                arg_separator = ':';
                p++;    // Skip over the ':' so the args parser below thinks it's encountered
                        // and equals sign
            }
            
            // Args
            m = 0;
            while (m < MAXMODIFIERLENGTH && *p && *p != ':')    {
                if (*p == '=' && arg_separator == '=')  {
                    p++;
                    continue;
                }
                
                args[m++] = *p++;
            }
            args[m] = '\0';
            
            mod = _query_modifier(ctx->template, modifier);
            if (mod)    {
                mod->modifier(modifier, args, marker, cur_value, sb);
                applied_modifier = 1;
                sb_append_ch(sb, '\0');
                
                if (cur_value && cur_value != value)    {
                    free(cur_value);
                }
                cur_value = sb_make_cstring(sb);
                
            } else {
                // Find the first parse context up the chain with a valid
                // template dictionary and modifier missing cb
                _parse_context* this_ctx = ctx;
                ngt_template* tpl = 0;
                while (this_ctx)    {
                    if (this_ctx && this_ctx->template && this_ctx->template->modifier_missing) {
                        tpl = this_ctx->template;
                        break;
                    }

                    this_ctx = this_ctx->parent;
                }

                if (tpl && tpl->modifier_missing)   {
                    // Give user code a chance to fill in this value
                    tpl->modifier_missing(modifier, args, marker, cur_value, sb);
                    applied_modifier = 1;
                    sb_append_ch(sb, '\0');
                    
                    if (cur_value && cur_value != value)    {
                        free(cur_value);
                    }
                    cur_value = sb_make_cstring(sb);
                } 
            }
            
            if (*p++ != ':')    {
                break;
            }
        }
        
        if (cur_value && cur_value != value)    {
            free(cur_value);
        }
    }
    
    if (!applied_modifier)  {
        // Output the value ourselves
        sb_append_str(ctx->out_sb, value);
    } else {
        sb_append_str(ctx->out_sb, sb_cstring(sb));
    }
}

/**
 * Helper function - Expands a variable marker in the template
 */
void _process_variable(const char* marker, const char* modifiers, _parse_context* ctx)  {
    char* value;
    
    value = (char*)_get_string_value_ref(ctx->active_dictionary, marker);
    if (!value) {
        
        // Find the first parse context up the chain with a valid
        // template dictionary and variable missing cb
        _parse_context* this_ctx = ctx;
        ngt_template* tpl = 0;
        while (this_ctx)    {
            if (this_ctx && this_ctx->template && this_ctx->template->variable_missing) {
                tpl = this_ctx->template;
                break;
            }
            
            this_ctx = this_ctx->parent;
        }
        
        if (tpl && tpl->variable_missing)   {
            // Give user code a chance to fill in this value
            value = tpl->variable_missing(marker);
        }
        
        if (!value) {
            return;
        }
    }
    
    _process_modifiers(marker, modifiers, value, ctx);
}

/**
 * Helper function - Determines if the marker is a special separator section and, if so, 
 *                  processes it
 *
 * Returns nonzero (true) if it was a separator section, zero otherwise
 */
int _process_separator_section(const char* marker, _parse_context* ctx) {
    char* separator_name;
    char* saved_section;
    char* resume;
    int saved_output_pos;
    
    // Before doing normal expansion logic, check to see if this is a separator
    separator_name = (char*)malloc(strlen(ctx->current_section) + strlen("_separator") + 1);
    sprintf(separator_name, "%s_separator", ctx->current_section);
    if (strcmp(marker, separator_name)) {
        // Not a separator
        free(separator_name);
        return 0;
    }
    
    free(separator_name);
    saved_output_pos = ctx->out_sb->pos;
                        
    // We leave it up to the implementor to put the separator in the right place.  We expand it
    // here and now
    saved_section = ctx->current_section;
    ctx->current_section = (char*)marker;
        
    resume = _process(ctx);
        
    if (ctx->last_expansion)    {
        // We don't expand separators unless we're not the last or only one
        ctx->out_sb->pos = saved_output_pos;
    }
        
    ctx->in_ptr = resume;
    ctx->current_section = saved_section;
    
    return 1;
}

/**
 * Helper function - Expands a section in the template
 */
void _process_section(const char* marker, _parse_context* ctx, int is_include)  {
    list *d_list_value;
    list_element* child;
    char* resume;
    _parse_context* section_ctx;
    int saved_out_pos;
    
    if (_process_separator_section(marker, ctx))    {
        // Section was a separator, which has to be handled differently
        return;
    }
    
    // We loop through each dictionary in the dictionary list for this marker 
    // (0 or more), and for each one we recursively process the template there
    d_list_value = (list*)_get_dictionary_list_ref(ctx->active_dictionary, marker);
    
    if (is_include) {
        section_ctx = ctx;
    } else {
        section_ctx = _duplicate_context(ctx);
        section_ctx->parent = ctx;
        section_ctx->expanding_include = 0;
    }
    
    section_ctx->current_section = (char*)marker;
    resume = section_ctx->in_ptr;
    
    child = 0;
    if (d_list_value)   {
        child = list_head(d_list_value);
    }
    
    do {
        section_ctx->last_expansion = (child && list_next(child) == 0) ? 1: 0;
        section_ctx->active_dictionary = child? (ngt_dictionary*)list_data(child) : 0;
        section_ctx->in_ptr = ctx->in_ptr;
        saved_out_pos = ctx->out_sb->pos;
            
        resume = _process(section_ctx);
        sb_append_ch(ctx->out_sb, '\0');
        ctx->out_sb->pos--;
                        
        if (resume < 0) {
            // There was an error in the inner section
            fprintf(stderr, "Error expanding %s section\n", marker);
            exit(-1);
        }
            
        if (!section_ctx->active_dictionary || !section_ctx->active_dictionary->should_expand)  {
            // We only use the output if we had an actual dictionary, else we have to throw it 
            // out because we didn't ultimately expand anything
            ctx->out_sb->pos = saved_out_pos;
        }
    
    } while (child && (child = list_next(child)) != 0);
    
    if (!is_include)    {
        free(section_ctx);
    }
    ctx->in_ptr = resume;   // Now we can skip over the read template section
}

/**
 * Helper function - Processes template include markers
 */
void _process_include(const char* marker, _parse_context* ctx)  {
    struct _include_params_tag* params;
    _parse_context* include_ctx;
    
    params = _get_include_params_ref(ctx->active_dictionary, marker);
    if (!params || !params->get_template)   {
        // Can't do anything with this one
        return;
    }
        
    // The way this works is if someone has set a filename for us, we'll pass that to 
    // the get_template callback, else we'll just pass the marker name and hope they know
    // what to do with it
    if (!params->template)  {
        if (params->filename)   {
            params->template = params->get_template(params->filename);
        } else {
            params->template = params->get_template(marker);
        }
    }

    if (!params->template)  {
        // We got nothin'
        return;
    }   
    
    include_ctx = _duplicate_context(ctx);
    include_ctx->parent = ctx;
    include_ctx->in_ptr = params->template;
    include_ctx->template_line = 1;
    include_ctx->expanding_include = 1;
    
    // Now we can treat it just like a normal section, then restore the original template
    _process_section(marker, include_ctx, 1);
    
    free(include_ctx);
}

/**
 * Internal implementation of the template process function
 *
 * Returns -1 if there was an error, otherwise records a pointer to the position in the template when the function ended
 */ 
char* _process(_parse_context *ctx) {
    int m, mod;
    char marker[MAXMARKERLENGTH];
    char modifiers[MAXMODIFIERLENGTH];

    ctx->mode = MODE_NORMAL;
    m = mod = 0;
    
    while (*ctx->in_ptr)    {
        if (ctx->mode & MODE_MARKER)    {
            EAT_WHITESPACE(ctx->in_ptr);
            
            if (ctx->mode & MODE_MARKER_DELIMITER)  {
                _process_set_delimiter(ctx);
                ctx->mode = MODE_NORMAL;
                continue;

            } else if (_match_marker(ctx->in_ptr, &(ctx->active_end_delimiter)))    {
                // At the end of the marker
                marker[m] = '\0';
                modifiers[mod] = '\0';
                ctx->in_ptr += ctx->active_end_delimiter.length;
                
                if (ctx->mode & MODE_MARKER_VARIABLE)   {
                    _process_variable(marker, modifiers, ctx);
                    
                } else if (ctx->mode & MODE_MARKER_SECTION) {
                    _process_section(marker, ctx, 0);
                     
                } else if (ctx->mode & MODE_MARKER_ENDSECTION)  {
                    
                    if (strcmp(ctx->current_section, marker) != 0)  {
                        fprintf(stderr, "End section '%s' does not match start section '%s'\n", marker, ctx->current_section);
                        return (char*)-1;
                    } else {                    
                        // We're done expanding this section
                        return ctx->in_ptr;
                    }
                } else if (ctx->mode & MODE_MARKER_INCLUDE) {
                    _process_include(marker, ctx);
                    
                }
                
                ctx->mode = MODE_NORMAL;
                continue;
            } else if (ctx->mode == MODE_MARKER)    {
                switch(*ctx->in_ptr)    {
                    case '!':   ctx->mode |= MODE_MARKER_COMMENT;       ctx->in_ptr++;  break;
                    case '#':   ctx->mode |= MODE_MARKER_SECTION;       ctx->in_ptr++;  break;
                    case '/':   ctx->mode |= MODE_MARKER_ENDSECTION;    ctx->in_ptr++;  break;
                    case '=':   ctx->mode |= MODE_MARKER_DELIMITER;     ctx->in_ptr++;  break;
                    case '>':   ctx->mode |= MODE_MARKER_INCLUDE;       ctx->in_ptr++;  break;
                    default:    ctx->mode |= MODE_MARKER_VARIABLE;      break;
                }
                
                continue;
            } else if (ctx->mode & MODE_MARKER_VARIABLE)    {
                if (*ctx->in_ptr == ':' && !(ctx->mode & MODE_MARKER_MODIFIER)) {
                    ctx->mode |= MODE_MARKER_MODIFIER;
                    ctx->in_ptr++;
                    continue;
                } 
                
            } else if (!isalnum(*ctx->in_ptr) && *ctx->in_ptr != '_' && !(ctx->mode & MODE_MARKER_COMMENT)) {
                // Can't have embedded funky characters inside
                fprintf(stderr, "Illegal character (%c) inside template marker\n", *ctx->in_ptr);
                return (char*)-1;
            }
                                    
            if (ctx->mode & MODE_MARKER_COMMENT)    {
                // Ignore the actual comment contents
            } else {
                if (ctx->mode & MODE_MARKER_MODIFIER)   {
                    if (!(ctx->mode & MODE_MARKER_VARIABLE))    {
                        fprintf(stderr, "Illegal modifier on line %d\n", ctx->template_line);
                        return (char*)-1;
                    }
                    
                    modifiers[mod++] = *ctx->in_ptr;
                    if (mod == MAXMODIFIERLENGTH)   {
                        modifiers[mod-1] = 0;
                        fprintf(stderr, "Template modifier \"%s\" exceeds maximum modifier length of %d characters\n", modifiers, MAXMODIFIERLENGTH);
                        return (char*)-1;
                    }
                    
                } else {
                    marker[m++] = *ctx->in_ptr;

                    if (m == MAXMARKERLENGTH)   {
                        marker[m-1] = 0;
                        fprintf(stderr, "Template marker \"%s\" exceeds maximum marker length of %d characters\n", marker, MAXMARKERLENGTH);
                        return (char*)-1;
                    }
                }   
            }
            
            ctx->in_ptr++;          
            continue;
        }
        
        if (_match_marker(ctx->in_ptr, &(ctx->active_start_delimiter))) {
            ctx->in_ptr += ctx->active_start_delimiter.length;  // Skip over those characters
            ctx->mode = MODE_MARKER;
            memset(marker, 0, MAXMARKERLENGTH);
            memset(modifiers, 0, MAXMODIFIERLENGTH);
            m = 0;
            mod = 0;
        } else {
            if (*ctx->in_ptr == '\n')   {
                // Newline, reset the line whitespace pointer
                ctx->line_ws_start = ctx->in_ptr+1;
                ctx->line_ws_range = 0;
                ctx->template_line++;
                
            } else if ((ctx->in_ptr - ctx->line_ws_range == ctx->line_ws_start) &&
                (*ctx->in_ptr == ' ' || *ctx->in_ptr == '\t'))  {
                // We're still in a chunk of whitespace before any real content, 
                // keep track of that so we can expand template includes correctly
                ctx->line_ws_range++;               
            }
            
            sb_append_ch(ctx->out_sb, *ctx->in_ptr++);
            
            if (ctx->template_line > 1 && *(ctx->in_ptr-1) == '\n') {
                // We're currenlty expanding an include section, which means we need
                // to copy the accumulated whitespace at the beginning of each line
                // other than the first one
                _append_line_whitespace(ctx, ctx);
            }
            
        }
    }
    
    return ctx->in_ptr;
}
