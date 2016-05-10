set( SRCS "Renderer.cpp" "Stage.cpp" "StageLoadBundle.cpp")
set( INCS "Renderer.hpp" "Stage.hpp")

set(FAKE GeneratedHeaders)

#add_custom_command(
#	OUTPUT GeneratedHeaders
#	COMMAND lua  ${CMAKE_CURRENT_SOURCE_DIR}/GenShaderHeader.lua Default_vs  #${CMAKE_CURRENT_SOURCE_DIR}/Default.vs > Default_vs.h
#)


#add_custom_command(
#	OUTPUT GeneratedHeaders
#	COMMAND lua  ${CMAKE_CURRENT_SOURCE_DIR}/GenerateAllHeaders.lua ${CMAKE_CURRENT_SOURCE_DIR}
#)

add_custom_command(
	OUTPUT GeneratedHeaders
	COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/GenerateAllHeaders.py ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${OPENCL_INCLUDE_DIRS})

message(Found OpenCL headers: ${OPENCL_INCLUDE_DIRS})
#message(Found CGAL headers: ${CGAL_INCLUDE_DIRS})

add_custom_target(renderer_generated_headers DEPENDS GeneratedHeaders)

add_library(Renderer ${SRCS} ${INCS})

add_dependencies(Renderer renderer_generated_headers lua)

target_link_libraries(Renderer lua Core physfs ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} ${OPENCL_LIBRARIES} assimp ${CGAL_LIBRARIES} ${BOOST_THREAD_LIBRARIES} ${GMP_LIBRARIES})

source_group(headers FILES ${INCS})
