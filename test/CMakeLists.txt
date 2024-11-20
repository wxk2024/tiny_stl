cmake_minimum_required(VERSION 3.22)
Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.4.0 # or a later release
)

FetchContent_MakeAvailable(Catch2)


include_directories(../)
add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

add_executable(test_SmartPtr test_SmartPtr.cpp)
target_link_libraries(test_SmartPtr PRIVATE Catch2::Catch2WithMain)

add_executable(test_Map test_Map.cpp)
target_link_libraries(test_Map PRIVATE Catch2::Catch2WithMain)

add_executable(test_Variant test_Variant.cpp)
target_link_libraries(test_Variant PRIVATE Catch2::Catch2WithMain)