#pragma once

#include "include/stpStore.hpp"

#include <memory>

namespace steppable::parser
{
    using STP_InterpState = std::shared_ptr<STP_InterpStoreLocal>;

    int STP_init();

    std::shared_ptr<STP_InterpStoreLocal> STP_getState();

    int STP_destroy();
} // namespace steppable::parser
