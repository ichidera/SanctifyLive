package com.sanctifylive.display;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.view.View;

/**
 * SlideView — mirrors OutputWindow from the Qt desktop exactly.
 *
 * Rules (matching OutputWindow.cpp):
 *   - Fill entire surface with slide background color.
 *   - If text is empty, do nothing further.
 *   - Font size = max(12sp, height / 8) — bold, white, centered, word-wrapped.
 *   - No slide (blackout): fill black, nothing else.
 *
 * This view is updated from the main thread via setSlide() / setBlackout(),
 * both of which call invalidate().
 */
public class SlideView extends View {

    // ── State ─────────────────────────────────────────────────────────────
    private int     bgColor  = Color.BLACK;
    private String  text     = null;   // null == blackout / no slide
    private int     fgColor  = Color.WHITE;

    // ── Paint ─────────────────────────────────────────────────────────────
    private final Paint bgPaint  = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint txtPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Rect  drawRect = new Rect();

    // ── Construction ──────────────────────────────────────────────────────

    public SlideView(Context context) {
        super(context);
        init();
    }

    public SlideView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public SlideView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        txtPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
        txtPaint.setTextAlign(Paint.Align.CENTER);
    }

    // ── Public API ────────────────────────────────────────────────────────

    /** Called when the server pushes a new live slide. */
    public void setSlide(String slideText, int backgroundColor, int foregroundColor) {
        this.text    = slideText;
        this.bgColor = backgroundColor;
        this.fgColor = foregroundColor;
        postInvalidate();   // safe to call from any thread
    }

    /** Called when the server sends a blackout frame. */
    public void setBlackout() {
        this.text    = null;
        this.bgColor = Color.BLACK;
        postInvalidate();
    }

    // ── Drawing ───────────────────────────────────────────────────────────

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        int w = getWidth();
        int h = getHeight();

        // Fill background (black if blackout, slide color otherwise)
        bgPaint.setColor(bgColor);
        canvas.drawRect(0, 0, w, h, bgPaint);

        // No text (blackout or empty slide) — stop here
        if (text == null || text.isEmpty()) return;

        // Font size mirrors Qt: max(12, height / 8), converted to px
        float density = getResources().getDisplayMetrics().density;
        float minPx   = 12 * density;
        float sizePx  = Math.max(minPx, h / 8f);

        txtPaint.setColor(fgColor);
        txtPaint.setTextSize(sizePx);

        // Inset 40dp on all sides (matching Qt's rect().adjusted(40,40,-40,-40))
        int pad = (int)(40 * density);
        drawRect.set(pad, pad, w - pad, h - pad);

        drawTextWrapped(canvas, text, drawRect, txtPaint);
    }

    /**
     * Draws word-wrapped, vertically-centred text inside rect.
     * Android's StaticLayout is the proper tool; we use a manual
     * multi-line approach so we have zero extra dependencies.
     */
    private void drawTextWrapped(Canvas canvas, String rawText, Rect rect, Paint paint) {
        // Split on explicit newlines first, then word-wrap each line
        String[] paragraphs = rawText.split("\n", -1);
        float lineHeight = paint.getFontSpacing();
        int availWidth = rect.width();

        // Build final list of display lines
        java.util.List<String> lines = new java.util.ArrayList<>();
        for (String para : paragraphs) {
            if (para.isEmpty()) {
                lines.add("");
                continue;
            }
            // Word-wrap
            String[] words = para.split(" ", -1);
            StringBuilder current = new StringBuilder();
            for (String word : words) {
                String candidate = current.length() == 0 ? word : current + " " + word;
                if (paint.measureText(candidate) <= availWidth) {
                    current = new StringBuilder(candidate);
                } else {
                    if (current.length() > 0) lines.add(current.toString());
                    current = new StringBuilder(word);
                }
            }
            if (current.length() > 0) lines.add(current.toString());
        }

        float totalHeight = lines.size() * lineHeight;
        // Vertical centre within rect
        float startY = rect.top + (rect.height() - totalHeight) / 2f + lineHeight * 0.8f;
        float centreX = rect.left + rect.width() / 2f;

        for (String line : lines) {
            canvas.drawText(line, centreX, startY, paint);
            startY += lineHeight;
        }
    }
}
