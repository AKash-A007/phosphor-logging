#include "serializer.hpp"

#include <string_view>

namespace phosphor::logging::extensions::ael
{

namespace
{

inline void appendIfPresent(AelSerializer::Map& out, std::string_view key,
                            const std::optional<std::string>& value)
{
    if (value)
    {
        out.emplace(std::string(key), *value);
    }
}

inline std::string joinComma(const std::vector<std::string>& values)
{
    std::string result;

    for (const auto& value : values)
    {
        if (!result.empty())
        {
            result += ',';
        }
        result += value;
    }

    return result;
}

} // namespace

AelSerializer::Map AelSerializer::toMap(const AelBase& object)
{
    Map result;

    result.emplace(std::string(details::keyVersion), object.version);
    result.emplace(std::string(details::keyNamespace), object.nameSpace);

    appendIfPresent(result, details::keySubtype, object.subType);
    appendIfPresent(result, details::keyErrorClass, object.errorClass);
    appendIfPresent(result, details::keySeverityOverride,
                    object.severityOverride);
    appendIfPresent(result, details::keyCorrelationId, object.correlationId);

    if (const auto* simple = dynamic_cast<const AelSimple*>(&object))
    {
        result.emplace(std::string(details::keyType), "SIMPLE");
        result.emplace(std::string(details::keyAfid), simple->afid);

        if (!simple->fruList.empty())
        {
            result.emplace(std::string(details::keyFruList),
                           joinComma(simple->fruList));
        }
    }
    else if (const auto* complex = dynamic_cast<const AelComplex*>(&object))
    {
        result.emplace(std::string(details::keyType), "COMPLEX");
        result.emplace(std::string(details::keyUri), complex->dataUri);
    }

    return result;
}

} // namespace phosphor::logging::extensions::ael
