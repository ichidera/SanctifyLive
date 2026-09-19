#include "ui/ScripturePanel.h"

#include <algorithm>

#include <QAbstractTableModel>
#include <QButtonGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/scripture/ScriptureLibrary.h"
#include "ui/Theme.h"

// ScriptureTableModel is a thin QAbstractTableModel wrapper around
// ScriptureLibrary::verses(translationCode) -- Translation / Reference /
// Scripture columns, one row per verse of whichever translation is
// currently active. Deliberately NOT a QTableWidget populated with
// 31,000+ QTableWidgetItems: a real model/view split is what keeps
// scrolling responsive over a whole translation's text. Declared here
// (matching ScripturePanel.h's forward declaration) rather than in an
// unnamed namespace, since it needs no Q_OBJECT (no signals/slots/
// properties of its own) and is only ever touched through a pointer
// outside this file.
//
// Supports an optional keyword filter (Words search mode): when set,
// the model shows only matching verses, and every row index the view
// deals in is a VIEW row into that filtered subset -- sourceRow()/
// viewRowForSourceRow() are the two directions of translating between
// a view row and the corresponding index into
// ScriptureLibrary::verses(translationCode) (the "source" row). When no
// filter is active the two are identical, which is the common case
// (Reference mode always browses the unfiltered list -- see
// ScripturePanel).
class ScriptureTableModel : public QAbstractTableModel
{
public:
    enum Column
    {
        ColumnTranslation = 0,
        ColumnReference = 1,
        ColumnScripture = 2,
        ColumnCount = 3,
    };

