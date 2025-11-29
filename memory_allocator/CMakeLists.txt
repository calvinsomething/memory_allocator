add_library(${CMAKE_PROJECT_NAME}
	MappedSegmentAllocator.cpp
	Chunk.cpp)

message("adding target named ${CMAKE_PROJECT_NAME}")

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC .)
