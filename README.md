# Ristretto

<p align="center">
  <img src=./.github/ristretto.png width="256">
  <br>
  <sub>This plugin is named after the type of coffee. The logo is a sort-of depiction of what it looks like.</sub>
</p>

Ristretto is a plugin for Aroma that provides a foundation for smart home automation on the Wii U with a HTTP server. This will allow for other devices to communicate with the Wii U, and can then be used to extend existing home automation sofftware.

Currently, integrations exist for [Homebridge](https://github.com/wiiu-smarthome/homebridge-wiiu) and [Home Assistant](https://github.com/wiiu-smarthome/ha-wiiu), with hopefully more in the future. These platforms can serve as a bridge for an external service, like Google Home or Alexa, to integrate with the console. Have fun!

For more information, see the [Wii U Smart Home Project](https://github.com/wiiu-smarthome).

## Technical Limitations
There are some limitations that currently prevent this from perfection. Some are unavoidable, others can be mitigated or solved in the future.

1. **The console cannot be powered on remotely.** This may be solved in the future by emulating a GamePad or Wii Remote, but the console does not support [Wake-on-LAN](https://en.wikipedia.org/wiki/Wake-on-LAN). In the meantime, consider buying another smart device that serves as a "power button pusher".
2. **Problems with the application being in the background state.** If the Home menu, electronic manual, or any system applet is open, the server will not respond. Because of how Cafe OS, the Wii U's operating system, is designed, the server ends up getting put in the "background" where it loses access to some system resources and memory. Some operations like trying to shutdown the console will cause a crash. Some operations, like switching to the e-manual or home button menu, are one time operations. You can switch in, but you cannot switch out. Blog post on this coming soon, there may be a solution. 
3. **Offline server in Wii mode.** At this point, the console is no longer in Cafe OS. The "fix" to this would be implementing a server through IOS patching, which is being considered but not ready yet. However, patching the Wii IOS would be the solution for running a server on the original Wii console, which would be amazing. Stay tuned.

## Installation

1. Copy the file `Ristretto.wps` into `sd:/wiiu/environments/aroma/plugins`.
2. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/aroma/modules`.
2. Requires the [NotificationModule](https://github.com/wiiu-env/NotificationModule) in `sd:/wiiu/environments/aroma/modules`.
3. Requires the [SDHotSwapModule](https://github.com/wiiu-env/SDHotSwapModule) in `sd:/wiiu/environments/aroma/modules`.

Start the environment and the backend should load the plugin. By default, the port runs on :8572. You'll know the server is working when you open `http://(wiiu_ip_address):8572` in your browser and you should see the text "Ristretto".

## Building

For building you need:

- [wups](https://github.com/Maschell/WiiUPluginSystem)
- [wut](https://github.com/devkitpro/wut)
- [libnotifications](https://github.com/wiiu-env/libnotifications)
- [libsdutils](https://github.com/wiiu-env/libsdutils)
- [libwupsbackend](https://github.com/wiiu-env/libwupsbackend)

Install them (in this order) according to their README's. Don't forget the dependencies of the libs itself.

Then you should be able to compile via `make` (with no logging) or `make DEBUG=1` (with logging).

Two other libraries, [MiniJson](https://github.com/zsmj2017/MiniJson) and a modified version of [tinyhttp](https://github.com/kissbeni/tinyhttp) are compiled with the source.

### Buildflags

#### Logging

Building via `make` only logs errors (via OSReport). To enable logging via the [LoggingModule](https://github.com/wiiu-env/LoggingModule) set `DEBUG` to `1` or `VERBOSE`.

`make` Logs errors only (via OSReport).
`make DEBUG=1` Enables information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).
`make DEBUG=VERBOSE` Enables verbose information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).

If the [LoggingModule](https://github.com/wiiu-env/LoggingModule) is not present, it'll fallback to UDP (Port 4405) and [CafeOS](https://github.com/wiiu-env/USBSerialLoggingModule) logging.

### Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t ristretto-builder

# make
docker run -it --rm -v ${PWD}:/project ristretto-builder make DEBUG=1

# make clean
docker run -it --rm -v ${PWD}:/project ristretto-builder make clean
```

#### Format the code via docker

`docker run --rm -v ${PWD}:/src ghcr.io/wiiu-env/clang-format:13.0.0-2 -r ./src -i`

## Credits
Ristretto is a big project. It explores so many different areas of the Wii U and opens the door to more opportunities when it comes to home automation, homebrew, reverse engineering and so much more.

- [Maschell](https://github.com/maschell) - For everything he has done with the Wii U homebrew scene, and for helping me in general
- [Daniel K.O.](https://github.com/dkosmari) - Helping in general, debugging, advice with sockets, threading, function hooking
- [TraceEntertains](https://github.com/TraceEntertains) - Defining endian functions (e.g. bswap32) for Wii U, config menu, research with title information
- [Wish](https://github.com/wish13yt) - Original Ristretto logo
- [Artisan71](https://github.com/Artisan71) - Current Ristretto logo
- You, for checking out this project.