    explicit ScriptureTableModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}

    // Switches which bundled translation this model shows. A no-op
    // (does NOT reset selection/filter state) if `code` is already
    // active, so re-clicking the same translation in the list doesn't
    // needlessly disturb the current view.
    void setTranslation(const QString &code)
    {
        if (code == m_translationCode)
            return;
        beginResetModel();
        m_translationCode = code;
        // A keyword filter's matches are translation-specific (the same
        // verse can contain "shepherd" in KJV and not in another
        // translation's wording of it) -- carrying it over across a
        // translation switch would show stale, possibly-wrong results,
        // so it's cleared rather than re-applied blindly.
        m_filterWords.clear();
        m_filteredRows.clear();
        endResetModel();
    }
    const QString &translation() const { return m_translationCode; }

    void setKeywordFilter(const QStringList &words)
    {
        beginResetModel();
        m_filterWords = words;
        rebuildFilteredRows();
        endResetModel();
    }
    void clearKeywordFilter() { setKeywordFilter({}); }
    bool isFiltering() const { return !m_filterWords.isEmpty(); }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() || m_translationCode.isEmpty())
            return 0;
        return isFiltering() ? m_filteredRows.size() : ScriptureLibrary::verses(m_translationCode).size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : ColumnCount;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return QVariant();
        const ScriptureVerse *verse = verseAt(index.row());
        if (!verse)
            return QVariant();
        switch (index.column()) {
        case ColumnTranslation:
            return m_translationCode;
        case ColumnReference:
            return verse->reference;
        case ColumnScripture:
            return verse->text;
        default:
            return QVariant();
        }
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return QVariant();
        switch (section) {
        case ColumnTranslation:
            return tr("Translation");
        case ColumnReference:
            return tr("Reference");
        case ColumnScripture:
            return tr("Scripture");
        default:
            return QVariant();
        }
    }

    // Translates a view row (what the table/selection model deals in)
    // to the corresponding row in ScriptureLibrary::verses(translation()).
    // -1 if out of range or no translation is active.
    int sourceRow(int viewRow) const
    {
        if (m_translationCode.isEmpty())
            return -1;
        if (!isFiltering())
            return (viewRow >= 0 && viewRow < ScriptureLibrary::verses(m_translationCode).size()) ? viewRow : -1;
        if (viewRow < 0 || viewRow >= m_filteredRows.size())
            return -1;
        return m_filteredRows.at(viewRow);
    }

    // The inverse of sourceRow(): which view row (if any) currently
    // shows the given ScriptureLibrary verses() row. Used to select and
    // scroll to a verse resolved by Reference-mode parsing.
    int viewRowForSourceRow(int sourceRow) const
    {
        if (m_translationCode.isEmpty() || sourceRow < 0)
            return -1;
        if (!isFiltering())
            return sourceRow < ScriptureLibrary::verses(m_translationCode).size() ? sourceRow : -1;
        const auto it = std::lower_bound(m_filteredRows.begin(), m_filteredRows.end(), sourceRow);
        if (it == m_filteredRows.end() || *it != sourceRow)
            return -1;
        return int(it - m_filteredRows.begin());
    }

    // Row accessor for ScripturePanel's selection handling, which needs
    // the real verse (book/chapter/verse/text), not just whatever a
    // particular column's data() call returns. Takes a VIEW row.
    const ScriptureVerse *verseAt(int viewRow) const
    {
        const int src = sourceRow(viewRow);
        if (m_translationCode.isEmpty())
            return nullptr;
        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses(m_translationCode);
        if (src < 0 || src >= all.size())
            return nullptr;
        return &all.at(src);
    }

    // Combines a set of SOURCE rows (i.e. already translated via
    // sourceRow(), sorted, deduplicated) into one reference string and
    // one text block -- the "compress into one slide unless multiple
    // verses are selected" behavior. A single verse round-trips as
    // itself. Multiple verses are grouped into runs of Bible-consecutive
    // verses (same book, same chapter, verse numbers incrementing by
    // exactly 1): a shift-click range comes out as "Esther 8:9-10"; a
    // Ctrl-click set of non-adjacent verses (EasyWorship's "hold Ctrl,
    // click each verse" workflow) still combines sensibly as "Esther
    // 8:9; Esther 8:12" rather than silently pretending they're
    // adjacent.
    void combinedReferenceAndText(const QVector<int> &sourceRows, QString *outReference, QString *outText) const
    {
        outReference->clear();
        outText->clear();
        if (sourceRows.isEmpty() || m_translationCode.isEmpty())
            return;

        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses(m_translationCode);
        QVector<const ScriptureVerse *> selected;
        selected.reserve(sourceRows.size());
        for (int row : sourceRows) {
            if (row >= 0 && row < all.size())
                selected.append(&all.at(row));
        }
        if (selected.isEmpty())
            return;

        if (selected.size() == 1) {
            *outReference = selected.first()->reference;
            *outText = selected.first()->text;
            return;
        }

        QStringList referenceParts;
        QStringList textParts;
        int runStart = 0;
        for (int i = 1; i <= selected.size(); ++i) {
            // Row-adjacency (not "verse number + 1"), because rows are
            // already in canonical Bible order within a translation:
            // John 3:36 is immediately followed by John 4:1 in
            // verses(), so this naturally treats a cross-chapter
            // continuation ("John 3:16-4:2") as ONE run, not two --
            // without needing special-cased "was that the last verse of
            // its chapter" arithmetic. The book check still stops a run
            // from silently bridging into the next book (Genesis 50:26
            // is immediately followed by Exodus 1:1 in row terms, but
            // "Genesis 50:20-Exodus 1:3" isn't a reference anyone
            // writes).
            const bool continuesRun = i < selected.size() && selected.at(i)->book == selected.at(i - 1)->book
                && sourceRows.at(i) == sourceRows.at(i - 1) + 1;
            if (continuesRun)
                continue;

            const ScriptureVerse *first = selected.at(runStart);
            const ScriptureVerse *last = selected.at(i - 1);
            if (runStart == i - 1)
                referenceParts << first->reference;
            else if (first->chapter == last->chapter)
                referenceParts << QStringLiteral("%1 %2:%3-%4").arg(first->book).arg(first->chapter).arg(first->verse).arg(last->verse);
            else
                referenceParts << QStringLiteral("%1 %2:%3-%4:%5")
                                      .arg(first->book)
                                      .arg(first->chapter)
                                      .arg(first->verse)
                                      .arg(last->chapter)
                                      .arg(last->verse);
            for (int j = runStart; j < i; ++j)
                textParts << selected.at(j)->text;
            runStart = i;
        }
        *outReference = referenceParts.join(QStringLiteral("; "));
        *outText = textParts.join(QLatin1Char(' '));
    }

