#include "Types.h"
#include "../Utils/StringUtils.h"
#include "../Utils/MapUtils.h"

namespace JScr::Runtime
{
    using namespace JScr::Utils;

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