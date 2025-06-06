add_library(${CMAKE_PROJECT_NAME} MemoryAllocator.cpp)

message("adding target named ${CMAKE_PROJECT_NAME}")

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC .)