private:
    void rebuildFilteredRows()
    {
        m_filteredRows.clear();
        if (m_filterWords.isEmpty() || m_translationCode.isEmpty())
            return; // not filtering; rowCount()/sourceRow() use the full list directly

        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses(m_translationCode);
        m_filteredRows.reserve(all.size() / 8); // rough guess; grows if needed
        for (int i = 0; i < all.size(); ++i) {
            bool matchesAll = true;
            for (const QString &word : std::as_const(m_filterWords)) {
                if (!all.at(i).text.contains(word, Qt::CaseInsensitive)) {
                    matchesAll = false;
                    break;
                }
            }
            if (matchesAll)
                m_filteredRows.append(i);
        }
    }

    QString m_translationCode;
    QStringList m_filterWords;
    QVector<int> m_filteredRows; // sorted ascending -- built by a single forward pass over verses()
};

ScripturePanel::ScripturePanel(QWidget *parent) : QWidget(parent)
{
    m_model = new ScriptureTableModel(this);
    m_activeTranslation = ScriptureLibrary::availableTranslations().isEmpty()
        ? QString()
        : ScriptureLibrary::availableTranslations().first().code; // "KJV" -- first in the bundled list
    m_model->setTranslation(m_activeTranslation);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(buildSearchBar());
    layout->addWidget(buildReferenceHintArea());

    auto *mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(8);
    mainRow->addWidget(buildTranslationsColumn(), /*stretch=*/0);

    m_table = new QTableView(this);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Extended (not Contiguous): shift-click/shift-arrow/drag for a
    // range, AND Ctrl-click to add non-adjacent verses -- matching
    // EasyWorship's documented "hold Ctrl, click each verse" workflow
    // for building a reading like Genesis 1:1,3,5.
    // combinedReferenceAndText() (above) already groups whatever comes
    // out of this into consecutive runs, so a discontiguous selection
    // degrades gracefully rather than pretending the verses are
    // adjacent.
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ScriptureTableModel::ColumnTranslation,
                                                       QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ScriptureTableModel::ColumnReference,
                                                       QHeaderView::ResizeToContents);
    m_table->setWordWrap(false);
    m_table->setShowGrid(false);
    mainRow->addWidget(m_table, /*stretch=*/1);

    layout->addLayout(mainRow, /*stretch=*/1);
    layout->addWidget(buildBottomBar());

    // Land on Genesis 1:1, matching the target design, before any
    // signals are wired up (see below) -- construction time is before
    // OperatorWindow has necessarily made this the active Content Tab,
    // and Item Preview shouldn't jump to a verse just because this
    // panel exists somewhere in the background.
    if (m_model->rowCount() > 0)
        m_table->selectRow(0);

    // All signal wiring is deliberately deferred to here, after every
    // widget involved exists. Several handlers (mode toggle, search
    // text, reset, translation change) touch m_table/m_model/
    // m_hintLabel, and Qt fires toggled()/textChanged()/
    // currentItemChanged() synchronously for the initial widget state
    // set above and in the build*() calls -- connecting earlier would
    // run those handlers before their dependencies exist.
    connect(m_wordsModeButton, &QToolButton::toggled, this, [this](bool checked) {
        if (checked)
            setMode(SearchMode::Words);
    });
    connect(m_referenceModeButton, &QToolButton::toggled, this, [this](bool checked) {
        if (checked)
            setMode(SearchMode::Reference);
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ScripturePanel::onSearchTextChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() {
        // "Chapter resolved, no verse typed yet, operator just pressed
        // Enter" defaults to that chapter's verse 1 rather than doing
        // nothing -- informed by EasyWorship/FreeShow prior art on this
        // exact gap.
        if (m_mode == SearchMode::Reference) {
            const ScriptureReference::Parsed parsed = ScriptureReference::parse(m_activeTranslation, m_searchEdit->text());
            if (!parsed.valid && parsed.stage == ScriptureReference::Stage::Verse) {
                const int firstRow = ScriptureLibrary::rowForReference(m_activeTranslation, parsed.book, parsed.chapter, 1);
                if (firstRow >= 0)
                    applySelection(firstRow, firstRow);
            }
        }
        emitForSelection(/*activate=*/true);
    });
    m_searchEdit->installEventFilter(this); // Tab accepts the first suggestion chip, see eventFilter()
    connect(m_resetButton, &QToolButton::clicked, this, &ScripturePanel::onResetClicked);
    connect(m_translationsList, &QListWidget::currentItemChanged, this, &ScripturePanel::onTranslationRowChanged);
    connect(m_moreAvailableButton, &QPushButton::clicked, this, &ScripturePanel::onMoreAvailableClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &, const QItemSelection &) { emitForSelection(/*activate=*/false); });
    connect(m_table, &QTableView::activated, this, [this](const QModelIndex &) { emitForSelection(/*activate=*/true); });
    connect(m_table, &QTableView::doubleClicked, this,
            [this](const QModelIndex &) { emitForSelection(/*activate=*/true); });

    updateHintLabel(ScriptureReference::parse(m_activeTranslation, m_searchEdit->text()));
    refreshReferenceCount();
}

