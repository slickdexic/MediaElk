#include "ui/settings/GlobalSettingsWidget.h"
#include "ui_GlobalSettingsWidget.h"

#include "file_search/MovieFilesOrganizer.h"
#include "settings/Settings.h"

#include <QFileDialog>
#include <QMessageBox>

// The directory table has five columns. We define the indices here
// to avoid more magic numbers
static constexpr int tableDirectoryTypeIndex = 0;
static constexpr int tableDirectoryPathIndex = 1;
static constexpr int tableDirectorySeparateFoldersIndex = 2;
static constexpr int tableDirectoryFixDateIndex = 3;
static constexpr int tableDirectoryReloadIndex = 4;
static constexpr int tableDirectoryDisableIndex = 5;

Q_DECLARE_METATYPE(QTableWidgetItem*)

GlobalSettingsWidget::GlobalSettingsWidget(QWidget* parent) : QWidget(parent), ui(new Ui::GlobalSettingsWidget)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    QFont smallFont = ui->labelGlobal->font();
    smallFont.setPointSize(smallFont.pointSize() - 1);
    ui->labelGlobal->setFont(smallFont);
#endif

    ui->dirs->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->dirs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->dirs->setSelectionBehavior(QAbstractItemView::SelectRows);

    // clang-format off
    connect(ui->buttonAddDir,           &QAbstractButton::clicked,         this, &GlobalSettingsWidget::chooseDirToAdd);
    connect(ui->buttonRemoveDir,        &QAbstractButton::clicked,         this, &GlobalSettingsWidget::removeDir);
    connect(ui->buttonMovieFilesToDirs, &QAbstractButton::clicked,         this, &GlobalSettingsWidget::organize);
    connect(ui->dirs,                   &QTableWidget::currentCellChanged, this, &GlobalSettingsWidget::dirListRowChanged);
    connect(ui->dirs,                   &QTableWidget::cellChanged,        this, &GlobalSettingsWidget::dirListEntryChanged);
    // clang-format on

    ui->comboStartupSection->addItem(tr("Movies"), "movies");
    ui->comboStartupSection->addItem(tr("TV Shows"), "tvshows");
    ui->comboStartupSection->addItem(tr("Concerts"), "concerts");
    ui->comboStartupSection->addItem(tr("Music"), "music");
    ui->comboStartupSection->addItem(tr("Import"), "import");

    ui->comboTheme->addItem(tr("Auto"), "auto");
    ui->comboTheme->addItem(tr("Light"), "light");
    ui->comboTheme->addItem(tr("Dark"), "dark");
}

GlobalSettingsWidget::~GlobalSettingsWidget()
{
    delete ui;
}

void GlobalSettingsWidget::setSettings(Settings& settings)
{
    m_settings = &settings;
}

void GlobalSettingsWidget::chooseDirToAdd()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Choose a directory containing your movies, TV show or concerts"),
        QDir::homePath(),
        QFileDialog::ReadOnly | QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    // Issue #1577: On macOS, for some reason, the main window goes on top of the settings window,
    // hiding it.  Explicitly raise it as a workaround.
    window()->raise();
    window()->activateWindow();

    if (dir.isEmpty()) {
        // User aborted file dialog.
        return;
    }
    QDir path(dir);
    if (path.isReadable()) {
        mediaelch::MediaDirectory settingsDir;
        settingsDir.path.setPath(path);
        // A lot of users store their movies in separate folders.  Therefore, we set it per default.
        settingsDir.separateFolders = true;
        settingsDir.fixDate = false;
        settingsDir.disabled = false;
        addDir(settingsDir);
    }
}

