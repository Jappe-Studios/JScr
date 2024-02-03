#pragma once
#include <string>
#include <algorithm>

namespace JScr::Utils
{
    class StringUtils
    {
    public:
        static std::string Trim(const std::string& str)
        {
            auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c)
            {
                return std::isspace(c);
            });

            auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c)
            {
                return std::isspace(c);
            });

            return (start < end.base()) ? std::string(start, end.base()) : "";
        }

        static std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to)
        {
            size_t start_pos = 0;
            std::string ret = str;
            while ((start_pos = ret.find(from, start_pos)) != std::string::npos)
            {
                ret.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', advance start_pos after replacement
            }
            return std::move(ret);
        }

        static bool EndsWith(const std::string& str, const std::string& suffix)
        {
            if (str.length() >= suffix.length())
            {
                return (str.substr(str.length() - suffix.length()) == suffix);
            }
            return false;
        }
    };
}