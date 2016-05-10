set( SRCS "G2AssimpScene.cpp" "G2AssimpSceneTextureGeneration.cpp" KGPhysicsManager.cpp)
set( INCS "G2AssimpScene.h")

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${OPENCL_INCLUDE_DIRS} ${CGAL_INCLUDE_DIRS} ${BULLET_INCLUDE_DIRS} bullet3/src )

add_library(Core ${SRCS} ${INCS})
target_link_libraries(Core assimp ${BULLET_LIBRARIES} ${CGAL_LIBRARIES} ${BOOST_THREAD_LIBRARIES} ${GMP_LIBRARIES})
source_group(headers FILES ${INCS})
