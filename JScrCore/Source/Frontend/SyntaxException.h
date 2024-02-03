#pragma once
#include <stdexcept>
#include "../Utils/Vector.h"

namespace JScr::Frontend
{
	class SyntaxException : public std::exception
	{
	public:
        SyntaxException(const std::string& filedir, const JScr::Utils::Vector2i& begin, const std::string& description)
            : m_filedir(filedir), m_begin(begin), m_errCode(GenerateErrorCode(description)), m_description(description)
        {}

        static SyntaxException Unknown(const std::string& filedir, const JScr::Utils::Vector2i& begin)
        {
            return SyntaxException(filedir, begin, "Unknown syntax error.");
        }

    public:
        const std::string&            FileDir() const { return m_filedir; }
        const JScr::Utils::Vector2i& BeginPos() const { return m_begin; }
        const std::uint16_t&        ErrorCode() const { return m_errCode; }
        const std::string&        Description() const { return m_description; }

        std::string ToString() const
        {
            return "Syntax error at: \"" + FileDir() + "\" [" + std::to_string(BeginPos().x) + ":" + std::to_string(BeginPos().y) + "] (" + std::to_string(ErrorCode()) + ") \"" + Description() + "\"";
        }
	private:
        std::uint16_t GenerateErrorCode(std::string errCode)
        {
            std::uint16_t num = 0;

            for (char c : errCode)
            {
                num += (int) c;
            }

            return num;
        }
    private:
        const std::string& m_filedir;
        const JScr::Utils::Vector2i& m_begin;
        const std::uint16_t& m_errCode;
        const std::string& m_description;
	};
}
