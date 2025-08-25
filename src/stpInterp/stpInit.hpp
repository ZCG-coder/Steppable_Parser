#pragma once

#include "stpInterp/stpStore.hpp"

#include <memory>

namespace steppable::parser
{
    using STP_InterpState = std::shared_ptr<STP_InterpStoreLocal>;
    const extern STP_InterpState _storage;

    STP_InterpState STP_getState();

    int STP_destroy();
} // namespace steppable::parser
