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

#ifndef GEOCACHE_ERRORS_H_
#define GEOCACHE_ERRORS_H_

typedef enum {
GEOCACHE_DEBUG,
GEOCACHE_INFO,
GEOCACHE_WARNING,
GEOCACHE_ERROR
} geocache_log_level;

typedef enum {
   GEOCACHE_REPORT_LOG,
   GEOCACHE_REPORT_MSG,
   GEOCACHE_REPORT_IMG
} geocache_error_reporting;

#endif /* ERRORS_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/
