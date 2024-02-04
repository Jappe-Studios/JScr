#pragma once
#include <string>
#include <vector>
#include <any>
#include "../Runtime/Types.h"

namespace JScr::Frontend
{
    using std::string;
    using std::any;

    enum NodeType
    {
        // STATEMENTS
        PROGRAM,
        IMPORT_STMT,
        ANNOTATION_USAGE_DECLARATION,
        VAR_DECLARATION,
        FUNCTION_DECLARATION,
        OBJECT_DECLARATION,
        ENUM_DECLARATION,
        RETURN_DECLARATION,
        DELETE_DECLARATION,
        IF_ELSE_DECLARATION,
        WHILE_DECLARATION,
        FOR_DECLARATION,

        // EXPRESSIONS
        ASSIGNMENT_EXPR,
        EQUALITY_CHECK_EXPR,
        MEMBER_EXPR,
        UNARY_EXPR,
        LAMBDA_EXPR,
        CALL_EXPR,
        INDEX_EXPR,
        OBJECT_CONSTRUCTOR_EXPR,

        // LITERALS
        PROPERTY,
        ARRAY_LITERAL,
        NUMERIC_LITERAL,
        FLOAT_LITERAL,
        DOUBLE_LITERAL,
        STRING_LITERAL,
        CHAR_LITERAL,
        IDENTIFIER,
        BINARY_EXPR,
    };

    class Stmt
    {
    public:
        virtual void abstract() const = 0;
        Stmt(const NodeType& kind) : m_kind(kind) {}

        const NodeType& Kind() const { return m_kind; }

    private:
        const NodeType& m_kind;
    };

    class Program : public Stmt
    {
    public:
        Program(const string& fileDir, const std::vector<Stmt>& body) : Stmt(NodeType::PROGRAM), m_fileDir(fileDir), m_body(body) {}
        void abstract() const override {}

        const string& FileDir() const         { return m_fileDir; }
        std::vector<Stmt>& Body()             { return m_body; }
    private:
        const string& m_fileDir;
        std::vector<Stmt> m_body;
    };

    class ImportStmt : public Stmt
    {
    public:
        ImportStmt(const std::vector<string>& target, const string& alias) : Stmt(NodeType::IMPORT_STMT), m_target(target), m_alias(alias) {}
        void abstract() const override {}

        const std::vector<string>& Target() const { return m_target; }
        const string& Alias() const               { return m_alias; }
    private:
        const std::vector<string>& m_target;
        const string& m_alias;
    };

    class AnnotationUsageDeclaration : public Stmt
    {
    public:
        AnnotationUsageDeclaration(const string& ident, const std::vector<Expr>& args) : Stmt(NodeType::ANNOTATION_USAGE_DECLARATION), m_ident(ident), m_args(args) {}
        void abstract() const override {}

        const string& Ident() const { return m_ident; };
        const std::vector<Expr>& Args() const { return m_args; };
    private:
        const string& m_ident;
        const std::vector<Expr>& m_args;
    };

    class VarDeclaration : public Stmt
    {
    public:
        VarDeclaration(const std::vector<AnnotationUsageDeclaration> annotatedWith, const bool& constant, const bool& export_, const Types::Type& type, const string& identifier, const Expr& value)
            : Stmt(NodeType::VAR_DECLARATION), m_annotatedWith(annotatedWith), m_constant(constant), m_export(export_), m_type(type), m_identifier(identifier), m_value(value)
        {}
        void abstract() const override {}

        const std::vector<AnnotationUsageDeclaration>& AnnotatedWith() const { return m_annotatedWith; }
        const bool& Constant() const { return m_constant; }
        const bool& Export() const { return m_export; }
        const Types::Type& Type() const { return m_type; }
        const string& Identifier() const { return m_identifier; }
        const Expr& Value() const { return m_value; }
    private:
        const std::vector<AnnotationUsageDeclaration>& m_annotatedWith;
        const bool& m_constant;
        const bool& m_export;
        const Types::Type& m_type;
        const string& m_identifier;
        const Expr& m_value;
    };

    class FunctionDeclaration : public Stmt
    {
    public:
        FunctionDeclaration(const std::vector<AnnotationUsageDeclaration>& annotatedWith, const bool& export_, const std::vector<VarDeclaration>& parameters, const string& identifier, const Types::Type& type, const std::vector<Stmt>& body, const bool& instantReturn)
            : Stmt(NodeType::FUNCTION_DECLARATION), m_annotatedWith(annotatedWith), m_export(export_), m_parameters(parameters), m_identifier(identifier), m_type(type), m_body(body), m_instantReturn(instantReturn)
        {}
        void abstract() const override {}

