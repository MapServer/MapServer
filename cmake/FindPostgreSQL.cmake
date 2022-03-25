# - Find PostgreSQL
# Find the PostgreSQL includes and client library
# This module defines
#  POSTGRESQL_INCLUDE_DIR, where to find POSTGRESQL.h
#  POSTGRESQL_LIBRARY, the libraries needed to use POSTGRESQL.
#  POSTGRESQL_FOUND, If false, do not try to use PostgreSQL.
#
# Copyright (c) 2013 Thomas Bonfort, Andy Colson
#

find_program(PG_CONFIG NAMES pg_config
   PATHS
   $ENV{ProgramFiles}/PostgreSQL/*/bin
   $ENV{SystemDrive}/PostgreSQL/*/bin
)

if (PG_CONFIG)
   exec_program( ${PG_CONFIG} ARGS "--includedir" OUTPUT_VARIABLE PG_INC_PATH )
   exec_program( ${PG_CONFIG} ARGS "--libdir" OUTPUT_VARIABLE PG_LIB_PATH )
   find_path(POSTGRESQL_INCLUDE_DIR libpq-fe.h
      PATHS
      ${PG_INC_PATH}
      NO_DEFAULT_PATH
   )
   find_library(POSTGRESQL_LIBRARY NAMES pq libpq
      PATHS
      ${PG_LIB_PATH}
      NO_DEFAULT_PATH
   )
else()
   message(WARNING "pg_config not found, will try some defaults")
   find_path(POSTGRESQL_INCLUDE_DIR libpq-fe.h
     ${PG_INC_PATH}
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
     ${PG_LIB_PATH}
     /usr/lib
     /usr/local/lib
     /usr/lib/postgresql
     /usr/lib64
     /usr/local/lib64
     /usr/lib64/postgresql
     $ENV{ProgramFiles}/PostgreSQL/*/lib/ms
     $ENV{SystemDrive}/PostgreSQL/*/lib/ms
   )
endif()

set(POSTGRESQL_INCLUDE_DIRS ${POSTGRESQL_INCLUDE_DIR})
set(POSTGRESQL_LIBRARIES ${POSTGRESQL_LIBRARY})
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PostgreSQL DEFAULT_MSG POSTGRESQL_LIBRARY POSTGRESQL_INCLUDE_DIR)
mark_as_advanced(POSTGRESQL_LIBRARY POSTGRESQL_INCLUDE_DIR)
