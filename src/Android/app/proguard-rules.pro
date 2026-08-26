# Keep our own classes intact (tiny app, not much to strip)
-keep class com.sanctifylive.display.** { *; }

# org.json is part of the Android SDK — no extra keep needed
