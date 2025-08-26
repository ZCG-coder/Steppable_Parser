SET(TREE_SITTER_RUNTIME "${CMAKE_CURRENT_SOURCE_DIR}/tree-sitter/lib")
SET(TREE_SITTER_LANG "${CMAKE_CURRENT_SOURCE_DIR}/src")

SET(TREE_SITTER_SRC ${TREE_SITTER_RUNTIME}/src/lib.c ${TREE_SITTER_LANG}/parser.c)

SET(PROJECT_SRC src/main.cpp src/stpInit.cpp src/stpStore.cpp src/stpParseUtils.cpp src/stpTypeNames.c src/stpApplyOperator.cpp)
SET(PROJECT_SRC_DBG src/mainTest.cpp src/stpInit.cpp src/stpStore.cpp src/stpParseUtils.cpp src/stpTypeNames.c src/stpApplyOperator.cpp)

IF(NOT DEFINED STP_BASE_DIRECTORY)
    MESSAGE(FATAL_ERROR "This project is not supposed to be build standalone without Steppable.")
ENDIF()
INCLUDE_DIRECTORIES(${TREE_SITTER_RUNTIME}/include ${TREE_SITTER_LANG} ${STP_BASE_DIRECTORY}/include)
LINK_LIBRARIES(steppable func)

ADD_EXECUTABLE(stp_parse ${PROJECT_SRC} ${TREE_SITTER_SRC})
ADD_EXECUTABLE(stp_parse_d ${PROJECT_SRC_DBG} ${TREE_SITTER_SRC})

# If using MSVC, set compiler flags for warnings
IF(MSVC)
    TARGET_COMPILE_OPTIONS(stp_parse PRIVATE /W4 /WX)
    TARGET_COMPILE_OPTIONS(stp_parse_d PRIVATE /W4 /WX)
ELSE()
    TARGET_COMPILE_OPTIONS(stp_parse PRIVATE -Wall -Wextra -pedantic)
    TARGET_COMPILE_OPTIONS(stp_parse_d PRIVATE -Wall -Wextra -pedantic)
ENDIF()
