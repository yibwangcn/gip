#include "core/scenario/IScenario.hpp"

namespace IP::core::scenario
{
    std::pair<Pistache::Http::Code, std::string> IScenario::run()
    {
        auto precheck_res = precheck();
        if (precheck_res.first != Pistache::Http::Code::Ok)
        {
            return precheck_res;
        }

        auto process_res = process();
        if (process_res.first != Pistache::Http::Code::Ok)
        {
            return process_res;
        }

        return afterProcess();
    }

    void IScenario::stopProcess()
    {
        stop_signal_.store(true);
    }
} // namespace IP::core::scenario