void GlobalSettingsWidget::loadSettings()
{
    // Stream Details
    ui->chkAutoLoadStreamDetails->setChecked(m_settings->autoLoadStreamDetails());

    ui->chkDownloadActorImages->setChecked(m_settings->downloadActorImages());
    ui->chkIgnoreArticlesWhenSorting->setChecked(m_settings->ignoreArticlesWhenSorting());
    ui->chkCheckForUpdates->setChecked(m_settings->checkForUpdates());

    for (int i = 0, n = ui->comboStartupSection->count(); i < n; ++i) {
        if (ui->comboStartupSection->itemData(i, Qt::UserRole) == m_settings->startupSection()) {
            ui->comboStartupSection->setCurrentIndex(i);
            break;
        }
    }

    for (int i = 0, n = ui->comboTheme->count(); i < n; ++i) {
        if (ui->comboTheme->itemData(i, Qt::UserRole) == m_settings->theme()) {
            ui->comboTheme->setCurrentIndex(i);
            break;
        }
    }

    // Directories
    ui->dirs->setRowCount(0);
    ui->dirs->clearContents();
    QVector<mediaelch::MediaDirectory> movieDirectories = m_settings->directorySettings().movieDirectories();
    for (elch_ssize_t i = 0, n = movieDirectories.count(); i < n; ++i) {
        addDir(movieDirectories.at(i), mediaelch::MediaDirectoryType::Movies);
    }
    QVector<mediaelch::MediaDirectory> tvShowDirectories = m_settings->directorySettings().tvShowDirectories();
    for (elch_ssize_t i = 0, n = tvShowDirectories.count(); i < n; ++i) {
        addDir(tvShowDirectories.at(i), mediaelch::MediaDirectoryType::TvShows);
    }
    QVector<mediaelch::MediaDirectory> concertDirectories = m_settings->directorySettings().concertDirectories();
    for (elch_ssize_t i = 0, n = concertDirectories.count(); i < n; ++i) {
        addDir(concertDirectories.at(i), mediaelch::MediaDirectoryType::Concerts);
    }
    QVector<mediaelch::MediaDirectory> downloadDirectories = m_settings->directorySettings().downloadDirectories();
    for (elch_ssize_t i = 0, n = downloadDirectories.count(); i < n; ++i) {
        mediaelch::MediaDirectory dir;
        dir.path = downloadDirectories.at(i).path;
        addDir(downloadDirectories.at(i), mediaelch::MediaDirectoryType::Downloads);
    }
    QVector<mediaelch::MediaDirectory> musicDirectories = m_settings->directorySettings().musicDirectories();
    for (elch_ssize_t i = 0, n = musicDirectories.count(); i < n; ++i) {
        addDir(musicDirectories.at(i), mediaelch::MediaDirectoryType::Music);
    }
    dirListRowChanged(ui->dirs->currentRow());

    // Exclude words
    ui->excludeWordsText->setPlainText(m_settings->excludeWords().join(","));

    ui->useYoutubePluginUrls->setChecked(m_settings->useYoutubePluginUrls());
}

void GlobalSettingsWidget::saveSettings()
{
    m_settings->setUseYoutubePluginUrls(ui->useYoutubePluginUrls->isChecked());
    m_settings->setAutoLoadStreamDetails(ui->chkAutoLoadStreamDetails->isChecked());
    m_settings->setDownloadActorImages(ui->chkDownloadActorImages->isChecked());
    m_settings->setIgnoreArticlesWhenSorting(ui->chkIgnoreArticlesWhenSorting->isChecked());
    m_settings->setCheckForUpdates(ui->chkCheckForUpdates->isChecked());
    m_settings->setStartupSection(
        ui->comboStartupSection->itemData(ui->comboStartupSection->currentIndex(), Qt::UserRole).toString());
    m_settings->setTheme(ui->comboTheme->itemData(ui->comboTheme->currentIndex(), Qt::UserRole).toString());

    // save directories
    QVector<mediaelch::MediaDirectory> movieDirectories;
    QVector<mediaelch::MediaDirectory> tvShowDirectories;
    QVector<mediaelch::MediaDirectory> concertDirectories;
    QVector<mediaelch::MediaDirectory> downloadDirectories;
    QVector<mediaelch::MediaDirectory> musicDirectories;
    for (int row = 0, n = ui->dirs->rowCount(); row < n; ++row) {
        mediaelch::MediaDirectory dir;
        dir.path.setPath(ui->dirs->item(row, tableDirectoryPathIndex)->text());
        dir.separateFolders = ui->dirs->item(row, tableDirectorySeparateFoldersIndex)->checkState() == Qt::Checked;
        dir.fixDate = ui->dirs->item(row, tableDirectoryFixDateIndex)->checkState() == Qt::Checked;
        dir.autoReload = ui->dirs->item(row, tableDirectoryReloadIndex)->checkState() == Qt::Checked;
        dir.disabled = ui->dirs->item(row, tableDirectoryDisableIndex)->checkState() == Qt::Checked;

        const int index = dynamic_cast<QComboBox*>(ui->dirs->cellWidget(row, tableDirectoryTypeIndex))->currentIndex();
        if (dynamic_cast<QComboBox*>(ui->dirs->cellWidget(row, 0))->currentIndex() == 0) {
            movieDirectories.append(dir);
        } else if (index == 1) {
            tvShowDirectories.append(dir);
        } else if (index == 2) {
            concertDirectories.append(dir);
        } else if (index == 3) {
            downloadDirectories.append(dir);
        } else if (index == 4) {
            musicDirectories.append(dir);
        }
    }

    m_settings->directorySettings().setMovieDirectories(movieDirectories);
    m_settings->directorySettings().setTvShowDirectories(tvShowDirectories);
    m_settings->directorySettings().setConcertDirectories(concertDirectories);
    m_settings->directorySettings().setDownloadDirectories(downloadDirectories);
    m_settings->directorySettings().setMusicDirectories(musicDirectories);

    // exclude words
    m_settings->setExcludeWords(ui->excludeWordsText->toPlainText());
}

