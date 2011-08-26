#include "geocache.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_hash.h>

geocache_cfg* geocache_configuration_create(apr_pool_t *pool) {
   geocache_cfg *cfg = (geocache_cfg*)apr_pcalloc(pool, sizeof(geocache_cfg));
   cfg->caches = apr_hash_make(pool);
   cfg->sources = apr_hash_make(pool);
   cfg->tilesets = apr_hash_make(pool);
   cfg->image_formats = apr_hash_make(pool);

   geocache_configuration_add_image_format(cfg,
         geocache_imageio_create_png_format(pool,"PNG",GEOCACHE_COMPRESSION_FAST),
         "PNG");
   geocache_configuration_add_image_format(cfg,
         geocache_imageio_create_png_q_format(pool,"PNG",GEOCACHE_COMPRESSION_FAST,256),
         "PNG8");
   geocache_configuration_add_image_format(cfg,
         geocache_imageio_create_jpeg_format(pool,"JPEG",95),
         "JPEG");
   cfg->merge_format = geocache_configuration_get_image_format(cfg,"PNG");
   return cfg;
}

geocache_source *geocache_configuration_get_source(geocache_cfg *config, const char *key) {
   return (geocache_source*)apr_hash_get(config->sources, (void*)key, APR_HASH_KEY_STRING);
}

geocache_cache *geocache_configuration_get_cache(geocache_cfg *config, const char *key) {
   return (geocache_cache*)apr_hash_get(config->caches, (void*)key, APR_HASH_KEY_STRING);
}

geocache_tileset *geocache_configuration_get_tileset(geocache_cfg *config, const char *key) {
   return (geocache_tileset*)apr_hash_get(config->tilesets, (void*)key, APR_HASH_KEY_STRING);
}

geocache_image_format *geocache_configuration_get_image_format(geocache_cfg *config, const char *key) {
   return (geocache_image_format*)apr_hash_get(config->image_formats, (void*)key, APR_HASH_KEY_STRING);
}

void geocache_configuration_add_source(geocache_cfg *config, geocache_source *source, const char * key) {
   apr_hash_set(config->sources, key, APR_HASH_KEY_STRING, (void*)source);
}

void geocache_configuration_add_tileset(geocache_cfg *config, geocache_tileset *tileset, const char * key) {
   apr_hash_set(config->tilesets, key, APR_HASH_KEY_STRING, (void*)tileset);
}

void geocache_configuration_add_cache(geocache_cfg *config, geocache_cache *cache, const char * key) {
   apr_hash_set(config->caches, key, APR_HASH_KEY_STRING, (void*)cache);
}

void geocache_configuration_add_image_format(geocache_cfg *config, geocache_image_format *format, const char * key) {
   apr_hash_set(config->image_formats, key, APR_HASH_KEY_STRING, (void*)format);
}

int extractNameAndTypeAttributes(xmlDoc *doc, xmlAttr *attribute, char **name, char **type) {
   *name = *type = NULL;
   while(attribute && attribute->name && attribute->children)
   {
      if(!xmlStrcmp(attribute->name, BAD_CAST "name")) {
         *name = (char*)xmlNodeListGetString(doc, attribute->children, 1);
      }
      if(!xmlStrcmp(attribute->name, BAD_CAST "type")) {
         *type = (char*)xmlNodeListGetString(doc, attribute->children, 1);
      }
      attribute = attribute->next;
   }
   if(*name && *type) {
      return GEOCACHE_SUCCESS;
   } else {
      return GEOCACHE_FAILURE;
   }
}

