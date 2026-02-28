# KDiskMark
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-orange.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![GitHub (pre-)release](https://img.shields.io/github/release/JonMagon/KDiskMark/all.svg)](https://github.com/JonMagon/KDiskMark/releases)
[![Main](https://github.com/JonMagon/KDiskMark/actions/workflows/main.yml/badge.svg)](https://github.com/JonMagon/KDiskMark/actions/workflows/main.yml)

**KDiskMark** is an HDD and SSD benchmark tool with a very friendly graphical user interface. **KDiskMark** with its presets and powerful GUI calls [Flexible I/O Tester](https://github.com/axboe/fio) and handles the output to provide an easy to view and interpret comprehensive benchmark result. The application is written in C++ with Qt and *doesn't have* any KDE dependencies.

<p align="center">
   <img src="https://raw.githubusercontent.com/JonMagon/KDiskMark/master/assets/images/kdiskmark.png"/>
</p>
<p align="center">
   <img width="849" height="523" alt="KDiskMark-mac" src="https://github.com/user-attachments/assets/5ddafe5c-9a23-4aaa-aa34-98514dbb8762" />
   KDiskMark running on Mac OS X 10.9.5 with Qt 5.9.8 and a QtCurve widget theme.
</p>

## Features
* Configurable block size, queues, and threads (jobs/processes!) count for each test
* Multiple I/O engines supported, including both high-performance asynchronous ones used
  only by select applications and more common synchronous ones that might give a better
  idea of a drive's everyday performance.
* Many languages support
* Report generation

Note that the synchronous I/O engines do not support having multiple queues in direct I/O submit mode, but do so in "offload mode" (see `man fio`). In direct mode tests with multiple queues are thus adapted; a QiTj test will become Q1Ti*j if possible.

Also note that there is a kernel limit on Mac to the number of queues that is possible with the `posixaio` engine (16), and that it appears to be impossible to use more than 8 threads/jobs.

## Report Example
```
                        KDiskMark (3.0.0): https://github.com/JonMagon/KDiskMark
                    Flexible I/O Tester (fio-3.30): https://github.com/axboe/fio
--------------------------------------------------------------------------------
* MB/s = 1,000,000 bytes/s [SATA/600 = 600,000,000 bytes/s]
* KB = 1000 bytes, KiB = 1024 bytes

[Read]
Sequential   1 MiB (Q=  8, T= 1):   508.897 MB/s [    497.0 IOPS] < 13840.05 us>
Sequential   1 MiB (Q=  1, T= 1):   438.278 MB/s [    428.0 IOPS] <  2280.14 us>
    Random   4 KiB (Q= 32, T= 1):   354.657 MB/s [  88664.6 IOPS] <   352.37 us>
    Random   4 KiB (Q=  1, T= 1):    44.166 MB/s [  11041.6 IOPS] <    88.48 us>

[Write]
Sequential   1 MiB (Q=  8, T= 1):   460.312 MB/s [    449.5 IOPS] < 15153.11 us>
Sequential   1 MiB (Q=  1, T= 1):   333.085 MB/s [    325.3 IOPS] <  2349.82 us>
    Random   4 KiB (Q= 32, T= 1):   315.170 MB/s [  78792.5 IOPS] <   383.86 us>
    Random   4 KiB (Q=  1, T= 1):    91.040 MB/s [  22760.3 IOPS] <    39.80 us>

Profile: Default
   Test: 1 GiB (x5) [Measure: 5 sec / Interval: 5 sec]
   Date: 2022-08-24 16:10:33
     OS: opensuse-tumbleweed 20220821 [linux 5.19.2-1-default]
```

## Dependencies
### Required
* GCC/Clang C++17 (or later)
* [CMake](https://cmake.org/) >= 3.12
* [Extra CMake Modules](https://github.com/KDE/extra-cmake-modules) >= 5.73
* [Qt](https://www.qt.io/) with Widgets and DBus >= 5.9
* [Flexible I/O Tester](https://github.com/axboe/fio) with libaio >= 3.1
    * `libaio` development package.

### Optional external libraries
* [PolicyKit](https://gitlab.freedesktop.org/polkit/polkit) Agent
    * `PolkitQt-1` bindings.
* [SingleApplication](https://github.com/itay-grudev/SingleApplication)
    * prevents launch of multiple application instances.

## Installation
Official, Linux-only binaries are available on the [Releases](https://github.com/JonMagon/KDiskMark/releases/latest) page.
More [Linux installation options](https://github.com/JonMagon/KDiskMark#installation).

## Building
You can build **KDiskMark** by using the following commands from inside the source directory:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make && sudo make install
```

### Building with Qt5
To build **KDiskMark** with Qt5 instead of the default Qt6, use the `USE_QT5` flag during the CMake configuration step:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -D USE_QT5=ON ..
make && sudo make install
```

## Localisation [![Crowdin](https://badges.crowdin.net/kdiskmark/localized.svg)](https://crowdin.com/project/kdiskmark)
To help with localisation you can use [Crowdin](https://crowdin.com/project/kdiskmark) or translate files in `data/translations` with [Qt Linguist](https://doc.qt.io/Qt-5/linguist-translators.html) directly. To add a new language, copy `data/translations/kdiskmark.ts` to `data/translations/kdiskmark_<ISO 639-1 language code>_<ISO 3166-1 alpha-2 language code>.ts`, translate it, then add the file to the TS_FILES variable in CMakeLists.txt, and create a pull request. It is also possible to add localised Comment and Keywords sections into `data/kdiskmark.desktop` and message for PolicyKit authorisation into `data/dev.jonmagon.kdiskmark.helper.policy`.

Languages currently available:
* Chinese (Simplified)
* Chinese (Traditional)
* Czech
* Dutch
* English (default)
* Finnish
* French
* German
* Hindi
* Hungarian
* Italian
* Japanese
* Polish
* Portuguese (Brazilian)
* Russian
* Slovak
* Spanish (Mexico)
* Spanish (Spain)
* Swedish
* Turkish
* Ukrainian

## TODO
- [ ] Text-based user interface
- [x] Performance profiles (mix, peak, real-world)

## Special Thanks
* Artem Grinev (<agrinev@manjaro.org>) for his help with assembling the AppImage package.

Thanks to the package maintainers, translators, and all users for supporting the project.

## Credits
**Application Icon**  
Copyright (c) https://www.iconfinder.com/baitisstudio

If you have any ideas, critics, suggestions or whatever you want to call it, please open an issue.
