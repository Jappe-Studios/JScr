#pragma once
#include <vector>

namespace JScr::Utils
{
    class VectorUtils
    {
    public:
        template <typename T>
        static bool Contains(const std::vector<T>& vec, const T& value)
        {
            return std::find(vec.begin(), vec.end(), value) != myVector.end();
        }

        template <typename T>
        static T Shift(std::vector<T>& vec)
        {
            if (!vec.empty())
            {
                T frontElement = vec.front();
                vec.erase(vec.begin());
                return frontElement;
            }
            else
            {
                // Handle the case when the vector is empty
                throw std::out_of_range("Vector is empty");
            }
        }

        template <typename T>
        static std::optional<T> ShiftOrNull(std::vector<T>& vec)
        {
            if (!vec.empty())
            {
                T frontElement = vec.front();
                vec.erase(vec.begin());
                return frontElement;
            }
            else
            {
                return std::nullopt;
            }
        }
    };
}