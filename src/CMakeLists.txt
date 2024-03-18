set(LIBRARY_NAME logger)

add_library(${LIBRARY_NAME} STATIC
  logger.c
)

target_include_directories(${LIBRARY_NAME}
  PUBLIC include
)
