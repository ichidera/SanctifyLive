#include "ui/SongsPanel.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/song/SongLibrary.h"
#include "ui/SongEditorDialog.h"

SongsPanel::SongsPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search songs by title, author, or lyrics\u2026"));
    layout->addWidget(m_searchEdit);

    auto *mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(8);
    mainRow->addWidget(buildSongListColumn(), /*stretch=*/0);
    mainRow->addWidget(buildBreakdownArea(), /*stretch=*/1);
    layout->addLayout(mainRow, /*stretch=*/1);

    layout->addWidget(buildBottomBar());

    // Load quietly and settle on an initial selection before any signal
    // is connected -- construction time is before OperatorWindow has
    // necessarily made this the active Content Tab, and Item Preview
    // shouldn't jump to a song just because this panel exists somewhere
    // in the background. Same reasoning/pattern as ScripturePanel and
    // MediaLibraryPanel's constructors.
    loadLibrary();
    refreshSongList();
    refreshBreakdownForCurrentSong();

    connect(m_searchEdit, &QLineEdit::textChanged, this, &SongsPanel::onSearchTextChanged);
    connect(m_songList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *, QListWidgetItem *) { onSongSelectionChanged(); });
    connect(m_songList, &QListWidget::itemActivated, this, &SongsPanel::onSongActivated);
    connect(m_breakdownTable, &QTableWidget::cellClicked, this, &SongsPanel::onSectionClicked);
    connect(m_breakdownTable, &QTableWidget::cellActivated, this, &SongsPanel::onSectionActivated);
    connect(m_breakdownTable, &QTableWidget::cellDoubleClicked, this, &SongsPanel::onSectionActivated);
    connect(m_addButton, &QPushButton::clicked, this, &SongsPanel::onAddSongClicked);
    connect(m_editButton, &QPushButton::clicked, this, &SongsPanel::onEditSongClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &SongsPanel::onDeleteSongClicked);

    onSongSelectionChanged(); // sets the initial Edit/Delete enabled state
}

QWidget *SongsPanel::buildSongListColumn()
{
    auto *header = new QLabel(tr("SONGS"), this);
    header->setObjectName("panelTitle");

    m_songList = new QListWidget(this);
    m_songList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_songList->setFixedWidth(200);

    m_addButton = new QPushButton(tr("+ Add"), this);
    m_editButton = new QPushButton(tr("Edit"), this);
    m_deleteButton = new QPushButton(tr("Delete"), this);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_editButton);
    buttonRow->addWidget(m_deleteButton);

    auto *column = new QWidget(this);
    auto *columnLayout = new QVBoxLayout(column);
    columnLayout->setContentsMargins(0, 0, 0, 0);
    columnLayout->setSpacing(6);
    columnLayout->addWidget(header);
    columnLayout->addWidget(m_songList, /*stretch=*/1);
    columnLayout->addLayout(buttonRow);
    return column;
}

QWidget *SongsPanel::buildBreakdownArea()
{
    m_songTitleLabel = new QLabel(this);
    m_songTitleLabel->setObjectName("panelTitle");

    // Section / Lyrics, mirroring ScripturePanel's Translation/Reference/
    // Scripture table -- a labeled-row breakdown of whatever's currently
    // selected, the same "browse the structure of the current item"
    // shape used throughout the Content Tabs.
    m_breakdownTable = new QTableWidget(0, 2, this);
    m_breakdownTable->setHorizontalHeaderLabels({tr("Section"), tr("Lyrics")});
    m_breakdownTable->horizontalHeader()->setStretchLastSection(true);
    m_breakdownTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_breakdownTable->verticalHeader()->setVisible(false);
    m_breakdownTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_breakdownTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_breakdownTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_breakdownTable->setWordWrap(false);
    m_breakdownTable->setShowGrid(false);

    auto *area = new QWidget(this);
    auto *areaLayout = new QVBoxLayout(area);
    areaLayout->setContentsMargins(0, 0, 0, 0);
    areaLayout->setSpacing(6);
    areaLayout->addWidget(m_songTitleLabel);
    areaLayout->addWidget(m_breakdownTable, /*stretch=*/1);
    return area;
}

