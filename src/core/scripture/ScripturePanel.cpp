#include "ScripturePanel.h"

#include <algorithm>

#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include "../common/IconFactory.h"
#include "../output/LiveAppearancePreview.h"
#include "../schedule/Slide.h"
#include "ScriptureFormatting.h"
#include "ScriptureSearchEdit.h"
#include "TranslationLibraryDialog.h"

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

QString formatReferenceDisplay(const BibleLibrary::ParsedReference &ref)
{
    if (ref.verseStart <= 0)
        return QStringLiteral("%1 %2").arg(ref.book).arg(ref.chapter);
    if (ref.verseEnd > ref.verseStart)
        return QStringLiteral("%1 %2:%3-%4").arg(ref.book).arg(ref.chapter).arg(ref.verseStart).arg(ref.verseEnd);
    return QStringLiteral("%1 %2:%3").arg(ref.book).arg(ref.chapter).arg(ref.verseStart);
}
}

ScripturePanel::ScripturePanel(QWidget *parent) : QWidget(parent)
{
    m_libraryAvailable = m_library.openDefault();

    // ---------- Top row: reference/keyword search + translation + add ----------
    m_searchBox = new ScriptureSearchEdit(this);
    connect(m_searchBox, &QLineEdit::returnPressed, this, &ScripturePanel::onSearchSubmitted);
    connect(m_searchBox, &ScriptureSearchEdit::liveWordQuery, this, &ScripturePanel::onLiveWordQuery);
    connect(m_searchBox, &ScriptureSearchEdit::wordWrapToggleRequested, this,
            &ScripturePanel::onWordWrapToggleRequested);
    connect(m_searchBox, &ScriptureSearchEdit::sortOrderChangeRequested, this,
            &ScripturePanel::onSortOrderChangeRequested);
    connect(m_searchBox, &ScriptureSearchEdit::refreshRequested, this, [this]() {
        if (m_searchResultsActive)
            showSearchResults(m_library.search(currentTranslationCode(), m_lastSearchQuery), m_lastSearchQuery);
        else if (!m_currentBook.isEmpty())
            populateVerseList(m_currentBook, m_currentChapter);
    });

    m_translationMenu = new QMenu(this);
    m_translationButton = new QToolButton(this);
    m_translationButton->setPopupMode(QToolButton::InstantPopup);
    m_translationButton->setMenu(m_translationMenu);
    m_translationButton->setMinimumWidth(80);
    m_translationButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_plusMenu = new QMenu(this);
    m_plusButton = new QToolButton(this);
    m_plusButton->setIcon(IconFactory::plusAdd(16));
    m_plusButton->setPopupMode(QToolButton::InstantPopup);
    m_plusButton->setMenu(m_plusMenu);
    m_plusButton->setToolTip(tr("Add a Bible, or organize your translations into folders/collections."));
    rebuildPlusMenu();

    auto *searchRowLayout = new QHBoxLayout();
    searchRowLayout->addWidget(m_searchBox, 1);
    searchRowLayout->addWidget(m_translationButton);
    searchRowLayout->addWidget(m_plusButton);

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
    connect(m_verseList, &QListWidget::itemSelectionChanged, this, &ScripturePanel::onVerseSelectionChanged);

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

    // Live-appearance preview -- driven by the verse list's selection
    // (see onVerseSelectionChanged/updatePreview), through the exact same
    // composition (composeProjectedScripture) and rendering
    // (LiveAppearancePreview/SlideRenderer) that "Send Selected" and
    // double-click actually use, so what's shown here is guaranteed to
    // match what goes out.
    m_preview = new LiveAppearancePreview(this);
    auto *previewPanel = new QWidget(this);
    auto *previewPanelLayout = new QVBoxLayout(previewPanel);
    previewPanelLayout->setContentsMargins(0, 0, 0, 0);
    previewPanelLayout->addWidget(new QLabel(tr("Live Appearance"), previewPanel));
    previewPanelLayout->addWidget(m_preview, 1);

    auto *bodySplitter = new QSplitter(this);
    bodySplitter->addWidget(m_bookTree);
    bodySplitter->addWidget(chapterPanel);
    bodySplitter->addWidget(versePanel);
    bodySplitter->addWidget(previewPanel);
    bodySplitter->setStretchFactor(0, 2);
    bodySplitter->setStretchFactor(1, 2);
    bodySplitter->setStretchFactor(2, 4);
    bodySplitter->setStretchFactor(3, 3);

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
        m_translationButton->setEnabled(false);
        m_plusButton->setEnabled(false);
        m_bookTree->setEnabled(false);
        m_chapterGrid->setEnabled(false);
        m_verseList->setEnabled(false);
        m_sendSelectedButton->setEnabled(false);
        m_footerLabel->setText(m_library.lastError());
        m_preview->clearSlide();
        return;
    }

    populateTranslations();
}

ScripturePanel::~ScripturePanel() = default;

QString ScripturePanel::currentTranslationCode() const
{
    return m_currentTranslationCode;
}

