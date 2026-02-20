#include "helper.h"

#include <QCoreApplication>
#include <QtDBus>
#include <QFile>
#ifdef USE_PRIVILEGED_HELPER
#include <PolkitQt1/Authority>
#include <PolkitQt1/Subject>
#else
#warning "This build does not use a privileged helper application!"
#endif

#include <signal.h>

#include <sys/ioctl.h>
#include <sys/statfs.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>

#include <QLoggingCategory>
#include <syslog.h>

static QtMessageHandler oldHandler = nullptr;
static QFile *logFile = nullptr;

void theMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString lMsg;
    switch (type) {
        case QtDebugMsg:
            lMsg = "Debug: " + msg;
            break;
        case QtInfoMsg:
            lMsg = "Info: " + msg;
            break;
        case QtWarningMsg:
            lMsg = "Warning: " + msg;
            break;
        case QtCriticalMsg:
            lMsg = "Critical: " + msg;
            break;
        default:
            if (oldHandler) {
                oldHandler(type, context, msg);
            }
            return;
            break;
    }
    if (type != QtInfoMsg) {
        lMsg += " (";
        lMsg += (context.file && strlen(context.file))? context.file : __FILE__;
        if (context.line) {
            lMsg += QString::asprintf(":%d", context.line);
        }
        if (context.function && strlen(context.function)) {
            lMsg += QString::asprintf(" \"%s\"", context.function);
        }
        lMsg += ")";
    }
    errno = 0;
    if (logFile->write(qPrintable(lMsg + "\n")) < 0 || !logFile->flush()) {
        char *smsg = strdup(qPrintable(lMsg));
        if (smsg && strlen(smsg)) {
            syslog(LOG_ERR, "%s failed to write message \"%s\" (errno=%d)",
                   __PRETTY_FUNCTION__, smsg, errno);
            free(smsg);
        }
    }
}

HelperAdaptor::HelperAdaptor(Helper *parent) :
    QDBusAbstractAdaptor(parent)
{
    m_parentHelper = parent;
}

QVariantMap HelperAdaptor::initSession(const QString &PATH)
{
    return m_parentHelper->initSession(PATH);
}

QVariantMap HelperAdaptor::endSession()
{
    return m_parentHelper->endSession();
}

QVariantMap HelperAdaptor::prepareBenchmarkFile(const QString &benchmarkFile, int fileSize, bool fillZeros)
{
    return m_parentHelper->prepareBenchmarkFile(benchmarkFile, fileSize, fillZeros);
}

QVariantMap HelperAdaptor::startBenchmarkTest(int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros, bool cacheBypass, bool continuousGeneration,
                                              int blockSize, int queueDepth, int threads, const QString &rw, const QString &engine)
{
    return m_parentHelper->startBenchmarkTest(
      measuringTime, fileSize, randomReadPercentage, fillZeros, cacheBypass,
      continuousGeneration, blockSize, queueDepth, threads, rw, engine);
}

QVariantMap HelperAdaptor::flushPageCache()
{
    return m_parentHelper->flushPageCache();
}

QVariantMap HelperAdaptor::removeBenchmarkFile()
{
    return m_parentHelper->removeBenchmarkFile();
}

QVariantMap HelperAdaptor::stopCurrentTask()
{
    return m_parentHelper->stopCurrentTask();
}

QVariantMap HelperAdaptor::checkCowStatus(const QString &path)
{
    return m_parentHelper->checkCowStatus(path);
}

QVariantMap HelperAdaptor::createNoCowDirectory(const QString &path)
{
    return m_parentHelper->createNoCowDirectory(path);
}