void GlobalSettingsWidget::addDir(mediaelch::MediaDirectory directory, mediaelch::MediaDirectoryType dirType)
{
    QString dir = QDir::toNativeSeparators(directory.path.path());
    if (!dir.isEmpty()) {
        bool exists = false;
        for (int i = 0, n = ui->dirs->rowCount(); i < n; ++i) {
            if (ui->dirs->item(i, tableDirectoryPathIndex)->text() == dir) {
                exists = true;
            }
        }

        if (!exists) {
            int row = ui->dirs->rowCount();
            ui->dirs->insertRow(row);
            auto* item = new QTableWidgetItem(dir);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
            item->setToolTip(dir);

            auto* itemCheck = new QTableWidgetItem();
            itemCheck->setCheckState(directory.separateFolders ? Qt::Checked : Qt::Unchecked);

            auto* itemFixDate = new QTableWidgetItem();
            itemFixDate->setCheckState(directory.fixDate ? Qt::Checked : Qt::Unchecked);

            auto* itemCheckReload = new QTableWidgetItem();
            itemCheckReload->setCheckState(directory.autoReload ? Qt::Checked : Qt::Unchecked);

            auto* itemCheckDisabled = new QTableWidgetItem();
            itemCheckDisabled->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            itemCheckDisabled->setCheckState(directory.disabled ? Qt::Checked : Qt::Unchecked);

            auto* box = new QComboBox();
            box->setProperty("itemCheck", QVariant::fromValue(itemCheck));
            box->setProperty("itemFixDate", QVariant::fromValue(itemFixDate));
            box->setProperty("itemCheckReload", QVariant::fromValue(itemCheckReload));
            box->setProperty("itemCheckDisabled", QVariant::fromValue(itemCheckDisabled));
            box->addItems(
                QStringList() << tr("Movies") << tr("TV Shows") << tr("Concerts") << tr("Downloads") << tr("Music"));
            if (dirType == mediaelch::MediaDirectoryType::Movies) {
                box->setCurrentIndex(0);
            } else if (dirType == mediaelch::MediaDirectoryType::TvShows) {
                box->setCurrentIndex(1);
            } else if (dirType == mediaelch::MediaDirectoryType::Concerts) {
                box->setCurrentIndex(2);
            } else if (dirType == mediaelch::MediaDirectoryType::Downloads) {
                box->setCurrentIndex(3);
            } else if (dirType == mediaelch::MediaDirectoryType::Music) {
                box->setCurrentIndex(4);
            }

            ui->dirs->setCellWidget(row, tableDirectoryTypeIndex, box);
            ui->dirs->setItem(row, tableDirectoryPathIndex, item);
            ui->dirs->setItem(row, tableDirectorySeparateFoldersIndex, itemCheck);
            ui->dirs->setItem(row, tableDirectoryFixDateIndex, itemFixDate);
            ui->dirs->setItem(row, tableDirectoryReloadIndex, itemCheckReload);
            ui->dirs->setItem(row, tableDirectoryDisableIndex, itemCheckDisabled);


            connect(box, elchOverload<int>(&QComboBox::currentIndexChanged), this, [this, box]() {
                onDirTypeChanged(box);
            });

            onDirTypeChanged(box);
        }
    }
}


void GlobalSettingsWidget::removeDir()
{
    int row = ui->dirs->currentRow();
    if (row < 0) {
        return;
    }
    ui->dirs->removeRow(row);
}

