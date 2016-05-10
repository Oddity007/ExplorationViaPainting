set( SRCS "stb_image.cpp")
set( INCS "stb_image.h")

add_library(stb ${SRCS} ${INCS})
source_group(headers FILES ${INCS})