Helper::Helper() : m_helperAdaptor(new HelperAdaptor(this))
{
    if (!QDBusConnection::sessionBus().isConnected() || !QDBusConnection::sessionBus().registerService(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface")) ||
        !QDBusConnection::sessionBus().registerObject(QStringLiteral("/Helper"), this)) {
        qWarning() << QDBusConnection::sessionBus().lastError().message();
        qApp->quit();
    }

    m_serviceWatcher = new QDBusServiceWatcher(this);
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, qApp, [this](const QString &service) {
        m_serviceWatcher->removeWatchedService(service);
        if (m_serviceWatcher->watchedServices().isEmpty()) {
            qApp->quit();
        }
    });

    QObject::connect(this, &Helper::taskFinished, m_helperAdaptor, &HelperAdaptor::taskFinished);
    auto configLocation = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/kdiskmark/";
    if (!configLocation.isEmpty()) {
        logFile = new QFile(configLocation + "kdiskmark_helper.log");
        if (logFile->open(QFile::Append)) {
            oldHandler = qInstallMessageHandler(theMessageOutput);
            QLoggingCategory::defaultCategory()->setEnabled(QtInfoMsg, true);
            QLoggingCategory::defaultCategory()->setEnabled(QtWarningMsg, true);
        } else {
            qWarning() << "Can't open logfile" << logFile->fileName();
        }
    } else {
        qWarning() << "Can't create logging facility";
    }
}

QVariantMap Helper::initSession(const QString &PATH)
{
    if (!calledFromDBus()) {
        qWarning() << Q_FUNC_INFO << "not called via DBus!";
        return {};
    }

    if (m_serviceWatcher->watchedServices().contains(message().service())) {
        return {{"success", true}};
    }

    if (!m_serviceWatcher->watchedServices().isEmpty()) {
        return {{"success", false}, {"error", "There are already registered DBus connection."}};
    }

    if (!PATH.isEmpty()) {
        // KDiskMark handed us its PATH, set it so that we can find the same `fio`.
	   // NB: we're started via DBus, so there's no guarantee we get the same PATH!!
        qputenv("PATH", PATH.toUtf8());
    }

#ifdef USE_PRIVILEGED_HELPER
    PolkitQt1::SystemBusNameSubject subject(message().service());
    PolkitQt1::Authority *authority = PolkitQt1::Authority::instance();

    PolkitQt1::Authority::Result result = PolkitQt1::Authority::No;
    QEventLoop e;
    connect(authority, &PolkitQt1::Authority::checkAuthorizationFinished, &e, [&e, &result](PolkitQt1::Authority::Result _result) {
        result = _result;
        e.quit();
    });

    authority->checkAuthorization(QStringLiteral("dev.jonmagon.kdiskmark.helper.init"), subject, PolkitQt1::Authority::AllowUserInteraction);
    e.exec();

    if (authority->hasError()) {
        qCritical() << "Encountered error while checking authorization, error code: " << authority->lastError() << authority->errorDetails();
        authority->clearError();
    }

    switch (result) {
    case PolkitQt1::Authority::Yes:
        // track who called into us so we can close when all callers have gone away
        m_serviceWatcher->addWatchedService(message().service());
        return {{"success", true}};
    default:
        sendErrorReply(QDBusError::AccessDenied);
        if (m_serviceWatcher->watchedServices().isEmpty())
            qApp->quit();
        return {};
    }
#else
        // track who called into us so we can close when all callers have gone away
        m_serviceWatcher->addWatchedService(message().service());
        return {{"success", true}};
#endif
}

QVariantMap Helper::endSession()
{
    if (!isCallerAuthorized()) {
        return {};
    }

    qApp->exit();

    return {};
}

bool Helper::testFilePath(const QString &benchmarkPath)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (QFileInfo(benchmarkPath).isSymbolicLink()) {
#else
    // detects *.lnk on Windows, but there's not Windows version, whatever
    if (QFileInfo(benchmarkPath).isSymLink()) {
#endif
        qWarning("The path should not be symbolic link.");
        return false;
    }

    if (benchmarkPath.startsWith("/dev")) {
        qWarning("Cannot specify a raw device.");
        return false;
    }

    return true;
}

