set(LIBRARY_NAME output_redirection)

add_library(${LIBRARY_NAME} STATIC
    output_redirection.cpp
)

target_include_directories(${LIBRARY_NAME}
  PUBLIC include
)
