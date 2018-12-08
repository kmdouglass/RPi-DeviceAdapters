# Changelog

## [0.3.0]
### Added
- The Micro-Manager Video4Linux device adapter
  (https://github.com/micro-manager/micro-manager/tree/mm2/DeviceAdapters/Video4Linux)
- The `mm.py` script now supports reading both `/dev/gpio` and `/dev/videoX` device files, where
  `X` is an integer that corresponds to a specific webcam or Raspberry Pi camera device. Up to 8
  cameras (device files `video0` .. `video7`) are supported.

## [0.2.0]
### Added
- RPi-GPIO device adapter
- Automated deployment of the build artifacts to GitHub releases

[0.3.0]: https://github.com/kmdouglass/RPi-DeviceAdapters/releases/tag/0.3.0
[0.2.0]: https://github.com/kmdouglass/RPi-DeviceAdapters/releases/tag/0.2.0
