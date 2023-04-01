#include "utils/math.hpp"
#include <cmath>

#include "serialize/serializer.hpp"

namespace Sowa {
    template<>
    YamlNode Serializer<Size>::Save(const Size& in) {
        YamlNode doc;
        doc["W"] = in.w;
        doc["H"] = in.h;
        return doc;
    }

    template<>
    bool Serializer<Size>::Load(Size& out, const YamlNode& doc) {
        out.w = doc["W"].as<int>(0);
        out.h = doc["H"].as<int>(0);
        return true;
    }
} // namespace Sowa