QVariantMap Helper::prepareBenchmarkFile(const QString &benchmarkPath, int fileSize, bool fillZeros)
{
    if (!isCallerAuthorized()) {
        return {};
    }

    // If benchmarking has been done, but removeBenchmarkFile has not been called,
    // and benchmarking on a new file is called, then reject the request. The *previous* file must be removed first.
    if (!m_benchmarkFile.fileName().isNull()) {
        return {{"success", false}, {"error", "A new benchmark session should be started."}};
    }

    if (!testFilePath(benchmarkPath)) {
        return {{"success", false}, {"error", "The path to the file is incorrect."}};
    }

    m_benchmarkFile.setFileTemplate(QStringLiteral("%1/%2").arg(benchmarkPath).arg("kdiskmark-XXXXXX.tmp"));

    if (!m_benchmarkFile.open()) {
        return {{"success", false}, {"error", QStringLiteral("An error occurred while creating the benchmark file: %1").arg(m_benchmarkFile.errorString())}};
    }
    m_benchmarkFile.close();

    m_process = new QProcess();
    m_process->start("fio", QStringList()
                     << QStringLiteral("--output-format=json")
                     << QStringLiteral("--create_only=1")
                     << QStringLiteral("--filename=%1").arg(m_benchmarkFile.fileName())
                     << QStringLiteral("--size=%1m").arg(fileSize)
                     << QStringLiteral("--zero_buffers=%1").arg(fillZeros)
                     << QStringLiteral("--name=prepare"));

    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=] (int exitCode, QProcess::ExitStatus exitStatus) {
        const auto allStdout = QString(m_process->readAllStandardOutput());
        const auto allStderr = QString(m_process->readAllStandardError());
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qWarning() << "prepareBenchmarkFile() failed, stderr=" << allStderr << ", stdout=" << allStdout;
        }
        emit taskFinished(exitStatus == QProcess::NormalExit, allStdout, allStderr);
    });

    if (m_process->waitForStarted()) {
        qInfo() << Q_FUNC_INFO << m_process->program() + " " + m_process->arguments().join(" ");
        return {{"success", true}};
    } else {
        qCritical() << Q_FUNC_INFO << "failed to start:" << m_process->program() + " " + m_process->arguments().join(" ")
            << "PATH=" << qgetenv("PATH");
        return {{"success", false}};
    }
}

QVariantMap Helper::startBenchmarkTest(int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros, bool cacheBypass, bool continuousGeneration,
                                       int blockSize, int queueDepth, int threads, const QString &rw, const QString &engine)
{
    if (!isCallerAuthorized()) {
        return {};
    }

    if (m_benchmarkFile.fileName().isNull() || !QFile(m_benchmarkFile.fileName()).exists()) {
        return {{"success", false}, {"error", "The benchmark file was not pre-created."}};
    }

    QStringList arguments = {QStringLiteral("--output-format=json")};
    if (engine != "fio-default") {
        arguments << QStringLiteral("--ioengine=") + engine;
    }

    m_process = new QProcess();
    m_process->start("fio", arguments
                     << QStringLiteral("--randrepeat=0")
                     << QStringLiteral("--refill_buffers=%1").arg(continuousGeneration)
                     << QStringLiteral("--end_fsync=1")
                     << QStringLiteral("--direct=%1").arg(cacheBypass)
                     << QStringLiteral("--rwmixread=%1").arg(randomReadPercentage)
                     << QStringLiteral("--filename=%1").arg(m_benchmarkFile.fileName())
                     << QStringLiteral("--name=%1").arg(rw)
                     << QStringLiteral("--size=%1m").arg(fileSize)
                     << QStringLiteral("--zero_buffers=%1").arg(fillZeros)
                     << QStringLiteral("--bs=%1k").arg(blockSize)
                     << QStringLiteral("--runtime=%1").arg(measuringTime)
                     << QStringLiteral("--rw=%1").arg(rw)
                     << QStringLiteral("--iodepth=%1").arg(queueDepth)
                     << QStringLiteral("--numjobs=%1").arg(threads));

    connect(m_process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=] (int exitCode, QProcess::ExitStatus exitStatus) {
        const auto allStdout = QString(m_process->readAllStandardOutput());
        const auto allStderr = QString(m_process->readAllStandardError());
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            qInfo() << "startBenchmarkTest:" << m_process->program() + " " + m_process->arguments().join(" ");

            qWarning() << "\tfailed, stderr=" << allStderr << ", stdout=" << allStdout;
        }
        emit taskFinished(exitStatus == QProcess::NormalExit, allStdout, allStderr);
    });

    if (m_process->waitForStarted()) {
        return {{"success", true}};
    } else {
        return {{"success", false}};
    }
}

