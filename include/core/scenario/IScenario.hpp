#ifndef __I_SCENARIO_HPP__
#define __I_SCENARIO_HPP__

#include <atomic>
#include <string>
#include <utility>

#include <pistache/http.h>

namespace IP::core::scenario
{
    class IScenario
    {
    public:
        virtual ~IScenario() = default;

        std::pair<Pistache::Http::Code, std::string> run();
        void stopProcess();

    protected:
        virtual std::pair<Pistache::Http::Code, std::string> precheck() = 0;
        virtual std::pair<Pistache::Http::Code, std::string> process() = 0;
        virtual std::pair<Pistache::Http::Code, std::string> afterProcess() = 0;
        bool stopRequested() const { return stop_signal_.load(); }

    private:
        std::atomic<bool> stop_signal_{false};
    };

} // namespace IP::core::scenario

#endif /* __I_SCENARIO_HPP__ */