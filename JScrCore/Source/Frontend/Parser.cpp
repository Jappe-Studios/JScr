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

    std::unique_ptr<Stmt> Parser::ParseImportStmt()
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

    Parser::ParseTypeCtx& Parser::ParseType()
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
        {
            auto v = ParseTypeCtxObjOrEnum(constant, exported, enumOrObjTk.value(), Types::FromString(type.value().Value()), annotations);
            return v;
        }
            
    
        auto v = ParseTypeCtxVar(constant, exported, Types::FromString(type.value().Value()).CopyWithLambdaTypes(functionTypeList), annotations);
        return v;
    }

    std::unique_ptr<Stmt> Parser::ParseTypePost()
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

    std::unique_ptr<Stmt> Parser::ParseFnDeclaration(ParseTypeCtxVar type, Lexer::Token name)
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

    vector<VarDeclaration>& Parser::ParseDeclarativeArgs()
    {
        m_outline++;

        Expect(Lexer::TokenType::OPEN_PAREN, "Expected open parenthesis inside declarative arguments list.");
        auto args = At().Type() == Lexer::TokenType::CLOSE_PAREN ? vector<VarDeclaration>() : ParseDeclarativeArgsList();
        Expect(Lexer::TokenType::CLOSE_PAREN, "Expected closing parenthesis inside declarative arguments list.");

        m_outline--;
        return args;
    }

    vector<VarDeclaration>& Parser::ParseDeclarativeArgsList()
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

    std::unique_ptr<Stmt> Parser::ParseVarDeclaration(ParseTypeCtxVar type, Lexer::Token name)
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
            declaration = std::make_unique<VarDeclaration>(VarDeclaration(type.GetAnnotations(), type.IsConstant(), type.IsExported(), type.getType(), name.Value(), std::make_optional<std::unique_ptr<Expr>>(ParseExpr())));
        }
        else
        {
            declaration = std::make_unique<VarDeclaration>(VarDeclaration(type.GetAnnotations(), type.IsConstant(), type.IsExported(), type.getType(), name.Value(), std::make_optional<std::unique_ptr<Expr>>(ParseObjectConstructorExpr(type.getType(), true))));
        }
        if (m_outline <= 1) Expect(Lexer::TokenType::SEMICOLON, "Outline variable declaration statement must end with semicolon.");
        m_outline--;

        return std::move(declaration);
    }

    std::unique_ptr<Stmt> Parser::ParseObjectStmt(vector<AnnotationUsageDeclaration> annotations, std::string typeIdent, bool annotation)
    {
        Expect(Lexer::TokenType::OPEN_BRACE, "Open brace expected in object declaration.");

        vector<Property> objectProperties = {};
        bool export_ = false; // TODO
        m_outline++;
        while (NotEOF() && At().Type() != Lexer::TokenType::CLOSE_BRACE)
        {
            auto type = ParseType();
            auto key = Expect(Lexer::TokenType::IDENTIFIER, "Object declaration key expected.").Value();

            if (typeid(type) == typeid(ParseTypeCtxObjOrEnum))
                ThrowSyntaxError("Cannot declare enum or object inside object declaration.");

            const auto* typeAsVar = dynamic_cast<const ParseTypeCtxVar*>(&type);

            // Allows shorthand key: pair -> { key, }.
            if (At().Type() == Lexer::TokenType::COMMA)
            {
                Eat();
                objectProperties.push_back(Property(key, typeAsVar->getType(), nullopt));
                continue;
            }
            // Allows shorthand key: pair -> { key }.
            else if (At().Type() == Lexer::TokenType::CLOSE_BRACE)
            {
                objectProperties.push_back(Property(key, typeAsVar->getType(), nullopt));
                continue;
            }

            // { key: val }
            Expect(Lexer::TokenType::COLON, "Missing colon following identifier in ObjectExpr.");
            auto value = ParseExpr();

            objectProperties.push_back(Property(key, typeAsVar->getType(), std::optional<std::unique_ptr<Expr>>(std::move(value))));
            if (At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                Expect(Lexer::TokenType::COMMA, "Expected comma or closing bracket following property.");
            }
        }
        m_outline--;

        Expect(Lexer::TokenType::CLOSE_BRACE, "Object declaration missing closing brace.");
        return std::make_unique<ObjectDeclaration>(ObjectDeclaration(annotations, export_, typeIdent, objectProperties, annotation));
    }

    std::unique_ptr<Stmt> Parser::ParseEnumStmt(vector<AnnotationUsageDeclaration> annotations, std::string typeIdent)
    {
        Expect(Lexer::TokenType::OPEN_BRACE, "Open brace expected in enum declaration.");

        vector<string> objectProperties = {};
        bool export_ = false; // TODO
        m_outline++;
        while (NotEOF() && At().Type() != Lexer::TokenType::CLOSE_BRACE)
        {
            auto key = Expect(Lexer::TokenType::IDENTIFIER, "Enum entry expected.").Value();
            // Allows shorthand key: pair -> { key, }.
            if (At().Type() == Lexer::TokenType::COMMA)
            {
                Eat();
                objectProperties.push_back(key);
                continue;
            }
            // Allows shorthand key: pair -> { key }.
            else if (At().Type() == Lexer::TokenType::CLOSE_BRACE)
            {
                objectProperties.push_back(key);
                continue;
            }

            if (At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                Expect(Lexer::TokenType::COMMA, "Expected comma or closing bracket following enum entry.");
            }
        }
        m_outline--;

        Expect(Lexer::TokenType::CLOSE_BRACE, "Enum declaration missing closing brace.");
        return std::make_unique<EnumDeclaration>(EnumDeclaration(annotations, export_, typeIdent, objectProperties));
    }

    std::unique_ptr<Stmt> Parser::ParseReturnStmt()
    {
        Eat();

        m_outline++;
        auto val = ReturnDeclaration(*ParseExpr());
        Expect(Lexer::TokenType::SEMICOLON, "Return statement must end with semicolon.");
        m_outline--;

        return std::make_unique<ReturnDeclaration>(val);
    }

    std::unique_ptr<Stmt> Parser::ParseDeleteStmt()
    {
        Eat();

        m_outline++;
        auto val = DeleteDeclaration(Expect(Lexer::TokenType::IDENTIFIER, "Delete identifier expected.").Value());
        Expect(Lexer::TokenType::SEMICOLON, "Delete statement must end with semicolon.");
        m_outline--;

        return std::make_unique<DeleteDeclaration>(val);
    }

    std::unique_ptr<Stmt> Parser::ParseIfElseStmt()
    {
        auto ParseElseIf = [&]()
        {
            Eat();

            // Condition
            m_outline++;
            Expect(Lexer::TokenType::OPEN_PAREN, "Open paren expected after 'if' keyword.");
            auto condition = ParseExpr();
            Expect(Lexer::TokenType::CLOSE_PAREN, "Close paren expected after 'if' condition.");
            m_outline--;

            // Body
            vector<std::unique_ptr<Stmt>> body = {};
            if (At().Type() == Lexer::TokenType::OPEN_BRACE)
            {
                Eat();
                while (At().Type() != Lexer::TokenType::EOF_TOKEN && At().Type() != Lexer::TokenType::CLOSE_BRACE)
                {
                    body.push_back(ParseStmt());
                }
                Expect(Lexer::TokenType::CLOSE_BRACE, "Closing brace expected inside 'if' statement.");
            }
            else
            {
                body.push_back(ParseStmt());
            }

            return IfElseDeclaration::IfBlock(*condition, body);
        };

        vector<IfElseDeclaration::IfBlock> blocks = {};
        vector<std::unique_ptr<Stmt>> elseBody = {};

        blocks.push_back(ParseElseIf());

        if (At().Type() == Lexer::TokenType::ELSE)
        {
            Eat();

            if (At().Type() == Lexer::TokenType::IF)
            {
                blocks.push_back(ParseElseIf());
            }
            else
            {
                // `else` Body
                if (At().Type() == Lexer::TokenType::OPEN_BRACE)
                {
                    Eat();
                    while (At().Type() != Lexer::TokenType::EOF_TOKEN && At().Type() != Lexer::TokenType::CLOSE_BRACE)
                    {
                        elseBody.push_back(ParseStmt());
                    }
                    Expect(Lexer::TokenType::CLOSE_BRACE, "Closing brace expected inside 'else' statement.");
                }
                else
                {
                    elseBody.push_back(ParseStmt());
                }
            }
        }

        return std::make_unique<IfElseDeclaration>(IfElseDeclaration(blocks, elseBody));
    }

    std::unique_ptr<Stmt> Parser::ParseWhileStmt()
    {
        Eat();

        // Condition
        m_outline++;
        Expect(Lexer::TokenType::OPEN_PAREN, "Open paren expected after 'while' keyword.");
        auto condition = ParseExpr();
        Expect(Lexer::TokenType::CLOSE_PAREN, "Close paren expected after 'while' condition.");
        m_outline--;

        // Body
        vector<std::unique_ptr<Stmt>> body = {};
        if (At().Type() == Lexer::TokenType::OPEN_BRACE)
        {
            Eat();
            while (At().Type() != Lexer::TokenType::EOF_TOKEN && At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                body.push_back(ParseStmt());
            }
            Expect(Lexer::TokenType::CLOSE_BRACE, "Closing brace expected inside 'while' statement.");
        }
        else
        {
            body.push_back(ParseStmt());
        }

        return std::make_unique<WhileDeclaration>(WhileDeclaration(*condition, body));
    }

    std::unique_ptr<Stmt> Parser::ParseForStmt()
    {
        Eat();

        // Condition
        m_outline++;

        Expect(Lexer::TokenType::OPEN_PAREN, "Open paren expected after 'for' keyword.");
        auto variableDecl = ParseStmt();
        Expect(Lexer::TokenType::SEMICOLON, "Semicolon expected after variable declaration in 'for' loop.");

        auto condition = ParseExpr();
        Expect(Lexer::TokenType::SEMICOLON, "Semicolon expected after condition in 'for' loop.");

        auto action = ParseExpr();
        Expect(Lexer::TokenType::CLOSE_PAREN, "Close paren expected after 'for' condition.");

        m_outline--;

        // Body
        vector<std::unique_ptr<Stmt>> body = {};
        if (At().Type() == Lexer::TokenType::OPEN_BRACE)
        {
            Eat();
            while (At().Type() != Lexer::TokenType::EOF_TOKEN && At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                body.push_back(ParseStmt());
            }
            Expect(Lexer::TokenType::CLOSE_BRACE, "Closing brace expected inside 'for' statement.");
        }
        else
        {
            body.push_back(ParseStmt());
        }

        return std::make_unique<ForDeclaration>(ForDeclaration(*variableDecl, *condition, *action, body));
    }

    std::unique_ptr<Expr> Parser::ParseExpr()
    {
        return ParseAssignmentExpr();
    }

    std::unique_ptr<Expr> Parser::ParseAssignmentExpr()
    {
        auto left = ParseArrayExpr();

        if (At().Type() == Lexer::TokenType::EQUALS)
        {
            Eat();
            m_outline++;
            auto value = ParseAssignmentExpr();
            if (m_outline <= 1) Expect(Lexer::TokenType::SEMICOLON, "Semicolon expected after outline assignment expr.");
            m_outline--;
            return std::make_unique<AssignmentExpr>(AssignmentExpr(*left, *value));
        }
        else if (At().Type() == Lexer::TokenType::OPEN_BRACE)
        {
            m_outline++;
            auto value = ParseObjectConstructorExpr(&left);
            if (m_outline <= 1) Expect(Lexer::TokenType::SEMICOLON, "Semicolon expected after outline assignment expr.");
            m_outline--;
            return std::make_unique<AssignmentExpr>(AssignmentExpr(*left, *value));
        }

        return std::move(left);
    }

    std::unique_ptr<Expr> Parser::ParseObjectConstructorExpr(std::any targetVariableIdent, bool tviAsType)
    {
        auto TargetVarIdentIsIdentifier = [&]()
        {
            try
            {
                // Attempt to cast the std::any content to std::unique_ptr<Expr>
                auto exprPtr = std::any_cast<std::unique_ptr<Expr>>(targetVariableIdent);

                // Check if the object is an Identifier
                if (typeid(*exprPtr) == typeid(Identifier))
                {
                    return true;
                }
            }
            catch (const std::bad_any_cast& e)
            {
                return false;
            }

            return false;
        };

        // targetVariableIdent can be either a Types::Type or Identifier : Expr.
        if (!tviAsType && !TargetVarIdentIsIdentifier())
        {
            ThrowSyntaxError("Object constructor assignment only works for identifiers.");
        }

        Expect(Lexer::TokenType::OPEN_BRACE, "Open brace expected in object constructor.");

        vector<Property> objectProperties = {};
        while (NotEOF() && At().Type() != Lexer::TokenType::CLOSE_BRACE)
        {
            auto key = Expect(Lexer::TokenType::IDENTIFIER, "Object constructor key expected.").Value();

            // { key: val }
            Expect(Lexer::TokenType::COLON, "Missing colon following identifier in ObjectConstructorExpr.");
            auto value = ParseExpr();

            objectProperties.push_back(Property(key, nullopt, std::optional<std::unique_ptr<Expr>>(std::move(value))));
            if (At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                Expect(Lexer::TokenType::COMMA, "Expected comma or closing bracket following property.");
            }
        }

        Expect(Lexer::TokenType::CLOSE_BRACE, "Object constructor missing closing brace.");
        return std::make_unique<ObjectConstructorExpr>(ObjectConstructorExpr(targetVariableIdent, tviAsType, objectProperties));
    }

    std::unique_ptr<Expr> Parser::ParseArrayExpr()
    {
        if (At().Type() != Lexer::TokenType::OPEN_BRACE)
        {
            return ParseLambdaFuncExpr();
        }

        Eat();
        vector<std::unique_ptr<Expr>> arrayElements = {};

        while (NotEOF() && At().Type() != Lexer::TokenType::CLOSE_BRACE)
        {
            auto value = ParseExpr();
            arrayElements.push_back(value);
            if (At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                Expect(Lexer::TokenType::COMMA, "Expected comma or closing bracket following array element.");
            }
        }

        Expect(Lexer::TokenType::CLOSE_BRACE, "Array literal missing closing brace.");
        return std::make_unique<ArrayLiteral>(ArrayLiteral(arrayElements));
    }

    std::unique_ptr<Expr> Parser::ParseLambdaFuncExpr()
    {
        if (At().Type() != Lexer::TokenType::LAMBDA)
        {
            return ParseBoolExpr();
        }

        function<vector<Identifier>()> ParseIdentList = [&]()
        {
            vector<Identifier> list = {};

            auto Add = [&](std::unique_ptr<Expr> e)
            {
                if (e->Kind() != NodeType::IDENTIFIER)
                {
                    ThrowSyntaxError("Identifier required as a param in lambda expression.");
                }

                list.push_back(*(dynamic_cast<const Identifier*>(&*e)));
            };

            m_outline++;
            Eat();
            Expect(Lexer::TokenType::OPEN_PAREN, "Open paren expected in lambda expression.");
            if (At().Type() == Lexer::TokenType::CLOSE_PAREN)
                return list;

            Add(ParsePrimaryExpr());

            while (At().Type() == Lexer::TokenType::COMMA)
            {
                Add(ParsePrimaryExpr());
            }

            Expect(Lexer::TokenType::CLOSE_PAREN, "Close paren expected in lambda expression.");
            m_outline--;

            return list;
        };

        auto identList = ParseIdentList();
        vector<std::unique_ptr<Stmt>> body = {};
        bool instaret = false;
        if (At().Type() == Lexer::TokenType::OPEN_BRACE)
        {
            m_outline--;
            Eat();
            while (At().Type() != Lexer::TokenType::EOF_TOKEN && At().Type() != Lexer::TokenType::CLOSE_BRACE)
            {
                body.push_back(ParseStmt());
            }
            Expect(Lexer::TokenType::CLOSE_BRACE, "Closing brace expected inside lambda declaration.");
            m_outline++;
        }
        else
        {
            instaret = true;
            Expect(Lexer::TokenType::EQUALS, "Lambda arrow expected: Equals");
            Expect(Lexer::TokenType::MORE_THAN, "Lambda arrow expected: MoreThan");
            body.push_back(ParseStmt());
        }

        return std::make_unique<LambdaExpr>(LambdaExpr(identList, body, instaret));
    }

    std::unique_ptr<Expr> Parser::ParseBoolExpr()
    {
        auto left = ParseComparisonExpr();

        if (At().Type() == Lexer::TokenType::OR && m_tokens[1].Type() == Lexer::TokenType::OR)
        {
            Eat(); Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseBoolExpr(), EqualityCheckExpr::Type::OR));
        } 
        else if (At().Type() == Lexer::TokenType::AND && m_tokens[1].Type() == Lexer::TokenType::AND)
        {
            Eat(); Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseBoolExpr(), EqualityCheckExpr::Type::AND));
        }

        return left;
    }

    std::unique_ptr<Expr> Parser::ParseComparisonExpr()
    {
        auto left = ParseAdditiveExpr();

        if (At().Type() == Lexer::TokenType::EQUALS && m_tokens[1].Type() == Lexer::TokenType::EQUALS)
        {
            Eat(); Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseAdditiveExpr(), EqualityCheckExpr::Type::EQUALS));
        }
        else if (At().Type() == Lexer::TokenType::NOT && m_tokens[1].Type() == Lexer::TokenType::EQUALS)
        {
            Eat(); Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseAdditiveExpr(), EqualityCheckExpr::Type::NOT_EQUALS));
        }
        else if (At().Type() == Lexer::TokenType::LESS_THAN)
        {
            Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseAdditiveExpr(), EqualityCheckExpr::Type::LESS_THAN));
        }
        else if (At().Type() == Lexer::TokenType::LESS_THAN && m_tokens[1].Type() == Lexer::TokenType::EQUALS)
        {
            Eat(); Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseAdditiveExpr(), EqualityCheckExpr::Type::LESS_THAN_OR_EQUALS));
        }
        else if (At().Type() == Lexer::TokenType::MORE_THAN)
        {
            Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseAdditiveExpr(), EqualityCheckExpr::Type::MORE_THAN));
        }
        else if (At().Type() == Lexer::TokenType::MORE_THAN && m_tokens[1].Type() == Lexer::TokenType::EQUALS)
        {
            Eat(); Eat();
            return std::make_unique<EqualityCheckExpr>(EqualityCheckExpr(*left, *ParseAdditiveExpr(), EqualityCheckExpr::Type::MORE_THAN_OR_EQUALS));
        }

        return left;
    }

    std::unique_ptr<Expr> Parser::ParseAdditiveExpr()
    {
        auto left = ParseMultiplicitaveExpr();

        while (At().Value() == "+" || At().Value() == "-")
        {
            auto operator_ = Eat().Value();
            auto right = ParseMultiplicitaveExpr();
            left.reset(new BinaryExpr(*left, *right, operator_[0]));
        }

        return left;
    }

    std::unique_ptr<Expr> Parser::ParseMultiplicitaveExpr()
    {
        auto left = ParseUnaryExpr();

        while (At().Value() == "/" || At().Value() == "*" || At().Value() == "%")
        {
            auto operator_ = Eat().Value();
            auto right = ParseUnaryExpr();
            left.reset(new BinaryExpr(*left, *right, operator_[0]));
        }

        return left;
    }

    std::unique_ptr<Expr> Parser::ParseUnaryExpr()
    {
        string operator_ = "";
        std::unique_ptr<Expr> obj = nullptr;

        if (At().Value() == "+" || At().Value() == "-")
        {
            operator_ = Eat().Value();
            obj = ParseCallMemberExpr();
            return std::make_unique<UnaryExpr>(UnaryExpr(*obj, operator_));
        }

        return ParseCallMemberExpr();
    }

    std::unique_ptr<Expr> Parser::ParseCallMemberExpr()
    {
        auto member = ParseMemberExpr();

        if (At().Type() == Lexer::TokenType::OPEN_PAREN)
        {
            return ParseCallExpr(std::move(member));
        }
        else if (At().Type() == Lexer::TokenType::OPEN_BRACKET)
        {
            return ParseIndexExpr(std::move(member));
        }

        return member;
    }

    std::unique_ptr<Expr> Parser::ParseIndexExpr(std::unique_ptr<Expr> caller)
    {
        m_outline++;
        Expect(Lexer::TokenType::OPEN_BRACKET, "Open bracket expected inside index expression.");
        std::unique_ptr<Expr> callExpr = std::make_unique<IndexExpr>(IndexExpr(*ParseExpr(), *caller));
        Expect(Lexer::TokenType::CLOSE_BRACKET, "Closing bracket expected inside index expression.");
        m_outline--;

        if (At().Type() == Lexer::TokenType::OPEN_BRACKET)
        {
            callExpr = ParseIndexExpr(std::move(callExpr));
        }

        return callExpr;
    }

    std::unique_ptr<Expr> Parser::ParseCallExpr(std::unique_ptr<Expr> caller)
    {
        std::unique_ptr<Expr> callExpr = std::make_unique<CallExpr>(ParseArgs([&]()
        {
            if (m_outline <= 1) Expect(Lexer::TokenType::SEMICOLON, "Semicolon expected after outline call expression.");
        }), *caller);

        if (At().Type() == Lexer::TokenType::OPEN_PAREN)
        {
            callExpr = ParseCallExpr(std::move(callExpr));
        }

        return std::move(callExpr);
    }

    /**
    @param[in] preEnd Make something execute after the closing paren.
    @returns The args.
    */
    vector<std::unique_ptr<Expr>> Parser::ParseArgs(optional<std::function<void()>> preEnd)
    {
        m_outline++;

        Expect(Lexer::TokenType::OPEN_PAREN, "Expected open parenthesis inside arguments list.");
        auto args = At().Type() == Lexer::TokenType::CLOSE_PAREN ? vector<std::unique_ptr<Expr>>() : ParseArgumentsList();

        Expect(Lexer::TokenType::CLOSE_PAREN, "Expected closing parenthesis inside arguments list.");
        preEnd.value();

        m_outline--;
        return std::move(args);
    }

    vector<std::unique_ptr<Expr>> Parser::ParseArgumentsList()
    {
        vector<std::unique_ptr<Expr>> args = { ParseAssignmentExpr() };

        while (At().Type() == Lexer::TokenType::COMMA)
        {
            args.push_back(ParseAssignmentExpr());
        }

        return std::move(args);
    }

    std::unique_ptr<Expr> Parser::ParseMemberExpr()
    {
        auto object = ParsePrimaryExpr();

        while (At().Type() == Lexer::TokenType::DOT)
        {
            Eat();
            std::unique_ptr<Expr> property = nullptr;

            // get identifier
            property = ParsePrimaryExpr();

            object.reset(new MemberExpr(*object, *property));
        }

        return object;
    }

    std::unique_ptr<Expr> Parser::ParsePrimaryExpr()
    {
        auto tk = At().Type();

        switch (tk)
        {
        case Lexer::TokenType::IDENTIFIER:
            return std::make_unique<Identifier>(Eat().Value());
        case Lexer::TokenType::NUMBER:
            return std::make_unique<NumericLiteral>(std::stoi(Eat().Value()));
        case Lexer::TokenType::FLOAT_NUMBER:
            return std::make_unique<FloatLiteral>(std::stof(Eat().Value()));
        case Lexer::TokenType::DOUBLE_NUMBER:
            return std::make_unique<DoubleLiteral>(std::stod(Eat().Value()));
        case Lexer::TokenType::STRING:
            return std::make_unique<StringLiteral>(Eat().Value());
        case Lexer::TokenType::CHAR:
            return std::make_unique<CharLiteral>(Eat().Value()[0]);
        case Lexer::TokenType::OPEN_PAREN:
        {
            Eat();
            auto value = ParseExpr();
            Expect(Lexer::TokenType::CLOSE_PAREN, "Unexpected token found inside parenthesised expression. Expected closing parenthesis.");
            return value;
        }

        default:
            ThrowSyntaxError("Unexpected token found while parsing! " + At().Value());
        }
    }
}