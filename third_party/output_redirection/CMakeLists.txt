set(LIBRARY_NAME output_redirection)

add_library(${LIBRARY_NAME} STATIC
    output_redirection.c
)

target_include_directories(${LIBRARY_NAME}
  PUBLIC include
)
