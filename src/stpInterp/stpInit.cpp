#include "include/stpStore.hpp"

#include <memory>
#include <stdexcept>

namespace steppable::parser
{
    using InterpState = std::unique_ptr<STP_InterpStoreLocal>;

    namespace
    {
        std::shared_ptr<STP_InterpStoreLocal> _storage = nullptr;
    }

    int STP_init()
    {
        _storage = std::make_shared<STP_InterpStoreLocal>();
        return 0;
    }

    std::shared_ptr<STP_InterpStoreLocal> STP_getState() { return _storage; }

    int STP_destroy()
    {
        if (_storage == nullptr)
            throw std::runtime_error("Uninitialized state is destroyed.");

        _storage.reset();
        return 0;
    }
} // namespace steppable::parser
