# SanctifyLive Android Display — Wire Protocol

## Overview
The desktop (core) runs a **SlideServer** on TCP port **55432**.
The Android app connects as a client and receives slide frames.
This is mostly a **push** protocol — the server pushes every time
`liveContentChanged` fires — with one small exception: right after
connecting, the client sends a single **hello** line identifying itself
(see below), because the server treats "the sanctuary screen" and "an
operator's phone" differently.

## Connection
- Default port: `55432`
- USB/ADB:  `adb forward tcp:55432 tcp:55432`  → Android connects to `127.0.0.1:55432`
- WiFi:     Android connects to the PC's LAN IP directly on port `55432`

## Hello (client → server, sent once, immediately after connecting)
```json
{"type":"hello","role":"display","w":1920,"h":1080}
```
- `role`: `"display"` (the default — a sanctuary/stage screen mirroring
  the live output) or `"phone"` (an operator's handheld device, which
  can be given different content — see "Roles" below). Set from the
  app's settings dialog.
- `w`/`h`: the device's screen resolution in pixels, landscape or
  portrait as it actually sits. Used only as a hint the server may use
  later for finer per-device sizing; today it isn't required for
  correct rendering, since each client always crops to fill whatever
  size it actually is.
- If a client never sends a hello (or an old build that predates this),
  it's treated as `role: "display"` with unknown resolution — today's
  original behavior, unchanged.

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
failed to load, or it just hasn't finished encoding yet — see
"Progressive image loading" below), the client falls back to the solid
`bg` color.

`fx`/`fy` (optional, default `0.5`/`0.5`): the image's *focus point*,
normalized 0–1 across its width/height, set by the operator via
"Edit Framing..." in the Media Library. When a client's own aspect ratio
doesn't match the image's, this is the point in the source image that
stays centered in the crop instead of the crop always defaulting to dead
center — e.g. so a portrait phone doesn't crop out a subject that a
16:9 image had off to one side.
```json
{"type":"slide","text":"","bg":"#1a1a2e","fg":"#ffffff","image":"<base64 JPEG bytes>","fx":0.35,"fy":0.4}
```

### Progressive image loading
Large source images are decoded and encoded off the server's UI thread
so a multi-megapixel photo never stalls the operator's whole app. To
keep the display from sitting blank in the meantime, the server pushes
the slide frame **immediately** with the solid `bg` color and no
`image` field, then pushes a **second, otherwise-identical** slide
frame a moment later once encoding finishes, this time with `image`
included. A client's `onSlide` handler is expected to be called more
than once for the same logical slide because of this — there's nothing
to detect or special-case, just render whatever the latest frame says.

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

## Roles: display vs. phone
Every connected client is either a `display` or a `phone` (see Hello,
above). Both kinds speak the exact same frame format — there's no
protocol difference in the messages themselves — but the server may
send *different content* to each group:

- `display` clients always mirror `ScheduleModel::liveSlide()` exactly,
  same as before this existed.
- `phone` clients mirror the same live slide **unless** the operator has
  set a phone-only override (`ScheduleModel::sendPhoneOverride()`, e.g.
  via "Send to Phone Only" in the Media Library, or "Send Live to Phone
  Only" in the Live menu), in which case they get that instead, until
  the operator clears it ("Mirror Main Display on Phone").

This is meant for things like keeping a stage-monitor phone showing the
current song's next verse while the sanctuary screen has already cut to
an announcement slide, without needing a second full operator console.

## Client behavior
- On connecting: send the hello line above before doing anything else.
- On `slide`: render text with given bg/fg colors, full screen. If `image` is present, draw it cover-fit (biased toward `fx`/`fy`) as the background instead of the solid color, then draw the text on top.
- On `blackout`: fill screen black, show nothing.
- On `wake`: force the screen on and bring the display to the front, even over the lock screen.
- On `ping`: no response needed (one-way keepalive).
- On disconnect: show a subtle "Disconnected" overlay; keep last slide visible.
- Reconnect automatically every 3 seconds (re-sending hello on each new connection).

## Notes for core implementor (you)
Add `SlideServer` in `/src/core/SlideServer.h/.cpp`.
Connect `ScheduleModel::liveContentChanged` → push current slide frame to all `display` clients.
Connect `ScheduleModel::phoneContentChanged` → push the phone-specific frame to all `phone` clients.
Connect `ScheduleModel::scheduleChanged` (blackout path) similarly.
Use `QTcpServer` + `QTcpSocket`. One `QJsonDocument` per write, append `\n`.
Set `QAbstractSocket::LowDelayOption` on each accepted socket (disables
Nagle's algorithm / `TCP_NODELAY`) — without it, small JSON frames can sit
buffered for tens of milliseconds waiting to coalesce with more data
before the OS sends them, which is felt as a visible lag between the
operator clicking and the display updating.
Decode/encode background images on a worker thread (`QtConcurrent::run`
+ `QFutureWatcher`) rather than inline in the frame-building code — a
large source photo can take real time to decode, and that must never
block the GUI thread or the rest of the push pipeline.

