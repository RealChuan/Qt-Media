#include "crashpad.hpp"
#include "breakpad.hpp"

#include <utils/utils.hpp>

#include <crashpad/client/crash_report_database.h>
#include <crashpad/client/crashpad_client.h>
#include <crashpad/client/settings.h>

#include <filesystem>
#include <iostream>

namespace Dump {

class Crashpad::CrashpadPrivate
{
public:
    explicit CrashpadPrivate(const std::string &dumpPath,
                             const std::string &libexecPath,
                             const std::string &reportUrl,
                             bool crashReportingEnabled,
                             Crashpad *q)
        : q_ptr(q)
        , dumpPath(dumpPath)
        , libexecPath(libexecPath)
        , reportUrl(reportUrl)
        , crashReportingEnabled(crashReportingEnabled)
    {
        if (!initialize()) {
            throw std::runtime_error("Failed to initialize Crashpad");
        }
    }
    ~CrashpadPrivate() = default;

    bool initialize()
    {
        try {
            std::filesystem::create_directories(dumpPath);
        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Failed to create dump directory: " << e.what() << std::endl;
            return false;
        }

        // 构建handler路径
        auto handlerPath = libexecPath + "/crashpad_handler";

#ifdef _WIN32
        handlerPath += ".exe";
        base::FilePath database(toWide(dumpPath));
        base::FilePath handler(toWide(handlerPath));
#else
        base::FilePath database(dumpPath);
        base::FilePath handler(handlerPath);
#endif

        databasePtr = crashpad::CrashReportDatabase::Initialize(database);
        if (!databasePtr) {
            std::cerr << "Failed to initialize CrashReportDatabase" << std::endl;
            return false;
        }

        if (auto *settings = databasePtr->GetSettings()) {
            settings->SetUploadsEnabled(crashReportingEnabled);
        }

#ifdef __linux__
        bool asynchronous_start = false;
#else
        bool asynchronous_start = true;
#endif

        // 启动Crashpad handler
        crashpadClientPtr = std::make_unique<crashpad::CrashpadClient>();
        bool success = crashpadClientPtr->StartHandler(handler,
                                                       database,
                                                       database,
                                                       reportUrl,
                                                       {},                  // annotations
                                                       {"--no-rate-limit"}, // arguments
                                                       true,                // restartable
                                                       asynchronous_start);

        if (!success) {
            std::cerr << "Failed to start Crashpad handler" << std::endl;
            return false;
        }

        std::cout << "Crashpad initialized successfully" << std::endl;
        std::cout << "Dump path: " << dumpPath << std::endl;
        std::cout << "Report URL: " << reportUrl << std::endl;
        std::cout << "Reporting enabled: " << (crashReportingEnabled ? "yes" : "no") << std::endl;

        return true;
    }

    Crashpad *q_ptr;

    std::string dumpPath;
    std::string libexecPath;
    std::string reportUrl;
    bool crashReportingEnabled;
    std::unique_ptr<crashpad::CrashpadClient> crashpadClientPtr;
    std::unique_ptr<crashpad::CrashReportDatabase> databasePtr;
};

Crashpad::Crashpad(const std::string &dumpPath,
                   const std::string &libexecPath,
                   const std::string &reportUrl,
                   bool crashReportingEnabled)
    : d_ptr(std::make_unique<CrashpadPrivate>(dumpPath,
                                              libexecPath,
                                              reportUrl,
                                              crashReportingEnabled,
                                              this))
{}

Crashpad::~Crashpad() = default;

std::string Crashpad::getDumpPath() const
{
    return d_ptr->dumpPath;
}

std::string Crashpad::getReportUrl() const
{
    return d_ptr->reportUrl;
}

bool Crashpad::isReportingEnabled() const
{
    return d_ptr->crashReportingEnabled;
}

} // namespace Dump
