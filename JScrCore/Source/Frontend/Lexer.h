#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include "../Utils/Vector.h"
#include "../Utils/Range.h"
#include <map>
using namespace JScr::Utils;

namespace JScr::Frontend
{
	class Lexer
	{
	public:
        enum TokenType
        {
            null,
            TYPE,

            // Literal Types
            NUMBER,
            FLOAT_NUMBER,
            DOUBLE_NUMBER,
            STRING,
            CHAR,
            IDENTIFIER,

            // Keywords
            FUNCTION, LAMBDA,
            CONST, EXPORT,
            RETURN,
            IF, ELSE, WHILE, FOR,
            OBJECT, ANNOTATION_OBJECT, ENUM,
            DELETE, IMPORT, AS,

            // Grouping and Operators
            BINARY_OPERATOR,
            LESS_THAN, MORE_THAN,
            AND, OR,
            NOT,
            EQUALS,
            COMMA, COLON, DOT, AT,
            SEMICOLON,
            OPEN_PAREN, CLOSE_PAREN,
            OPEN_BRACE, CLOSE_BRACE,
            OPEN_BRACKET, CLOSE_BRACKET,
            EOF_TOKEN // <-- Signifies the end of file.
        };

        static const std::unordered_map<std::string, TokenType> KEYWORDS;

        class Token
        {
        public:
            Token(const std::string& value, const TokenType& type) : m_value(value), m_type(type) {}
            Token(const char& value, const TokenType& type) : m_value(std::string(1, value)), m_type(type) {}

            const std::string& Value() const { return m_value; }
            const TokenType& Type() const { return m_type; }
        private:
            const std::string& m_value;
            const TokenType& m_type;
        };

        static bool IsAlpha(char& src) { return toupper(src) != tolower(src); }
        
        static bool IsSkippable(char& src) { return src == ' ' || src == '\n' || src == '\t' || src == '\r'; }
        
        static bool IsInt(char src)
        {
            int c = src;
            std::vector<int> bounds = {(int)'0', (int)'9'};
            return c >= bounds[0] && c <= bounds[1];
        }

        static std::map<Lexer::Token, Range>& Tokenize(const std::string& filedir);

	private:
		Lexer() {}
	};
}