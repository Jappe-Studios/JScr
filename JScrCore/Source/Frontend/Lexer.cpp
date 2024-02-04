#include "Lexer.h"
#include "SyntaxException.h"
#include "../Runtime/Types.h"
#include "../Utils/MapUtils.h"
#include "../Utils/VectorUtils.h"

namespace JScr::Frontend
{
	const std::unordered_map<std::string, Lexer::TokenType> Lexer::KEYWORDS{
		{ "const", TokenType::CONST },
		{ "export", TokenType::EXPORT },
		{ "return", TokenType::RETURN },
		{ "if", TokenType::IF },
		{ "else", TokenType::ELSE },
		{ "while", TokenType::WHILE },
		{ "for", TokenType::FOR },
		{ "function", TokenType::FUNCTION },
		{ "lambda", TokenType::LAMBDA },
		{ "object", TokenType::OBJECT },
		{ "@object", TokenType::ANNOTATION_OBJECT },
		{ "enum", TokenType::ENUM },
		{ "delete", TokenType::DELETE },
		{ "import", TokenType::IMPORT },
		{ "as", TokenType::AS },
	};

	std::map<Lexer::Token, Range>& Lexer::Tokenize(const std::string& filedir)
	{
		std::ifstream file(filedir);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open input file at \"" + filedir + "\".");
		}

		std::map<Lexer::Token, Range> tokens = {};

		unsigned int line = 1, col = 0;

		bool insideComment = false;
		bool insideCommentMultiline = false;

		char current;
		file.get(current);

		auto Shift = [&]()
		{
			char temp = current;
			file.get(current);
			col++;

			if (current == '\n')
			{
				line++;
				col = 0;
			}
			return temp;
		};

		auto PeekNextChar = [&]()
		{
			file.clear();
			return file.peek();
		};

		auto Push = [&](Lexer::Token tk)
		{
			tokens.insert({ std::move(tk), Range(Vector2i(line, col), Vector2i(line, col + tk.Value().length())) });
		};

		// Will return true if this is the beginning or the end of a comment.
		// Shifts twice before returning true to get rid of comment beginning and end parts.
		auto CommentModifier = [&]()
		{
			if (file.peek() == EOF || file.peek() == EOF)
			{
				file.clear();
				return false;
			}

			if (current == '/')
			{
				if (PeekNextChar() == '/') // "//"
				{
					insideComment = true;
					insideCommentMultiline = false;
					Shift(); Shift();
					return true;
				}
				else if (PeekNextChar() == '*') // "/*"
				{
					insideComment = true;
					insideCommentMultiline = true;
					Shift(); Shift();
					return true;
				}
			}
			else if (current == '*')
			{
				if (PeekNextChar() == '/') // "*/"
				{
					insideComment = false;
					insideCommentMultiline = false;
					Shift(); Shift();
					return true;
				}
			}

			return false;
		};

		while (!file.eof())
		{
			// SKIP COMMENTS
			while (CommentModifier() || insideComment)
			{
				if (file.eof())
					break;

				auto shifted = Shift();

				if (shifted == '\n')
				{
					if (insideComment && !insideCommentMultiline)
						insideComment = false;
				}
			}

			// BEGIN PARSING ONE CHARACTER TOKENS
			if (current == '(')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::OPEN_PAREN));
			}
			else if (current == ')')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::CLOSE_PAREN));
			}
			else if (current == '{')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::OPEN_BRACE));
			}
			else if (current == '}')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::CLOSE_BRACE));
			}
			else if (current == '[')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::OPEN_BRACKET));
			}
			else if (current == ']')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::CLOSE_BRACKET));
			}

			// HANDLE BINARY OPERATORS & COMMENTS
			else if (current == '+' || current == '-' || current == '*' || current == '/' || current == '%')
			{
				//if (!CommentModifier())
				Push(Lexer::Token(Shift(), Lexer::TokenType::BINARY_OPERATOR));
			}

			// HANDLE CONDITIONAL & ASSIGNMENT TOKENS
			else if (current == '=')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::EQUALS));
			}
			else if (current == ';')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::SEMICOLON));
			}
			else if (current == ':')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::COLON));
			}
			else if (current == ',')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::COMMA));
			}
			else if (current == '.')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::DOT));
			}
			else if (current == '@')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::AT));
			}
			else if (current == '<')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::LESS_THAN));
			}
			else if (current == '>')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::MORE_THAN));
			}
			else if (current == '&')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::AND));
			}
			else if (current == '|')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::OR));
			}
			else if (current == '!')
			{
				Push(Lexer::Token(Shift(), Lexer::TokenType::NOT));
			}

			// HANDLE MULTICHARACTER KEYWORDS, TOKENS, IDENTIFIERS ETC...
			else
			{
				// Handle multicaracter tokens
				if (Lexer::IsInt(current))
				{
					std::string num = "";
					bool        dot = false;
					while (!file.eof() && (Lexer::IsInt(current) || current == '.'))
					{
						if (dot && current == '.') break;
						if (current == '.')        dot = true;
						num += Shift();
					}

					if (toupper(current) == toupper('d'))
					{
						Shift();
						Push(Lexer::Token(num, Lexer::TokenType::DOUBLE_NUMBER));
					}
					else if (toupper(current) == toupper('f') || dot)
					{
						if (toupper(current) == toupper('f'))
							Shift();
						Push(Lexer::Token(num, Lexer::TokenType::FLOAT_NUMBER));
					}
					else
					{
						Push(Lexer::Token(num, Lexer::TokenType::NUMBER));
					}
				}
				else if (Lexer::IsAlpha(current))
				{
					std::string ident = "";
					while (!file.eof() && Lexer::IsAlpha(current))
					{
						ident += Shift();
					}

					// check for reserved keywords
					Lexer::TokenType reserved = Lexer::TokenType::null;
					auto it = Lexer::KEYWORDS.find(ident);
					if (it != Lexer::KEYWORDS.end()) reserved = it->second;
					else if (VectorUtils::Contains(MapUtils::ValuesOf(Runtime::Types::types), ident)) reserved = Lexer::TokenType::TYPE;

					if (reserved != null)
					{
						Push(Lexer::Token(ident, (Lexer::TokenType) reserved));
					}
					else
					{
						Push(Lexer::Token(ident, Lexer::TokenType::IDENTIFIER));
					}
				}
				else if (IsSkippable(current))
				{
					Shift(); // SKIP THE CURRENT CHARACTER
				}
				else if (current == '"')
				{
					Shift(); // < begin quote
					std::string ident = "";

					while (!file.eof() && current != '"')
					{
						ident += Shift();
					}

					Shift(); // < end quote

					Push(Lexer::Token(ident, Lexer::TokenType::STRING));
				}
				else if (current == '\'')
				{
					Shift(); // < begin quote
					char ident = Shift();
					Shift(); // < end quote

					Push(Lexer::Token(ident, Lexer::TokenType::CHAR));
				}
				else
				{
					throw SyntaxException(filedir, Vector2i(line, col), "Unrecognized character found in source.");
				}
			}
		}

		Push(Lexer::Token("EndOfFile", Lexer::TokenType::EOF_TOKEN));
		file.close();
		return tokens;
	}
}