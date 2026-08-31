#include "ScripturePanel.h"

#include <algorithm>

#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include "../common/IconFactory.h"

namespace {
constexpr int kVerseRole = Qt::UserRole;
constexpr int kChapterRole = Qt::UserRole + 1;
constexpr int kVerseNumRole = Qt::UserRole + 2;

QTreeWidgetItem *makeSectionHeader(QTreeWidget *tree, const QString &text)
{
    auto *item = new QTreeWidgetItem(tree, {text});
    QFont font = item->font(0);
    font.setBold(true);
    font.setPointSizeF(font.pointSizeF() * 0.9);
    item->setFont(0, font);
    item->setForeground(0, QColor("#8a8a8a"));
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    return item;
}

// Truncates a search-result snippet so long verses don't dominate the
// results list; the full text is still what actually gets sent live.
QString elideSnippet(const QString &text, int maxLength = 130)
{
    if (text.length() <= maxLength)
        return text;
    return text.left(maxLength).trimmed() + QStringLiteral("...");
}
}

ScripturePanel::ScripturePanel(QWidget *parent) : QWidget(parent)
{
    m_libraryAvailable = m_library.openDefault();

    // ---------- Top row: reference/keyword search + translation ----------
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText(tr("Search or type a reference, e.g. John 3:16"));
    m_searchBox->addAction(IconFactory::search(14), QLineEdit::LeadingPosition);
    connect(m_searchBox, &QLineEdit::returnPressed, this, &ScripturePanel::onSearchSubmitted);
    connect(m_searchBox, &QLineEdit::textEdited, this, &ScripturePanel::onSearchTextEdited);

    m_translationCombo = new QComboBox(this);
    m_translationCombo->setMinimumWidth(90);
    connect(m_translationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScripturePanel::onTranslationChanged);

    auto *searchRowLayout = new QHBoxLayout();
    searchRowLayout->addWidget(m_searchBox, 1);
    searchRowLayout->addWidget(m_translationCombo);

    // ---------- Column 1: books, grouped Old/New Testament ----------
    m_bookTree = new QTreeWidget(this);
    m_bookTree->setHeaderHidden(true);
    m_bookTree->setIndentation(14);
    m_bookTree->setMinimumWidth(160);
    connect(m_bookTree, &QTreeWidget::itemClicked, this, &ScripturePanel::onBookSelected);

    // ---------- Column 2: chapter number grid ----------
    m_chapterGrid = new QListWidget(this);
    m_chapterGrid->setViewMode(QListView::IconMode);
    m_chapterGrid->setResizeMode(QListView::Adjust);
    m_chapterGrid->setMovement(QListView::Static);
    m_chapterGrid->setGridSize(QSize(40, 30));
    m_chapterGrid->setSpacing(3);
    m_chapterGrid->setMinimumWidth(180);
    connect(m_chapterGrid, &QListWidget::itemClicked, this, &ScripturePanel::onChapterSelected);

    auto *chapterPanel = new QWidget(this);
    auto *chapterPanelLayout = new QVBoxLayout(chapterPanel);
    chapterPanelLayout->setContentsMargins(0, 0, 0, 0);
    chapterPanelLayout->addWidget(new QLabel(tr("Chapter"), chapterPanel));
    chapterPanelLayout->addWidget(m_chapterGrid, 1);

    // ---------- Column 3: verse list / search results ----------
    m_verseList = new QListWidget(this);
    m_verseList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_verseList->setWordWrap(true);
    m_verseList->setAlternatingRowColors(true);
    connect(m_verseList, &QListWidget::itemActivated, this, &ScripturePanel::onVerseItemActivated);
    connect(m_verseList, &QListWidget::itemDoubleClicked, this, &ScripturePanel::onVerseItemActivated);

    m_sendSelectedButton = new QPushButton(tr("Send Selected \u25B6"), this);
    m_sendSelectedButton->setToolTip(
        tr("Send the selected verse(s) live. Select a range to project a whole passage as one slide."));
    connect(m_sendSelectedButton, &QPushButton::clicked, this, &ScripturePanel::onSendSelectedClicked);

    auto *versePanel = new QWidget(this);
    auto *versePanelLayout = new QVBoxLayout(versePanel);
    versePanelLayout->setContentsMargins(0, 0, 0, 0);
    versePanelLayout->addWidget(new QLabel(tr("Verse"), versePanel));
    versePanelLayout->addWidget(m_verseList, 1);
    versePanelLayout->addWidget(m_sendSelectedButton);

    auto *bodySplitter = new QSplitter(this);
    bodySplitter->addWidget(m_bookTree);
    bodySplitter->addWidget(chapterPanel);
    bodySplitter->addWidget(versePanel);
    bodySplitter->setStretchFactor(0, 2);
    bodySplitter->setStretchFactor(1, 2);
    bodySplitter->setStretchFactor(2, 4);

    m_footerLabel = new QLabel(this);
    m_footerLabel->setStyleSheet("color: #888; padding: 2px 6px;");
    m_footerLabel->setWordWrap(true);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(4, 4, 4, 4);
    rootLayout->setSpacing(4);
    rootLayout->addLayout(searchRowLayout);
    rootLayout->addWidget(bodySplitter, 1);
    rootLayout->addWidget(m_footerLabel);

    if (!m_libraryAvailable) {
        m_searchBox->setEnabled(false);
        m_translationCombo->setEnabled(false);
        m_bookTree->setEnabled(false);
        m_chapterGrid->setEnabled(false);
        m_verseList->setEnabled(false);
        m_sendSelectedButton->setEnabled(false);
        m_footerLabel->setText(m_library.lastError());
        return;
    }

    populateTranslations();
}

