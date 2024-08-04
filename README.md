# ScreenViewer

## Introduction
ScreenViewer is my C++ implementation inspired by the popular software [TeamViewer](https://www.teamviewer.com/en/).
This is currently work in progress, right now there is available public server, streamer and client which can talk to each other anywhere in the world.

## TODO
- Get rid of OpenCV 
- Add Windows support
- Add some register option and open to public users
- In next weeks, I will replace the bridge server with direct connection, using the NAT-hole-punching technique.
- Later on, I will separate the video channel from input channel, in order to enable sending video with UDP, and mouse/keyboard events with TCP, to ensure, that no user-event is lost.
- Create user-friendly UI

- Add Wayland support <-- it turns out that it is not so trivial as it is with X11, leaving this for later (if ever).


## Launching database for tests
```bash
./run-test-postgres-database.sh
```