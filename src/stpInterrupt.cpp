#include <functional>
#ifndef WINDOWS
    #include <csignal>
#endif

namespace steppable::parser
{
    void STP_addCtrlCHandler(const std::function<void(void)>& predicate)
    {
#ifdef WINDOWS
    #include <windows.h>
        static std::function<void(void)> handler;
        handler = predicate;
        static BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
        {
            if (fdwCtrlType == CTRL_C_EVENT)
            {
                if (handler)
                    handler();
                return TRUE;
            }
            return FALSE;
        }
        SetConsoleCtrlHandler(CtrlHandler, TRUE);
#else
        static std::function<void(void)> handler;
        handler = predicate;
        (void)std::signal(SIGINT, [](int) {
            if (handler)
                handler();
        });
#endif
    }
} // namespace steppable::parser