#pragma once
//
// PreviewLivePanel.h
// Center "Preview" / "Live" slide panels shown side by side in a splitter
// so the user can resize each independently.
//
#include <QWidget>

class QLabel;

class PreviewLivePanel : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewLivePanel(QWidget* parent = nullptr);

private:
    QWidget* buildSlideBox(const QString& headerText, const QColor& headerColor,
                            const QString& reference, const QString& body);
};
