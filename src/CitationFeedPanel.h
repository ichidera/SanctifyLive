#pragma once
//
// CitationFeedPanel.h
// Right-hand scrolling feed of recently displayed citations (used for both
// the top "history" list and the bottom red full-passage transcript, since
// both are the same kind of scrollable card list with different styling).
//
#include <QWidget>

class QListWidget;

class CitationFeedPanel : public QWidget
{
    Q_OBJECT
public:
    // If redVariant is true, renders plain red numbered verse lines like the
    // bottom-right panel; otherwise renders bordered citation cards like the
    // top-right panel.
    explicit CitationFeedPanel(bool redVariant, QWidget* parent = nullptr);

private:
    QListWidget* m_list = nullptr;
    void populateCardVariant();
    void populateRedVariant();
};
