package com.sanctifylive.display;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * SlideClient manages the TCP connection to the SanctifyLive desktop server.
 *
 * Protocol: newline-delimited JSON (see PROTOCOL.md).
 *
 * Threading model:
 *   - All network I/O runs on a single daemon thread created here.
 *   - Callbacks are delivered on the main thread via Handler.
 *   - The caller never touches a Socket directly.
 *
 * Reconnection:
 *   - If the connection drops, we wait RECONNECT_DELAY_MS and try again.
 *   - Call stop() to permanently shut down (e.g. Activity.onDestroy).
 */
public class SlideClient {

    private static final String TAG               = "SlideClient";
    private static final int    CONNECT_TIMEOUT   = 5_000;  // ms
    private static final int    RECONNECT_DELAY   = 3_000;  // ms

    // ── Callback interface ────────────────────────────────────────────────

    public interface Listener {
        void onConnected();
        void onDisconnected();
        /** bg and fg are packed ARGB ints (Color.parseColor output). */
        void onSlide(String text, int bg, int fg);
        void onBlackout();
    }

    // ── Fields ────────────────────────────────────────────────────────────

    private final Handler      mainHandler = new Handler(Looper.getMainLooper());
    private final AtomicBoolean running    = new AtomicBoolean(false);

    private volatile String host;
    private volatile int    port;
    private Listener        listener;

    private Thread   ioThread;
    private Socket   socket;

    // ── Lifecycle ─────────────────────────────────────────────────────────

    public SlideClient(String host, int port, Listener listener) {
        this.host     = host;
        this.port     = port;
        this.listener = listener;
    }

    /** Start (or restart) the background I/O loop. */
    public synchronized void start() {
        if (running.getAndSet(true)) return;   // already running

        ioThread = new Thread(this::ioLoop, "SlideClient-IO");
        ioThread.setDaemon(true);
        ioThread.start();
    }

    /** Permanently stop. Do not call start() again after this. */
    public synchronized void stop() {
        running.set(false);
        closeSocket();
        if (ioThread != null) {
            ioThread.interrupt();
            ioThread = null;
        }
    }

    /** Reconnect to a new host/port (e.g. user changed settings). */
    public void reconnect(String newHost, int newPort) {
        this.host = newHost;
        this.port = newPort;
        closeSocket();   // causes the read loop to throw, triggering reconnect
    }

    // ── I/O loop ──────────────────────────────────────────────────────────

    private void ioLoop() {
        while (running.get()) {
            try {
                connectAndRead();
            } catch (Exception e) {
                if (running.get()) {
                    Log.d(TAG, "Connection lost — " + e.getMessage());
                    fireDisconnected();
                    sleep(RECONNECT_DELAY);
                }
            }
        }
    }

    private void connectAndRead() throws IOException {
        Socket s = new Socket();
        s.setTcpNoDelay(true);
        s.setSoTimeout(10_000);   // 10 s read timeout; server pings every 2 s
        s.connect(new InetSocketAddress(host, port), CONNECT_TIMEOUT);

        synchronized (this) { socket = s; }
        fireConnected();

        BufferedReader reader = new BufferedReader(
                new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));

        String line;
        while ((line = reader.readLine()) != null) {
            if (!running.get()) break;
            parseLine(line.trim());
        }

        closeSocket();
        fireDisconnected();
    }

    private void parseLine(String line) {
        if (line.isEmpty()) return;
        try {
            JSONObject obj = new JSONObject(line);
            String type = obj.optString("type", "");
            switch (type) {
                case "slide": {
                    String text = obj.optString("text", "");
                    String bgStr = obj.optString("bg", "#000000");
                    String fgStr = obj.optString("fg", "#ffffff");
                    int bg = parseColor(bgStr, 0xFF000000);
                    int fg = parseColor(fgStr, 0xFFFFFFFF);
                    fireSlide(text, bg, fg);
                    break;
                }
                case "blackout":
                    fireBlackout();
                    break;
                case "ping":
                    // keepalive — no action needed
                    break;
                default:
                    Log.w(TAG, "Unknown frame type: " + type);
            }
        } catch (Exception e) {
            Log.w(TAG, "Bad frame: " + line + " — " + e.getMessage());
        }
    }

    // ── Helpers ───────────────────────────────────────────────────────────

    private synchronized void closeSocket() {
        if (socket != null) {
            try { socket.close(); } catch (IOException ignored) {}
            socket = null;
        }
    }

    private static void sleep(long ms) {
        try { Thread.sleep(ms); } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
    }

    private static int parseColor(String hex, int fallback) {
        try {
            return android.graphics.Color.parseColor(hex);
        } catch (Exception e) {
            return fallback;
        }
    }

    // ── Callbacks (always on main thread) ─────────────────────────────────

    private void fireConnected()  { mainHandler.post(() -> { if (listener != null) listener.onConnected(); }); }
    private void fireDisconnected() { mainHandler.post(() -> { if (listener != null) listener.onDisconnected(); }); }
    private void fireBlackout()   { mainHandler.post(() -> { if (listener != null) listener.onBlackout(); }); }
    private void fireSlide(String text, int bg, int fg) {
        mainHandler.post(() -> { if (listener != null) listener.onSlide(text, bg, fg); });
    }
}
