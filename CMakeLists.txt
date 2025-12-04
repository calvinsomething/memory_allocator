cmake_minimum_required(VERSION 3.22)
project(memory_allocator)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${PROJECT_NAME})

set(PROJECT_BUILD_TYPE ${CMAKE_BUILD_TYPE})

# if this is the top level project of the build
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL Debug)
		set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
		execute_process(COMMAND
			${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/compile_commands.json
		)
	elseif(CMAKE_BUILD_TYPE STREQUAL Test)
		message("Configuring tests build...")
		add_subdirectory(test)
	endif()
endif()
