#include "Types.h"
#include "../Utils/StringUtils.h"
#include "../Utils/MapUtils.h"
using namespace JScr::Utils;

namespace std
{
    template <>
    struct hash<JScr::Runtime::Types::Type>
    {
        std::size_t operator()(const JScr::Runtime::Types::Type& type) const
        {
            return std::hash<unsigned short>()(type.Uid());
        }
    };
}

namespace JScr::Runtime
{
	const std::unordered_map<Types::Type, std::string> Types::types = {
        { Type::Dynamic(), "dynamic" },
        { Type::Void(),    "void"    },
        { Type::Bool(),    "bool"    },
        { Type::Int(),     "int"     },
        { Type::Float(),   "float"   },
        { Type::Double(),  "double"  },
        { Type::String(),  "string"  },
        { Type::Char(),    "char"    },
	};

    Types::Type Types::FromString(const string& input)
    {
        string val = StringUtils::ReplaceAll(StringUtils::Trim(input), " ", "");
        bool array = StringUtils::EndsWith(input, "[]");

        if (array)
        {
            val = StringUtils::ReplaceAll(val, "[]", "");
            auto arrayType = FromString(val);
            return Type::Array(arrayType);
        }

        auto type = MapUtils::GetKeyByValue(types, val);

        if (!type.has_value())
        {
            return Type::Object(val);
        }

        return type.value();
    }
}