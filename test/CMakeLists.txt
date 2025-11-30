set(TARGET_NAME mem_alloc_test)

include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(${TARGET_NAME}
	mem_alloc_test.cpp
)

target_link_libraries(${TARGET_NAME}
	GTest::gtest_main
	${CMAKE_PROJECT_NAME}
)

include(GoogleTest)
gtest_discover_tests(${TARGET_NAME})
