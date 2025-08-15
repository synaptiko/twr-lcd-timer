# Firmware for my custom LCD timer

Based on HARDWARIO Radio LCD Thermostat

## Debug build

```
cmake -B obj/debug . -G Ninja -DTYPE=debug -DCMAKE_TOOLCHAIN_FILE=sdk/toolchain/toolchain.cmake
ninja -C obj/debug
```

## Release build

```
cmake -B obj/release . -G Ninja -DTYPE=release -DCMAKE_TOOLCHAIN_FILE=sdk/toolchain/toolchain.cmake
ninja -C obj/release
```

## Flash

```
bcf flash --device /dev/tty.usbserial-1240 --skip-verify
```
