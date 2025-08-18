set(TREE_SITTER_RUNTIME "${CMAKE_SOURCE_DIR}/tree-sitter/lib")
set(TREE_SITTER_LANG    "${CMAKE_SOURCE_DIR}/src")

set(TREE_SITTER_SRC
    ${TREE_SITTER_RUNTIME}/src/lib.c
    ${TREE_SITTER_LANG}/parser.c
)

set(PROJECT_SRC
    src/main.cpp
)

include_directories(
    ${TREE_SITTER_RUNTIME}/include
    ${TREE_SITTER_LANG}
)

add_executable(stp_parse
    ${PROJECT_SRC}
    ${TREE_SITTER_SRC}
)

# If using MSVC, set compiler flags for warnings
if(MSVC)
    target_compile_options(stp_parse PRIVATE /W4 /WX)
else()
    target_compile_options(stp_parse PRIVATE -Wall -Wextra -pedantic)
endif()