QWidget *ScripturePanel::buildSearchBar()
{
    m_wordsModeButton = new QToolButton(this);
    m_wordsModeButton->setObjectName("modeToggleButton");
    m_wordsModeButton->setText(tr("Words"));
    m_wordsModeButton->setCheckable(true);
    m_wordsModeButton->setToolTip(
        tr("Search by words in the verse text, like a search engine -- e.g. \u201cLord is my shepherd\u201d."));

    m_referenceModeButton = new QToolButton(this);
    m_referenceModeButton->setObjectName("modeToggleButton");
    m_referenceModeButton->setText(tr("Reference"));
    m_referenceModeButton->setCheckable(true);
    m_referenceModeButton->setChecked(true); // default mode, matches the target design's "Genesis 1:1" search box
    m_referenceModeButton->setToolTip(
        tr("Jump straight to a book, chapter, and verse -- e.g. \u201cJohn 3:16\u201d or \u201cJohn 3:16-18\u201d."));

    auto *modeGroup = new QButtonGroup(this);
    modeGroup->setExclusive(true);
    modeGroup->addButton(m_wordsModeButton);
    modeGroup->addButton(m_referenceModeButton);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Book, chapter, verse\u2026 e.g. \u201cJohn 3:16\u201d"));
    m_searchEdit->setText(tr("Genesis 1:1"));

    m_resetButton = new QToolButton(this);
    m_resetButton->setText(tr("\u21BA")); // counterclockwise arrow, reads as "reset"
    m_resetButton->setAutoRaise(true);
    m_resetButton->setToolTip(tr("Clear the search box and jump back to the top of the list."));

    auto *bar = new QWidget(this);
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(4);
    barLayout->addWidget(m_wordsModeButton);
    barLayout->addWidget(m_referenceModeButton);
    barLayout->addWidget(m_searchEdit, /*stretch=*/1);
    barLayout->addWidget(m_resetButton);
    return bar;
}

QWidget *ScripturePanel::buildReferenceHintArea()
{
    // Reference mode's "the app tells you what's valid next" feedback:
    // a status line (current stage / valid range / error) plus, while
    // the book is still ambiguous, a row of clickable book-name chips.
    // Hidden entirely in Words mode, where neither applies.
    m_hintLabel = new QLabel(this);
    m_hintLabel->setObjectName("nextSlideLabel");
    m_hintLabel->setWordWrap(true);

    m_suggestionsRow = new QWidget(this);
    m_suggestionsLayout = new QHBoxLayout(m_suggestionsRow);
    m_suggestionsLayout->setContentsMargins(0, 0, 0, 0);
    m_suggestionsLayout->setSpacing(6);
    m_suggestionsRow->setVisible(false);

    auto *container = new QWidget(this);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(4);
    containerLayout->addWidget(m_hintLabel);
    containerLayout->addWidget(m_suggestionsRow);
    return container;
}

