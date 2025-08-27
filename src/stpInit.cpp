#include "stpInterp/stpInit.hpp"

#include "stpInterp/stpStore.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

namespace steppable::parser
{
    const auto _storage = std::make_shared<STP_InterpStoreLocal>(STP_InterpStoreLocal()); // NOLINT(*-err58-cpp)

    STP_InterpState STP_getState() { return _storage; }

    int STP_destroy()
    {
        if (_storage == nullptr)
            throw std::runtime_error("Uninitialized state is destroyed.");
        return 0;
    }
} // namespace steppable::parser
