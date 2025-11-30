add_library(${CMAKE_PROJECT_NAME}
	Chunk.cpp
	LinearAllocator.cpp
	MappedSegmentAllocator.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC .)
