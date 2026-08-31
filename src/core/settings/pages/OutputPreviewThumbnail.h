#ifndef SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTPREVIEWTHUMBNAIL_H_
#define SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTPREVIEWTHUMBNAIL_H_

#include <QWidget>

// The small "what will actually go out" thumbnail that sits under the
// category list in SettingsWindow's left column, regardless of which
// category is selected. It just draws a handful of sample lines over a
// dark background plus a resolution badge -- enough to reassure the
// operator that an output is configured, without wiring up a live
// texture from OutputWindow (that's a natural follow-up once this
// widget's placement/behavior is settled).
class OutputPreviewThumbnail : public QWidget
{
    Q_OBJECT

public:
    explicit OutputPreviewThumbnail(QWidget *parent = nullptr);

    QSize sizeHint() const override;

public slots:
    // Text shown as the sample slide body, and the resolution badge in
    // the corner (e.g. "1920x1080"). Called by whichever OutputSettingsPage
    // currently has focus, so the thumbnail always reflects the last
    // output the operator was editing.
    void setSampleLines(const QStringList &lines);
    void setResolutionLabel(const QString &label);
    void setEnabledLook(bool enabled); // dims the thumbnail when monitorIndex == -1 ("None")

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList m_sampleLines{
        tr("song text line one"),
        tr("song text line two"),
        tr("song text line three"),
        tr("song text line four"),
    };
    QString m_resolutionLabel;
    bool m_enabledLook = true;
};

#endif // SANCTIFYLIVE_CORE_SETTINGS_PAGES_OUTPUTPREVIEWTHUMBNAIL_H_
