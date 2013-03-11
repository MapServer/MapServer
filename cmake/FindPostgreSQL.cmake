# - Find PostgreSQL
# Find the PostgreSQL includes and client library
# This module defines
#  POSTGRESQL_INCLUDE_DIR, where to find POSTGRESQL.h
#  POSTGRESQL_LIBRARY, the libraries needed to use POSTGRESQL.
#  POSTGRESQL_FOUND, If false, do not try to use PostgreSQL.
#
# Copyright (c) 2013 Thomas Bonfort
#

find_path(POSTGRESQL_INCLUDE_DIR libpq-fe.h
  /usr/include/server
  /usr/include/postgresql
  /usr/include/pgsql/server
  /usr/local/include/pgsql/server
  /usr/include/postgresql/server
  /usr/include/postgresql/*/server
  /usr/local/include/postgresql/server
  /usr/local/include/postgresql/*/server
  $ENV{ProgramFiles}/PostgreSQL/*/include/server
  $ENV{SystemDrive}/PostgreSQL/*/include/server
)

find_library(POSTGRESQL_LIBRARY NAMES pq libpq
  PATHS
  /usr/lib
  /usr/local/lib
  /usr/lib/postgresql
  /usr/lib64
  /usr/local/lib64
  /usr/lib64/postgresql
  $ENV{ProgramFiles}/PostgreSQL/*/lib/ms
  $ENV{SystemDrive}/PostgreSQL/*/lib/ms
)

set(POSTGRESQL_INCLUDE_DIRS ${POSTGRESQL_INCLUDE_DIR})
set(POSTGRESQL_LIBRARIES ${POSTGRESQL_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(POSTGRESQL DEFAULT_MSG POSTGRESQL_LIBRARY POSTGRESQL_INCLUDE_DIR)
mark_as_advanced(POSTGRESQL_LIBRARY POSTGRESQL_INCLUDE_DIR)
