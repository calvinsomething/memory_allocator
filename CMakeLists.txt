cmake_minimum_required(VERSION 3.22)
project(memory_allocator)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_DIR src)


if(NOT CMAKE_BUILD_TYPE STREQUAL Release)
	set(BUILD_TESTS ON)
	set(CMAKE_BUILD_TYPE Debug)
endif()

# if this is the top level project of the build
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	if(BUILD_TESTS)
		set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
		execute_process(COMMAND
			${CMAKE_COMMAND} -E create_symlink ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}/compile_commands.json
		)

		message("Configuring tests build...")
		add_subdirectory(test)
	endif()
endif()

add_subdirectory(${SOURCE_DIR}/${PROJECT_NAME})
