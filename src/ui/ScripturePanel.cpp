#include "ui/ScripturePanel.h"

#include <QAbstractTableModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/scripture/ScriptureLibrary.h"

// ScriptureTableModel is a thin QAbstractTableModel wrapper around
// ScriptureLibrary::verses() -- Translation / Reference / Scripture
// columns, one row per verse. Deliberately NOT a QTableWidget populated
// with 31,000+ QTableWidgetItems: a real model/view split is what keeps
// scrolling responsive over a whole translation's text. Declared here
// (matching ScripturePanel.h's forward declaration) rather than in an
// unnamed namespace, since it needs no Q_OBJECT (no signals/slots/
// properties of its own) and is only ever touched through a pointer
// outside this file.
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
    bool isEnabled() const { return m_enabled; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;
        return m_enabled ? ScriptureLibrary::verses().size() : 0;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : ColumnCount;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || role != Qt::DisplayRole)
            return QVariant();

        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses();
        if (index.row() < 0 || index.row() >= all.size())
            return QVariant();
        const ScriptureVerse &verse = all.at(index.row());

        switch (index.column()) {
        case ColumnTranslation:
            return ScriptureLibrary::translationCode();
        case ColumnReference:
            return verse.reference;
        case ColumnScripture:
            return verse.text;
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

    // Row accessor for ScripturePanel's click/activate handlers, which
    // need the real verse (reference + full text), not just whatever a
    // particular column's data() call returns.
    const ScriptureVerse *verseAt(int row) const
    {
        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses();
        if (row < 0 || row >= all.size())
            return nullptr;
        return &all.at(row);
    }

    // Finds the first verse whose reference starts with `query`
    // (case-insensitive), falling back to "contains" on the reference,
    // then "contains" on the verse text itself -- so typing a full or
    // partial reference ("Genesis 1:1", "gen 1") jumps straight there,
    // while a plain word ("shepherd") still finds a matching verse
    // rather than coming up empty. Returns -1 if nothing matches.
    int findJumpRow(const QString &query) const
    {
        if (!m_enabled || query.trimmed().isEmpty())
            return -1;

        const QVector<ScriptureVerse> &all = ScriptureLibrary::verses();
        int containsReferenceMatch = -1;
        int containsTextMatch = -1;
        for (int i = 0; i < all.size(); ++i) {
            const ScriptureVerse &verse = all.at(i);
            if (verse.reference.startsWith(query, Qt::CaseInsensitive))
                return i;
            if (containsReferenceMatch < 0 && verse.reference.contains(query, Qt::CaseInsensitive))
                containsReferenceMatch = i;
            if (containsTextMatch < 0 && verse.text.contains(query, Qt::CaseInsensitive))
                containsTextMatch = i;
        }
        return containsReferenceMatch >= 0 ? containsReferenceMatch : containsTextMatch;
    }

private:
    bool m_enabled = true;
};

ScripturePanel::ScripturePanel(QWidget *parent) : QWidget(parent)
{
    m_model = new ScriptureTableModel(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(buildSearchBar());

    auto *mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(8);
    mainRow->addWidget(buildTranslationsColumn(), /*stretch=*/0);

    m_table = new QTableView(this);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ScriptureTableModel::ColumnTranslation,
                                                       QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ScriptureTableModel::ColumnReference,
                                                       QHeaderView::ResizeToContents);
    m_table->setWordWrap(false);
    m_table->setShowGrid(false);
    connect(m_table, &QTableView::clicked, this, &ScripturePanel::onRowClicked);
    connect(m_table, &QTableView::activated, this, &ScripturePanel::onRowActivated);
    connect(m_table, &QTableView::doubleClicked, this, &ScripturePanel::onRowActivated);
    mainRow->addWidget(m_table, /*stretch=*/1);

    layout->addLayout(mainRow, /*stretch=*/1);
    layout->addWidget(buildBottomBar());

    // Land on Genesis 1:1, matching the target design, without firing
    // previewRequested() -- construction time is before OperatorWindow
    // has necessarily made this the active Content Tab, and Item
    // Preview shouldn't jump to a verse just because this panel exists
    // somewhere in the background. A real click/search does the same
    // navigation later and DOES emit the signal.
    if (m_model->rowCount() > 0)
        m_table->selectRow(0);

    refreshReferenceCount();
}

