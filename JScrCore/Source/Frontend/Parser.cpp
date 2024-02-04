#include "Parser.h"

namespace JScr::Frontend
{
	Program Parser::ProduceAST(string filedir)
	{
		auto& tokenDictionary = Lexer::Tokenize(filedir);
		m_tokens = MapUtils::KeysOf(tokenDictionary);
		m_linesAndCols = MapUtils::ValuesOf(tokenDictionary);
		m_linesAndCols.push_back(m_linesAndCols.back()); // <-- Add duplicate of last item to prevent index out of range exception if syntax error on last token.
	
		auto program = Program(filedir, vector<Stmt>());

		this->m_filedir = filedir;

		// Parse until end of file
		while (NotEOF())
		{
			program.Body().push_back(ParseStmt());
		}

		return program;
	}

	Stmt& Parser::ParseStmt()
	{
        switch (At().Type())
        {
        case Lexer::TokenType::IMPORT:
            return ParseImportStmt();
        case Lexer::TokenType::EXPORT:
        case Lexer::TokenType::CONST:
        case Lexer::TokenType::ANNOTATION_OBJECT:
        case Lexer::TokenType::OBJECT:
        case Lexer::TokenType::ENUM:
        case Lexer::TokenType::AT:
        case Lexer::TokenType::TYPE:
            return ParseTypePost();
        case Lexer::TokenType::RETURN:
            return ParseReturnStmt();
        case Lexer::TokenType::DELETE:
            return ParseDeleteStmt();
        case Lexer::TokenType::IF:
            return ParseIfElseStmt();
        case Lexer::TokenType::WHILE:
            return ParseWhileStmt();
        case Lexer::TokenType::FOR:
            return ParseForStmt();
        case Lexer::TokenType::IDENTIFIER:
        {
            if ((m_tokens[1].Type() == Lexer::TokenType::CONST || m_tokens[1].Type() == Lexer::TokenType::EXPORT || m_tokens[1].Type() == Lexer::TokenType::IDENTIFIER) && m_outline == 0)
            {
                return ParseTypePost();
            }
            return ParseExpr();
        }
        default:
            return ParseExpr();
        }
	}

    Stmt& Parser::ParseImportStmt()
    {
        // TODO: insert return statement here
    }
}