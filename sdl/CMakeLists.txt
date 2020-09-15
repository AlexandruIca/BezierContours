cmake_minimum_required(VERSION 3.13)

project(Bezier)

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})

find_package(SDL2 REQUIRED)
find_package(glad REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)

add_executable(${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE spdlog::spdlog SDL2::SDL2 glad::glad glm::glm)