QWidget *SongsPanel::buildBottomBar()
{
    auto *settingsButton = new QToolButton(this);
    settingsButton->setText(tr("\u2699"));
    settingsButton->setAutoRaise(true);
    settingsButton->setEnabled(false);
    settingsButton->setToolTip(tr("Song settings aren't implemented yet -- see the roadmap in README.md."));

    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName("nextSlideLabel");
    m_countLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *bar = new QWidget(this);
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(4, 0, 4, 0);
    barLayout->addWidget(settingsButton);
    barLayout->addStretch(1);
    barLayout->addWidget(m_countLabel);
    return bar;
}

void SongsPanel::loadLibrary()
{
    QString errorMessage;
    if (!SongLibrary::load(SongLibrary::defaultStorePath(), &m_songs, &errorMessage)) {
        qWarning() << "SongsPanel: failed to load song library:" << errorMessage;
        m_songs.clear();
    }

    if (m_songs.isEmpty()) {
        // First run (or a load failure that left nothing usable): seed
        // with public-domain hymns so this tab has real, working content
        // immediately -- same reasoning as ScriptureLibrary shipping
        // real Bible text rather than an empty placeholder.
        m_songs = SongLibrary::sampleSongs();
        persistLibrary();
    }
}

void SongsPanel::persistLibrary()
{
    QString errorMessage;
    if (!SongLibrary::save(m_songs, SongLibrary::defaultStorePath(), &errorMessage))
        qWarning() << "SongsPanel: failed to save song library:" << errorMessage;
}

void SongsPanel::refreshSongList()
{
    const QString filter = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    const QString previouslySelectedId = currentSong() ? currentSong()->id : QString();

    m_songList->clear();
    for (const Song &song : std::as_const(m_songs)) {
        if (!filter.isEmpty()) {
            bool matches = song.title.contains(filter, Qt::CaseInsensitive)
                || song.author.contains(filter, Qt::CaseInsensitive);
            if (!matches) {
                for (const SongSection &section : song.sections) {
                    if (section.text.contains(filter, Qt::CaseInsensitive)) {
                        matches = true;
                        break;
                    }
                }
            }
            if (!matches)
                continue;
        }
        auto *item = new QListWidgetItem(song.title, m_songList);
        item->setData(Qt::UserRole, song.id);
        item->setToolTip(song.author.isEmpty() ? song.title : tr("%1 \u2014 %2").arg(song.title, song.author));
        if (song.id == previouslySelectedId)
            m_songList->setCurrentItem(item);
    }
    if (!m_songList->currentItem() && m_songList->count() > 0)
        m_songList->setCurrentRow(0);

    const int count = m_songList->count();
    m_countLabel->setText(tr("%1 %2").arg(QLocale().toString(count), count == 1 ? tr("song") : tr("songs")));
}

void SongsPanel::refreshBreakdownForCurrentSong()
{
    m_breakdownTable->setRowCount(0);
    const Song *song = currentSong();
    m_songTitleLabel->setText(song ? song->title : tr("No song selected"));
    if (!song)
        return;

    const QVector<const SongSection *> ordered = song->orderedSections();
    m_breakdownTable->setRowCount(ordered.size());
    for (int row = 0; row < ordered.size(); ++row) {
        const SongSection *section = ordered.at(row);
        auto *labelItem = new QTableWidgetItem(section->displayLabel());
        labelItem->setFlags(labelItem->flags() & ~Qt::ItemIsEditable);
        // simplified(): collapse the section's internal line breaks into
        // single spaces for this one-line-per-row breakdown view -- the
        // full, real line breaks are still what's in section->text and
        // what actually gets sent to Schedule/Item Preview.
        auto *textItem = new QTableWidgetItem(section->text.simplified());
        textItem->setFlags(textItem->flags() & ~Qt::ItemIsEditable);
        m_breakdownTable->setItem(row, 0, labelItem);
        m_breakdownTable->setItem(row, 1, textItem);
    }
}