void ScripturePanel::populateTranslations()
{
    const auto translations = m_library.translations();
    if (translations.isEmpty()) {
        m_footerLabel->setText(tr("No translations found in the Scripture database."));
        rebuildTranslationMenu();
        return;
    }

    m_currentTranslationCode = translations.first().code;
    m_searchBox->setLibrary(&m_library, m_currentTranslationCode);
    rebuildTranslationMenu();
    populateBookTree();
}

void ScripturePanel::selectTranslation(const QString &code)
{
    if (code.isEmpty() || code == m_currentTranslationCode)
        return;
    m_currentTranslationCode = code;
    m_searchBox->setTranslationCode(code);
    rebuildTranslationMenu();
    populateBookTree();
}

void ScripturePanel::rebuildTranslationMenu()
{
    m_translationMenu->clear();
    const auto translations = m_library.translations();
    for (const auto &translation : translations) {
        QAction *action = m_translationMenu->addAction(translation.code);
        action->setCheckable(true);
        action->setChecked(translation.code == m_currentTranslationCode);
        action->setToolTip(translation.name);
        const QString code = translation.code;
        connect(action, &QAction::triggered, this, [this, code]() { selectTranslation(code); });
    }

    if (!m_customFolders.isEmpty()) {
        m_translationMenu->addSeparator();
        QAction *header = m_translationMenu->addAction(tr("MY COLLECTIONS"));
        header->setEnabled(false);
        for (const QString &folder : m_customFolders) {
            QAction *folderAction = m_translationMenu->addAction(QStringLiteral("    %1").arg(folder));
            folderAction->setEnabled(false); // organizational placeholder; assigning translations isn't wired up yet
        }
    }

    m_translationMenu->addSeparator();
    QAction *more = m_translationMenu->addAction(tr("More Available..."));
    connect(more, &QAction::triggered, this, &ScripturePanel::openTranslationLibraryDialog);

    m_translationButton->setText(m_currentTranslationCode);
    for (const auto &translation : translations) {
        if (translation.code == m_currentTranslationCode) {
            m_translationButton->setToolTip(translation.name);
            break;
        }
    }
}

void ScripturePanel::rebuildPlusMenu()
{
    m_plusMenu->clear();

    auto addFolderAction = [this](const QString &label) {
        QAction *action = m_plusMenu->addAction(label);
        connect(action, &QAction::triggered, this, [this, label]() {
            bool ok = false;
            const QString name =
                QInputDialog::getText(this, label, tr("Name:"), QLineEdit::Normal, QString(), &ok);
            if (ok && !name.trimmed().isEmpty()) {
                m_customFolders << name.trimmed();
                rebuildTranslationMenu();
            }
        });
    };
    addFolderAction(tr("New Folder"));
    addFolderAction(tr("New Collection"));
    addFolderAction(tr("New My Folder"));
    addFolderAction(tr("New My Collection"));

    m_plusMenu->addSeparator();
    QAction *addFromDisk = m_plusMenu->addAction(IconFactory::diskImport(16), tr("Add Bible from Disk..."));
    connect(addFromDisk, &QAction::triggered, this, &ScripturePanel::promptAddBibleFromDisk);
}

void ScripturePanel::openTranslationLibraryDialog()
{
    QStringList installedCodes;
    for (const auto &translation : m_library.translations())
        installedCodes << translation.code;

    auto *dialog = new TranslationLibraryDialog(&m_library, installedCodes, this);
    QString lastAdded;
    connect(dialog, &TranslationLibraryDialog::translationAdded, this,
            [&lastAdded](const QString &code) { lastAdded = code; });
    dialog->exec();
    dialog->deleteLater();

    if (!lastAdded.isEmpty()) {
        rebuildTranslationMenu();
        selectTranslation(lastAdded);
    }
}

void ScripturePanel::promptAddBibleFromDisk()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Add Bible from Disk"), QString(), tr("Bible JSON files (*.json);;All files (*)"));
    if (path.isEmpty())
        return;

    QString code, error;
    if (!m_library.importTranslationFromJsonFile(path, &code, &error)) {
        QMessageBox::warning(this, tr("Couldn't Add Bible"), error);
        return;
    }

    rebuildTranslationMenu();
    selectTranslation(code);
    m_footerLabel->setText(tr("%1 added from disk.").arg(code));
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
    if (m_verseSortOrder == Qt::DescendingOrder)
        sortVerseList();
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
    QString book;
    int chapter = 0;
    int verseStart = 0;
    int verseEnd = 0;
    if (!resolveSelectionRange(&book, &chapter, &verseStart, &verseEnd))
        return;
    sendVerses(book, chapter, verseStart, verseEnd);
}

