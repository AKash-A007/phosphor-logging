#include "validator.hpp"

namespace phosphor::logging::extensions::ael
{

bool AelValidator::validate(const AelBase& obj)
{
    // SIMPLE → AFID required
    if (auto s = dynamic_cast<const AelSimple*>(&obj))
    {
        return !s->afid.empty();
    }

    // COMPLEX → URI required
    if (auto c = dynamic_cast<const AelComplex*>(&obj))
    {
        return !c->dataUri.empty();
    }

    return false;
}

} // namespace phosphor::logging::extensions::ael
