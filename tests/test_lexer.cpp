#include <gtest/gtest.h>
#include "../src/lexer/Lexer.hpp"

using namespace luna::lexer;

TEST(LexerTest, SimpleAssignment) {
    std::string_view src = "x := 5";
    Lexer lexer(src);
    Token t1 = lexer.next();
    EXPECT_EQ(t1.type, TokenType::Identifier);
    EXPECT_EQ(t1.lexeme, "x");

    Token t2 = lexer.next();
    EXPECT_EQ(t2.type, TokenType::Assign);
    EXPECT_EQ(t2.lexeme, ":=");

    Token t3 = lexer.next();
    EXPECT_EQ(t3.type, TokenType::Integer);
    EXPECT_EQ(t3.lexeme, "5");

    Token t4 = lexer.next();
    EXPECT_EQ(t4.type, TokenType::Eof);
}
