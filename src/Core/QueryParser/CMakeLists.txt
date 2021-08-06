
flex_target(lexer "${CMAKE_CURRENT_SOURCE_DIR}/flex.ll"
        "${CMAKE_CURRENT_SOURCE_DIR}/Lexer.cpp")

bison_target(parser "${CMAKE_CURRENT_SOURCE_DIR}/grammar.yy"
        "${CMAKE_CURRENT_SOURCE_DIR}/Parser.cpp")

add_flex_bison_dependency(lexer parser)


add_library(QueryParser AST.cpp NormalizationVisitor.cpp TAPNQueryParser.cpp ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS})
