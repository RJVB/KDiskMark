#include "appsettings.h"

#include "cmake.h"

#include <QCoreApplication>
#include <QTranslator>
#include <QStandardPaths>
#include <QLibraryInfo>
#include <QSettings>
#include <QMetaEnum>

#include <QDebug>

#ifdef __APPLE__
// for sysctl
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

QTranslator AppSettings::s_appTranslator;
QTranslator AppSettings::s_qtTranslator;

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings(this))
{
}

void AppSettings::setupLocalization()
{
    applyLocale(locale());
}

QLocale AppSettings::locale() const
{
    return m_settings->value(QStringLiteral("Locale"), defaultLocale()).value<QLocale>();
}

void AppSettings::setLocale(const QLocale &locale)
{
    if (locale != this->locale()) {
        m_settings->setValue(QStringLiteral("Locale"), locale);
        applyLocale(locale);
    }
}

void AppSettings::applyLocale(const QLocale &locale)
{
    const QLocale newLocale = locale == defaultLocale() ? QLocale::system() : locale;
    QLocale::setDefault(newLocale);

    QCoreApplication::removeTranslator(&s_appTranslator);
    QCoreApplication::removeTranslator(&s_qtTranslator);

    if (newLocale.language() != QLocale::English) {
#ifdef __APPLE__
        const auto translationsLocation = QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/translations/");
#else
        const auto translationsLocation = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("translations"), QStandardPaths::LocateDirectory);
#endif
        bool appTranslatorLoaded = s_appTranslator.load(newLocale, QStringLiteral(PROJECT_NAME), QStringLiteral("_"), translationsLocation);
        if (appTranslatorLoaded) {
            QCoreApplication::installTranslator(&s_appTranslator);
        } else {
            qWarning() << "Failed to load application translations for locale" << newLocale << "from" << translationsLocation;
            qWarning() << "\t" << QStandardPaths::locateAll(QStandardPaths::AppDataLocation, QStringLiteral("translations"), QStandardPaths::LocateDirectory);;
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto translationsLocation = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    bool qtTranslatorLoaded = s_qtTranslator.load(newLocale, QStringLiteral("qt"), QStringLiteral("_"), translationsLocation);
#else
    const auto translationsLocation = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    bool qtTranslatorLoaded = s_qtTranslator.load(newLocale, QStringLiteral("qt"), QStringLiteral("_"), translationsLocation);
#endif

    if (qtTranslatorLoaded) {
        QCoreApplication::installTranslator(&s_qtTranslator);
    } else {
        qDebug() << "Failed to load Qt translations for locale" << newLocale << "from" << translationsLocation;
    }
}

QLocale AppSettings::defaultLocale()
{
    return QLocale::c();
}

Global::PerformanceProfile AppSettings::getPerformanceProfile() const
{
    return (Global::PerformanceProfile)m_settings->value(QStringLiteral("Benchmark/PerformanceProfile"), defaultPerformanceProfile()).toInt();
}

void AppSettings::setPerformanceProfile(Global::PerformanceProfile performanceProfile)
{
    m_settings->setValue(QStringLiteral("Benchmark/PerformanceProfile"), performanceProfile);
}

Global::PerformanceProfile AppSettings::defaultPerformanceProfile()
{
    return Global::PerformanceProfile::Default;
}

bool AppSettings::getMixedState() const
{
    return m_settings->value(QStringLiteral("Benchmark/Mixed"), defaultMixedState()).toBool();
}

void AppSettings::setMixedState(bool state)
{
    m_settings->setValue(QStringLiteral("Benchmark/Mixed"), state);
}

bool AppSettings::defaultMixedState()
{
    return false;
}

Global::BenchmarkMode AppSettings::getBenchmarkMode() const
{
    return (Global::BenchmarkMode)m_settings->value(QStringLiteral("Benchmark/Mode"), defaultBenchmarkMode()).toInt();
}

void AppSettings::setBenchmarkMode(Global::BenchmarkMode benchmarkMode)
{
    m_settings->setValue(QStringLiteral("Benchmark/Mode"), benchmarkMode);
}

Global::BenchmarkMode AppSettings::defaultBenchmarkMode()
{
    return Global::BenchmarkMode::ReadWriteMix;
}

Global::BenchmarkTestData AppSettings::getBenchmarkTestData() const
{
    return (Global::BenchmarkTestData)m_settings->value(QStringLiteral("Benchmark/TestData"), defaultBenchmarkTestData()).toInt();
}

void AppSettings::setBenchmarkTestData(Global::BenchmarkTestData benchmarkTestData)
{
    m_settings->setValue(QStringLiteral("Benchmark/TestData"), benchmarkTestData);
}

Global::BenchmarkTestData AppSettings::defaultBenchmarkTestData()
{
    return Global::BenchmarkTestData::Random;
}

int AppSettings::getLoopsCount() const
{
    return m_settings->value(QStringLiteral("Benchmark/LoopsCount"), defaultLoopsCount()).toInt();
}

void AppSettings::setLoopsCount(int loopsCount)
{
    m_settings->setValue(QStringLiteral("Benchmark/LoopsCount"), loopsCount);
}

int AppSettings::defaultLoopsCount()
{
    return 5;
}

int AppSettings::getFileSize() const
{
    return m_settings->value(QStringLiteral("Benchmark/FileSize"), defaultFileSize()).toInt();
}

void AppSettings::setFileSize(int fileSize)
{
    m_settings->setValue(QStringLiteral("Benchmark/FileSize"), fileSize);
}

int AppSettings::defaultFileSize()
{
    return 1024;
}

int AppSettings::getMeasuringTime() const
{
    return m_settings->value(QStringLiteral("Benchmark/MeasuringTime"), defaultMeasuringTime()).toInt();
}

void AppSettings::setMeasuringTime(int measuringTime)
{
    m_settings->setValue(QStringLiteral("Benchmark/MeasuringTime"), measuringTime);
}

int AppSettings::defaultMeasuringTime()
{
    return 5;
}

int AppSettings::getIntervalTime() const
{
    return m_settings->value(QStringLiteral("Benchmark/IntervalTime"), defaultIntervalTime()).toInt();
}

void AppSettings::setIntervalTime(int intervalTime)
{
    m_settings->setValue(QStringLiteral("Benchmark/IntervalTime"), intervalTime);
}

int AppSettings::defaultIntervalTime()
{
    return 5;
}

int AppSettings::getRandomReadPercentage() const
{
    return m_settings->value(QStringLiteral("Benchmark/RandomReadPercentage"), defaultRandomReadPercentage()).toInt();
}

void AppSettings::setRandomReadPercentage(int randomReadPercentage)
{
    m_settings->setValue(QStringLiteral("Benchmark/RandomReadPercentage"), randomReadPercentage);
}

int AppSettings::defaultRandomReadPercentage()
{
    return 70;
}

QString AppSettings::getIOEngineName() const
{
    return m_settings->value(QStringLiteral("Benchmark/IOEngine"), QStringLiteral("")).toString();
}

void AppSettings::setIOEngineName(const QString &engineName)
{
    m_settings->setValue(QStringLiteral("Benchmark/IOEngine"), engineName);
}

bool AppSettings::getCacheBypassState() const
{
    return m_settings->value(QStringLiteral("Benchmark/BypassCache"), defaultCacheBypassState()).toBool();
}

void AppSettings::setCacheBypassState(bool state)
{
    m_settings->setValue(QStringLiteral("Benchmark/BypassCache"), state);
}

bool AppSettings::defaultCacheBypassState()
{
    return true;
}
bool AppSettings::getContinuousGenerationState() const {
    return m_settings->value(QStringLiteral("Benchmark/ContinuousGeneration"), defaultContinuousGenerationState()).toBool();
}

void AppSettings::setContinuousGenerationState(bool continuousGenerationState) {
    m_settings->setValue(QStringLiteral("Benchmark/ContinuousGeneration"), continuousGenerationState);
}

bool AppSettings::defaultContinuousGenerationState() {
    return false;
}

bool AppSettings::getFlusingCacheState() const
{
    return m_settings->value(QStringLiteral("Benchmark/FlushingCache"), defaultFlushingCacheState()).toBool();
}

void AppSettings::setFlushingCacheState(bool state)
{
    m_settings->setValue(QStringLiteral("Benchmark/FlushingCache"), state);
}

bool AppSettings::defaultFlushingCacheState()
{
    return true;
}

bool AppSettings::getCoWDetectionState() const
{
    return m_settings->value(QStringLiteral("Benchmark/CoWDetection"), defaultCoWDetectionState()).toBool();
}

void AppSettings::setCoWDetectionState(bool state)
{
    m_settings->setValue(QStringLiteral("Benchmark/CoWDetection"), state);
}

bool AppSettings::defaultCoWDetectionState()
{
    return true;
}

Global::ComparisonUnit AppSettings::getComparisonUnit() const
{
    return (Global::ComparisonUnit)m_settings->value(QStringLiteral("Interface/ComparisonUnit"), defaultComparisonUnit()).toInt();
}

void AppSettings::setComparisonUnit(Global::ComparisonUnit comparisonUnit)
{
    m_settings->setValue(QStringLiteral("Interface/ComparisonUnit"), comparisonUnit);
}

Global::ComparisonUnit AppSettings::defaultComparisonUnit()
{
    return Global::ComparisonUnit::MBPerSec;
}

Global::Theme AppSettings::getTheme() const
{
    return (Global::Theme)m_settings->value(QStringLiteral("Interface/Theme"), defaultTheme()).toInt();
}

void AppSettings::setTheme(Global::Theme theme)
{
    m_settings->setValue(QStringLiteral("Interface/Theme"), theme);
}

Global::Theme AppSettings::defaultTheme()
{
    return Global::Theme::UseFusion;
}

bool AppSettings::adaptParamsForIOEngine(Global::BenchmarkParams &params) const
{
    if (getIOEngineName() == "fio-default") {
        // this actually stands for "psync", a synchronous mode which
        // makes fio warn about (but otherwise ignore) multiple queues.
        // We do this in a separate function rather than in getBenchmarkParams()
        // to ensure that this remains a pure runtime change.
        if (params.Queues > 1) {
            qDebug() << Q_FUNC_INFO << "Using" << getIOEngineName() << "=> capping Queues=" << params.Queues << "to 1";
            params.Queues = 1;
            return true;
        }
    }
#ifdef __APPLE__
    if (getIOEngineName() == "posixaio") {
        uint32_t aioprocmax = 0;
        size_t len = sizeof(aioprocmax);
        int sysret = sysctlbyname("kern.aioprocmax", &aioprocmax, &len, nullptr, 0);
        // fio's iodepth should not exceed kern.aioprocmax, at least for smaller blocksizes.
        // For larger ones there is a margin (of approx. Queues=BlockSize/8 it seems), probably
        // before the iodepth no longer translates directly to a "number of concurrently open AIO queues".
        if (params.BlockSize < 256
                && sysret == 0 && aioprocmax > 0
                && params.Queues > aioprocmax) {
            qDebug() << Q_FUNC_INFO << "Using" << getIOEngineName() << "on Mac => capping Queues=" << params.Queues << "to 1";
            params.Queues = aioprocmax;
            return true;
        }
    }
#endif
    return false;
}

Global::BenchmarkParams AppSettings::getBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile) const
{
    Global::BenchmarkParams defaultSet = defaultBenchmarkParams(test, profile, Global::BenchmarkPreset::Standard);
    if (profile == Global::PerformanceProfile::RealWorld) return defaultSet;

    QString settingKey = QStringLiteral("Benchmark/Params/%1/%2/%3")
            .arg(QMetaEnum::fromType<Global::PerformanceProfile>().valueToKey(profile))
            .arg(QMetaEnum::fromType<Global::BenchmarkTest>().valueToKey(test));

    return {
        (Global::BenchmarkIOPattern)m_settings->value(settingKey.arg("Pattern"), defaultSet.Pattern).toInt(),
        m_settings->value(settingKey.arg("BlockSize"), defaultSet.BlockSize).toInt(),
        m_settings->value(settingKey.arg("Queues"), defaultSet.Queues).toInt(),
        m_settings->value(settingKey.arg("Threads"), defaultSet.Threads).toInt()
    };
}

