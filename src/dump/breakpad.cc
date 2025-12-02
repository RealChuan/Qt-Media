#include "breakpad.hpp"

#include <utils/utils.hpp>

#ifdef _WIN32
#include <client/windows/handler/exception_handler.h>
#elif __APPLE__
#include <client/mac/handler/exception_handler.h>
#elif __linux__
#include <client/linux/handler/exception_handler.h>
#endif

#include <filesystem>
#include <iostream>
#include <stdexcept>

#ifdef Q_OS_WIN
#define CrashReportName "CrashReport.exe"
#else
#define CrashReportName "CrashReport"
#endif

namespace Dump {

class BreakpadPrivate
{
public:
    explicit BreakpadPrivate(const std::string &dump_path, Breakpad *q);
    ~BreakpadPrivate() = default;

    bool initialize();

    bool handleCrash(const std::string &dump_path, bool succeeded);

    Breakpad *q_ptr;

    std::string dumpPath;
    Breakpad::CrashCallback crashCallback;
    std::unique_ptr<google_breakpad::ExceptionHandler> handlerPtr;
};

namespace {

#ifdef _WIN32

std::string toUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return {};

    int size_needed = WideCharToMultiByte(CP_UTF8,
                                          0,
                                          wstr.c_str(),
                                          (int) wstr.size(),
                                          nullptr,
                                          0,
                                          nullptr,
                                          nullptr);
    if (size_needed == 0)
        return {};

    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8,
                        0,
                        wstr.c_str(),
                        (int) wstr.size(),
                        &str[0],
                        size_needed,
                        nullptr,
                        nullptr);
    return str;
}

bool windowsCallback(const wchar_t *dump_path,
                     const wchar_t *id,
                     void *context,
                     EXCEPTION_POINTERS *exinfo,
                     MDRawAssertionInfo *assertion,
                     bool succeeded)
{
    auto *private_impl = static_cast<BreakpadPrivate *>(context);
    std::string full_path = toUtf8(dump_path) + toUtf8(id) + ".dmp";
    return private_impl->handleCrash(full_path, succeeded);
}

#elif __APPLE__

bool macCallback(const char *dump_path, const char *id, void *context, bool succeeded)
{
    auto *private_impl = static_cast<BreakpadPrivate *>(context);
    std::string full_path = std::string(dump_path) + std::string(id) + ".dmp";
    return private_impl->handleCrash(full_path, succeeded);
}

#elif __linux__

bool linuxCallback(const google_breakpad::MinidumpDescriptor &descriptor,
                   void *context,
                   bool succeeded)
{
    auto *private_impl = static_cast<BreakpadPrivate *>(context);
    return private_impl->handleCrash(descriptor.path(), succeeded);
}

#endif

} // namespace

BreakpadPrivate::BreakpadPrivate(const std::string &dump_path, Breakpad *q)
    : q_ptr(q)
    , dumpPath(dump_path)
{
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize Breakpad");
    }
}

bool BreakpadPrivate::initialize()
{
    try {
        std::filesystem::create_directories(dumpPath);
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Failed to create dump directory: " << e.what() << std::endl;
        return false;
    }

#ifdef _WIN32
    handlerPtr = std::make_unique<google_breakpad::ExceptionHandler>(
        toWide(dumpPath),
        nullptr,
        &windowsCallback,
        this,
        google_breakpad::ExceptionHandler::HANDLER_ALL);
#elif __APPLE__
    handlerPtr = std::make_unique<google_breakpad::ExceptionHandler>(dumpPath,
                                                                     nullptr,
                                                                     &macCallback,
                                                                     this,
                                                                     true,
                                                                     nullptr);

#elif __linux__
    handlerPtr = std::make_unique<google_breakpad::ExceptionHandler>(
        google_breakpad::MinidumpDescriptor(dumpPath), nullptr, &linuxCallback, this, true, -1);
#endif

    if (!handlerPtr) {
        std::cerr << "Failed to create ExceptionHandler" << std::endl;
        return false;
    }

    return true;
}

bool BreakpadPrivate::handleCrash(const std::string &dump_path, bool succeeded)
{
    // 调用用户设置的回调函数
    if (crashCallback) {
        return crashCallback(dump_path, succeeded);
    }

    // 默认处理：输出日志信息
    std::cout << "Crash dump " << (succeeded ? "succeeded" : "failed") << ": " << dump_path
              << std::endl;
    return succeeded;
}

Breakpad::Breakpad(const std::string &dump_path)
    : d_ptr(std::make_unique<BreakpadPrivate>(dump_path, this))
{}

Breakpad::~Breakpad() = default;

void Breakpad::setCrashCallback(CrashCallback callback)
{
    d_ptr->crashCallback = std::move(callback);
}

bool Breakpad::writeMinidump()
{
    return d_ptr->handlerPtr && d_ptr->handlerPtr->WriteMinidump();
}

std::string Breakpad::getDumpPath() const
{
    return d_ptr->dumpPath;
}

#ifdef _WIN32
std::wstring toWide(const std::string &str)
{
    if (str.empty())
        return {};

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int) str.size(), nullptr, 0);
    if (size_needed == 0)
        return {};

    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int) str.size(), &wstr[0], size_needed);
    return wstr;
}
#endif

void openCrashReporter()
{
    const auto reporterPath = qApp->applicationDirPath() + "/" + CrashReportName;
    QStringList args{Utils::crashPath(), Utils::logPath(), qApp->applicationFilePath()};
    args.append(qApp->arguments());
    QProcess process;
    process.startDetached(reporterPath, args);
}

} // namespace Dump
