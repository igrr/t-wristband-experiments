# TTGO T-Wristband example

This repository contains an ESP-IDF project which is supposed to run on [TTGO/LilyGO T-Wristband](https://github.com/Xinyuan-LilyGO/LilyGO-T-Wristband).

## How to use

This project uses IDF tag `v4.2-dev` and CMake build system.

Build and flash as usual for and IDF application (`idf.py build flash monitor`).

## To do:

- [x] Touchpad button
- [x] Enter sleep after some time
- [x] LCD driver
- [x] RTC driver
- [x] Time output to LCD
- [ ] Nicer time output (rotate text)
- [ ] Settings mode (long press, menus)
- [ ] Phone connection (?)
- [ ] Time sync
- [ ] IMU driver