QWidget *ScripturePanel::buildTranslationsColumn()
{
    auto *header = new QLabel(tr("SCRIPTURES"), this);
    header->setObjectName("panelTitle");

    // Single-select, not checkboxes: matches EasyWorship's Resource
    // Library ("select a version from the list on the left" -- one
    // translation is "current" at a time). All four listed here are
    // real, fully bundled, and clickable -- no greyed-out placeholders.
    m_translationsList = new QListWidget(this);
    m_translationsList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_translationsList->setFixedWidth(120);

    for (const ScriptureTranslation &t : ScriptureLibrary::availableTranslations()) {
        auto *item = new QListWidgetItem(t.code, m_translationsList);
        item->setToolTip(t.displayName);
        item->setData(Qt::UserRole, t.code);
        if (t.code == m_activeTranslation)
            m_translationsList->setCurrentItem(item);
    }

    // A real link, not a disabled stub: clicking it is honest about
    // what it does today (explain where licensed translations like NIV
    // would come from), even though the Store itself doesn't exist yet.
    m_moreAvailableButton = new QPushButton(tr("More Available\u2026"), this);
    m_moreAvailableButton->setObjectName("linkButton");
    m_moreAvailableButton->setCursor(Qt::PointingHandCursor);

    auto *column = new QWidget(this);
    auto *columnLayout = new QVBoxLayout(column);
    columnLayout->setContentsMargins(0, 0, 0, 0);
    columnLayout->setSpacing(6);
    columnLayout->addWidget(header);
    columnLayout->addWidget(m_translationsList, /*stretch=*/1);
    columnLayout->addWidget(m_moreAvailableButton);
    return column;
}

QWidget *ScripturePanel::buildBottomBar()
{
    // Mirrors MediaLibraryPanel's bottom bar: settings-style controls on
    // the left (an honest disabled stub -- no per-translation settings
    // exist yet), a live count on the right.
    auto *settingsButton = new QToolButton(this);
    settingsButton->setText(tr("\u2699"));
    settingsButton->setAutoRaise(true);
    settingsButton->setEnabled(false);
    settingsButton->setToolTip(tr("Scripture settings aren't implemented yet -- see the roadmap in README.md."));

    auto *expandButton = new QToolButton(this);
    expandButton->setText(tr("\u25BE"));
    expandButton->setAutoRaise(true);
    expandButton->setEnabled(false);
    expandButton->setToolTip(tr("Display options aren't implemented yet."));

    m_referenceCountLabel = new QLabel(this);
    m_referenceCountLabel->setObjectName("nextSlideLabel");
    m_referenceCountLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *bar = new QWidget(this);
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(4, 0, 4, 0);
    barLayout->addWidget(settingsButton);
    barLayout->addWidget(expandButton);
    barLayout->addStretch(1);
    barLayout->addWidget(m_referenceCountLabel);
    return bar;
}

void ScripturePanel::setMode(SearchMode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;

    {
        // Switching modes changes what the text in the box even means
        // (a keyword query vs. a reference being built up) -- carrying
        // it over would just be confusing, so start clean. Blocked so
        // this reset doesn't itself trigger onSearchTextChanged() before
        // the rest of this function has settled the model/UI state.
        const QSignalBlocker blocker(m_searchEdit);
        m_searchEdit->clear();
    }
    m_model->clearKeywordFilter();
    rebuildSuggestionChips({});

    if (mode == SearchMode::Words) {
        m_searchEdit->setPlaceholderText(tr("Search words in the verse text\u2026"));
        m_hintLabel->clear();
    } else {
        m_searchEdit->setPlaceholderText(tr("Book, chapter, verse\u2026 e.g. \u201cJohn 3:16\u201d"));
        updateHintLabel(ScriptureReference::parse(m_activeTranslation, QString()));
    }

    if (m_model->rowCount() > 0) {
        m_table->selectRow(0);
        m_table->scrollToTop();
    }
    refreshReferenceCount();
    m_searchEdit->setFocus();
}

void ScripturePanel::setActiveTranslation(const QString &code)
{
    if (code == m_activeTranslation || code.isEmpty())
        return;
    m_activeTranslation = code;
    m_model->setTranslation(code);

    // Re-run whatever search was active against the new translation,
    // rather than just clearing it -- switching from KJV to RVA while
    // looking at John 3 should still be looking at John 3, in RVA.
    onSearchTextChanged(m_searchEdit->text());
    if (m_model->rowCount() > 0 && !m_table->selectionModel()->hasSelection())
        m_table->selectRow(0);
    refreshReferenceCount();
}

void ScripturePanel::onSearchTextChanged(const QString &text)
{
    if (m_mode == SearchMode::Words)
        updateWordsMode(text);
    else
        updateReferenceMode(text);
}

