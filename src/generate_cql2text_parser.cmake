message("Generating cql2textparser.cpp")

if (NOT BISON_FOUND)
  message(FATAL_ERROR "Bison not found")
endif()

execute_process(COMMAND "${BISON_EXECUTABLE}" "--no-lines" "-d" "-p" "cql2text" "-ocql2textparser.cpp" "cql2textparser.y"
                RESULT_VARIABLE STATUS)

if(STATUS AND NOT STATUS EQUAL 0)
  message(FATAL_ERROR "bison failed")
endif()