char* parseSource(xmlNode *node, geocache_cfg *config, geocache_context *r) {
   if(xmlStrcmp(node->name, BAD_CAST "source")) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "SEVERE: found tag %s instead of <source>",node->name);
      return r->get_error_message(r);
   }
   xmlNode *cur_node;
   char *name = NULL, *type = NULL, *msg;

   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);

   if(!name || !strlen(name))
      return "mandatory attribute \"name\" not found in <source>";
   else {
      /* check we don't already have a source defined with this name */
      if(geocache_configuration_get_source(config, name)) {
         r->set_error(r, GEOCACHE_PARSE_ERROR, "duplicate source with name \"%s\"",name);
         return r->get_error_message(r);
      }
   }
   if(!type || !strlen(type))
      return "mandatory attribute \"type\" not found in <source>";
   geocache_source *source = NULL;
   if(!strcmp(type,"wms")) {
      source = geocache_source_wms_create(r);
   } else {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "unknown source type %s for source \"%s\"", type, name);
      return r->get_error_message(r);
   }
   if(source == NULL) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse source \"%s\"", name);
      return r->get_error_message(r);
   }
   source->name = name;
   for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "srs")) {
         char* value = (char*)xmlNodeGetContent(cur_node);
         source->srs = value;
      }
   }

   msg = source->configuration_parse(node,source,r);
   if(msg) return msg;
   msg = source->configuration_check(source, r);
   if(msg) return msg;
   geocache_configuration_add_source(config,source,name);

   return NULL;
}

char* parseFormat(xmlNode *node, geocache_cfg *config, geocache_context *r) {
   char *name = NULL,  *type = NULL;
   xmlChar *value = NULL;
   geocache_image_format *format = NULL;
   xmlNode *cur_node;
   if(xmlStrcmp(node->name, BAD_CAST "format")) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "SEVERE: <%s> is not a format tag",node->name);
      return r->get_error_message(r);
   }
   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);
   if(!name || !strlen(name))
      return "mandatory attribute \"name\" not found in <format>";
   if(!type || !strlen(type))
      return "mandatory attribute \"type\" not found in <format>";
   if(!strcmp(type,"PNG")) {
      int colors = -1;
      geocache_compression_type compression = GEOCACHE_COMPRESSION_DEFAULT;
      for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
         if(cur_node->type != XML_ELEMENT_NODE) continue;
         if(!xmlStrcmp(cur_node->name, BAD_CAST "compression")) {
            value = xmlNodeGetContent(cur_node);
            if(!xmlStrcmp(value, BAD_CAST "fast")) {
               compression = GEOCACHE_COMPRESSION_FAST;
            } else if(!xmlStrcmp(value, BAD_CAST "best")) {
               compression = GEOCACHE_COMPRESSION_BEST;
            } else {
               r->set_error(r, GEOCACHE_PARSE_ERROR, "unknown compression type %s for format \"%s\"", value, name);
               return r->get_error_message(r);
            }
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "colors")) {
            value = xmlNodeGetContent(cur_node);
            char *endptr;
            colors = (int)strtol((char*)value,&endptr,10);
            if(*endptr != 0 || colors < 2 || colors > 256) {
               r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse colors \"%s\" for format \"%s\""
                     "(expecting an  integer between 2 and 256 "
                     "eg <colors>256</colors>",
                     value,name);
               return r->get_error_message(r);
            }
            xmlFree(value);
         } else {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "unknown tag %s for format \"%s\"", cur_node->name, name);
            return r->get_error_message(r);
         }
         if(colors != -1) {
            format = geocache_imageio_create_png_format(r->pool,name,compression);
         } else {
            format = geocache_imageio_create_png_q_format(r->pool,name,compression, colors);
         }
      }
   } else if(!strcmp(type,"JPEG")){
      int quality = 95;
      for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
         if(cur_node->type != XML_ELEMENT_NODE) continue;
         if(!xmlStrcmp(cur_node->name, BAD_CAST "quality")) {
            value = xmlNodeGetContent(cur_node);
            char *endptr;
            quality = (int)strtol((char*)value,&endptr,10);
            if(*endptr != 0 || quality < 1 || quality > 100) {
               r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse quality \"%s\" for format \"%s\""
                     "(expecting an  integer between 1 and 100 "
                     "eg <quality>90</quality>",
                     value,name);
               return r->get_error_message(r);
            }
            xmlFree(value);
         }
      }
      format = geocache_imageio_create_jpeg_format(r->pool,name,quality);
   } else {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "unknown format type %s for format \"%s\"", type, name);
      return r->get_error_message(r);
   }
   if(format == NULL) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse format \"%s\"", name);
      return r->get_error_message(r);
   }


   geocache_configuration_add_image_format(config,format,name);
   return NULL;
}

