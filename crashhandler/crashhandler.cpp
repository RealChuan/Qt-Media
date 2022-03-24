#include "crashhandler.h"
#include "utils/utils.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QUrl>

inline QString getDumpFileName()
{
    QString strTime = QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss-zzz");

    QString path = QString("%1/crashes/%2-%3-%4-%5.dmp")
                       .arg(Utils::getConfigPath())
                       .arg(qAppName())
                       .arg(qApp->applicationVersion())
                       .arg(strTime)
                       .arg(qApp->applicationPid());

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        file.close();
        return path;
    }
    qDebug() << "Path: " << path << "\n"
             << "Error: " << file.errorString();
    return QString();
}

#if defined(Q_OS_WIN32)

#include <DbgHelp.h>
#include <tchar.h>

#pragma comment(lib, "dbghelp.lib")

inline void CreateMiniDump(EXCEPTION_POINTERS *pep)
{
    QString path = getDumpFileName();
    if (path.isEmpty())
        return;

    HANDLE hFile = CreateFile(path.toStdWString().c_str(),
                              GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE)) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pep;
        mdei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(),
                          GetCurrentProcessId(),
                          hFile,
                          MiniDumpNormal,
                          &mdei,
                          NULL,
                          NULL);
        CloseHandle(hFile);
    } else {
        qDebug() << strerror(errno);
    }
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER)
{
    return NULL;
}

BOOL PreventSetUnhandledExceptionFilter()
{
    HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));

    if (hKernel32 == NULL)
        return FALSE;

    void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");

    if (pOrgEntry == NULL)
        return FALSE;

    unsigned char newJump[100];

    DWORD dwOrgEntryAddr = DWORD(pOrgEntry);

    dwOrgEntryAddr += 5; // add 5 for 5 op-codes for jmp far

    void *pNewFunc = &MyDummySetUnhandledExceptionFilter;

    DWORD dwNewEntryAddr = DWORD(pNewFunc);
    DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
    newJump[0] = 0xE9; // JMP absolute
    memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
    SIZE_T bytesWritten;
    BOOL bRet = WriteProcessMemory(GetCurrentProcess(),
                                   pOrgEntry,
                                   newJump,
                                   sizeof(pNewFunc) + 1,
                                   &bytesWritten);
    return bRet;
}

LONG WINAPI UnhandledExceptionFilterEx(LPEXCEPTION_POINTERS lpExceptionInfo)
{
    if (IsDebuggerPresent())
        return EXCEPTION_CONTINUE_SEARCH;

    CreateMiniDump(lpExceptionInfo);

    Utils::openCrashAndLogPath();

    return EXCEPTION_CONTINUE_SEARCH;
}

#endif

void Utils::setCrashHandler()
{
    const QString path = Utils::getConfigPath() + "/crashes";
    if (!Utils::generateDirectorys(path))
        return;

        //#ifdef QT_NO_DEBUG

#if defined(Q_OS_WIN32)
    SetUnhandledExceptionFilter(UnhandledExceptionFilterEx);
    PreventSetUnhandledExceptionFilter();
#endif

    //#endif
}

void Utils::openCrashAndLogPath()
{
    QDir dir;
    QString urlCrash = Utils::getConfigPath() + "/crashes";
    QString urlLog = Utils::getConfigPath() + "/log";
    if (dir.exists(urlCrash))
        QDesktopServices::openUrl(QUrl(urlCrash, QUrl::TolerantMode));
    if (dir.exists(urlLog))
        QDesktopServices::openUrl(QUrl(urlLog, QUrl::TolerantMode));
}
