cmake_minimum_required(VERSION 3.10)
project(lisp C)

add_compile_options(-std=c99 -Wall -Wpedantic -Wextra -Werror -g)
add_executable(
        ${PROJECT_NAME}
        main.c
        mpc.c
)

target_link_libraries(${PROJECT_NAME}
        readline
        m
)