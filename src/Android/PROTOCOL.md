# SanctifyLive Android Display — Wire Protocol

## Overview
The desktop (core) runs a **SlideServer** on TCP port **55432**.
The Android app connects as a client and receives slide frames.
This is a **push** protocol — the server pushes every time `liveContentChanged` fires.

## Connection
- Default port: `55432`
- USB/ADB:  `adb forward tcp:55432 tcp:55432`  → Android connects to `127.0.0.1:55432`
- WiFi:     Android connects to the PC's LAN IP directly on port `55432`

## Frame Format (server → client)

All frames are **newline-delimited JSON** (`\n` terminated), UTF-8.

### Slide frame
```json
{"type":"slide","text":"Amazing Grace\nHow sweet the sound","bg":"#000000","fg":"#ffffff"}
```

`image` (optional): base64-encoded JPEG, present only when the live slide
has a real background image (`Slide::backgroundImagePath`). When present,
the client draws it cover-fit (scale to fill, crop overflow — same as the
desktop's `Qt::KeepAspectRatioByExpanding`) instead of the solid `bg`
color, then draws `text` on top exactly as it would over a color
background. The server downsamples to a max dimension before encoding so
frames stay small over the wire; when absent (no image, or the file
failed to load), the client falls back to the solid `bg` color as before.
```json
{"type":"slide","text":"","bg":"#1a1a2e","fg":"#ffffff","image":"<base64 JPEG bytes>"}
```

### Blackout frame
```json
{"type":"blackout"}
```

### Wake frame
Sent on operator demand (not automatically), when the operator taps
"Wake Display" for a connected device. Tells the client to force its
screen on and come to the front even over the lock screen — for the
case where the tablet's screen has gone to sleep from an idle timeout
or a physical power-button press, which the app's normal
keep-screen-on/turn-screen-on behavior does not by itself reverse.
```json
{"type":"wake"}
```

### Heartbeat (every 2 s when idle)
```json
{"type":"ping"}
```

## Client behavior
- On `slide`: render text with given bg/fg colors, full screen. If `image` is present, draw it cover-fit as the background instead of the solid color, then draw the text on top.
- On `blackout`: fill screen black, show nothing.
- On `wake`: force the screen on and bring the display to the front, even over the lock screen.
- On `ping`: no response needed (one-way keepalive).
- On disconnect: show a subtle "Disconnected" overlay; keep last slide visible.
- Reconnect automatically every 3 seconds.

## Notes for core implementor (you)
Add `SlideServer` in `/src/core/SlideServer.h/.cpp`.
Connect `ScheduleModel::liveContentChanged` → push current slide frame to all clients.
Connect `ScheduleModel::scheduleChanged` (blackout path) similarly.
Use `QTcpServer` + `QTcpSocket`. One `QJsonDocument` per write, append `\n`.
Set `QAbstractSocket::LowDelayOption` on each accepted socket (disables
Nagle's algorithm / `TCP_NODELAY`) — without it, small JSON frames can sit
buffered for tens of milliseconds waiting to coalesce with more data
before the OS sends them, which is felt as a visible lag between the
operator clicking and the display updating.
