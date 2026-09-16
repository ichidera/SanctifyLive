#include "ui/ScripturePanel.h"

#include <algorithm>

#include <QAbstractTableModel>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
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
// ScriptureLibrary::verses() -- Translation / Reference / Scripture
// columns, one row per verse. Deliberately NOT a QTableWidget populated
// with 31,000+ QTableWidgetItems: a real model/view split is what keeps
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
// ScriptureLibrary::verses() (the "source" row). When no filter is
// active the two are identical, which is the common case (Reference
// mode always browses the unfiltered list -- see ScripturePanel).
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

    // KJV is the only translation that actually has verses loaded (see
    // ScriptureLibrary), so "enabled" is effectively "is the KJV
    // checkbox checked" -- unchecking it empties the table rather than
    // hiding rows one at a time, since there's nothing else to fall
    // back to yet.
    void setEnabled(bool enabled)
    {
        if (enabled == m_enabled)
            return;
        beginResetModel();
        m_enabled = enabled;
        endResetModel();
    }

    // words are ANDed together (case-insensitive substring match) --
    // a verse must contain every word to match. An empty list clears
    // filtering entirely (full, unfiltered translation).
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
        if (parent.isValid() || !m_enabled)
            return 0;
        return isFiltering() ? m_filteredRows.size() : ScriptureLibrary::verses().size();
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
            return ScriptureLibrary::translationCode();
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
    // to the corresponding row in ScriptureLibrary::verses(). -1 if out
    // of range or nothing's enabled.
    int sourceRow(int viewRow) const
    {
        if (!m_enabled)
            return -1;
        if (!isFiltering())
            return (viewRow >= 0 && viewRow < ScriptureLibrary::verses().size()) ? viewRow : -1;
        if (viewRow < 0 || viewRow >= m_filteredRows.size())
            return -1;
        return m_filteredRows.at(viewRow);
    }

    // The inverse of sourceRow(): which view row (if any) currently
    // shows the given ScriptureLibrary::verses() row. Used to select
    // and scroll to a verse resolved by Reference-mode parsing.
    int viewRowForSourceRow(int sourceRow) const
    {
        if (!m_enabled || sourceRow < 0)
            return -1;
        if (!isFiltering())
            return sourceRow < ScriptureLibrary::verses().size() ? sourceRow : -1;
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
        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses();
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
    // exactly 1): a normal shift-click range comes out as "Esther
    // 8:9-10"; a handful of unrelated Words-mode search hits still
    // combine sensibly as "Esther 8:9; John 3:16" rather than silently
    // pretending they're adjacent.
    void combinedReferenceAndText(const QVector<int> &sourceRows, QString *outReference, QString *outText) const
    {
        outReference->clear();
        outText->clear();
        if (sourceRows.isEmpty())
            return;

        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses();
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
            const bool continuesRun = i < selected.size() && selected.at(i)->book == selected.at(i - 1)->book
                && selected.at(i)->chapter == selected.at(i - 1)->chapter
                && selected.at(i)->verse == selected.at(i - 1)->verse + 1;
            if (continuesRun)
                continue;

            const ScriptureVerse *first = selected.at(runStart);
            const ScriptureVerse *last = selected.at(i - 1);
            referenceParts << (runStart == i - 1 ? first->reference
                                                  : QStringLiteral("%1 %2:%3-%4")
                                                        .arg(first->book)
                                                        .arg(first->chapter)
                                                        .arg(first->verse)
                                                        .arg(last->verse));
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
        if (m_filterWords.isEmpty())
            return; // not filtering; rowCount()/sourceRow() use the full list directly

        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses();
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

    bool m_enabled = true;
    QStringList m_filterWords;
    QVector<int> m_filteredRows; // sorted ascending -- built by a single forward pass over verses()
};

ScripturePanel::ScripturePanel(QWidget *parent) : QWidget(parent)
{
    m_model = new ScriptureTableModel(this);

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
    // Contiguous, not Extended: a slide is a run of consecutive verses,
    // so ctrl-click-style discontiguous multi-select isn't offered --
    // shift-click/shift-arrow/drag to extend a single range instead.
    m_table->setSelectionMode(QAbstractItemView::ContiguousSelection);
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
    // text, reset) touch m_table/m_model/m_hintLabel, and Qt fires
    // toggled()/textChanged() synchronously for the initial widget
    // state set above and in the build*() calls -- connecting earlier
    // would run those handlers before their dependencies exist.
    connect(m_wordsModeButton, &QToolButton::toggled, this, [this](bool checked) {
        if (checked)
            setMode(SearchMode::Words);
    });
    connect(m_referenceModeButton, &QToolButton::toggled, this, [this](bool checked) {
        if (checked)
            setMode(SearchMode::Reference);
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ScripturePanel::onSearchTextChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, [this]() { emitForSelection(/*activate=*/true); });
    connect(m_resetButton, &QToolButton::clicked, this, &ScripturePanel::onResetClicked);
    connect(m_translationsList, &QListWidget::itemChanged, this, &ScripturePanel::onTranslationItemChanged);
    connect(m_moreAvailableButton, &QPushButton::clicked, this, &ScripturePanel::onMoreAvailableClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &, const QItemSelection &) { emitForSelection(/*activate=*/false); });
    connect(m_table, &QTableView::activated, this, [this](const QModelIndex &) { emitForSelection(/*activate=*/true); });
    connect(m_table, &QTableView::doubleClicked, this,
            [this](const QModelIndex &) { emitForSelection(/*activate=*/true); });

    updateHintLabel(ScriptureReference::parse(m_searchEdit->text()));
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

    // A checkable list, not three separate QCheckBoxes: this matches
    // the target design's look (a selectable row per translation, KJV
    // highlighted as "current") and gets the existing QListWidget
    // selection styling (Theme.cpp) for free. Only KJV is actually
    // backed by data (see ScriptureLibrary) -- HCSB and RVA are shown,
    // per the target design, but disabled: there's no Store yet to
    // fetch them from (see README roadmap), so letting an operator
    // "check" one that silently shows no verses would be worse than not
    // listing it at all.
    m_translationsList = new QListWidget(this);
    m_translationsList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_translationsList->setFixedWidth(120);

    auto addTranslation = [this](const QString &code, bool available) {
        auto *item = new QListWidgetItem(code, m_translationsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(available ? Qt::Checked : Qt::Unchecked);
        if (!available) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            item->setToolTip(tr("Not installed yet -- see \u201cMore Available\u2026\u201d below."));
        }
        return item;
    };

    m_kjvItem = addTranslation(tr("KJV"), /*available=*/true);
    addTranslation(tr("HCSB"), /*available=*/false);
    addTranslation(tr("RVA"), /*available=*/false);
    m_translationsList->setCurrentItem(m_kjvItem);

    // A real link, not a disabled stub: clicking it is honest about
    // what it does today (explain where more translations will come
    // from), even though the Store itself doesn't exist yet. See this
    // class's doc comment.
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
        updateHintLabel(ScriptureReference::parse(QString()));
    }

    if (m_model->rowCount() > 0) {
        m_table->selectRow(0);
        m_table->scrollToTop();
    }
    refreshReferenceCount();
    m_searchEdit->setFocus();
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
    if (m_model->rowCount() > 0)
        m_table->selectRow(0); // land on the first result, so Item Preview reflects the new search
    refreshReferenceCount();
}

