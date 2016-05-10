cmake_minimum_required(VERSION 2.8)
project(gx)

# this script looks for SFML and sets compiler flags
include(cmake/CMakeLists.txt)

set( CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/.. )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_subdirectory(stb)

list (APPEND BULLET_LIBRARIES LinearMath)
list (APPEND BULLET_LIBRARIES BulletCollision)
list (APPEND BULLET_LIBRARIES BulletDynamics)
list (APPEND BULLET_LIBRARIES BulletSoftBody)
set(BULLET_ROOT bullet3)
add_subdirectory(bullet3)
#find_package(Bullet REQUIRED)
include_directories(bullet3/src)
#message(STATUS Libraries ${BULLET_LIBRARIES})
#IF(BULLET_FOUND)
#	MESSAGE( STATUS "Bullet found" )
#ELSE()
#	MESSAGE( STATUS "Bullet not found" )
#ENDIF()

add_subdirectory(assimp)
include_directories(assimp/include)

add_subdirectory(glfw-3.1.1)
include_directories(glfw-3.1.1/include)

#set(CGAL_DISABLE_GMP ON)
#add_subdirectory(CGAL)
#include_directories(CGAL/include)

include_directories(
	${BULLET_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}
	${GLFW_INCLUDE_DIRS}
)

#target_link_libraries(p4 glfw ${GLFW_LIBRARIES} assimp ${BULLET_LIBRARIES})


add_subdirectory(physfs)

add_subdirectory(Core)

add_subdirectory(lua)

# a library of rendering code
add_subdirectory(Renderer)

# the main application
add_subdirectory(application)
