cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(Sim)
add_executable(sim)
add_subdirectory(src)

# target_compile_options(sim PRIVATE -ggdb -Wall -Wextra -pedantic)
target_compile_options(sim PRIVATE -fsanitize=address,undefined -ggdb -Wall -Wextra -pedantic)
target_link_options(sim PRIVATE -fsanitize=address,undefined)

target_link_libraries(sim PRIVATE raylib)