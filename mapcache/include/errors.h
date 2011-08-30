/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef MAPCACHE_ERRORS_H_
#define MAPCACHE_ERRORS_H_

typedef enum {
MAPCACHE_DEBUG,
MAPCACHE_INFO,
MAPCACHE_WARNING,
MAPCACHE_ERROR
} mapcache_log_level;

typedef enum {
   MAPCACHE_REPORT_LOG,
   MAPCACHE_REPORT_MSG,
   MAPCACHE_REPORT_ERROR_IMG,
   MAPCACHE_REPORT_EMPTY_IMG,
   MAPCACHE_REPORT_CUSTOM_IMG
} mapcache_error_reporting;

#endif /* ERRORS_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/
