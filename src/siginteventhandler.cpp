#include "siginteventhandler.h"

#include <cstdlib>
#include <csignal>
#include <atomic>

RTTI_BEGIN_CLASS_NO_DEFAULT_CONSTRUCTOR(nap::SigIntEventHandler)
    RTTI_CONSTRUCTOR(nap::BaseApp&)
RTTI_END_CLASS

namespace nap
{
    static std::atomic<bool> sExit = { false };
    void sigterm_callback_handler(int signum)
    {
        sExit = true;
    }

    void sigkill_callback_handler(int signum)
    {
        exit(1);
    }

    SigIntEventHandler::SigIntEventHandler(BaseApp& app) : AppEventHandler(app)
    {
    }

    void SigIntEventHandler::start()
    {
        // Register signal and signal handler
        signal(SIGINT, sigterm_callback_handler);
        signal(SIGTERM, sigterm_callback_handler);
        signal(SIGKILL, sigkill_callback_handler);
    }


    void SigIntEventHandler::process()
    {
        if (sExit)
        {
            mApp.quit();
        }
    }
}