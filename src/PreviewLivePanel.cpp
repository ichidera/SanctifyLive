#include "PreviewLivePanel.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QFrame>

PreviewLivePanel::PreviewLivePanel(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    const QString verseBody =
        "1.  He that dwelleth in the secret place of the most High shall "
        "abide under the shadow of the Almighty.";

    auto* previewBox = buildSlideBox("Preview", QColor(Theme::kTextPrimary),
                                      "Pslam 91:1[NLT]", verseBody);
    auto* liveBox = buildSlideBox("Live", QColor(Theme::kAccentGreen),
                                   "Pslam 91:1[NLT]", verseBody);

    splitter->addWidget(previewBox);
    splitter->addWidget(liveBox);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    outer->addWidget(splitter);
}

QWidget* PreviewLivePanel::buildSlideBox(const QString& headerText, const QColor& headerColor,
                                          const QString& reference, const QString& body)
{
    auto* box = new QWidget;
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 8);
    layout->setSpacing(8);

    auto* header = new QLabel(headerText);
    header->setStyleSheet(QString("color:%1; font-size:15px; font-weight:700;")
                               .arg(headerColor.name()));
    layout->addWidget(header);

    auto* slideFrame = new QFrame;
    slideFrame->setStyleSheet(QString(
        "background-color:%1; border:1px solid %2; border-radius:6px;")
        .arg(Theme::kBgInset, Theme::kBorder));
    slideFrame->setMinimumHeight(180);

    auto* slideLayout = new QVBoxLayout(slideFrame);
    slideLayout->setContentsMargins(20, 16, 20, 16);
    slideLayout->setSpacing(12);

    auto* refLabel = new QLabel(reference);
    refLabel->setStyleSheet(QString("color:%1; font-weight:600;").arg(Theme::kTextSecondary));
    slideLayout->addWidget(refLabel);

    auto* bodyLabel = new QLabel(body);
    bodyLabel->setWordWrap(true);
    bodyLabel->setStyleSheet("font-size:18px; font-weight:600;");
    slideLayout->addWidget(bodyLabel);
    slideLayout->addStretch(1);

    layout->addWidget(slideFrame, 1);
    return box;
}
