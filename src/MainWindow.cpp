#include "MainWindow.h"
#include "Theme.h"
#include "TopToolBar.h"
#include "LiveTranscriptionPanel.h"
#include "PreviewLivePanel.h"
#include "CitationFeedPanel.h"
#include "LibraryPanel.h"
#include "QueuePanel.h"
#include "StatusBarWidget.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// Small helper: wraps a widget with consistent outer margins so panels don't
// touch the splitter handles directly.
QWidget* padded(QWidget* inner, int margin = 12)
{
    auto* wrapper = new QWidget;
    auto* layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->addWidget(inner);
    return wrapper;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("SanctifyLive");
    resize(1440, 860);

    auto* central = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // --- Top toolbar --------------------------------------------------
    rootLayout->addWidget(new TopToolBar(central));

    // --- Top row: transcription | preview/live | citation feed --------
    auto* topRow = new QSplitter(Qt::Horizontal, central);
    topRow->setChildrenCollapsible(false);
    topRow->addWidget(padded(new LiveTranscriptionPanel));
    topRow->addWidget(padded(new PreviewLivePanel));
    topRow->addWidget(padded(new CitationFeedPanel(/*redVariant=*/false)));
    topRow->setStretchFactor(0, 2);
    topRow->setStretchFactor(1, 5);
    topRow->setStretchFactor(2, 2);
    topRow->setSizes({260, 620, 260});

    // --- Bottom row: library | queue | red transcript ------------------
    auto* bottomRow = new QSplitter(Qt::Horizontal, central);
    bottomRow->setChildrenCollapsible(false);
    bottomRow->addWidget(padded(new LibraryPanel, 0));
    bottomRow->addWidget(padded(new QueuePanel));
    bottomRow->addWidget(padded(new CitationFeedPanel(/*redVariant=*/true)));
    bottomRow->setStretchFactor(0, 4);
    bottomRow->setStretchFactor(1, 2);
    bottomRow->setStretchFactor(2, 2);
    bottomRow->setSizes({620, 260, 260});

    // --- Vertical splitter joining the two rows ------------------------
    auto* mainSplitter = new QSplitter(Qt::Vertical, central);
    mainSplitter->setChildrenCollapsible(false);
    mainSplitter->addWidget(topRow);
    mainSplitter->addWidget(bottomRow);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 3);
    mainSplitter->setSizes({320, 460});

    rootLayout->addWidget(mainSplitter, 1);

    // --- Status bar -----------------------------------------------------
    rootLayout->addWidget(new StatusBarWidget(central));

    setCentralWidget(central);
}
