#include "utils.h"
#include "json.h"
#include "hostosinfo.h"

#include <QtWidgets>

void Utils::setUTF8Code()
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
}

void Utils::setQSS()
{
    Json json("./config/config.json", true);
    QStringList qssPath = json.getValue("qss_files").toStringList();
    QString qss;
    for(const QString& path: qssPath){
        qDebug() << QString(QObject::tr("Loading QSS file: %1.")).arg(path);
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)){
            qDebug() << QString(QObject::tr("Cannot open the file: %1!")).arg(path)
                     << file.errorString();
            continue;
        }
        qss.append(QLatin1String(file.readAll())).append("\n");
        file.close();
    }
    if(!qss.isEmpty())
        qApp->setStyleSheet(qss);
}

void Utils::loadFonts()
{
    Json json("./config/config.json", true);
    QStringList fontFiles = json.getValue("font_files").toStringList();

    for (const QString &file : fontFiles) {
        qDebug() << QString(QObject::tr("Loading Font file: %1")).arg(file);
        QFontDatabase::addApplicationFont(file);
    }
}

void Utils::windowCenter(QWidget *window)
{
    QSize size = qApp->primaryScreen()->availableSize() - window->size();
    int x = qMax(0, size.width() / 2);
    int y = qMax(0, size.height() / 2);
    window->move(x, y);
}

void Utils::msleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);

    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

QString compilerString()
{
#if defined(__apple_build_version__) // Apple clang has other version numbers
    QString isAppleString = QLatin1String(" (Apple)");
    return QLatin1String("Clang " ) + QString::number(__clang_major__) + QLatin1Char('.')
           + QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
    return QLatin1String("GCC " ) + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
    return QString("MSVC Version: %1").arg(_MSC_VER);
#endif
    return QLatin1String("<unknown compiler>");
}

void Utils::printBuildInfo()
{
    QString info = QString("Qt %1 (%2, %3 bit)")
                       .arg(qVersion())
                       .arg(compilerString())
                       .arg(QSysInfo::WordSize);
    qDebug() << QObject::tr("Build with: ") << info;
}

void Utils::setHighDpiEnvironmentVariable()
{
    if (Utils::HostOsInfo().isMacHost())
        return;

#ifdef Q_OS_WIN
    if (!qEnvironmentVariableIsSet("QT_OPENGL"))
        QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif

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
    if(!QFileInfo::exists(ConfigFile)){
        translator.load("./translator/language.zh_en.qm");
        CURRENT_LANGUAGE = Utils::English;
    }else{
        QSettings setting(ConfigFile, QSettings::IniFormat);
        setting.beginGroup("Language_config");     //向当前组追加前缀
        Utils::Language type = Utils::Language(setting.value("Language").toInt());
        setting.endGroup();

        switch (type) {
        case Utils::Chinese: translator.load("./translator/language.zh_cn.qm");
            CURRENT_LANGUAGE = Utils::Chinese; break;
        case Utils::English: translator.load("./translator/language.zh_en.qm");
            CURRENT_LANGUAGE = Utils::English; break;
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
