cmake_minimum_required(VERSION 3.22)
project(memory_allocator)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# if this is the top level project of the build
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(PROJECT_BUILD_TYPE ${CMAKE_BUILD_TYPE})

	if(NOT PROJECT_BUILD_TYPE)
		set(PROJECT_BUILD_TYPE Development)
	endif()
else()
	target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC .)
endif()

add_subdirectory(memory_allocator)

if(PROJECT_BUILD_TYPE STREQUAL Test)
	message("Configuring tests...")
	add_subdirectory(test)
elseif(PROJECT_BUILD_TYPE STREQUAL Development)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
	execute_process(COMMAND
		${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/compile_commands.json ${PROJECT_SOURCE_DIR}/compile_commands.json
	)
endif()

