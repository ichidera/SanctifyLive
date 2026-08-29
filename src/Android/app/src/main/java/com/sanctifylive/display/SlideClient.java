package com.sanctifylive.display;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Handler;
import android.os.Looper;
import android.util.Base64;
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
    // Must comfortably exceed the server's actual ping interval (15 s, see
    // SlideServer.cpp's kPingIntervalMs) with room for a couple of missed
    // or delayed pings from network jitter -- otherwise a perfectly healthy,
    // idle connection gets falsely treated as dropped on a timer.
    private static final int    SOCKET_READ_TIMEOUT = 40_000;  // ms

    // ── Callback interface ────────────────────────────────────────────────

    public interface Listener {
        void onConnected();
        void onDisconnected();
        /**
         * bg and fg are packed ARGB ints (Color.parseColor output).
         * image is non-null only when the server's frame carried a
         * background image (see PROTOCOL.md); already decoded to a
         * Bitmap on the IO thread so the callback never blocks the
         * caller decoding it itself. When null, render the solid bg
         * color as before. focusX/focusY (see PROTOCOL.md's fx/fy) say
         * which part of that image should stay in frame if it has to be
         * cropped to fit -- meaningless when image is null.
         */
        void onSlide(String text, int bg, int fg, Bitmap image, float focusX, float focusY);
        void onBlackout();
        /** Force the screen on and to the front, even over the lock screen. */
        void onWake();
    }

    // ── Fields ────────────────────────────────────────────────────────────

    private final Handler      mainHandler = new Handler(Looper.getMainLooper());
    private final AtomicBoolean running    = new AtomicBoolean(false);

    private volatile String host;
    private volatile int    port;
    private volatile String role;         // Prefs.ROLE_DISPLAY or Prefs.ROLE_PHONE
    private volatile int    screenWidth;  // pixels, as currently laid out -- best-effort hint only
    private volatile int    screenHeight;
    private Listener        listener;

    private Thread   ioThread;
    private Socket   socket;

    // ── Lifecycle ─────────────────────────────────────────────────────────

    public SlideClient(String host, int port, String role, int screenWidth, int screenHeight,
                        Listener listener) {
        this.host         = host;
        this.port         = port;
        this.role         = role;
        this.screenWidth  = screenWidth;
        this.screenHeight = screenHeight;
        this.listener     = listener;
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

    /**
     * Updates what this client identifies as in its next hello. Only
     * takes effect on the next connection -- the hello is sent once per
     * connect (see PROTOCOL.md) -- so pair this with reconnect() (or a
     * fresh start()) if the change needs to reach the server right away.
     */
    public void updateDeviceInfo(String newRole, int newScreenWidth, int newScreenHeight) {
        this.role         = newRole;
        this.screenWidth  = newScreenWidth;
        this.screenHeight = newScreenHeight;
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
        s.setSoTimeout(SOCKET_READ_TIMEOUT);
        s.connect(new InetSocketAddress(host, port), CONNECT_TIMEOUT);

        synchronized (this) { socket = s; }
        sendHello(s);
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

    /**
     * Sent once, immediately after connecting, so the server knows
     * whether to treat this device as the sanctuary display or an
     * operator's phone -- see PROTOCOL.md's "Hello" section. Best-effort:
     * a write failure here just means the server falls back to treating
     * us as role Display with unknown resolution, same as an old client
     * that predates this handshake entirely, so it's caught and logged
     * rather than tearing down the connection over it.
     */
    private void sendHello(Socket s) {
        try {
            JSONObject hello = new JSONObject();
            hello.put("type", "hello");
            hello.put("role", role);
            hello.put("w", screenWidth);
            hello.put("h", screenHeight);
            byte[] bytes = (hello.toString() + "\n").getBytes(StandardCharsets.UTF_8);
            s.getOutputStream().write(bytes);
            s.getOutputStream().flush();
        } catch (Exception e) {
            Log.w(TAG, "Failed to send hello — " + e.getMessage());
        }
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
                    float focusX = (float) obj.optDouble("fx", 0.5);
                    float focusY = (float) obj.optDouble("fy", 0.5);
                    // Decode here, on the IO thread, rather than posting
                    // the raw base64/bytes to the main thread and decoding
                    // there -- JPEG decode is the slow part of handling an
                    // image frame, and this keeps it off the UI thread so
                    // it can never show up as a frozen display.
                    Bitmap image = obj.has("image") ? decodeImage(obj.optString("image", "")) : null;
                    fireSlide(text, bg, fg, image, focusX, focusY);
                    break;
                }
                case "blackout":
                    fireBlackout();
                    break;
                case "wake":
                    fireWake();
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

    /** Returns null on any decode failure -- caller falls back to the solid bg color. */
    private static Bitmap decodeImage(String base64) {
        if (base64.isEmpty()) return null;
        try {
            byte[] bytes = Base64.decode(base64, Base64.DEFAULT);
            return BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
        } catch (Exception e) {
            Log.w(TAG, "Bad image frame — " + e.getMessage());
            return null;
        }
    }

    // ── Callbacks (always on main thread) ─────────────────────────────────

    private void fireConnected()  { mainHandler.post(() -> { if (listener != null) listener.onConnected(); }); }
    private void fireDisconnected() { mainHandler.post(() -> { if (listener != null) listener.onDisconnected(); }); }
    private void fireBlackout()   { mainHandler.post(() -> { if (listener != null) listener.onBlackout(); }); }
    private void fireWake()       { mainHandler.post(() -> { if (listener != null) listener.onWake(); }); }
    private void fireSlide(String text, int bg, int fg, Bitmap image, float focusX, float focusY) {
        mainHandler.post(() -> { if (listener != null) listener.onSlide(text, bg, fg, image, focusX, focusY); });
    }
}