ScripturePanel::~ScripturePanel() = default;

QString ScripturePanel::currentTranslationCode() const
{
    return m_translationCombo->currentData().toString();
}

void ScripturePanel::populateTranslations()
{
    const auto translations = m_library.translations();
    m_translationCombo->blockSignals(true);
    for (const auto &translation : translations)
        m_translationCombo->addItem(translation.code, translation.code);
    m_translationCombo->blockSignals(false);

    if (translations.isEmpty()) {
        m_footerLabel->setText(tr("No translations found in the Scripture database."));
        return;
    }

    m_translationCombo->setCurrentIndex(0);
    m_translationCombo->setToolTip(translations.first().name);
    populateBookTree();
}

void ScripturePanel::onTranslationChanged(int index)
{
    if (index < 0)
        return;
    m_translationCombo->setToolTip(m_translationCombo->currentText());
    for (const auto &translation : m_library.translations()) {
        if (translation.code == currentTranslationCode()) {
            m_translationCombo->setToolTip(translation.name);
            break;
        }
    }
    populateBookTree();
}

void ScripturePanel::populateBookTree()
{
    m_bookTree->clear();
    const QString code = currentTranslationCode();
    const QStringList books = m_library.bookNames(code);
    const int otCount = m_library.oldTestamentBookCount(code);

    QTreeWidgetItem *otSection = makeSectionHeader(m_bookTree, tr("OLD TESTAMENT"));
    QTreeWidgetItem *ntSection = makeSectionHeader(m_bookTree, tr("NEW TESTAMENT"));

    for (int i = 0; i < books.size(); ++i) {
        QTreeWidgetItem *parent = (i < otCount) ? otSection : ntSection;
        new QTreeWidgetItem(parent, {books.at(i)});
    }

    m_bookTree->expandItem(otSection);
    m_bookTree->expandItem(ntSection);

    m_currentBook.clear();
    m_currentChapter = 0;
    m_chapterGrid->clear();
    m_verseList->clear();
    m_searchResultsActive = false;

    if (otSection->childCount() > 0) {
        QTreeWidgetItem *first = otSection->child(0);
        m_bookTree->setCurrentItem(first);
        m_currentBook = first->text(0);
        populateChapterGrid(m_currentBook);
    }
    updateFooter();
}

void ScripturePanel::onBookSelected(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item || !item->parent())
        return; // ignore clicks on the OT/NT section headers

    m_currentBook = item->text(0);
    m_searchResultsActive = false;
    populateChapterGrid(m_currentBook);
}

void ScripturePanel::populateChapterGrid(const QString &bookName)
{
    m_chapterGrid->clear();
    const int chapters = m_library.chapterCount(currentTranslationCode(), bookName);
    for (int c = 1; c <= chapters; ++c) {
        auto *item = new QListWidgetItem(QString::number(c));
        item->setTextAlignment(Qt::AlignCenter);
        m_chapterGrid->addItem(item);
    }

    if (chapters > 0) {
        m_chapterGrid->setCurrentRow(0);
        m_currentChapter = 1;
        populateVerseList(bookName, 1);
    } else {
        m_currentChapter = 0;
        m_verseList->clear();
    }
    updateFooter();
}

