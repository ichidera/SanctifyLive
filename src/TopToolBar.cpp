#include "TopToolBar.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QFrame>
#include <QVariant>

TopToolBar::TopToolBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(56);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(12, 6, 12, 6);
    root->setSpacing(10);

    // --- Brand -----------------------------------------------------------
    auto* brandIcon = new QLabel("[+]");
    brandIcon->setStyleSheet(QString("color:%1; font-weight:700;").arg(Theme::kAccentGreen));
    auto* brand = new QLabel("SanctifyLive");
    brand->setProperty("role", QVariant("brand"));
    root->addWidget(brandIcon);
    root->addWidget(brand);

    auto* sep1 = new QFrame;
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet(QString("color:%1;").arg(Theme::kBorder));
    root->addSpacing(8);
    root->addWidget(sep1);
    root->addSpacing(8);

    // --- File actions ------------------------------------------------------
    const struct { const char* icon; const char* label; } fileActions[] = {
        {"\xF0\x9F\x93\x84", "New"},
        {"\xF0\x9F\x93\x82", "Open"},
        {"\xF0\x9F\x92\xBE", "Save"},
        {"\xF0\x9F\x9B\x92", "Store"},
        {"\xF0\x9F\x8C\x90", "Web"},
        {"\xF0\x9F\x93\xA1", "Remote"},
    };
    for (const auto& a : fileActions) {
        addAction(this, QString::fromUtf8(a.icon), QString::fromUtf8(a.label));
    }

    root->addStretch(1);

    // --- Right cluster -----------------------------------------------------
    auto* goLive = new QPushButton("\xE2\x96\xB6  Go Live");
    goLive->setProperty("role", QVariant("goLive"));
    root->addWidget(goLive);

    addStatePill(this, "\xF0\x9F\x94\x94 Alerts", false);
    addStatePill(this, "\xF0\x9F\x96\xBC Logo", false);
    addStatePill(this, "\xE2\x97\x8F Black", true);
    addStatePill(this, "\xE2\x9C\x96 Clear", false);
    addStatePill(this, "\xE2\x97\x8F Live", true);
}

QToolButton* TopToolBar::addAction(QWidget* container, const QString& icon, const QString& label)
{
    auto* btn = new QToolButton(container);
    btn->setText(icon + " " + label);
    btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    container->layout()->addWidget(btn);
    return btn;
}

QPushButton* TopToolBar::addStatePill(QWidget* container, const QString& text, bool filled)
{
    auto* btn = new QPushButton(text, container);
    if (filled) {
        btn->setStyleSheet(QString(
            "QPushButton { background-color:%1; color:white; border:none; font-weight:600; }")
            .arg(Theme::kAccentBlue));
    }
    container->layout()->addWidget(btn);
    return btn;
}
