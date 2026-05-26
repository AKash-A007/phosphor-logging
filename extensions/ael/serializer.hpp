/**
 * @file serializer.hpp
 * @brief Converts AEL objects into DBus map representation
 */

#pragma once

#include "model.hpp"

#include <map>
#include <string>
#include <string_view>

namespace phosphor::logging::extensions::ael
{

namespace details
{
constexpr std::string_view keyVersion{"AEL.VERSION"};
constexpr std::string_view keyNamespace{"AEL.NAMESPACE"};
constexpr std::string_view keyType{"AEL.TYPE"};
constexpr std::string_view keySubtype{"AEL.SUBTYPE"};
constexpr std::string_view keyAfid{"AEL.AFID"};
constexpr std::string_view keyFruList{"AEL.FRU_LIST"};
constexpr std::string_view keyUri{"AEL.DATA.URI"};
constexpr std::string_view keyErrorClass{"AEL.ERROR_CLASS"};
constexpr std::string_view keySeverityOverride{"AEL.SEVERITY_OVERRIDE"};
constexpr std::string_view keyCorrelationId{"AEL.CORRELATION_ID"};
} // namespace details

/**
 * @brief Serializes AEL objects into OemAdditionalData format
 */
class AelSerializer
{
  public:
    using Map = std::map<std::string, std::string>;

    /**
     * @brief Convert AEL object to DBus map
     */
    static Map toMap(const AelBase& obj);

  private:
    static void appendIfPresent(Map& m, std::string_view key,
                                const std::optional<std::string>& val);

    static std::string join(const std::vector<std::string>& vec);
};

} // namespace phosphor::logging::extensions::ael