void ScripturePanel::updateWordsMode(const QString &text)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    const QStringList words = text.split(whitespace, Qt::SkipEmptyParts);
    m_model->setKeywordFilter(words);
    if (m_model->rowCount() > 0) {
        m_table->selectRow(0); // land on the first result, so Item Preview reflects the new search
    } else {
        // Nothing matched: clear the table's selection so it visibly
        // shows nothing highlighted, and tell OperatorWindow to drop
        // whatever was previewed before this search rather than leaving
        // stale content displayed next to an empty results table.
        m_table->clearSelection();
        emit previewCleared();
    }
    refreshReferenceCount();
}

void ScripturePanel::updateReferenceMode(const QString &text)
{
    const ScriptureReference::Parsed parsed = ScriptureReference::parse(m_activeTranslation, text);

    if (m_model->isFiltering())
        m_model->clearKeywordFilter(); // Reference mode always browses the full, unfiltered list

    rebuildSuggestionChips(parsed.stage == ScriptureReference::Stage::Book
                                ? ScriptureReference::bookSuggestions(parsed.bookQuery)
                                : QVector<ScriptureReference::BookSuggestion>());
    updateHintLabel(parsed);

    if (parsed.valid)
        applySelection(parsed.firstRow, parsed.lastRow);

    refreshReferenceCount();
}

void ScripturePanel::updateHintLabel(const ScriptureReference::Parsed &parsed)
{
    using Stage = ScriptureReference::Stage;

    if (!parsed.error.isEmpty()) {
        m_hintLabel->setText(parsed.error);
        m_hintLabel->setStyleSheet(QStringLiteral("color: %1;").arg(Theme::kAccentRed));
        return;
    }
    m_hintLabel->setStyleSheet(QString()); // back to the default muted "nextSlideLabel" color

    switch (parsed.stage) {
    case Stage::Book:
        m_hintLabel->setText(parsed.bookAmbiguous
                                  ? tr("Multiple books match \u2014 keep typing, or pick one below (Tab accepts the first).")
                                  : tr("Type a book name\u2026 e.g. \u201cJohn\u201d or \u201c1 Samuel\u201d"));
        break;
    case Stage::Chapter: {
        int chapters = 0;
        for (const ScriptureBookInfo &info : ScriptureLibrary::books(m_activeTranslation)) {
            if (info.name == parsed.book) {
                chapters = info.chapterCount();
                break;
            }
        }
        m_hintLabel->setText(tr("%1 \u2014 enter a chapter (1\u2013%2)").arg(parsed.book).arg(chapters));
        break;
    }
    case Stage::Verse: {
        int maxVerse = 0;
        for (const ScriptureBookInfo &info : ScriptureLibrary::books(m_activeTranslation)) {
            if (info.name == parsed.book) {
                maxVerse = info.versesPerChapter.value(parsed.chapter - 1);
                break;
            }
        }
        m_hintLabel->setText(tr("%1 %2 \u2014 enter a verse (1\u2013%3), or press Enter for verse 1")
                                  .arg(parsed.book)
                                  .arg(parsed.chapter)
                                  .arg(maxVerse));
        break;
    }
    case Stage::Range:
        m_hintLabel->setText(parsed.endVerse > 0
                                  ? tr("Range selected \u2014 press Enter to add, or keep typing to extend it.")
                                  : tr("Verse selected \u2014 press Enter to add, or add \u201c-<verse>\u201d for a range."));
        break;
    }
}

