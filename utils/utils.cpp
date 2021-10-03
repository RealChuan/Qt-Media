#include "utils.h"
#include "hostosinfo.h"
#include "json.h"

#include <QTextCodec>
#include <QtWidgets>

void Utils::setUTF8Code()
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
}

void Utils::setQSS()
{
    Json json("./config/config.json", true);
    const QStringList qssPath = json.getValue("qss_files").toStringList();
    QString qss;
    for (const QString &path : qAsConst(qssPath)) {
        qDebug() << QObject::tr("Loading QSS file: %1.").arg(path);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << QObject::tr("Cannot open the file: %1!").arg(path) << file.errorString();
            continue;
        }
        qss.append(QLatin1String(file.readAll())).append("\n");
        file.close();
    }
    if (qss.isEmpty())
        return;
    qApp->setStyleSheet(qss);
}

void Utils::loadFonts()
{
    Json json("./config/config.json", true);
    const QStringList fontFiles = json.getValue("font_files").toStringList();

    for (const QString &file : qAsConst(fontFiles)) {
        qDebug() << QObject::tr("Loading Font file: %1").arg(file);
        QFontDatabase::addApplicationFont(file);
    }
}

void Utils::windowCenter(QWidget *window)
{
    const QRect rect = qApp->primaryScreen()->availableGeometry();
    int x = (rect.width() - window->width()) / 2 + rect.x();
    int y = (rect.height() - window->height()) / 2 + rect.y();
    x = qMax(0, x);
    y = qMax(0, y);
    window->move(x, y);
}

void Utils::msleep(int msec)
{
    if (msec <= 0) {
        qWarning() << QObject::tr("msec not <= 0!");
        return;
    }
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit, Qt::UniqueConnection);
    timer.start(msec);
    loop.exec();
}

QString compilerString()
{
#if defined(__apple_build_version__) // Apple clang has other version numbers
    QString isAppleString = QLatin1String(" (Apple)");
    return QLatin1String("Clang ") + QString::number(__clang_major__) + QLatin1Char('.')
           + QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
    return QLatin1String("GCC ") + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
    return QString("MSVC Version: %1").arg(_MSC_VER);
#endif
    return QLatin1String("<unknown compiler>");
}

void Utils::printBuildInfo()
{
    const QString info = QString("Qt %1 (%2, %3 bit)")
                             .arg(qVersion(), compilerString(), QString::number(QSysInfo::WordSize));
    qDebug() << QObject::tr("Build with: ") << info;
}

void Utils::setHighDpiEnvironmentVariable()
{
    if (Utils::HostOsInfo().isMacHost())
        return;

    //#ifdef Q_OS_WIN
    //if (!qEnvironmentVariableIsSet("QT_OPENGL"))
    //    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    //#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    // work around QTBUG-80934
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
}

void Utils::reboot()
{
    QString program = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    QString workingDirectory = QDir::currentPath();
    QProcess::startDetached(program, arguments, workingDirectory);
    QApplication::exit();
}

void Utils::saveLanguage(Utils::Language type)
{
    QSettings setting(ConfigFile, QSettings::IniFormat);
    setting.beginGroup("Language_config");
    setting.setValue("Language", type);
    setting.endGroup();
}

static Utils::Language CURRENT_LANGUAGE = Utils::Chinese;

void Utils::loadLanguage()
{
    static QTranslator translator;
    if (!QFileInfo::exists(ConfigFile)) {
        qInfo() << translator.load("./translator/language.zh_en.qm");
        CURRENT_LANGUAGE = Utils::English;
    } else {
        QSettings setting(ConfigFile, QSettings::IniFormat);
        setting.beginGroup("Language_config"); //向当前组追加前缀
        Utils::Language type = Utils::Language(setting.value("Language").toInt());
        setting.endGroup();

        switch (type) {
        case Utils::Chinese:
            qInfo() << translator.load("./translator/language.zh_cn.qm");
            CURRENT_LANGUAGE = Utils::Chinese;
            break;
        case Utils::English:
            qInfo() << translator.load("./translator/language.zh_en.qm");
            CURRENT_LANGUAGE = Utils::English;
            break;
        }
    }
    qApp->installTranslator(&translator);
}

Utils::Language Utils::getCurrentLanguage()
{
    return CURRENT_LANGUAGE;
}

bool Utils::createPath(const QString &path)
{
    QDir dir;
    if (dir.exists(path))
        return true;
    return dir.mkpath(path);
}