        const std::vector<AnnotationUsageDeclaration>& AnnotatedWith() const { return m_annotatedWith; }
        const bool& Export() const { return m_export; }
        const std::vector<VarDeclaration>& Parameters() const { return m_parameters; }
        const string& Identifier() const { return m_identifier; } // <-- name
        const Types::Type& Type() const { return m_type; }
        const std::vector<Stmt>& Body() const { return m_body; }
        const bool& InstantReturn() const { return m_instantReturn; }
    private:
        const std::vector<AnnotationUsageDeclaration>& m_annotatedWith;
        const bool& m_export;
        const std::vector<VarDeclaration>& m_parameters;
        const string& m_identifier;
        const Types::Type& m_type;
        const std::vector<Stmt>& m_body;
        const bool& m_instantReturn;
    };

    class ObjectDeclaration : public Stmt
    {
    public:
        ObjectDeclaration(const std::vector<AnnotationUsageDeclaration>& annotatedWith, const bool& export_, const string& identifier, const std::vector<Property>& properties, const bool& isAnnotationDeclaration)
            : Stmt(NodeType::OBJECT_DECLARATION), m_annotatedWith(annotatedWith), m_export(export_), m_identifier(identifier), m_properties(properties), m_isAnnotationDecl(isAnnotationDeclaration)
        {}
        void abstract() const override {}

        const std::vector<AnnotationUsageDeclaration>& AnnotatedWith() const { return m_annotatedWith; };
        const bool& Export() const { return m_export; }
        const string& Identifier() const { return m_identifier; } // <-- name
        const std::vector<Property>& Properties() const { return m_properties; }
        const bool& IsAnnotationDecl() const { return m_isAnnotationDecl; }
    private:
        const std::vector<AnnotationUsageDeclaration>& m_annotatedWith;
        const bool& m_export;
        const string& m_identifier;
        const std::vector<Property>& m_properties;
        const bool& m_isAnnotationDecl;
    };

    class EnumDeclaration : public Stmt
    {
    public:
        EnumDeclaration(const std::vector<AnnotationUsageDeclaration>& annotatedWith, const bool& export_, const string& identifier, const std::vector<string>& entries)
            : Stmt(NodeType::ENUM_DECLARATION), m_annotatedWith(annotatedWith), m_export(export_), m_identifier(identifier), m_entries(entries)
        {}
        void abstract() const override {}

        const std::vector<AnnotationUsageDeclaration>& AnnotatedWith() const { return m_annotatedWith; }
        const bool& Export() const { return m_export; }
        const string& Identifier() const { return m_identifier; } // <-- name
        const std::vector<string>& Entries() const { return m_entries; }
    private:
        const std::vector<AnnotationUsageDeclaration>& m_annotatedWith;
        const bool& m_export;
        const string& m_identifier;
        const std::vector<string>& m_entries;
    };

    class ReturnDeclaration : public Stmt
    {
    public:
        ReturnDeclaration(const Expr& value) : Stmt(NodeType::RETURN_DECLARATION), m_value(value) {}
        void abstract() const override {}

        const Expr& Value() const { return m_value; }
    private:
        const Expr& m_value;
    };

    class DeleteDeclaration : public Stmt
    {
    public:
        DeleteDeclaration(const string& value) : Stmt(NodeType::DELETE_DECLARATION), m_value(value) {}
        void abstract() const override {}

        const string& Value() const { return m_value; }
    private:
        const string& m_value;
    };

    class IfElseDeclaration : public Stmt
    {
    public:
        class IfBlock
        {
        public:
            IfBlock(const Expr& condition, const std::vector<Stmt>& body) : m_condition(condition), m_body(body) {}

            const Expr& Condition() const { return m_condition; }
            const std::vector<Stmt>& Body() const { return m_body; }
        private:
            const Expr& m_condition;
            const std::vector<Stmt>& m_body;
        };

        IfElseDeclaration(const std::vector<IfBlock>& blocks, const std::vector<Stmt>& elseBody) : Stmt(NodeType::IF_ELSE_DECLARATION), m_blocks(blocks), m_elseBody(elseBody) {}
        void abstract() const override {}