void ScripturePanel::updateReferenceMode(const QString &text)
{
    const ScriptureReference::Parsed parsed = ScriptureReference::parse(text);

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
                                  ? tr("Multiple books match \u2014 keep typing, or pick one below.")
                                  : tr("Type a book name\u2026 e.g. \u201cJohn\u201d or \u201c1 Samuel\u201d"));
        break;
    case Stage::Chapter: {
        int chapters = 0;
        for (const ScriptureBookInfo &info : ScriptureLibrary::books()) {
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
        for (const ScriptureBookInfo &info : ScriptureLibrary::books()) {
            if (info.name == parsed.book) {
                maxVerse = info.versesPerChapter.value(parsed.chapter - 1);
                break;
            }
        }
        m_hintLabel->setText(
            tr("%1 %2 \u2014 enter a verse (1\u2013%3)").arg(parsed.book).arg(parsed.chapter).arg(maxVerse));
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

void ScripturePanel::onTranslationItemChanged(QListWidgetItem *item)
{
    if (item != m_kjvItem)
        return; // HCSB/RVA rows are disabled and can't actually be toggled by the user
    m_model->setEnabled(item->checkState() == Qt::Checked);
    refreshReferenceCount();
}

void ScripturePanel::onMoreAvailableClicked()
{
    QMessageBox::information(
        this, tr("More Translations"),
        tr("Additional translations will be available from the SanctifyLive Store once it's built -- "
           "see the roadmap in README.md. For now, KJV is the only translation bundled with the app."));
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
