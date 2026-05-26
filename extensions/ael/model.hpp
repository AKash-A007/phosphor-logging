/**
 * @file model.hpp
 * @brief AEL metadata data model definitions
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace phosphor::logging::extensions::ael
{

/**
 * @brief Base representation of AEL metadata
 *
 * Contains common metadata fields used across all AEL types.
 */
struct AelBase
{
    /** @brief AEL schema version */
    std::string version{"1.0"};

    /** @brief Vendor namespace (e.g., AMD) */
    std::string nameSpace{"AMD"};

    /** @brief Optional subtype classification */
    std::optional<std::string> subType;

    /** @brief Optional error classification */
    std::optional<std::string> errorClass;

    /** @brief Optional severity override */
    std::optional<std::string> severityOverride;

    /**
     * @brief Correlation identifier for grouping related events
     *
     * Used to link multiple log entries belonging to the same
     * platform-level incident or workflow.
     */
    std::optional<std::string> correlationId;

    virtual ~AelBase() = default;
};

/**
 * @brief SIMPLE AEL metadata (inline representation)
 *
 * Lightweight metadata where all information is embedded directly.
 */
struct AelSimple final : public AelBase
{
    /** @brief Application Fault Identifier (required) */
    std::string afid;

    /** @brief List of associated FRU inventory paths */
    std::vector<std::string> fruList;
};

/**
 * @brief COMPLEX AEL metadata (external reference)
 *
 * Used when large or structured data is stored externally
 * and referenced via a URI.
 */
struct AelComplex final : public AelBase
{
    /** @brief URI pointing to external diagnostic data */
    std::string dataUri;
};

} // namespace phosphor::logging::extensions::ael
