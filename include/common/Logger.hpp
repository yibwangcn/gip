#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace IP::common
{
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    inline std::string convertLogLevelToString(const LogLevel &level)
    {
        switch (level)
        {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
        }
    }

    inline std::string currentTimestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                                now.time_since_epoch()) %
                            1000;

        std::tm local_time{};
        localtime_r(&time, &local_time);

        std::ostringstream oss;
        oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << millis.count();
        return oss.str();
    }

    class Logger
    {
    public:
        explicit Logger(const LogLevel &level = LogLevel::INFO)
            : log_level_(level) {}

        ~Logger()
        {
            flush();
        }

        template <typename T>
        Logger &operator<<(const T &value)
        {
            stream_ << value;
            return *this;
        }

        static void setLogFile(const std::string &file_path)
        {
            std::lock_guard<std::mutex> lock(logMutex());
            logFileStream().close();
            logFileStream().open(file_path, std::ios::out | std::ios::app);
        }

    private:
        void flush()
        {
            const std::string line = currentTimestamp() + " [" +
                                     convertLogLevelToString(log_level_) + "] " + stream_.str();

            std::lock_guard<std::mutex> lock(logMutex());
            std::fprintf(stdout, "%s\n", line.c_str());
            std::fflush(stdout);

            if (logFileStream().is_open())
            {
                logFileStream() << line << '\n';
                logFileStream().flush();
            }
        }

        static std::mutex &logMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        static std::ofstream &logFileStream()
        {
            static std::ofstream stream;
            return stream;
        }

        LogLevel log_level_;
        std::ostringstream stream_;
    };
} // namespace IP::common

#define TRC_DEBUG IP::common::Logger(IP::common::LogLevel::DEBUG)
#define TRC_INFO IP::common::Logger(IP::common::LogLevel::INFO)
#define TRC_WARNING IP::common::Logger(IP::common::LogLevel::WARNING)
#define TRC_ERROR IP::common::Logger(IP::common::LogLevel::ERROR)
#define TRC_CRITICAL IP::common::Logger(IP::common::LogLevel::CRITICAL)

#endif /* __LOGGER_HPP__ */
