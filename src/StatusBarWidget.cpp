#include "StatusBarWidget.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVariant>

StatusBarWidget::StatusBarWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(30);
    setStyleSheet(QString("background-color:%1; border-top:1px solid %2;")
                      .arg(Theme::kBgPanel, Theme::kBorder));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 2, 12, 2);

    auto* dot = new QLabel("\xE2\x97\x8F");
    dot->setStyleSheet(QString("color:%1;").arg(Theme::kAccentGreen));
    layout->addWidget(dot);

    auto* info = new QLabel("Stage Display: 192.168.0.11:55432   \xE2\x80\xA2   1 device connected");
    info->setProperty("role", QVariant("dim"));
    layout->addWidget(info);

    layout->addStretch(1);

    auto* wake = new QPushButton("\xE2\x9A\xA1 Wake Display");
    layout->addWidget(wake);
}
