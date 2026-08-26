package com.sanctifylive.display;

import android.content.Context;
import android.content.SharedPreferences;

/** Thin wrapper around SharedPreferences for host/port persistence. */
public final class Prefs {

    private static final String FILE    = "sanctify_prefs";
    private static final String KEY_HOST = "host";
    private static final String KEY_PORT = "port";

    private static final String DEFAULT_HOST = "127.0.0.1";
    private static final int    DEFAULT_PORT = 55432;

    private Prefs() {}

    public static String getHost(Context ctx) {
        return prefs(ctx).getString(KEY_HOST, DEFAULT_HOST);
    }

    public static int getPort(Context ctx) {
        return prefs(ctx).getInt(KEY_PORT, DEFAULT_PORT);
    }

    public static void save(Context ctx, String host, int port) {
        prefs(ctx).edit()
                .putString(KEY_HOST, host)
                .putInt(KEY_PORT, port)
                .apply();
    }

    private static SharedPreferences prefs(Context ctx) {
        return ctx.getSharedPreferences(FILE, Context.MODE_PRIVATE);
    }
}