char* parseCache(xmlNode *node, geocache_cfg *config, geocache_context *r) {
   char *name = NULL,  *type = NULL, *msg = NULL;
   geocache_cache *cache = NULL;
   if(xmlStrcmp(node->name, BAD_CAST "cache")) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "SEVERE: <%s> is not a cache tag",node->name);
      return r->get_error_message(r);
   }
   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);
   if(!name || !strlen(name)) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "mandatory attribute \"name\" not found in <cache>");
      return r->get_error_message(r);
   }
   else {
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_cache(config, name)) {
         r->set_error(r, GEOCACHE_PARSE_ERROR, "duplicate cache with name \"%s\"",name);
         return r->get_error_message(r);
      }
   }
   if(!type || !strlen(type)) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "mandatory attribute \"type\" not found in <cache>");
      return r->get_error_message(r);
   }
   if(!strcmp(type,"disk")) {
      cache = geocache_cache_disk_create(r);
   } else {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "unknown cache type %s for cache \"%s\"", type, name);
      return r->get_error_message(r);
   }
   if(cache == NULL) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse cache \"%s\"", name);
      return r->get_error_message(r);
   }
   cache->name = name;

   msg = cache->configuration_parse(node,cache,r);
   if(msg) return msg;
   msg = cache->configuration_check(cache, r);
   if(msg) return msg;
   geocache_configuration_add_cache(config,cache,name);
   return NULL;
}