        const std::vector<IfBlock>& Blocks() const { return m_blocks; }
        const std::vector<Stmt>& ElseBody() const { return m_elseBody; }
    private:
        const std::vector<IfBlock>& m_blocks;
        const std::vector<Stmt>& m_elseBody;
    };

    class WhileDeclaration : public Stmt
    {
    public:
        WhileDeclaration(const Expr& condition, const std::vector<Stmt>& body) : Stmt(NodeType::WHILE_DECLARATION), m_condition(condition), m_body(body) {}
        void abstract() const override {}

        const Expr& Condition() const { return m_condition; }
        const std::vector<Stmt>& Body() const { return m_body; }
    private:
        const Expr& m_condition;
        const std::vector<Stmt>& m_body;
    };

    class ForDeclaration : public Stmt
    {
    public:
        ForDeclaration(const Stmt& declaration, const Expr& condition, const Expr& action, const std::vector<Stmt>& body)
            : Stmt(NodeType::FOR_DECLARATION), m_declaration(declaration), m_condition(condition), m_action(action), m_body(body)
        {}
        void abstract() const override {}

        const Stmt& Declaration() const { return m_declaration; }
        const Expr& Condition() const { return m_condition; }
        const Expr& Action() const { return m_action; }
        const std::vector<Stmt>& Body() const { return m_body; }
    private:
        const Stmt& m_declaration;
        const Expr& m_condition;
        const Expr& m_action;
        const std::vector<Stmt>& m_body;
    };

    class Expr : public Stmt
    {
    public:
        virtual void abstract() const = 0;
        Expr(NodeType kind) : Stmt(kind) {}
    };

    class AssignmentExpr : public Expr
    {
    public:
        AssignmentExpr(const Expr& assigne, const Expr& value) : Expr(NodeType::ASSIGNMENT_EXPR), m_assigne(assigne), m_value(value) {}
        void abstract() const override {}

        const Expr& Assigne() const { return m_assigne; }
        const Expr& Value() const { return m_value; }
    private:
        const Expr& m_assigne;
        const Expr& m_value;
    };

    class EqualityCheckExpr : public Expr
    {
    public:
        enum Type
        {
            EQUALS, NOT_EQUALS, MORE_THAN, MORE_THAN_OR_EQUALS, LESS_THAN, LESS_THAN_OR_EQUALS, AND, OR
        };

        EqualityCheckExpr(const Expr& left, const Expr& right, const Type& operator_) : Expr(NodeType::EQUALITY_CHECK_EXPR), m_left(left), m_right(right), m_operator_(operator_) {}
        void abstract() const override {}

        const Expr& Left() const { return m_left; }
        const Expr& Right() const { return m_right; }
        const Type& Operator() const { return m_operator_; }
    private:
        const Expr& m_left;
        const Expr& m_right;
        const Type& m_operator_;
    };

    class BinaryExpr : public Expr
    {
    public:
        BinaryExpr(const Expr& left, const Expr& right, const char& operator_) : Expr(NodeType::BINARY_EXPR), m_left(left), m_right(right), m_operator_(operator_) {}
        void abstract() const override {}

        const Expr& Left() const { return m_left; }
        const Expr& Right() const { return m_right; }
        const char& Operator() const { return m_operator_; }
    private:
        const Expr& m_left;
        const Expr& m_right;
        const char& m_operator_;
    };

    class CallExpr : public Expr
    {
    public:
        CallExpr(const std::vector<Expr>& args, const Expr& caller) : Expr(NodeType::CALL_EXPR), m_args(args), m_caller(caller) {}
        void abstract() const override {}

        const std::vector<Expr>& Args() const { return m_args; }
        const Expr& Caller() const { return m_caller; }
    private:
        const std::vector<Expr>& m_args;
        const Expr& m_caller;
    };

    class IndexExpr : public Expr
    {
    public:
        IndexExpr(const Expr& arg, const Expr& caller) : Expr(NodeType::INDEX_EXPR), m_arg(arg), m_caller(caller) {}
        void abstract() const override {}

        const Expr& Arg() const { return m_arg; }
        const Expr& Caller() const { return m_caller; }
    private:
        const Expr& m_arg;
        const Expr& m_caller;
    };

    class ObjectConstructorExpr : public Expr
    {
    public:
        ObjectConstructorExpr(const any& targetVarIdent, const bool& targetVarIdentAsType, const std::vector<Property>& properties) : Expr(NodeType::OBJECT_CONSTRUCTOR_EXPR), m_targetVarIdent(targetVarIdent), m_targetVarIdentAsType(targetVarIdentAsType), m_properties(properties) {}
        void abstract() const override {}

