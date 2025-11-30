add_library(${PROJECT_NAME}
	Chunk.cpp
	LinearAllocator.cpp
	MappedSegmentAllocator.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC .)
