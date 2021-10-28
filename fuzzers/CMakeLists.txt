add_executable(shapefuzzer shapefuzzer.c)
target_compile_options(shapefuzzer PRIVATE -Wall -Wextra -Werror=format-security)
target_link_libraries(shapefuzzer ${MAPSERVER_LIBMAPSERVER})
