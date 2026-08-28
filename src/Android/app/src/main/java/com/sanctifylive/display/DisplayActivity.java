package com.sanctifylive.display;

import android.app.AlertDialog;
import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import com.sanctifylive.display.databinding.ActivityDisplayBinding;

/**
 * DisplayActivity — the single Activity of the SanctifyLive Display app.
 *
 * Responsibilities:
 *   1. Lock the screen to landscape and go fully immersive (no status bar,
 *      no nav bar, no notifications breaking through).
 *   2. Keep the screen on — this tablet lives at the pulpit.
 *   3. Own the SlideClient lifecycle (start on resume, stop on destroy).
 *   4. Deliver slide frames to SlideView.
 *   5. Provide a triple-tap gesture to surface the settings dialog,
 *      which auto-hides after 5 seconds so it never lingers.
 *
 * Extends plain Activity rather than AppCompatActivity: nothing here uses
 * an ActionBar or any other AppCompat-specific API, and build.gradle has
 * no third-party dependencies on purpose (see its comment) -- pulling in
 * AndroidX just for the base class isn't worth it.
 */
public class DisplayActivity extends Activity implements SlideClient.Listener {

    // ── Constants ─────────────────────────────────────────────────────────

    private static final int FAB_AUTO_HIDE_MS = 5_000;
    private static final int TAP_THRESHOLD    = 3;       // triple-tap to show settings

    // ── Fields ────────────────────────────────────────────────────────────

    private ActivityDisplayBinding binding;
    private SlideClient            client;
    private Handler                handler;

    private int  tapCount   = 0;
    private long lastTapMs  = 0;

    private final Runnable hideFab = () -> {
        binding.fabSettings.setVisibility(View.GONE);
        tapCount = 0;
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Keep screen on — tablet is always-on display
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Inflate
        binding = ActivityDisplayBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        handler = new Handler(Looper.getMainLooper());

        // Lock landscape
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);

        // Immersive fullscreen
        enterImmersiveMode();

        // Triple-tap anywhere → settings. Deliberately NOT using
        // GestureDetector.onSingleTapConfirmed here: that callback exists
        // to disambiguate a single tap from the start of a double-tap, so
        // when taps arrive quickly (exactly what a real "triple tap"
        // looks like), the detector's own double-tap window can absorb
        // taps 1+2 as a double-tap and never report them as confirmed
        // singles -- tapCount then never reliably reaches 3. Counting raw
        // ACTION_DOWN events ourselves is deterministic regardless of tap speed.
        binding.getRoot().setOnTouchListener((v, event) -> {
            if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
                handleTap();
            }
            return true;
        });

        // FAB → settings dialog
        binding.fabSettings.setOnClickListener(v -> showSettingsDialog());

        // Re-enter immersive whenever window focus returns
        // (e.g. after a dialog is dismissed, which briefly restores bars)
        getWindow().getDecorView().setOnSystemUiVisibilityChangeListener(
                visibility -> enterImmersiveMode());

        // Start the client
        startClient();
    }

    @Override
    protected void onResume() {
        super.onResume();
        enterImmersiveMode();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (client != null) client.stop();
        handler.removeCallbacksAndMessages(null);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) enterImmersiveMode();
    }

    // ── SlideClient.Listener ──────────────────────────────────────────────

    @Override
    public void onConnected() {
        binding.tvStatus.setText(R.string.status_connected);
        binding.tvStatus.setTextColor(Color.parseColor("#CC44FF44"));
        // Hide status after 3 s — no need to clutter the display
        handler.postDelayed(() -> binding.tvStatus.setVisibility(View.GONE), 3_000);
    }

    @Override
    public void onDisconnected() {
        binding.tvStatus.setVisibility(View.VISIBLE);
        binding.tvStatus.setText(R.string.status_reconnecting);
        binding.tvStatus.setTextColor(Color.parseColor("#CCFF4444"));
    }

    @Override
    public void onSlide(String text, int bg, int fg) {
        binding.slideView.setSlide(text, bg, fg);
    }

    @Override
    public void onBlackout() {
        binding.slideView.setBlackout();
    }

    // ── Immersive fullscreen ───────────────────────────────────────────────

    private void enterImmersiveMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            // API 30+
            WindowInsetsController ctrl = getWindow().getInsetsController();
            if (ctrl != null) {
                ctrl.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                ctrl.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            // API 26-29 fallback
            //noinspection deprecation
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        }
    }

    // ── Triple-tap gesture ────────────────────────────────────────────────

    private void handleTap() {
        long now = System.currentTimeMillis();
        if (now - lastTapMs > 1_500) tapCount = 0;   // reset if too slow
        lastTapMs = now;
        tapCount++;

        if (tapCount >= TAP_THRESHOLD) {
            tapCount = 0;
            handler.removeCallbacks(hideFab);
            binding.fabSettings.setVisibility(View.VISIBLE);
            handler.postDelayed(hideFab, FAB_AUTO_HIDE_MS);
        }
    }

    // ── Settings dialog ───────────────────────────────────────────────────

    private void showSettingsDialog() {
        View dialogView = getLayoutInflater().inflate(R.layout.dialog_settings, null);
        EditText etHost = dialogView.findViewById(R.id.etHost);
        EditText etPort = dialogView.findViewById(R.id.etPort);

        etHost.setText(Prefs.getHost(this));
        etPort.setText(String.valueOf(Prefs.getPort(this)));

        new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog)
                .setTitle(R.string.app_name)
                .setView(dialogView)
                .setPositiveButton("Connect", (d, which) -> {
                    String host = etHost.getText().toString().trim();
                    String portStr = etPort.getText().toString().trim();
                    if (host.isEmpty()) host = "127.0.0.1";
                    int port;
                    try { port = Integer.parseInt(portStr); }
                    catch (NumberFormatException e) { port = 55432; }

                    Prefs.save(this, host, port);
                    reconnectClient(host, port);
                    dismissKeyboard(dialogView);
                })
                .setNegativeButton("Cancel", (d, which) -> dismissKeyboard(dialogView))
                .setOnDismissListener(d -> {
                    // Re-enter immersive — dialog briefly restored bars
                    handler.postDelayed(this::enterImmersiveMode, 200);
                    binding.fabSettings.setVisibility(View.GONE);
                })
                .show();
    }

    // ── Client management ─────────────────────────────────────────────────

    private void startClient() {
        String host = Prefs.getHost(this);
        int    port = Prefs.getPort(this);
        client = new SlideClient(host, port, this);
        client.start();
    }

    private void reconnectClient(String host, int port) {
        if (client != null) {
            client.reconnect(host, port);
        }
        binding.tvStatus.setVisibility(View.VISIBLE);
        binding.tvStatus.setText(R.string.status_reconnecting);
    }

    // ── Misc ──────────────────────────────────────────────────────────────

    private void dismissKeyboard(View v) {
        InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null) imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
    }

    /** Block the back button — operators must not accidentally exit. */
    @Override
    public void onBackPressed() {
        // Intentionally do nothing.
        // If you ever need to exit: use the FAB → settings → force-stop in Android settings.
    }
}