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

### Blackout frame
```json
{"type":"blackout"}
```

### Heartbeat (every 2 s when idle)
```json
{"type":"ping"}
```

## Client behavior
- On `slide`: render text with given bg/fg colors, full screen.
- On `blackout`: fill screen black, show nothing.
- On `ping`: no response needed (one-way keepalive).
- On disconnect: show a subtle "Disconnected" overlay; keep last slide visible.
- Reconnect automatically every 3 seconds.

## Notes for core implementor (you)
Add `SlideServer` in `/src/core/SlideServer.h/.cpp`.
Connect `ScheduleModel::liveContentChanged` → push current slide frame to all clients.
Connect `ScheduleModel::scheduleChanged` (blackout path) similarly.
Use `QTcpServer` + `QTcpSocket`. One `QJsonDocument` per write, append `\n`.