QVariantMap Helper::flushPageCache()
{
    if (!isCallerAuthorized()) {
        return {};
    }

    if (m_benchmarkFile.fileName().isNull() || !QFile(m_benchmarkFile.fileName()).exists()) {
        return {{"success", false}, {"error", "A benchmark file must first be created."}};
    }

    QFile file("/proc/sys/vm/drop_caches");

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write("1");
        file.close();
    }
    else {
        return {{"success", false}, {"error", file.errorString()}};
    }

    return {{"success", true}};
}

QVariantMap Helper::removeBenchmarkFile()
{
    if (!isCallerAuthorized()) {
        return {};
    }

    if (m_benchmarkFile.fileName().isNull() || !QFile(m_benchmarkFile.fileName()).exists()) {
        return {{"success", false}, {"error", "Cannot remove the benchmark file, because it doesn't exist."}};
    }

    m_benchmarkFile.close();

    return {{"success", m_benchmarkFile.remove()}};
}

QVariantMap Helper::stopCurrentTask()
{
    if (!isCallerAuthorized()) {
        return {};
    }

    if (!m_process) {
        return {{"success", false}, {"error", "The pointer to the process is empty."}};
    }

    if (m_process->state() == QProcess::Running || m_process->state() == QProcess::Starting) {
        m_process->terminate();
        m_process->waitForFinished(-1);
    }

    return {{"success", true}};
}

QVariantMap Helper::checkCowStatus(const QString &path)
{
    if (!isCallerAuthorized()) {
        return {};
    }

    if (!testFilePath(path)) {
        return {{"success", false}, {"error", "The path is incorrect."}};
    }

    struct statfs fs_info;
    if (statfs(path.toUtf8().constData(), &fs_info) != 0) {
        return {{"success", false}, {"error", QStringLiteral("Cannot get filesystem info: %1").arg(strerror(errno))}};
    }

    switch (fs_info.f_type) {
        case 0x9123683E: // BTRFS_SUPER_MAGIC
        case 0xca451a4e: // BCACHEFS_SUPER_MAGIC
        // APFS?
            break;
        default:
            return {{"success", true}, {"hasCow", false}};
    }

    int fd = open(path.toUtf8().constData(), O_RDONLY);
    if (fd < 0) {
        return {{"success", false}, {"error", QStringLiteral("Cannot open directory: %1").arg(strerror(errno))}};
    }

    unsigned long flags = 0;
    bool hasCow = false;
    if (ioctl(fd, FS_IOC_GETFLAGS, &flags) >= 0) {
        hasCow = !(flags & FS_NOCOW_FL);
    } else
    {
        qWarning() << QStringLiteral("Failed to get FS flags for") << path << QStringLiteral(":") << strerror(errno);
    }

    close(fd);
    return {{"success", true}, {"hasCow", hasCow}};
}

QVariantMap Helper::createNoCowDirectory(const QString &path)
{
    if (!isCallerAuthorized()) {
        return {};
    }

    if (!testFilePath(path)) {
        return {{"success", false}, {"error", "The path is incorrect."}};
    }

    QDir dir(path);
    QString newDir = dir.absoluteFilePath(QStringLiteral("kdiskmark_no_cow"));

    if (!QDir().mkpath(newDir)) {
        return {{"success", false}, {"error", QStringLiteral("Failed to create directory: %1").arg(newDir)}};
    }

    int fd = open(newDir.toUtf8().constData(), O_RDONLY);
    if (fd < 0) {
        return {{"success", false}, {"error", QStringLiteral("Cannot open new directory: %1").arg(strerror(errno))}};
    }

    unsigned long flags = 0;
    if (ioctl(fd, FS_IOC_GETFLAGS, &flags) < 0) {
        close(fd);
        return {{"success", false}, {"error", QStringLiteral("Cannot get flags: %1").arg(strerror(errno))}};
    }

    flags |= FS_NOCOW_FL;
    bool success = (ioctl(fd, FS_IOC_SETFLAGS, &flags) >= 0);
    close(fd);

    if (!success) {
        return {{"success", false}, {"error", QStringLiteral("Cannot set NoCow flag: %1").arg(strerror(errno))}};
    }

    return {{"success", true}, {"path", newDir}};
}

bool Helper::isCallerAuthorized()
{
    if (!calledFromDBus()) {
        return false;
    }

    return m_serviceWatcher->watchedServices().contains(message().service());
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Helper helper;
    a.exec();
}