bool ScripturePanel::resolveSelectionRange(QString *outBook, int *outChapter, int *outVerseStart,
                                            int *outVerseEnd) const
{
    const QList<QListWidgetItem *> selected = m_verseList->selectedItems();
    if (selected.isEmpty())
        return false;

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

    *outBook = book;
    *outChapter = chapter;
    if (!sameBookChapter) {
        // Discontiguous multi-book selection (only reachable from search
        // results): a single combined slide doesn't make sense, so this
        // resolves to just the first item -- the operator can send the
        // rest individually. Both the real send and the preview follow
        // this same rule, so the preview never implies more will be sent
        // than actually will be.
        *outVerseStart = ordered.first()->data(kVerseNumRole).toInt();
        *outVerseEnd = *outVerseStart;
        return true;
    }

    *outVerseStart = ordered.first()->data(kVerseNumRole).toInt();
    *outVerseEnd = ordered.last()->data(kVerseNumRole).toInt();
    return true;
}

bool ScripturePanel::composeVerses(const QString &book, int chapter, int verseStart, int verseEnd,
                                    QString *outReference, QString *outText) const
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
        return false;

    const QString reference = (verseEnd > verseStart)
        ? QStringLiteral("%1 %2:%3-%4").arg(book).arg(chapter).arg(verseStart).arg(verseEnd)
        : QStringLiteral("%1 %2:%3").arg(book).arg(chapter).arg(verseStart);

    // The RAW verse text (no footnote) -- this is what scriptureActivated
    // has always carried; OperatorWindow::onScriptureActivated is what
    // appends the "Reference (TRANSLATION)" footnote via
    // composeProjectedScripture before actually projecting it. See
    // updatePreview() below for how the preview mirrors that same
    // second step so it shows exactly what will be projected, not just
    // the raw verse body.
    if (outReference)
        *outReference = reference;
    if (outText)
        *outText = lines.join(QStringLiteral("\n"));
    return true;
}

void ScripturePanel::sendVerses(const QString &book, int chapter, int verseStart, int verseEnd)
{
    QString reference;
    QString text;
    if (!composeVerses(book, chapter, verseStart, verseEnd, &reference, &text))
        return;

    emit scriptureActivated(reference, text, currentTranslationCode());
}

void ScripturePanel::onVerseSelectionChanged()
{
    updatePreview();
}

void ScripturePanel::updatePreview()
{
    QString book;
    int chapter = 0;
    int verseStart = 0;
    int verseEnd = 0;
    if (!resolveSelectionRange(&book, &chapter, &verseStart, &verseEnd)) {
        m_preview->clearSlide();
        return;
    }

    QString reference;
    QString text;
    if (!composeVerses(book, chapter, verseStart, verseEnd, &reference, &text)) {
        m_preview->clearSlide();
        return;
    }

    // Same composeProjectedScripture() call OperatorWindow::onScriptureActivated
    // makes for the real send -- see src/core/scripture/ScriptureFormatting.h.
    const QString projected = composeProjectedScripture(text, reference, currentTranslationCode());
    m_preview->setSlide(Slide(reference, projected, Qt::black));
}

void ScripturePanel::setOutputProfile(const OutputProfile &profile)
{
    m_preview->setProfile(profile);
}

void ScripturePanel::onLiveWordQuery(const QString &text)
{
    // Fired by ScriptureSearchEdit whenever it's in word/sentence-search
    // mode (i.e. what's typed doesn't match the start of any book name).
    // Empty text means "back out of search-results mode" -- either the
    // box was cleared, or it just switched into reference mode instead
    // (which will drive the view itself via onSearchSubmitted/Enter).
    if (text.trimmed().isEmpty()) {
        if (m_searchResultsActive) {
            if (!m_currentBook.isEmpty())
                populateVerseList(m_currentBook, m_currentChapter);
            else
                m_verseList->clear();
            updateFooter();
        }
        return;
    }

    if (!m_libraryAvailable)
        return;

    m_lastSearchQuery = text;
    showSearchResults(m_library.search(currentTranslationCode(), text), text);
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
        m_searchBox->commitReference(formatReferenceDisplay(ref));
        return;
    }

    // Not a resolvable reference -- live word search already covers this
    // as the person types; Enter just re-runs it against the current text.
    m_lastSearchQuery = text;
    showSearchResults(m_library.search(currentTranslationCode(), text), text);
}

void ScripturePanel::onWordWrapToggleRequested(bool wordWrap)
{
    m_verseList->setWordWrap(wordWrap);
}

void ScripturePanel::onSortOrderChangeRequested(Qt::SortOrder order)
{
    m_verseSortOrder = order;
    sortVerseList();
}

void ScripturePanel::sortVerseList()
{
    QList<QListWidgetItem *> items;
    while (m_verseList->count() > 0)
        items << m_verseList->takeItem(0);

    std::stable_sort(items.begin(), items.end(), [this](QListWidgetItem *a, QListWidgetItem *b) {
        const int keyA = a->data(kChapterRole).toInt() * 1000 + a->data(kVerseNumRole).toInt();
        const int keyB = b->data(kChapterRole).toInt() * 1000 + b->data(kVerseNumRole).toInt();
        return m_verseSortOrder == Qt::AscendingOrder ? keyA < keyB : keyA > keyB;
    });

    for (auto *item : items)
        m_verseList->addItem(item);
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
    if (m_verseSortOrder == Qt::DescendingOrder)
        sortVerseList();

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
