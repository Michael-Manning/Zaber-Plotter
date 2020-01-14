cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

################### Variables. ####################
# Change if you want modify path or other values. #
###################################################

set(PROJECT_NAME zaberplotter)
# Output Variables
set(OUTPUT_DEBUG Debug/bin)
set(OUTPUT_RELEASE Release/bin)

############## CMake Project ################
#        The main options of project        #
#############################################

project(${PROJECT_NAME} C CXX)

set (CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(CMAKE_BUILD_TYPE "Debug")

# Define Release by default.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build type not specified: Use Release by default.")
endif(NOT CMAKE_BUILD_TYPE)

# Definition of Macros
add_definitions(
   -D_DEBUG 
   -D_CONSOLE 
   -DUNICODE
   -D_UNICODE
)

############## Artefacts Output #################
# Defines outputs , depending Debug or Release. #
#################################################

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUT_DEBUG}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUT_DEBUG}")
  set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUT_DEBUG}")
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUT_REL}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUT_REL}")
  set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OUTPUT_REL}")
endif()

################# Flags ################
# Defines Flags for Windows and Linux. #
########################################

if(MSVC)
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /MD /Od /Zi /EHsc")
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /GL /Od /Oi /Gy /Zi /EHsc")
endif(MSVC)
if(NOT MSVC)

   if(CMAKE_BUILD_TYPE STREQUAL "Debug" )
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g")
   else()
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
   endif()


   if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
   endif()
   if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14")
   endif()

endif(NOT MSVC)

################ Files ################
#   --   Add files to project.   --   #
#######################################

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/include/glfw)

link_libraries(glfw)

find_package(OpenGL REQUIRED)

include_directories(PUBLIC ${OPENGL_INCLUDE_DIR})
link_libraries(${OPENGL_gl_LIBRARY})
link_libraries(${OPENGL_glu_LIBRARY})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
# Setup CMake to use GTK+, tell the compiler where to look for headers
# and to the linker where to look for libraries
include_directories(${GTK3_INCLUDE_DIRS})
message(staus "GTK3_INCLUDE_DIRS = ${GTK3_INCLUDE_DIRS}")
link_directories(${GTK3_LIBRARY_DIRS})
message(status " GTK3_LIBRARY_DIRS = ${GTK3_LIBRARY_DIRS}")
# Add other flags to the compiler
add_definitions(${GTK3_CFLAGS_OTHER})
message(status " GTK3_CFLAGS_OTHER = ${GTK3_CFLAGS_OTHER}")
# Add an executable compiled from hello.c
link_libraries(${GTK3_LIBRARIES})
message(status "GTK3_LIBRARIES = ${GTK3_LIBRARIES}")

message(status "pkgs_cflags = ${PKGS_CFLAGS}")

file(GLOB SRC_FILES
   ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/*.c
   ${CMAKE_CURRENT_SOURCE_DIR}/include/gl3w/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/include/gl3w/*.c
   ${CMAKE_CURRENT_SOURCE_DIR}/include/zaber/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/include/zaber/*.c
   ${CMAKE_CURRENT_SOURCE_DIR}/include/nfd/*.c
   ${CMAKE_CURRENT_SOURCE_DIR}/include/nfd/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/Zplot/*.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/Zplot/*.c
)


include_directories( 
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui
   ${CMAKE_CURRENT_SOURCE_DIR}/include/glfw/include/GLFW
   ${CMAKE_CURRENT_SOURCE_DIR}/include/nfd/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/nfd
)

# Add executable to build.
add_executable(${PROJECT_NAME} 
   ${SRC_FILES}
)

