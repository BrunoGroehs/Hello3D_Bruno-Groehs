cmake_minimum_required(VERSION 3.10)
project(AtividadesCG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG master)

FetchContent_MakeAvailable(glfw glm)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/Common)
include_directories(${CMAKE_SOURCE_DIR}/include/glad)
include_directories(${glm_SOURCE_DIR})

set(GLAD_C_FILE "${CMAKE_SOURCE_DIR}/Common/glad.c")
if (NOT EXISTS ${GLAD_C_FILE})
    message(FATAL_ERROR "glad.c not found!")
endif()

add_compile_options(-Wno-pragmas)

if(WIN32)
    set(OPENGL_LIBS opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    set(OPENGL_LIBS ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    set(OPENGL_LIBS ${OPENGL_gl_LIBRARY})
endif()

add_executable(Hello3D src/Hello3D.cpp ${GLAD_C_FILE})
target_include_directories(Hello3D PRIVATE ${CMAKE_SOURCE_DIR}/include/glad ${glm_SOURCE_DIR})
target_link_libraries(Hello3D glfw ${OPENGL_LIBS})
