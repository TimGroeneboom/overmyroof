#pragma once

#include <appeventhandler.h>

namespace nap
{
    class NAPAPI SigIntEventHandler : public AppEventHandler
    {
    RTTI_ENABLE(AppEventHandler)
    public:
        SigIntEventHandler(BaseApp& app);

        void start() override;

        void process() override;
    private:

    };
}