/**
 * Standard Environment routines for ngtemplate.  Includes Global Dictionary defaults and standard
 * modifiers
 */ 

#include "internal.h"

/**
 * Standard :none modifier.  Just echoes value to output
 */
void _mod_none(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)    {
    sb_append_str(out_sb, value);
}

/**
 * :cstring_escape - Turns newlines into \n, tabs into \t, quotes into \", and so forth
 */
void _mod_cstring_escape(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)  {
    char* ptr;
    
    ptr = (char*)value;
    while (ptr && *ptr) {
        switch(*ptr)    {
            case '\a': sb_append_str(out_sb, "\\a");    break;
            case '\b': sb_append_str(out_sb, "\\b");    break;
            case '\f': sb_append_str(out_sb, "\\f");    break;
            case '\n': sb_append_str(out_sb, "\\n");    break;
            case '\r': sb_append_str(out_sb, "\\r");    break;
            case '\t': sb_append_str(out_sb, "\\t");    break;
            case '\v': sb_append_str(out_sb, "\\v");    break;
            case '\'':  sb_append_str(out_sb, "\\\'");  break;
            case '\"': sb_append_str(out_sb, "\\\"");   break; 
            case '\\': sb_append_str(out_sb, "\\\\");   break;
            case '\?': sb_append_str(out_sb, "\\\?");   break;
            default: sb_append_ch(out_sb, *ptr);
        }
        
        ptr++;
    }
}

/**
 * :html_escape - Turns ampersands into &amp;, and so forth
 */
void _mod_html_escape(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb) {
    char* ptr;
    
    ptr = (char*)value;
    while (ptr && *ptr) {
        switch(*ptr)    {
            /* TODO: What to do about &nbsp;? */
            case '&':   sb_append_str(out_sb, "&amp;");         break;
            case '<':   sb_append_str(out_sb, "&lt;");          break;
            case '>':   sb_append_str(out_sb, "&gt;");          break;
            
            default: sb_append_ch(out_sb, *ptr);
        }
        
        ptr++;
    }
}


/**
 * :xml_escape - Turns spaces into &nbsp; ampersands into &amp;, and so forth
 */
void _mod_xml_escape(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)  {
    char* ptr;
    
    ptr = (char*)value;
    while (ptr && *ptr) {
        switch(*ptr)    {
            case '\"':  sb_append_str(out_sb, "&quot;");        break;
            case '\'':  sb_append_str(out_sb, "&#39;");         break;
            case '&':   sb_append_str(out_sb, "&amp;");         break;
            case '<':   sb_append_str(out_sb, "&lt;");          break;
            case '>':   sb_append_str(out_sb, "&gt;");          break;
            
            default: sb_append_ch(out_sb, *ptr);
        }
        
        ptr++;
    }
}

/**
 * :url_escape - Turns spaces into + and so forth
 */
void _mod_url_escape(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb)  {
    char* ptr;
    char escape[10];
    ptr = (char*)value;
    while (ptr && *ptr) {
        if (*ptr == ' ')    {
            sb_append_ch(out_sb, '+');
        } else if (isalnum(*ptr) || 
            *ptr == '.' || 
            *ptr == ',' || 
            *ptr == '_' ||
            *ptr == ':' ||
            *ptr == '*' || 
            *ptr == '/' ||
            *ptr == '~' ||
            *ptr == '!' ||
            *ptr == '(' ||
            *ptr == ')' ||
            *ptr == '-')    {
            
            sb_append_ch(out_sb, *ptr);
        } else {
            snprintf(escape, 10, "%%%d", *ptr);
            sb_append_str(out_sb, escape);
        }
        ptr++;
    }
}

/**
 * :css_cleanse - Removes characters that are not valid in CSS values
 */
void _mod_css_cleanse(const char* name, const char* args, const char* marker, const char* value, stringbuilder* out_sb) {
    char* ptr;
    
    ptr = (char*)value;
    while (ptr && *ptr) {
        if (isalnum(*ptr) || 
            *ptr == ' ' ||
            *ptr == '_' ||
            *ptr == '.' ||
            *ptr == ',' ||
            *ptr == '!' ||
            *ptr == '#' ||
            *ptr == '%' ||
            *ptr == '-')    {
                
            sb_append_ch(out_sb, *ptr);
        }
        
        ptr++;
    }
}


/**
 * Sets up the ngtemplate global dictionary with standard values
 */
void _init_global_dictionary(ngt_dictionary* d) {
    ngt_set_string(d, "BI_SPACE", " ");
    ngt_set_string(d, "BI_NEWLINE", "\n");
}

/**
 * Sets up the standard modifier callbacks in a template.  They can be overriden by user code later
 */ 
void _init_standard_callbacks(ngt_template* tpl)    {
    ngt_add_modifier(tpl, "none", _mod_none);
    ngt_add_modifier(tpl, "cstring_escape", _mod_cstring_escape);
    
    // Pre and HTML escapes are currently the same since both preserve whitespace
    ngt_add_modifier(tpl, "html_escape", _mod_html_escape);
    ngt_add_modifier(tpl, "h", _mod_html_escape);
    ngt_add_modifier(tpl, "pre_escape", _mod_html_escape);
    ngt_add_modifier(tpl, "p", _mod_html_escape);
    
    ngt_add_modifier(tpl, "xml_escape", _mod_xml_escape);
    ngt_add_modifier(tpl, "x", _mod_xml_escape);
    
    ngt_add_modifier(tpl, "url_query_escape", _mod_url_escape);
    ngt_add_modifier(tpl, "u", _mod_url_escape);
    
    ngt_add_modifier(tpl, "css_cleanse", _mod_css_cleanse);
    ngt_add_modifier(tpl, "c", _mod_css_cleanse);   
}