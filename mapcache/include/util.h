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

#include <apr_version.h>
#ifndef UTIL_H_
#define UTIL_H_

#define GEOCACHE_MAX(a,b) (((a)>(b))?(a):(b))
#define GEOCACHE_MIN(a,b) (((a)<(b))?(a):(b))

#if APR_MAJOR_VERSION < 2 && APR_MINOR_VERSION < 3



/**
 * Create a new table whose contents are deep copied from the given
 * table. A deep copy operation copies all fields, and makes copies
 * of dynamically allocated memory pointed to by the fields.
 * @param p The pool to allocate the new table out of
 * @param t The table to clone
 * @return A deep copy of the table passed in
 */
APR_DECLARE(apr_table_t *) apr_table_clone(apr_pool_t *p,
                                           const apr_table_t *t);


#endif

#if APR_MAJOR_VERSION < 2 && APR_MINOR_VERSION < 4

/**
 * Create a hard link to the specified file.
 * @param from_path The full path to the original file (using / on all systems)
 * @param to_path The full path to the new file (using / on all systems)
 * @remark Both files must reside on the same device.
 */
APR_DECLARE(apr_status_t) apr_file_link(const char *from_path, const char *to_path);

#endif


#endif /* UTIL_H_ */
/* vim: ai ts=3 sts=3 et sw=3
*/