char* parseTileset(xmlNode *node, geocache_cfg *config, geocache_context *r) {
   char *name = NULL, *type = NULL;
   geocache_tileset *tileset = NULL;
   xmlNode *cur_node;
   char* value;
   if(xmlStrcmp(node->name, BAD_CAST "tileset")) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "SEVERE: <%s> is not a tileset tag",node->name);
      return r->get_error_message(r);
   }
   extractNameAndTypeAttributes(node->doc, node->properties, &name, &type);
   if(!name || !strlen(name)) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "mandatory attribute \"name\" not found in <tileset>");
      return r->get_error_message(r);
   }
   else {
      /* check we don't already have a cache defined with this name */
      if(geocache_configuration_get_tileset(config, name)) {
         r->set_error(r, GEOCACHE_PARSE_ERROR, "duplicate tileset with name \"%s\"",name);
         return r->get_error_message(r);
      }
   }
   tileset = geocache_tileset_create(r);
   tileset->name = name;
   for(cur_node = node->children; cur_node; cur_node = cur_node->next) {
      if(cur_node->type != XML_ELEMENT_NODE) continue;
      if(!xmlStrcmp(cur_node->name, BAD_CAST "cache")) {
         value = (char*)xmlNodeGetContent(cur_node);
         geocache_cache *cache = geocache_configuration_get_cache(config,value);
         if(!cache) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" references cache \"%s\","
                  " but it is not configured",name,value);
            return r->get_error_message(r);
         }
         tileset->cache = cache;
         xmlFree(BAD_CAST value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "source")) {
         value = (char*)xmlNodeGetContent(cur_node);
         geocache_source *source = geocache_configuration_get_source(config,value);
         if(!source) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" references source \"%s\","
                  " but it is not configured",name,value);
            return r->get_error_message(r);
         }
         tileset->source = source;
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "srs")) {
         value = (char*)xmlNodeGetContent(cur_node);
         tileset->srs = value;
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "size")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int *sizes, nsizes;
         if(GEOCACHE_SUCCESS != geocache_util_extract_int_list(value,' ',&sizes,&nsizes,r) ||
               nsizes != 2) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse size array %s."
                  "(expecting two space separated integers, eg <size>256 256</size>",
                  value);
            return r->get_error_message(r);
         }
         tileset->tile_sx = sizes[0];
         tileset->tile_sy = sizes[1];
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "extent")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int nvalues;
         double *values;
         if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(value,' ',&values,&nvalues,r) ||
               nvalues != 4) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse extent array %s."
                  "(expecting 4 space separated numbers, got %d (%f %f %f %f)"
                  "eg <extent>-180 -90 180 90</extent>",
                  value,nvalues,values[0],values[1],values[2],values[3]);
            return r->get_error_message(r);
         }
         tileset->extent[0] = values[0];
         tileset->extent[1] = values[1];
         tileset->extent[2] = values[2];
         tileset->extent[3] = values[3];
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "resolutions")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int nvalues;
         double *values;
         if(GEOCACHE_SUCCESS != geocache_util_extract_double_list(value,' ', &values, &nvalues, r) ||
               !nvalues) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse resolutions array %s."
                  "(expecting space separated numbers, "
                  "eg <resolutions>1 2 4 8 16 32</resolutions>",
                  value);
            return r->get_error_message(r);
         }
         tileset->resolutions = values;
         tileset->levels = nvalues;
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "metatile")) {
         value = (char*)xmlNodeGetContent(cur_node);
         int *values, nvalues;
         if(GEOCACHE_SUCCESS != geocache_util_extract_int_list(value,' ', &values, &nvalues, r) ||
               nvalues != 2) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse metatile dimension %s."
                  "(expecting 2 space separated integers, "
                  "eg <metatile>5 5</metatile>",
                  value);
            return r->get_error_message(r);
         }
         tileset->metasize_x = values[0];
         tileset->metasize_y = values[1];
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "expires")) {
         value = (char*)xmlNodeGetContent(cur_node);
         char *endptr;
         tileset->expires = (int)strtol(value,&endptr,10);
         if(*endptr != 0) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse expires %s."
                  "(expecting an  integer, "
                  "eg <expires>3600</expires>",
                  value);  
            return r->get_error_message(r);
         }
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "metabuffer")) {
         value = (char*)xmlNodeGetContent(cur_node);
         char *endptr;
         tileset->metabuffer = (int)strtol(value,&endptr,10);
         if(*endptr != 0) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse metabuffer %s."
                  "(expecting an  integer, "
                  "eg <metabuffer>1</metabuffer>",
                  value);
            return r->get_error_message(r);
         }
         xmlFree(value);
      } else if(!xmlStrcmp(cur_node->name, BAD_CAST "format")) {
         value = (char*)xmlNodeGetContent(cur_node);
         geocache_image_format *format = geocache_configuration_get_image_format(config,value);
         if(!format) {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" references format \"%s\","
                  " but it is not configured",name,value);
            return r->get_error_message(r);
         }
         tileset->format = format;
         xmlFree(value);
      }
   }
   /* check we have all we want */
   if(tileset->cache == NULL) {
      /* TODO: we should allow tilesets with no caches */
      r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no cache configured."
            " You must add a <cache> tag.", tileset->name);
      return r->get_error_message(r);
   }
   if(tileset->source == NULL) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no source configured."
            " You must add a <source> tag.", tileset->name);
      return r->get_error_message(r);
   }
   if(tileset->srs == NULL) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no srs configured."
            " You must add a <srs> tag.", tileset->name);
      return r->get_error_message(r);
   }
   if(tileset->extent[0] == tileset->extent[2] ||
         tileset->extent[1] == tileset->extent[3]) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no (or invalid) extent configured"
            " You must add/correct a <extent> tag.", tileset->name);
      return r->get_error_message(r);
   }
   if(!tileset->levels) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "tileset \"%s\" has no resolutions configured."
            " You must add a <resolutions> tag.", tileset->name);
      return r->get_error_message(r);
   }
   if(!tileset->format && (
         tileset->metasize_x != 1 ||
         tileset->metasize_y != 1 ||
         tileset->metabuffer != 0)) {
      tileset->format = config->merge_format;
   }


   geocache_configuration_add_tileset(config,tileset,name);
   return NULL;
}

