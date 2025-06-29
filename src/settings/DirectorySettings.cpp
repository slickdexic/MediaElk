#include "settings/DirectorySettings.h"

#include "utils/Meta.h"

#include <QDir>

void DirectorySettings::loadSettings()
{
    const auto loadDirectory = [&](const char* settingsKey, QVector<mediaelch::MediaDirectory>& directories) {
        directories.clear();
        const int size = m_settings->beginReadArray(settingsKey);
        for (int i = 0; i < size; ++i) {
            m_settings->setArrayIndex(i);
            mediaelch::MediaDirectory dir;
            dir.path.setPath(QDir::toNativeSeparators(m_settings->value("path").toString()));
            dir.separateFolders = m_settings->value("sepFolders", false).toBool();
            dir.fixDate = m_settings->value("fixDate", false).toBool();
            dir.autoReload = m_settings->value("autoReload", false).toBool();
            dir.disabled = m_settings->value("disabled", false).toBool();

            directories.append(dir);
        }
        m_settings->endArray();
    };

    loadDirectory("Directories/Movies", m_movieDirectories);
    loadDirectory("Directories/TvShows", m_tvShowDirectories);
    loadDirectory("Directories/Concerts", m_concertDirectories);
    loadDirectory("Directories/Downloads", m_downloadDirectories);
    loadDirectory("Directories/Music", m_musicDirectories);
}

void DirectorySettings::saveSettings()
{
    const auto saveDirectory = [&](const char* settingsKey, QVector<mediaelch::MediaDirectory>& directories) {
        m_settings->beginWriteArray(settingsKey);
        const elch_ssize_t size = directories.count();
        for (int i = 0; i < size; ++i) {
            m_settings->setArrayIndex(i);
            m_settings->setValue("path", directories.at(i).path.path());
            m_settings->setValue("sepFolders", directories.at(i).separateFolders);
            m_settings->setValue("fixDate", directories.at(i).fixDate);
            m_settings->setValue("autoReload", directories.at(i).autoReload);
            m_settings->setValue("disabled", directories.at(i).disabled);
        }
        m_settings->endArray();
    };

    saveDirectory("Directories/Movies", m_movieDirectories);
    saveDirectory("Directories/Concerts", m_concertDirectories);
    saveDirectory("Directories/Downloads", m_downloadDirectories);
    saveDirectory("Directories/Music", m_musicDirectories);

    m_settings->beginWriteArray("Directories/TvShows");
    for (elch_ssize_t i = 0, n = m_tvShowDirectories.count(); i < n; ++i) {
        m_settings->setArrayIndex(qsizetype_to_int(i));
        m_settings->setValue("path", m_tvShowDirectories.at(i).path.path());
        m_settings->setValue("autoReload", m_tvShowDirectories.at(i).autoReload);
        m_settings->setValue("disabled", m_tvShowDirectories.at(i).disabled);
    }
    m_settings->endArray();
}

const QVector<mediaelch::MediaDirectory>& DirectorySettings::movieDirectories() const
{
    return m_movieDirectories;
}

const QVector<mediaelch::MediaDirectory>& DirectorySettings::tvShowDirectories() const
{
    return m_tvShowDirectories;
}

const QVector<mediaelch::MediaDirectory>& DirectorySettings::concertDirectories() const
{
    return m_concertDirectories;
}

const QVector<mediaelch::MediaDirectory>& DirectorySettings::musicDirectories() const
{
    return m_musicDirectories;
}

const QVector<mediaelch::MediaDirectory>& DirectorySettings::downloadDirectories() const
{
    return m_downloadDirectories;
}

void DirectorySettings::setMovieDirectories(QVector<mediaelch::MediaDirectory> dirs)
{
    m_movieDirectories = dirs;
}

void DirectorySettings::setTvShowDirectories(QVector<mediaelch::MediaDirectory> dirs)
{
    m_tvShowDirectories = dirs;
}

void DirectorySettings::setConcertDirectories(QVector<mediaelch::MediaDirectory> dirs)
{
    m_concertDirectories = dirs;
}

void DirectorySettings::setMusicDirectories(QVector<mediaelch::MediaDirectory> dirs)
{
    m_musicDirectories = dirs;
}

void DirectorySettings::setDownloadDirectories(QVector<mediaelch::MediaDirectory> dirs)
{
    m_downloadDirectories = dirs;
}