void ScripturePanel::onChapterSelected(QListWidgetItem *item)
{
    if (!item)
        return;
    m_currentChapter = item->text().toInt();
    m_searchResultsActive = false;
    populateVerseList(m_currentBook, m_currentChapter);
}

void ScripturePanel::populateVerseList(const QString &bookName, int chapter)
{
    m_verseList->clear();
    m_searchResultsActive = false;

    const auto verses = m_library.versesInChapter(currentTranslationCode(), bookName, chapter);
    for (const auto &v : verses) {
        auto *item = new QListWidgetItem(QStringLiteral("%1  %2").arg(v.verse).arg(v.text));
        item->setData(kVerseRole, bookName);
        item->setData(kChapterRole, chapter);
        item->setData(kVerseNumRole, v.verse);
        m_verseList->addItem(item);
    }
    updateFooter();
}

void ScripturePanel::onVerseItemActivated(QListWidgetItem *item)
{
    if (!item)
        return;

    const QString book = item->data(kVerseRole).toString();
    const int chapter = item->data(kChapterRole).toInt();
    const int verse = item->data(kVerseNumRole).toInt();

    // A double-click from search results should also update the
    // browse columns, so the operator sees where the verse came from
    // and can select neighboring verses if they want a fuller passage.
    if (m_searchResultsActive) {
        QTreeWidgetItemIterator it(m_bookTree);
        while (*it) {
            if ((*it)->parent() && (*it)->text(0) == book) {
                m_bookTree->setCurrentItem(*it);
                break;
            }
            ++it;
        }
        m_currentBook = book;
        populateChapterGrid(book);
        for (int row = 0; row < m_chapterGrid->count(); ++row) {
            if (m_chapterGrid->item(row)->text().toInt() == chapter) {
                m_chapterGrid->setCurrentRow(row);
                break;
            }
        }
        m_currentChapter = chapter;
        populateVerseList(book, chapter);
        for (int row = 0; row < m_verseList->count(); ++row) {
            if (m_verseList->item(row)->data(kVerseNumRole).toInt() == verse) {
                m_verseList->setCurrentRow(row);
                break;
            }
        }
    }

    sendVerses(book, chapter, verse, verse);
}

void ScripturePanel::onSendSelectedClicked()
{
    const QList<QListWidgetItem *> selected = m_verseList->selectedItems();
    if (selected.isEmpty())
        return;

    // Sort by row so a shift-click range comes out in reading order.
    QList<QListWidgetItem *> ordered = selected;
    std::sort(ordered.begin(), ordered.end(), [this](QListWidgetItem *a, QListWidgetItem *b) {
        return m_verseList->row(a) < m_verseList->row(b);
    });

    const QString book = ordered.first()->data(kVerseRole).toString();
    const int chapter = ordered.first()->data(kChapterRole).toInt();
    const bool sameBookChapter = std::all_of(ordered.begin(), ordered.end(), [&](QListWidgetItem *item) {
        return item->data(kVerseRole).toString() == book && item->data(kChapterRole).toInt() == chapter;
    });

    if (!sameBookChapter) {
        // Discontiguous multi-book selection (only reachable from search
        // results): sending a single combined slide doesn't make sense,
        // so just send the first one and let the operator send the rest
        // individually.
        sendVerses(book, chapter, ordered.first()->data(kVerseNumRole).toInt(),
                   ordered.first()->data(kVerseNumRole).toInt());
        return;
    }

    const int verseStart = ordered.first()->data(kVerseNumRole).toInt();
    const int verseEnd = ordered.last()->data(kVerseNumRole).toInt();
    sendVerses(book, chapter, verseStart, verseEnd);
}

void ScripturePanel::sendVerses(const QString &book, int chapter, int verseStart, int verseEnd)
{
    const QString code = currentTranslationCode();
    QStringList lines;
    for (int v = verseStart; v <= verseEnd; ++v) {
        QString text;
        if (m_library.verseText(code, book, chapter, v, &text)) {
            if (verseEnd > verseStart)
                lines << QStringLiteral("%1 %2").arg(v).arg(text);
            else
                lines << text;
        }
    }
    if (lines.isEmpty())
        return;

    const QString reference = (verseEnd > verseStart)
        ? QStringLiteral("%1 %2:%3-%4").arg(book).arg(chapter).arg(verseStart).arg(verseEnd)
        : QStringLiteral("%1 %2:%3").arg(book).arg(chapter).arg(verseStart);

    emit scriptureActivated(reference, lines.join(QStringLiteral("\n")), code);
}

