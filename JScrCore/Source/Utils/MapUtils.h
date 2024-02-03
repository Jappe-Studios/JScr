#pragma once
#include <unordered_map>
#include <optional>

namespace JScr::Utils
{
    class MapUtils
    {
    public:
        template <typename K, typename V>
        static std::optional<K> GetKeyByValue(const std::unordered_map<K, V>& myMap, const V& value)
        {
            for (const auto& pair : myMap)
            {
                if (pair.second == value)
                {
                    return pair.first; // Return the key when the value is found
                }
            }
            return std::nullopt; // Return null when the value is not found
        }

        template <typename K, typename V>
        static std::vector<V> ValuesOf(const std::unordered_map<K, V>& myMap)
        {
            std::vector<V> values;
            for (const auto& pair : myMap)
            {
                values.push_back(pair.second);
            }
            return std::move(values);
        }
    };
}