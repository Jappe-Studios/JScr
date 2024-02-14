#include "Parser.h"

namespace JScr::Frontend
{
	Program Parser::ProduceAST(string filedir)
	{
		auto& tokenDictionary = Lexer::Tokenize(filedir);
		m_tokens = MapUtils::KeysOf(tokenDictionary);
		m_linesAndCols = MapUtils::ValuesOf(tokenDictionary);
		m_linesAndCols.push_back(m_linesAndCols.back()); // <-- Add duplicate of last item to prevent index out of range exception if syntax error on last token.
	
		auto program = Program(filedir, vector<std::unique_ptr<Stmt>>());

		this->m_filedir = filedir;

		// Parse until end of file
		while (NotEOF())
		{
            program.Body().push_back(ParseStmt());
		}

		return std::move(program);
	}

	std::unique_ptr<Stmt> Parser::ParseStmt()
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

            std::unique_ptr<Expr> parsedExpr = ParseExpr();
            std::unique_ptr<Stmt> parsedExprAsStmt = std::move(parsedExpr);
            return parsedExprAsStmt;
        }
        default:
            std::unique_ptr<Expr> parsedExpr = ParseExpr();
            std::unique_ptr<Stmt> parsedExprAsStmt = std::move(parsedExpr);
            return parsedExprAsStmt;
        }
	}

    const std::unique_ptr<Stmt> Parser::ParseImportStmt()
    {
        Eat();

        auto target = vector<string>();
        optional<string> alias = nullopt;

        while (NotEOF() && At().Type() != Lexer::TokenType::SEMICOLON)
        {
            target.push_back(Eat().Value());

            if (At().Type() == Lexer::TokenType::DOT)
            {
                Eat();
            }
            else
            {
                break;
            }
        }

        if (At().Type() == Lexer::TokenType::AS)
        {
            Eat();
            alias = Expect(Lexer::TokenType::IDENTIFIER, "Identifier expected after `as` keyword.").Value();
        }

        Expect(Lexer::TokenType::SEMICOLON, "Semicolon expected after import statement.");
        return std::make_unique<ImportStmt>(ImportStmt(target, alias.value()));
    }

    const Parser::ParseTypeCtx& Parser::ParseType()
    {
        auto annotations = vector<AnnotationUsageDeclaration>();
        optional<Lexer::Token> enumOrObjTk = nullopt;
        optional<Lexer::Token> type = nullopt;
        vector<Lexer::Token> functionTypeListTk = {};
        bool constant = false;
        bool exported = false;

        // Annotations
        while (At().Type() == Lexer::TokenType::AT)
        {
            Eat();

            auto identifier = Expect(Lexer::TokenType::IDENTIFIER, "Annotation type expected.");
            vector<std::unique_ptr<Expr>> args{};

            if (At().Type() == Lexer::TokenType::OPEN_PAREN)
            {
                args = ParseArgs();
            }

            annotations.push_back(AnnotationUsageDeclaration(identifier.Value(), args));
        }

        function<bool()> IsType = [&]()
        {
            return At().Type() == Lexer::TokenType::TYPE || At().Type() == Lexer::TokenType::IDENTIFIER;
        };

        while ((IsType() && type == nullopt) ||
               ((At().Type() == Lexer::TokenType::ANNOTATION_OBJECT || At().Type() == Lexer::TokenType::OBJECT || At().Type() == Lexer::TokenType::ENUM) && enumOrObjTk == nullopt) ||
               (At().Type() == Lexer::TokenType::CONST && !constant) ||
               (At().Type() == Lexer::TokenType::EXPORT && !exported) ||
               (At().Type() == Lexer::TokenType::FUNCTION && functionTypeListTk.empty())
              )
        {
            if (At().Type() == Lexer::TokenType::CONST)
            {
                constant = true;
                Eat();
                continue;
            }
            else if (IsType())
            {
                type.emplace(Eat());
                if (At().Type() == Lexer::TokenType::OPEN_BRACKET)
                {
                    Eat();
                    Expect(Lexer::TokenType::CLOSE_BRACKET, "Closing bracket expected after open bracket in array declaration.");
                    type.emplace(Lexer::Token(type.value().Value(), Lexer::TokenType::TYPE));
                }
                continue;
            }
            else if (At().Type() == Lexer::TokenType::ANNOTATION_OBJECT || At().Type() == Lexer::TokenType::OBJECT || At().Type() == Lexer::TokenType::ENUM)
            {
                enumOrObjTk.emplace(Eat());
                continue;
            }
            else if (At().Type() == Lexer::TokenType::EXPORT)
            {
                Eat();
                exported = true;
                continue;
            }
            else if (At().Type() == Lexer::TokenType::FUNCTION)
            {
                Eat();

                m_outline++;
                Expect(Lexer::TokenType::OPEN_PAREN, "Open paren expected in lambda function declaration keyword.");

                if (At().Type() == Lexer::TokenType::CLOSE_PAREN)
                    continue;

                functionTypeListTk.push_back(Expect(Lexer::TokenType::TYPE, "Type expected in lambda function declaration keyword."));

                while (At().Type() == Lexer::TokenType::COMMA)
                {
                    functionTypeListTk.push_back(Expect(Lexer::TokenType::TYPE, "Type expected in lambda function declaration keyword."));
                }

                Expect(Lexer::TokenType::CLOSE_PAREN, "Close paren expected in lambda function declaration keyword.");
                m_outline--;
                continue;
            }

            break;
        }

        if (type == nullopt)
            ThrowSyntaxError("No declaration type specified.");

        // Do lambda types
        auto functionTypeList = vector<Types::Type>();
        if (!functionTypeListTk.empty())
        {
            for (const auto& item : functionTypeListTk)
                functionTypeList.push_back(Types::FromString(item.Value()));
        }

        // [RETURN]

        if (enumOrObjTk != nullopt)
            return ParseTypeCtxObjOrEnum(constant, exported, enumOrObjTk.value(), Types::FromString(type.value().Value()), annotations);
    
        return ParseTypeCtxVar(constant, exported, Types::FromString(type.value().Value()).CopyWithLambdaTypes(functionTypeList), annotations);
    }

    const std::unique_ptr<Stmt> Parser::ParseTypePost()
    {
        auto& type = ParseType();

        if (typeid(type) == typeid(ParseTypeCtxObjOrEnum))
        {
            const auto* tp = dynamic_cast<const ParseTypeCtxObjOrEnum*>(&type);
            
            if (tp->IsConstant())
                ThrowSyntaxError("Cannot declare enum or object as constant.");

            if (tp->getType().Type() == Lexer::TokenType::OBJECT)
                return ParseObjectStmt(tp->GetAnnotations(), tp->getIdentifierT().Data());
            if (tp->getType().Type() == Lexer::TokenType::ANNOTATION_OBJECT)
                return ParseObjectStmt(tp->GetAnnotations(), tp->getIdentifierT().Data(), true);
            else if (tp->getType().Type() == Lexer::TokenType::ENUM)
                return ParseEnumStmt(tp->GetAnnotations(), tp->getIdentifierT().Data());

            ThrowSyntaxError("Internal Error: Invalid type.");
        }

        // Get identifier
        auto identifier = Expect(Lexer::TokenType::IDENTIFIER, "Expected identifier after type for function/variable declarations.");
        const auto* typeAsVar = dynamic_cast<const ParseTypeCtxVar*>(&type);

        // Return: Function
        if (At().Type() == Lexer::TokenType::OPEN_PAREN)
        {
            if (type.IsConstant()) ThrowSyntaxError("Functions cannot be declared constant.");
            return ParseFnDeclaration(*typeAsVar, identifier);
        }

        return ParseVarDeclaration(*typeAsVar, identifier);
    }

    const std::unique_ptr<Stmt> Parser::ParseFnDeclaration(ParseTypeCtxVar type, Lexer::Token name)
    {
        auto& args = ParseDeclarativeArgs();
        for (const auto& arg : args)
        {
            if (arg.Kind() != NodeType::VAR_DECLARATION)
                ThrowSyntaxError("Inside function declaration expected parameters to be variable declarations.");
        }

        auto body = vector<std::unique_ptr<Stmt>>();
        bool instaRet = false;
        if (At().Type() == Lexer::TokenType::OPEN_BRACE)
        {
            Eat();
            while (At().Type() != Lexer::TokenType::EOF_TOKEN && At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                body.push_back(ParseStmt());
            }
            Expect(Lexer::TokenType::CLOSE_BRACE, "Closing brace expected inside function declaration.");
        }
        else
        {
            instaRet = true;
            Expect(Lexer::TokenType::EQUALS, "Lambda arrow expected: Equals");
            Expect(Lexer::TokenType::MORE_THAN, "Lambda arrow expected: MoreThan");
            body.push_back(ParseStmt());
        }

        auto fn = FunctionDeclaration(type.GetAnnotations(), type.IsExported(), args, name.Value(), type.getType(), body, instaRet);
        return std::make_unique<FunctionDeclaration>(fn);
    }

    const vector<VarDeclaration>& Parser::ParseDeclarativeArgs()
    {
        m_outline++;

        Expect(Lexer::TokenType::OPEN_PAREN, "Expected open parenthesis inside declarative arguments list.");
        auto args = At().Type() == Lexer::TokenType::CLOSE_PAREN ? vector<VarDeclaration>() : ParseDeclarativeArgsList();
        Expect(Lexer::TokenType::CLOSE_PAREN, "Expected closing parenthesis inside declarative arguments list.");

        m_outline--;
        return args;
    }

    const vector<VarDeclaration>& Parser::ParseDeclarativeArgsList()
    {
        auto ParseParamVar = [&]()
        {
            auto stmt = ParseTypePost();

            if (stmt->Kind() != NodeType::VAR_DECLARATION)
                ThrowSyntaxError("Variable declaration expected inside declarative parameters list.");

            return *(dynamic_cast<const VarDeclaration*>(&*stmt));
        };

        vector<VarDeclaration> args = { ParseParamVar() };

        while (At().Type() == Lexer::TokenType::COMMA)
        {
            args.push_back(ParseParamVar());
        }

        return args;
    }

    const std::unique_ptr<Stmt> Parser::ParseVarDeclaration(ParseTypeCtxVar type, Lexer::Token name)
    {
        auto MkNoval = [&]()
        {
            if (type.IsConstant())
                ThrowSyntaxError("Must assign value to constant expression. No value provided.");

            return VarDeclaration(type.GetAnnotations(), false, type.IsExported(), type.getType(), name.Value(), nullopt);
        };

        if (m_outline == 0 && At().Type() == Lexer::TokenType::SEMICOLON)
        {
            Eat();
            return std::make_unique<VarDeclaration>(MkNoval());
        }
        else if (m_outline > 0 && At().Type() != Lexer::TokenType::EQUALS)
        {
            return std::make_unique<VarDeclaration>(MkNoval());
        }

        std::unique_ptr<VarDeclaration> declaration = nullptr;
        m_outline++;
        if (At().Type() == Lexer::TokenType::EQUALS)
        {
            Eat();
            declaration = std::make_unique<VarDeclaration>(VarDeclaration(type.GetAnnotations(), type.IsConstant(), type.IsExported(), type.getType(), name.Value(), std::make_optional<Expr&>(*ParseExpr())));
        }
        else
        {
            declaration = std::make_unique<VarDeclaration>(VarDeclaration(type.GetAnnotations(), type.IsConstant(), type.IsExported(), type.getType(), name.Value(), std::make_optional<Expr&>(*ParseObjectConstructorExpr(type.getType(), true))));
        }
        if (m_outline <= 1) Expect(Lexer::TokenType::SEMICOLON, "Outline variable declaration statement must end with semicolon.");
        m_outline--;

        return std::move(declaration);
    }

    const std::unique_ptr<Stmt> Parser::ParseObjectStmt(vector<AnnotationUsageDeclaration> annotations, std::string typeIdent, bool annotation)
    {
        // TODO: insert return statement here
    }
    const std::unique_ptr<Stmt> Parser::ParseEnumStmt(vector<AnnotationUsageDeclaration> annotations, std::string typeIdent)
    {
        // TODO: insert return statement here
    }
    const std::unique_ptr<Stmt> Parser::ParseReturnStmt()
    {
        // TODO: insert return statement here
    }
    const std::unique_ptr<Stmt> Parser::ParseDeleteStmt()
    {
        // TODO: insert return statement here
    }
    const std::unique_ptr<Stmt> Parser::ParseIfElseStmt()
    {
        // TODO: insert return statement here
    }
    const std::unique_ptr<Stmt> Parser::ParseWhileStmt()
    {
        // TODO: insert return statement here
    }
    const std::unique_ptr<Stmt> Parser::ParseForStmt()
    {
        // TODO: insert return statement here
    }
    const std::unique_ptr<Expr> Parser::ParseExpr()
    {
        return std::unique_ptr<Expr>();
    }
}