void ScripturePanel::onSearchTextEdited(const QString &text)
{
    // Clearing the box manually backs out of search-results mode and
    // returns to whatever book/chapter was being browsed before.
    if (text.trimmed().isEmpty() && m_searchResultsActive) {
        if (!m_currentBook.isEmpty())
            populateVerseList(m_currentBook, m_currentChapter);
        else
            m_verseList->clear();
        updateFooter();
    }
}

void ScripturePanel::onSearchSubmitted()
{
    const QString text = m_searchBox->text().trimmed();
    if (text.isEmpty() || !m_libraryAvailable)
        return;

    const BibleLibrary::ParsedReference ref = m_library.parseReference(currentTranslationCode(), text);
    if (ref.valid) {
        jumpToReference(ref);
        if (ref.verseStart > 0)
            sendVerses(ref.book, ref.chapter, ref.verseStart, ref.verseEnd);
        return;
    }

    const auto results = m_library.search(currentTranslationCode(), text);
    showSearchResults(results, text);
}

void ScripturePanel::jumpToReference(const BibleLibrary::ParsedReference &ref)
{
    QTreeWidgetItemIterator it(m_bookTree);
    while (*it) {
        if ((*it)->parent() && (*it)->text(0) == ref.book) {
            m_bookTree->setCurrentItem(*it);
            break;
        }
        ++it;
    }

    m_currentBook = ref.book;
    populateChapterGrid(ref.book);
    for (int row = 0; row < m_chapterGrid->count(); ++row) {
        if (m_chapterGrid->item(row)->text().toInt() == ref.chapter) {
            m_chapterGrid->setCurrentRow(row);
            break;
        }
    }
    m_currentChapter = ref.chapter;
    populateVerseList(ref.book, ref.chapter);
    m_searchResultsActive = false;

    if (ref.verseStart > 0) {
        for (int row = 0; row < m_verseList->count(); ++row) {
            const int v = m_verseList->item(row)->data(kVerseNumRole).toInt();
            if (v >= ref.verseStart && v <= ref.verseEnd)
                m_verseList->item(row)->setSelected(true);
        }
        if (m_verseList->count() > 0)
            m_verseList->scrollToItem(m_verseList->item(qMax(0, ref.verseStart - 1)));
    }
}

void ScripturePanel::showSearchResults(const QVector<BibleLibrary::SearchResult> &results, const QString &query)
{
    m_searchResultsActive = true;
    m_verseList->clear();

    for (const auto &r : results) {
        auto *item = new QListWidgetItem(
            QStringLiteral("%1 %2:%3 \u2014 %4").arg(r.book).arg(r.chapter).arg(r.verse).arg(elideSnippet(r.text)));
        item->setData(kVerseRole, r.book);
        item->setData(kChapterRole, r.chapter);
        item->setData(kVerseNumRole, r.verse);
        m_verseList->addItem(item);
    }

    if (results.isEmpty())
        m_footerLabel->setText(tr("No results for \u201c%1\u201d.").arg(query));
    else
        m_footerLabel->setText(tr("%1 result(s) for \u201c%2\u201d \u2014 double-click to jump and send live.")
                                    .arg(results.size())
                                    .arg(query));
}

void ScripturePanel::updateFooter()
{
    if (!m_libraryAvailable)
        return;
    if (m_searchResultsActive)
        return; // showSearchResults() already set an appropriate message

    QString translationName;
    for (const auto &translation : m_library.translations()) {
        if (translation.code == currentTranslationCode()) {
            translationName = translation.name;
            break;
        }
    }

    if (m_currentBook.isEmpty() || m_currentChapter <= 0) {
        m_footerLabel->setText(translationName);
        return;
    }

    m_footerLabel->setText(tr("%1 \u00b7 %2 %3 \u00b7 %4 verse(s)")
                                .arg(translationName, m_currentBook)
                                .arg(m_currentChapter)
                                .arg(m_verseList->count()));
}
