cmake_minimum_required(VERSION 3.20)

project(dum_gippity VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

find_package(SDL3 CONFIG QUIET)

if(NOT SDL3_FOUND)
  message(STATUS "SDL3 not found via find_package, fetching from source...")
  FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(SDL3)
endif()

add_executable(${PROJECT_NAME}
  src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  SDL_MAIN_USE_CALLBACKS=0
)
