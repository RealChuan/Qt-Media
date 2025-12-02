#pragma once

#include "dump_global.hpp"

#include <functional>
#include <memory>
#include <string>

namespace Dump {

class BreakpadPrivate;
class DUMP_EXPORT Breakpad
{
    Q_DISABLE_COPY_MOVE(Breakpad)
public:
    using CrashCallback = std::function<bool(const std::string &dump_path, bool succeeded)>;

    explicit Breakpad(const std::string &dump_path);
    ~Breakpad();

    void setCrashCallback(CrashCallback callback);

    bool writeMinidump();

    std::string getDumpPath() const;

private:
    std::unique_ptr<BreakpadPrivate> d_ptr;
};

#ifdef _WIN32
std::wstring toWide(const std::string &str);
#endif

DUMP_EXPORT void openCrashReporter();

} // namespace Dump
