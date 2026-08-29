package com.sanctifylive.display;

import android.content.Context;
import android.content.SharedPreferences;

/** Thin wrapper around SharedPreferences for host/port/role persistence. */
public final class Prefs {

    private static final String FILE    = "sanctify_prefs";
    private static final String KEY_HOST = "host";
    private static final String KEY_PORT = "port";
    private static final String KEY_ROLE = "role";

    private static final String DEFAULT_HOST = "127.0.0.1";
    private static final int    DEFAULT_PORT = 55432;

    /** Matches the "role" values SlideServer understands -- see PROTOCOL.md. */
    public static final String ROLE_DISPLAY = "display";
    public static final String ROLE_PHONE   = "phone";
    private static final String DEFAULT_ROLE = ROLE_DISPLAY;

    private Prefs() {}

    public static String getHost(Context ctx) {
        return prefs(ctx).getString(KEY_HOST, DEFAULT_HOST);
    }

    public static int getPort(Context ctx) {
        return prefs(ctx).getInt(KEY_PORT, DEFAULT_PORT);
    }

    /** One of ROLE_DISPLAY or ROLE_PHONE -- what this device identifies as in its hello. */
    public static String getRole(Context ctx) {
        return prefs(ctx).getString(KEY_ROLE, DEFAULT_ROLE);
    }

    public static void save(Context ctx, String host, int port, String role) {
        prefs(ctx).edit()
                .putString(KEY_HOST, host)
                .putInt(KEY_PORT, port)
                .putString(KEY_ROLE, role)
                .apply();
    }

    private static SharedPreferences prefs(Context ctx) {
        return ctx.getSharedPreferences(FILE, Context.MODE_PRIVATE);
    }
}