        const any& TargetVarIdent() const { return m_targetVarIdent; }
        const bool& TargetVarIdentAsType() const { return m_targetVarIdentAsType; }
        const std::vector<Property>& Properties() const { return m_properties; }
    private:
        const any& m_targetVarIdent;
        const bool& m_targetVarIdentAsType;
        const std::vector<Property>& m_properties;
    };

    class MemberExpr : public Expr
    {
    public:
        MemberExpr(const Expr& object, const Expr& property) : Expr(NodeType::MEMBER_EXPR), m_object(object), m_property(property) {}
        void abstract() const override {}

        const Expr& Object() const { return m_object; }
        const Expr& Property() const { return m_property; }
    private:
        const Expr& m_object;
        const Expr& m_property;
    };

    class UnaryExpr : public Expr
    {
    public:
        UnaryExpr(const Expr& object, const string& operator_) : Expr(NodeType::UNARY_EXPR), m_object(object), m_operator(operator_) {}
        void abstract() const override {}

        const Expr& Object() const { return m_object; }
        const string& Operator() const { return m_operator; }
    private:
        const Expr& m_object;
        const string& m_operator;
    };

    class LambdaExpr : public Expr
    {
    public:
        LambdaExpr(const std::vector<Identifier>& paramIdents, const std::vector<Stmt>& body, const bool& instantReturn) : Expr(NodeType::LAMBDA_EXPR), m_paramIdents(paramIdents), m_body(body), m_instantReturn(instantReturn) {}
        void abstract() const override {}

        const std::vector<Identifier>& ParamIdents() const { return m_paramIdents; }
        const std::vector<Stmt>& Body() const { return m_body; }
        const bool& InstantReturn() const { return m_instantReturn; }
    private:
        const std::vector<Identifier>& m_paramIdents;
        const std::vector<Stmt>& m_body;
        const bool& m_instantReturn;
    };

    class Identifier : public Expr
    {
    public:
        Identifier(const string& symbol) : Expr(NodeType::IDENTIFIER), m_symbol(symbol) {}
        void abstract() const override {}

        const string& Symbol() const { return m_symbol; }
    private:
        const string& m_symbol;
    };

    class ArrayLiteral : public Expr
    {
    public:
        ArrayLiteral(const std::vector<Expr>& value) : Expr(NodeType::ARRAY_LITERAL), m_value(value) {}
        void abstract() const override {}

        const std::vector<Expr>& Value() const { return m_value; }
    private:
        const std::vector<Expr>& m_value;
    };

    class NumericLiteral : public Expr
    {
    public:
        NumericLiteral(const int& value) : Expr(NodeType::NUMERIC_LITERAL), m_value(value) {}
        void abstract() const override {}

        const int& Value() const { return m_value; }
    private:
        const int& m_value;
    };

    class FloatLiteral : public Expr
    {
    public:
        FloatLiteral(const float& value) : Expr(NodeType::FLOAT_LITERAL), m_value(value) {}
        void abstract() const override {}

        const float& Value() const { return m_value; }
    private:
        const float& m_value;
    };

    class DoubleLiteral : public Expr
    {
    public:
        DoubleLiteral(const double& value) : Expr(NodeType::DOUBLE_LITERAL), m_value(value) {}
        void abstract() const override {}

        const double& Value() const { return m_value; }
    private:
        const double& m_value;
    };

    class StringLiteral : public Expr
    {
    public:
        StringLiteral(const string& value) : Expr(NodeType::STRING_LITERAL), m_value(value) {}
        void abstract() const override {}

        const string& Value() const { return m_value; }
    private:
        const string& m_value;
    };

    class CharLiteral : public Expr
    {
    public:
        CharLiteral(const char& value) : Expr(NodeType::CHAR_LITERAL), m_value(value) {}
        void abstract() const override {}

        const char& Value() const { return m_value; }
    private:
        const char& m_value;
    };

    class Property : public Expr
    {
    public:
        Property(const string& key, const Types::Type& type, const Expr& value) : Expr(NodeType::PROPERTY), m_key(key), m_type(type), m_value(value) {}
        void abstract() const override {}

        const string& Key() const { return m_key; }
        const Types::Type& Type() const { return m_type; }
        const Expr& Value() const { return m_value; }
    private:
        const string& m_key;
        const Types::Type& m_type;
        const Expr& m_value;
    };
}