void AppSettings::setBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile, Global::BenchmarkParams params)
{
    QString settingKey = QStringLiteral("Benchmark/Params/%1/%2/%3")
            .arg(QMetaEnum::fromType<Global::PerformanceProfile>().valueToKey(profile))
            .arg(QMetaEnum::fromType<Global::BenchmarkTest>().valueToKey(test));

    m_settings->setValue(settingKey.arg("Pattern"), params.Pattern);
    m_settings->setValue(settingKey.arg("BlockSize"), params.BlockSize);
    m_settings->setValue(settingKey.arg("Queues"), params.Queues);
    m_settings->setValue(settingKey.arg("Threads"), params.Threads);
}

Global::BenchmarkParams AppSettings::defaultBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile, Global::BenchmarkPreset preset)
{
    switch (profile)
    {
        case Global::PerformanceProfile::Default:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            case Global::BenchmarkTest::Test_2:
                if (preset == Global::BenchmarkPreset::Standard)
                return { Global::BenchmarkIOPattern::SEQ, 1024,  1,  1 };
                else
                return { Global::BenchmarkIOPattern::SEQ,  128, 32,  1 };
            case Global::BenchmarkTest::Test_3:
                if (preset == Global::BenchmarkPreset::Standard)
                return { Global::BenchmarkIOPattern::RND,    4, 32,  1 };
                else
                return { Global::BenchmarkIOPattern::RND,    4, 32, 16 };
            case Global::BenchmarkTest::Test_4:
                return { Global::BenchmarkIOPattern::RND,    4,  1,  1 };
            }
            break;
        case Global::PerformanceProfile::Peak:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            case Global::BenchmarkTest::Test_2:
                if (preset == Global::BenchmarkPreset::Standard)
                return { Global::BenchmarkIOPattern::RND,    4, 32,  1 };
                else
                return { Global::BenchmarkIOPattern::RND,    4, 32, 16 };
            default:
                // dear compiler, there's nothing more to handle here
                break;
            }
            break;
        case Global::PerformanceProfile::RealWorld:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  1,  1 };
            case Global::BenchmarkTest::Test_2:
                return { Global::BenchmarkIOPattern::RND,    4,  1,  1 };
            default:
                // dear compiler, there's nothing more to handle here
                break;
            }
            break;
        case Global::PerformanceProfile::Demo:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            default:
                // dear compiler, there's nothing more to handle here
                break;
            }
            break;
    }
    Q_UNREACHABLE();
}
