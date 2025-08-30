SET(TREE_SITTER_RUNTIME "${CMAKE_CURRENT_SOURCE_DIR}/tree-sitter/lib")
SET(TREE_SITTER_LANG "${CMAKE_CURRENT_SOURCE_DIR}/src")

SET(TREE_SITTER_SRC ${TREE_SITTER_RUNTIME}/src/lib.c ${TREE_SITTER_LANG}/parser.c)

SET(PROJECT_SRC_COMMON
    src/stpInit.cpp
    src/stpStore.cpp
    src/stpParseUtils.cpp
    src/stpApplyOperator.cpp
    src/stpErrors.cpp
    src/stpExprHandler.cpp
    # Statement processors
    src/statementProcessors/stpAssignment.cpp
    src/statementProcessors/stpChunkProcessor.cpp
    src/statementProcessors/stpFunctionDeclStmt.cpp
    src/statementProcessors/stpIfElseStmt.cpp
    src/statementProcessors/stpSymbolDecl.cpp
)
SET(PROJECT_SRC src/main.cpp ${PROJECT_SRC_COMMON})
SET(PROJECT_SRC_DBG src/mainTest.cpp)

IF(NOT DEFINED STP_BASE_DIRECTORY)
    MESSAGE(FATAL_ERROR "This project is not supposed to be build standalone without Steppable.")
ENDIF()
INCLUDE_DIRECTORIES(${TREE_SITTER_RUNTIME}/include ${TREE_SITTER_LANG} ${STP_BASE_DIRECTORY}/include)
LINK_LIBRARIES(steppable)

ADD_EXECUTABLE(stp_parse ${PROJECT_SRC} ${TREE_SITTER_SRC})
ADD_EXECUTABLE(stp_parse_d ${PROJECT_SRC_DBG} ${TREE_SITTER_SRC})
