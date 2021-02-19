project(Test)
cmake_minimum_required(VERSION 3.19.4)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

set(GLFW_DIR "/usr/local/Cellar/glfw/3.3.2")
set(GLFW_INCLUDE_DIR ${GLFW_DIR}/include)
set(GLFW_LIBRARY_DIR ${GLFW_DIR}/lib)
set(GLFW_LIBRARIES ${GLFW_LIBRARY_DIR}/libglfw.dylib)

file(GLOB SOURCE_FILES *.cpp)

add_executable(${PROJECT_NAME}
    ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR} ${GLFW_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} ${GLEW_LIBRARIES})

message(${Vulkan_INCLUDE_DIR})