QWidget *ScripturePanel::buildSearchBar()
{
    auto *bookPickerButton = new QToolButton(this);
    bookPickerButton->setText(tr("\u2630")); // \u2630: trigram/list glyph, stands in for a future book/chapter picker
    bookPickerButton->setAutoRaise(true);
    bookPickerButton->setEnabled(false);
    bookPickerButton->setToolTip(tr("Browsing by book/chapter isn't implemented yet -- search by reference below."));

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search by reference (e.g. \u201cGenesis 1:1\u201d) or scripture text\u2026"));
    m_searchEdit->setText(tr("Genesis 1:1"));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ScripturePanel::onSearchTextChanged);

    auto *optionsButton = new QToolButton(this);
    optionsButton->setText(tr("\u2261"));
    optionsButton->setAutoRaise(true);
    optionsButton->setEnabled(false);
    optionsButton->setToolTip(tr("Search options aren't implemented yet."));

    auto *resetButton = new QToolButton(this);
    resetButton->setText(tr("\u21BA")); // \u21BA: counterclockwise arrow, reads as "reset"
    resetButton->setAutoRaise(true);
    resetButton->setToolTip(tr("Clear the search box and jump back to the top of the list."));
    connect(resetButton, &QToolButton::clicked, this, [this]() {
        m_searchEdit->clear();
        if (m_model->rowCount() > 0) {
            m_table->selectRow(0);
            m_table->scrollToTop();
        }
    });

    auto *bar = new QWidget(this);
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(4);
    barLayout->addWidget(bookPickerButton);
    barLayout->addWidget(m_searchEdit, /*stretch=*/1);
    barLayout->addWidget(optionsButton);
    barLayout->addWidget(resetButton);
    return bar;
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
    connect(m_translationsList, &QListWidget::itemChanged, this, &ScripturePanel::onTranslationItemChanged);

    // A real link, not a disabled stub: clicking it is honest about
    // what it does today (explain where more translations will come
    // from), even though the Store itself doesn't exist yet. See this
    // class's doc comment.
    auto *moreAvailable = new QPushButton(tr("More Available\u2026"), this);
    moreAvailable->setObjectName("linkButton");
    moreAvailable->setCursor(Qt::PointingHandCursor);
    connect(moreAvailable, &QPushButton::clicked, this, &ScripturePanel::onMoreAvailableClicked);

    auto *column = new QWidget(this);
    auto *layout = new QVBoxLayout(column);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(header);
    layout->addWidget(m_translationsList, /*stretch=*/1);
    layout->addWidget(moreAvailable);
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
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->addWidget(settingsButton);
    layout->addWidget(expandButton);
    layout->addStretch(1);
    layout->addWidget(m_referenceCountLabel);
    return bar;
}

void ScripturePanel::onSearchTextChanged(const QString &text)
{
    // A jump-to search, not a filter: the table always shows every
    // verse in the enabled translation(s) (matching the target design's
    // "31,102 references" count staying put while "Genesis 1:1" sits in
    // the search box) -- typing a reference or a word scrolls/selects
    // the best match instead of hiding everything else.
    const int row = m_model->findJumpRow(text);
    if (row < 0)
        return;
    const QModelIndex index = m_model->index(row, ScriptureTableModel::ColumnReference);
    m_table->setCurrentIndex(index);
    m_table->scrollTo(index, QAbstractItemView::PositionAtCenter);
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

void ScripturePanel::onRowClicked(const QModelIndex &index)
{
    if (const ScriptureVerse *verse = m_model->verseAt(index.row()))
        emit previewRequested(verse->reference, verse->text);
}

void ScripturePanel::onRowActivated(const QModelIndex &index)
{
    if (const ScriptureVerse *verse = m_model->verseAt(index.row()))
        emit scriptureActivated(verse->reference, verse->text);
}

void ScripturePanel::refreshReferenceCount()
{
    const int count = m_model->rowCount();
    m_referenceCountLabel->setText(
        tr("%1 %2").arg(QLocale().toString(count), count == 1 ? tr("reference") : tr("references")));
}
