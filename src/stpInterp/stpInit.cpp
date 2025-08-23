#include "include/stpInit.hpp"

#include "include/stpStore.hpp"

#include <memory>
#include <stdexcept>

namespace steppable::parser
{
    namespace
    {
        STP_InterpState _storage = nullptr;
    }

    int STP_init()
    {
        _storage = std::make_shared<STP_InterpStoreLocal>();
        return 0;
    }

    STP_InterpState STP_getState() { return _storage; }

    int STP_destroy()
    {
        if (_storage == nullptr)
            throw std::runtime_error("Uninitialized state is destroyed.");

        _storage.reset();
        return 0;
    }
} // namespace steppable::parser
