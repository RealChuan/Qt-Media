#pragma once

#include "dump_global.hpp"

#include <memory>
#include <string>

namespace Dump {

class DUMP_EXPORT Crashpad
{
    Q_DISABLE_COPY_MOVE(Crashpad)
public:
    explicit Crashpad(const std::string &dumpPath,
                      const std::string &libexecPath,
                      const std::string &reportUrl,
                      bool crashReportingEnabled);

    ~Crashpad();

    std::string getDumpPath() const;
    std::string getReportUrl() const;
    bool isReportingEnabled() const;

private:
    class CrashpadPrivate;
    std::unique_ptr<CrashpadPrivate> d_ptr;
};

} // namespace Dump