void GlobalSettingsWidget::organize()
{
    auto* organizer = new MovieFilesOrganizer(this);

    int row = ui->dirs->currentRow();
    if (dynamic_cast<QComboBox*>(ui->dirs->cellWidget(row, 0))->currentIndex() != 0
        || ui->dirs->item(row, tableDirectorySeparateFoldersIndex)->checkState() == Qt::Checked) {
        organizer->canceled(tr("Organizing movies does only work on "
                               "movies, not already sorted to "
                               "separate folders."));
        return;
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(tr("Are you sure?"));
    msgBox.setInformativeText(
        tr("This operation sorts all movies in this directory to separate "
           "sub-directories based on the file name. Click \"Ok\", if that's, what you want to do. "));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    switch (ret) {
    case QMessageBox::Ok:
        organizer->moveToDirs(
            mediaelch::DirectoryPath(ui->dirs->item(ui->dirs->currentRow(), tableDirectoryPathIndex)->text()));
        ui->dirs->item(ui->dirs->currentRow(), tableDirectorySeparateFoldersIndex)->setCheckState(Qt::Checked);
        break;
    case QMessageBox::Cancel:
    default: break;
    }
}

void GlobalSettingsWidget::onDirTypeChanged(QComboBox* box)
{
    if (box == nullptr) {
        return;
    }

    QTableWidgetItem* itemCheck = box->property("itemCheck").value<QTableWidgetItem*>();
    QTableWidgetItem* itemFixDate = box->property("itemFixDate").value<QTableWidgetItem*>();
    QTableWidgetItem* itemCheckReload = box->property("itemCheckReload").value<QTableWidgetItem*>();

    // TODO: Don't rely on index
    if (box->currentIndex() == 0) {
        itemCheck->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        itemFixDate->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        itemCheckReload->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    } else if (box->currentIndex() == 2) {
        itemCheck->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        itemFixDate->setFlags(Qt::NoItemFlags);
        itemFixDate->setCheckState(Qt::Unchecked);
        itemCheckReload->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);

    } else if (box->currentIndex() == 1 || box->currentIndex() == 4) {
        itemCheck->setFlags(Qt::NoItemFlags);
        itemCheck->setCheckState(Qt::Unchecked);
        itemFixDate->setFlags(Qt::NoItemFlags);
        itemFixDate->setCheckState(Qt::Unchecked);
        itemCheckReload->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);

    } else if (box->currentIndex() == 3) {
        itemCheck->setFlags(Qt::NoItemFlags);
        itemCheck->setCheckState(Qt::Unchecked);
        itemFixDate->setFlags(Qt::NoItemFlags);
        itemFixDate->setCheckState(Qt::Unchecked);
        itemCheckReload->setFlags(Qt::NoItemFlags);
        itemCheckReload->setCheckState(Qt::Unchecked);
    }
}


void GlobalSettingsWidget::dirListRowChanged(int currentRow)
{
    if (currentRow < 0 || currentRow >= ui->dirs->rowCount()) {
        // Somehow out of bounds?
        ui->buttonRemoveDir->setDisabled(true);
        ui->buttonMovieFilesToDirs->setDisabled(true);
        return;
    }
    ui->buttonRemoveDir->setDisabled(false);
    auto* typeWidget = ui->dirs->cellWidget(currentRow, tableDirectoryTypeIndex);
    if (typeWidget != nullptr && dynamic_cast<QComboBox*>(typeWidget)->currentIndex() == 0
        && ui->dirs->item(currentRow, tableDirectorySeparateFoldersIndex)->checkState() == Qt::Unchecked) {
        ui->buttonMovieFilesToDirs->setDisabled(false);
    } else {
        ui->buttonMovieFilesToDirs->setDisabled(true);
    }
}

void GlobalSettingsWidget::dirListEntryChanged(int row, int column)
{
    if (column != tableDirectoryPathIndex) {
        return;
    }
    QTableWidgetItem* dirCell = ui->dirs->item(row, tableDirectoryPathIndex);
    if (dirCell == nullptr) {
        return;
    }

    QColor defaultTextColor = QWidget::palette().color(QPalette::Text);
    QColor invalidColor(255, 0, 0);

    // if the directory is not readable, mark it red
    const QDir dir(dirCell->text());
    const QColor color = dir.isReadable() ? defaultTextColor : invalidColor;
    dirCell->setForeground(color);
}
