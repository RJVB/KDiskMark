#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QStorageInfo>
#include <QSettings>

#ifdef SINGLE_INSTANCE_ONLY
#include "singleapplication.h"
using ApplicationClass = SingleApplication;
#else
using ApplicationClass = QAPPLICATION_CLASS;
#endif

#include "cmake.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));
    QCoreApplication::setOrganizationName(QStringLiteral(PROJECT_NAME));
#if defined(__APPLE__)
    // RJVB's prefs for cross-platform operability:
    QSettings::setDefaultFormat(QSettings::IniFormat);
//     QCoreApplication::setOrganizationDomain(QStringLiteral("jonmagon.dev"));
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    ApplicationClass a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Disk benchmark application");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("<benchmark directory>", "the directory to test in");

    parser.process(a);
    const QStringList args = parser.positionalArguments();

    AppSettings().setupLocalization();

    MainWindow w;
    w.setFixedSize(w.size());
    if (!args.isEmpty() && !args.at(0).isEmpty()) {
        w.setTargetDirectory(args.at(0));
    }
    w.show();

    return a.exec();
}
