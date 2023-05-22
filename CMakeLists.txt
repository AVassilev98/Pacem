
cmake_minimum_required(VERSION 3.5)
project(Pacem)

include(FetchContent)
include(FindVulkan)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to conform to")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw QUIET)

find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

if(NOT glfw_FOUND)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 814b7929c5add4b0541ccad26fb81f28b71dc4d8
  )

  FetchContent_GetProperties(glfw)

  if(NOT glfw_POPULATED)
    FetchContent_Populate(glfw)
    add_subdirectory(${glfw_SOURCE_DIR} ${glfw_BINARY_DIR})
  endif()

  include_directories(${glfw_SOURCE_DIR}/include)
endif()

find_package(glad QUIET)

if(NOT glad_FOUND)
  FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG a5ca31c88a4cc5847ea012629aff3690f261c7c4
  )

  FetchContent_GetProperties(glad)

  if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
    include_directories(${glad_SOURCE_DIR}/include)
  endif()
endif()

find_package(glm QUIET)

if(NOT glm_FOUND)
  FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
  )

  FetchContent_GetProperties(glm)

  if(NOT glm_POPULATED)
    FetchContent_Populate(glm)
    add_subdirectory(${glm_SOURCE_DIR} ${glm_BINARY_DIR})
  endif()

  include_directories(${glm_SOURCE_DIR})
  include_directories(${glm_BINARY_DIR})

endif()

set(BUILD_SHARED_LIBS FALSE CACHE BOOL "x" FORCE)
set(ASSIMP_NO_EXPORT TRUE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_TESTS FALSE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT FALSE CACHE BOOL "x" FORCE)
set(ASSIMP_INSTALL_PDB FALSE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_ZLIB TRUE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS FALSE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER TRUE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_STL_IMPORTER TRUE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER TRUE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER TRUE CACHE BOOL "x" FORCE)
set(ASSIMP_BUILD_BLEND_IMPORTER TRUE CACHE BOOL "x" FORCE)
find_package(assimp QUIET)

if(NOT assimp_FOUND)
  FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
  )

  FetchContent_GetProperties(assimp)

  if(NOT assimp_POPULATED)
    FetchContent_Populate(assimp)
    add_subdirectory(${assimp_SOURCE_DIR} ${assimp_BINARY_DIR})
  endif()

  include_directories(${assimp_SOURCE_DIR}/include)
  include_directories(${assimp_BINARY_DIR}/include)
endif()

add_executable(Pacem
    src/main.cpp
    src/mesh.cpp
    src/renderer.cpp
    src/vkinit.cpp
    src/shader.cpp
    src/pipeline.cpp
    src/gpuresource.cpp
)

set_property(TARGET Pacem PROPERTY CXX_STANDARD 20)

target_include_directories(Pacem AFTER PUBLIC include)

target_link_libraries(Pacem
    Vulkan::Vulkan
    glfw
    assimp
)

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB SHADERS
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
  ${SHADER_SOURCE_DIR}/*.geom
  ${SHADER_SOURCE_DIR}/*.tesc
  ${SHADER_SOURCE_DIR}/*.tese
  ${SHADER_SOURCE_DIR}/*.mesh
  ${SHADER_SOURCE_DIR}/*.task
  ${SHADER_SOURCE_DIR}/*.rgen
  ${SHADER_SOURCE_DIR}/*.rchit
  ${SHADER_SOURCE_DIR}/*.rmiss)

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
  OUTPUT ${SHADER_BINARY_DIR}
  COMMENT "Creating ${SHADER_BINARY_DIR}"
)
target_compile_definitions(Pacem PUBLIC SHADER_PATH="${SHADER_BINARY_DIR}/")

foreach(ShaderSourceFile IN LISTS SHADERS)
  get_filename_component(FILENAME ${ShaderSourceFile} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable} -o ${SHADER_BINARY_DIR}/${FILENAME}.spv ${ShaderSourceFile}
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${ShaderSourceFile}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()
add_custom_target(ShaderCompilation DEPENDS ${SPV_SHADERS})
add_dependencies(Pacem ShaderCompilation)

set(RESOURCE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
target_compile_definitions(Pacem PUBLIC ASSET_PATH="${RESOURCE_SOURCE_DIR}/")