void SongsPanel::onSongSelectionChanged()
{
    refreshBreakdownForCurrentSong();
    const Song *song = currentSong();
    m_editButton->setEnabled(song != nullptr);
    m_deleteButton->setEnabled(song != nullptr);

    if (!song)
        return;
    const QVector<const SongSection *> ordered = song->orderedSections();
    if (ordered.isEmpty())
        return;
    m_breakdownTable->selectRow(0);
    const SongSection *first = ordered.first();
    emit previewRequested(tr("%1 \u2014 %2").arg(song->title, first->displayLabel()), first->text);
}

void SongsPanel::onSongActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    const Song *song = currentSong();
    if (!song)
        return;
    const QVector<const SongSection *> ordered = song->orderedSections();
    if (ordered.isEmpty())
        return;

    QStringList labels;
    QStringList texts;
    labels.reserve(ordered.size());
    texts.reserve(ordered.size());
    for (const SongSection *section : ordered) {
        labels << tr("%1 \u2014 %2").arg(song->title, section->displayLabel());
        texts << section->text;
    }
    emit wholeSongActivated(labels, texts);
}

void SongsPanel::onSectionClicked(int row, int column)
{
    Q_UNUSED(column);
    const Song *song = currentSong();
    if (!song)
        return;
    const QVector<const SongSection *> ordered = song->orderedSections();
    if (row < 0 || row >= ordered.size())
        return;
    const SongSection *section = ordered.at(row);
    emit previewRequested(tr("%1 \u2014 %2").arg(song->title, section->displayLabel()), section->text);
}

void SongsPanel::onSectionActivated(int row, int column)
{
    Q_UNUSED(column);
    const Song *song = currentSong();
    if (!song)
        return;
    const QVector<const SongSection *> ordered = song->orderedSections();
    if (row < 0 || row >= ordered.size())
        return;
    const SongSection *section = ordered.at(row);
    emit sectionActivated(tr("%1 \u2014 %2").arg(song->title, section->displayLabel()), section->text);
}

void SongsPanel::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    refreshSongList();
}

void SongsPanel::onAddSongClicked()
{
    SongEditorDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const Song newSong = dialog.result();
    m_songs.append(newSong);
    persistLibrary();
    refreshSongList();

    for (int i = 0; i < m_songList->count(); ++i) {
        if (m_songList->item(i)->data(Qt::UserRole).toString() == newSong.id) {
            m_songList->setCurrentRow(i);
            break;
        }
    }
}

void SongsPanel::onEditSongClicked()
{
    const Song *song = currentSong();
    if (!song)
        return;

    SongEditorDialog dialog(this, song);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const Song updated = dialog.result();
    for (Song &existing : m_songs) {
        if (existing.id == updated.id) {
            existing = updated;
            break;
        }
    }
    persistLibrary();
    refreshSongList();
    refreshBreakdownForCurrentSong();
}

void SongsPanel::onDeleteSongClicked()
{
    const Song *song = currentSong();
    if (!song)
        return;

    const auto reply = QMessageBox::question(this, tr("Delete Song"),
                                              tr("Delete \u201c%1\u201d? This can't be undone.").arg(song->title));
    if (reply != QMessageBox::Yes)
        return;

    const QString id = song->id;
    for (int i = 0; i < m_songs.size(); ++i) {
        if (m_songs.at(i).id == id) {
            m_songs.removeAt(i);
            break;
        }
    }
    persistLibrary();
    refreshSongList();
}

const Song *SongsPanel::currentSong() const
{
    QListWidgetItem *item = m_songList->currentItem();
    if (!item)
        return nullptr;
    const QString id = item->data(Qt::UserRole).toString();
    for (const Song &song : m_songs) {
        if (song.id == id)
            return &song;
    }
    return nullptr;
}
