/**
 * @file validator.hpp
 * @brief AEL metadata validation
 */

#pragma once

#include "model.hpp"

namespace phosphor::logging::extensions::ael
{

/**
 * @brief Validates AEL metadata objects against schema rules
 */
class AelValidator
{
  public:
    /**
     * @brief Validate metadata object
     * @param obj AEL metadata object
     * @return true if valid
     */
    static bool validate(const AelBase& obj);
};

} // namespace phosphor::logging::extensions::ael