char* geocache_configuration_parse(const char *filename, geocache_cfg *config, geocache_context *r) {
   xmlDocPtr doc;

   doc = xmlReadFile(filename, NULL, 0);
   if (doc == NULL) {
      return (char*)"mod_geocache: libxml2 failed to parse file. Is it valid XML?";
   }

   xmlNode *root_element = xmlDocGetRootElement(doc);

   if(root_element->type == XML_ELEMENT_NODE && !xmlStrcmp(root_element->name, BAD_CAST "geocache")) {
      xmlNode *children = root_element->children;
      xmlNode *cur_node;
      for(cur_node = children; cur_node; cur_node = cur_node->next) {
         if(cur_node->type != XML_ELEMENT_NODE) continue;
         if(!xmlStrcmp(cur_node->name, BAD_CAST "source")) {
            char *msg = parseSource(cur_node, config, r);
            if(msg)
               return msg;
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "cache")) {
            char *msg = parseCache(cur_node, config, r);
            if(msg)
               return msg;
         } 
         else if(!xmlStrcmp(cur_node->name, BAD_CAST "format")) {
            char *msg = parseFormat(cur_node, config, r);
            if(msg)
               return msg;
         }
         else if(!xmlStrcmp(cur_node->name, BAD_CAST "tileset")) {
            char *msg = parseTileset(cur_node, config, r);
            if(msg)
               return msg;
         }  else if(!xmlStrcmp(cur_node->name, BAD_CAST "services")) {
            xmlNode *service_node;
            for(service_node = cur_node->children; service_node; service_node = service_node->next) {
               if(service_node->type != XML_ELEMENT_NODE) continue;
               if(!xmlStrcmp(service_node->name, BAD_CAST "wms")) {
                  xmlChar* value = xmlNodeGetContent(service_node);
                  if(!value || !*value || xmlStrcmp(value, BAD_CAST "false")) {
                     config->services[GEOCACHE_SERVICE_WMS] = geocache_service_wms_create(r);
                  }
               }else if(!xmlStrcmp(service_node->name, BAD_CAST "tms")) {
                  xmlChar* value = xmlNodeGetContent(service_node);
                  if(!value || !*value || xmlStrcmp(value, BAD_CAST "false")) {
                     config->services[GEOCACHE_SERVICE_TMS] = geocache_service_tms_create(r);
                  }
               }
            }
         } else if(!xmlStrcmp(cur_node->name, BAD_CAST "merge_format")) {
            char* value = (char*) xmlNodeGetContent(cur_node);
            geocache_image_format *format = geocache_configuration_get_image_format(config,value);
            if(!format) {
               r->set_error(r, GEOCACHE_PARSE_ERROR, "merge_format tag references format %s but it is not configured",
                     value);
               return r->get_error_message(r);
               
            }
            config->merge_format = format;
         } else {
            r->set_error(r, GEOCACHE_PARSE_ERROR, "failed to parse geocache config file %s: unknown tag <%s>",
                  filename, cur_node->name);
            return r->get_error_message(r);
         }
      }

   } else {
      r->set_error(r, GEOCACHE_PARSE_ERROR, 
            "failed to parse geocache config file %s: "
            "document does not begin with <geocache> tag. found <%s>",
            filename,root_element->name);
      return r->get_error_message(r);
   }

   if(!config->services[GEOCACHE_SERVICE_WMS] &&
         !config->services[GEOCACHE_SERVICE_TMS]) {
      r->set_error(r, GEOCACHE_PARSE_ERROR, "no services configured."
            " You must add a <services> tag with <wms/> or <tms/> children");
      return r->get_error_message(r);
   }


   xmlFreeDoc(doc);
   xmlCleanupParser();

   return NULL;
}
