#pragma once
#include "Ast.h"
#include "../Runtime/Types.h"
#include "Lexer.h"
#include "../Utils/Vector.h"
#include "../Utils/VectorUtils.h"
#include "SyntaxException.h"

namespace JScr::Frontend
{
    using std::vector;
    using namespace JScr::Runtime;

    class Parser
    {
    private:
        class ParseTypeCtx
        {
        public:
            virtual ~ParseTypeCtx() = default;
            virtual bool IsConstant() const = 0;
            virtual bool IsExported() const = 0;
            virtual const std::vector<AnnotationUsageDeclaration>& GetAnnotations() const = 0;
        };

        class ParseTypeCtxVar : public ParseTypeCtx
        {
        private:
            bool m_constant;
            bool m_exported;
            std::vector<AnnotationUsageDeclaration> m_annotations;
            Types::Type m_type;

        public:
            ParseTypeCtxVar(bool constant, bool exported, Types::Type type, const std::vector<AnnotationUsageDeclaration>& annotations)
                : m_constant(constant), m_exported(exported), m_type(type), m_annotations(annotations)
            {}

            bool IsConstant() const override
            {
                return m_constant;
            }

            bool IsExported() const override
            {
                return m_exported;
            }

            const std::vector<AnnotationUsageDeclaration>& GetAnnotations() const override
            {
                return m_annotations;
            }

            Types::Type getType() const
            {
                return m_type;
            }
        };

        class ParseTypeCtxObjOrEnum : public ParseTypeCtx
        {
        private:
            bool m_constant;
            bool m_exported;
            std::vector<AnnotationUsageDeclaration> m_annotations;
            Lexer::Token m_type;
            Types::Type m_identifierT;

        public:
            ParseTypeCtxObjOrEnum(bool constant, bool exported, Lexer::Token type, Types::Type identifierT, const std::vector<AnnotationUsageDeclaration>& annotations)
                : m_constant(constant), m_exported(exported), m_type(type), m_identifierT(identifierT), m_annotations(annotations)
            {}

            bool IsConstant() const override
            {
                return m_constant;
            }

            bool IsExported() const override
            {
                return m_exported;
            }

            const std::vector<AnnotationUsageDeclaration>& GetAnnotations() const override
            {
                return m_annotations;
            }

            Lexer::Token getType() const
            {
                return m_type;
            }

            Types::Type getIdentifierT() const
            {
                return m_identifierT;
            }
        };

    public:
        Program ProduceAST(string filedir);

    private:
        vector<Lexer::Token> m_tokens = {};
        vector<Range> m_linesAndCols = {};
        string m_filedir = "";
        std::uint8_t m_outline = 0;

    private:
        bool NotEof() const { return m_tokens[0].Type() != Lexer::TokenType::EOF_TOKEN; }

        Lexer::Token At() const { return m_tokens[0]; }

        Vector2i AtPosBegin() const { return m_linesAndCols[0].Begin(); }

        Vector2i AtPosEnd() const { return m_linesAndCols[0].End(); }

        Lexer::Token Eat() { VectorUtils::Shift(m_linesAndCols); return VectorUtils::Shift(m_tokens); }

        Lexer::Token Expect(Lexer::TokenType type, string syntaxExceptionDescription)
        {
            auto prev = Eat();
            if (prev.Type() != type)
            {
                ThrowSyntaxError(syntaxExceptionDescription);
            }

            return prev;
        }

        void ThrowSyntaxError(string description)
        {
            throw SyntaxException(m_filedir, AtPosBegin(), description);
        }

    private:
        Stmt ParseStmt();
    };
}