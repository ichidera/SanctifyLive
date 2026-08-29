package com.sanctifylive.display;

import android.content.Context;
import android.graphics.Bitmap;
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
 *   - Fill entire surface with slide background color, or a background
 *     image (cover-fit, cropped to fill) when the frame carried one.
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
    private Bitmap  bgImage  = null;   // null == no image; draw solid bgColor instead
    // Normalized (0..1) point in bgImage that should stay in frame when
    // this view's aspect ratio doesn't match the image's -- mirrors
    // Slide::backgroundFocus / the wire protocol's fx/fy fields. Default
    // (0.5, 0.5) reproduces the old always-center-crop behavior exactly.
    private float   focusX   = 0.5f;
    private float   focusY   = 0.5f;

    // ── Paint ─────────────────────────────────────────────────────────────
    private final Paint bgPaint   = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint txtPaint  = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint imagePaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final Rect  drawRect  = new Rect();
    private final Rect  imageSrcRect = new Rect();
    private final Rect  imageDstRect = new Rect();

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

    /**
     * Called when the server pushes a new live slide. backgroundImage is
     * null unless the frame carried one (see PROTOCOL.md's "image"
     * field) -- when present it's drawn cover-fit instead of the solid
     * backgroundColor, matching OutputWindow.cpp's paintEvent. focusX/
     * focusY (see PROTOCOL.md's fx/fy) bias which part of the image
     * survives that crop.
     */
    public void setSlide(String slideText, int backgroundColor, int foregroundColor,
                          Bitmap backgroundImage, float focusX, float focusY) {
        this.text    = slideText;
        this.bgColor = backgroundColor;
        this.fgColor = foregroundColor;
        this.bgImage = backgroundImage;
        this.focusX  = focusX;
        this.focusY  = focusY;
        postInvalidate();   // safe to call from any thread
    }

    /** Called when the server sends a blackout frame. */
    public void setBlackout() {
        this.text    = null;
        this.bgColor = Color.BLACK;
        this.bgImage = null;
        this.focusX  = 0.5f;
        this.focusY  = 0.5f;
        postInvalidate();
    }

    // ── Drawing ───────────────────────────────────────────────────────────

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        int w = getWidth();
        int h = getHeight();

        if (bgImage != null && !bgImage.isRecycled() && w > 0 && h > 0) {
            drawCoverFit(canvas, bgImage, w, h);
        } else {
            // Fill background (black if blackout, slide color otherwise)
            bgPaint.setColor(bgColor);
            canvas.drawRect(0, 0, w, h, bgPaint);
        }

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
     * Draws bgImage scaled to fill the w×h surface, cropping any
     * overflow -- CSS background-size: cover, same idea as
     * OutputWindow.cpp's Qt::KeepAspectRatioByExpanding. The crop is
     * biased toward (focusX, focusY) instead of always dead-centering,
     * using the exact same sizing+clamping formula as OutputWindow.cpp
     * and ImageFramingDialog.cpp so the operator's "Edit Framing..."
     * preview matches what actually lands on screen here.
     */
    private void drawCoverFit(Canvas canvas, Bitmap bitmap, int w, int h) {
        float bitmapAspect = (float) bitmap.getWidth() / bitmap.getHeight();
        float viewAspect = (float) w / h;

        int srcW, srcH;
        if (bitmapAspect > viewAspect) {
            // Bitmap is relatively wider than the view -- crop its sides.
            srcH = bitmap.getHeight();
            srcW = Math.round(srcH * viewAspect);
        } else {
            // Bitmap is relatively taller than the view -- crop top/bottom.
            srcW = bitmap.getWidth();
            srcH = Math.round(srcW / viewAspect);
        }

        // Whichever axis has no slack (srcW == bitmap width, or srcH ==
        // bitmap height) naturally clamps to 0 below regardless of focus,
        // so this doesn't need a separate branch per axis.
        int maxX = Math.max(0, bitmap.getWidth() - srcW);
        int maxY = Math.max(0, bitmap.getHeight() - srcH);
        int srcX = clamp(Math.round(focusX * bitmap.getWidth() - srcW / 2f), 0, maxX);
        int srcY = clamp(Math.round(focusY * bitmap.getHeight() - srcH / 2f), 0, maxY);

        imageSrcRect.set(srcX, srcY, srcX + srcW, srcY + srcH);
        imageDstRect.set(0, 0, w, h);
        canvas.drawBitmap(bitmap, imageSrcRect, imageDstRect, imagePaint);
    }

    private static int clamp(int value, int min, int max) {
        return Math.max(min, Math.min(value, max));
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