void ScripturePanel::rebuildSuggestionChips(const QVector<ScriptureReference::BookSuggestion> &suggestions)
{
    QLayoutItem *child = nullptr;
    while ((child = m_suggestionsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    m_firstSuggestionCompletion = suggestions.isEmpty() ? QString() : suggestions.first().completedInput;
    m_suggestionsRow->setVisible(!suggestions.isEmpty());
    for (const ScriptureReference::BookSuggestion &suggestion : suggestions) {
        auto *chip = new QPushButton(suggestion.name, this);
        chip->setObjectName("suggestionChip");
        chip->setCursor(Qt::PointingHandCursor);
        const QString completedInput = suggestion.completedInput;
        connect(chip, &QPushButton::clicked, this, [this, completedInput]() {
            m_searchEdit->setText(completedInput);
            m_searchEdit->setFocus();
        });
        m_suggestionsLayout->addWidget(chip);
    }
    m_suggestionsLayout->addStretch(1);
}

void ScripturePanel::applySelection(int firstSourceRow, int lastSourceRow)
{
    const int firstView = m_model->viewRowForSourceRow(firstSourceRow);
    const int lastView = m_model->viewRowForSourceRow(lastSourceRow);
    if (firstView < 0 || lastView < 0)
        return;

    const QModelIndex topLeft = m_model->index(firstView, 0);
    const QModelIndex bottomRight = m_model->index(lastView, ScriptureTableModel::ColumnCount - 1);
    m_table->selectionModel()->select(QItemSelection(topLeft, bottomRight),
                                       QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    // select() alone does NOT move the selection model's "current"
    // index (a distinct concept from "selected", used for keyboard
    // navigation) -- without this, the row is highlighted correctly but
    // currentIndex() still reports the previous row (or none), which
    // matters both for Shift-arrow extending from the right place and
    // for anything checking "is a selection active" via currentIndex()
    // rather than hasSelection().
    m_table->selectionModel()->setCurrentIndex(topLeft, QItemSelectionModel::Current);
    m_table->scrollTo(topLeft, QAbstractItemView::PositionAtCenter);
    // Selecting triggers QItemSelectionModel::selectionChanged, already
    // wired to emitForSelection(false) in the constructor -- no need to
    // emit previewRequested() again here.
}

void ScripturePanel::onResetClicked()
{
    m_searchEdit->clear(); // synchronously runs onSearchTextChanged("") via whichever mode is active
    if (m_model->rowCount() > 0) {
        m_table->selectRow(0);
        m_table->scrollToTop();
    }
}

void ScripturePanel::onTranslationRowChanged()
{
    const QListWidgetItem *item = m_translationsList->currentItem();
    if (!item)
        return;
    setActiveTranslation(item->data(Qt::UserRole).toString());
}

void ScripturePanel::onMoreAvailableClicked()
{
    QMessageBox::information(
        this, tr("More Translations"),
        tr("KJV, ASV, NHEB, and RVA are bundled with SanctifyLive because they're all in the public domain. "
           "Translations like NIV or CSB require a paid licence from their publisher and aren't something an "
           "app can simply bundle -- those will need a Store, once one exists, to fetch under the right "
           "licence terms. See the roadmap in README.md."));
}

void ScripturePanel::emitForSelection(bool activate)
{
    const QVector<int> rows = selectedSourceRowsSorted();
    if (rows.isEmpty())
        return;
    QString reference;
    QString text;
    m_model->combinedReferenceAndText(rows, &reference, &text);
    if (reference.isEmpty())
        return;
    if (activate)
        emit scriptureActivated(reference, text);
    else
        emit previewRequested(reference, text);
}

QVector<int> ScripturePanel::selectedSourceRowsSorted() const
{
    QVector<int> rows;
    const QModelIndexList selected = m_table->selectionModel()->selectedRows();
    rows.reserve(selected.size());
    for (const QModelIndex &index : selected)
        rows.append(m_model->sourceRow(index.row()));
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    return rows;
}

void ScripturePanel::refreshReferenceCount()
{
    const int count = m_model->rowCount();
    m_referenceCountLabel->setText(
        tr("%1 %2").arg(QLocale().toString(count), count == 1 ? tr("reference") : tr("references")));
}

bool ScripturePanel::eventFilter(QObject *watched, QEvent *event)
{
    // Tab, in Reference mode, with a book suggestion showing: accept the
    // first suggestion, the same way Tab/Right-arrow accepts an inline
    // autocomplete suggestion in a browser address bar. This is the
    // keyboard-only path to the same thing clicking a suggestion chip
    // does -- without it, the chips would be mouse-only, which doesn't
    // fit an app operators run during a live, often one-handed,
    // service.
    if (watched == m_searchEdit && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Tab && m_mode == SearchMode::Reference
            && !m_firstSuggestionCompletion.isEmpty()) {
            m_searchEdit->setText(m_firstSuggestionCompletion);
            return true; // consumed -- don't let focus move to the next widget
        }
    }
    return QWidget::eventFilter(watched, event);
}
