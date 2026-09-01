#ifndef SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREFORMATTING_H_
#define SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREFORMATTING_H_

#include <QString>

// Builds the actual text that gets projected for a scripture selection --
// the verse body plus a "Reference (TRANSLATION)" footnote, the way most
// projection software footnotes Scripture.
//
// This used to be inlined once, in OperatorWindow::onScriptureActivated,
// which was fine when that was the only place it happened. Now that
// ScripturePanel also needs to build the same string for its live-appearance
// preview (see ScripturePanel::updatePreview), inlining it twice would mean
// two copies that are only accidentally identical -- exactly the kind of
// drift that would make the preview lie about what "Send Selected" actually
// puts on screen. Callers on both sides should use this and nothing else.
inline QString composeProjectedScripture(const QString &verseText, const QString &reference,
                                          const QString &translationCode)
{
    return QStringLiteral("%1\n\n%2 (%3)").arg(verseText, reference, translationCode);
}

#endif // SANCTIFYLIVE_CORE_SCRIPTURE_SCRIPTUREFORMATTING_H_
