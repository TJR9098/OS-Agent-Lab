set(TARGET_NAME Lexer)
set(TARGET_SRC lexer.cpp)
add_library(${TARGET_NAME}  ${TARGET_SRC})
install(TARGETS Lexer DESTINATION ${CMAKE_BINARY_DIR}/lib COMPONENT tcc-front-end)
