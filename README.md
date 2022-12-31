# C-DvD

A bouncing DVD logo for the terminal

<video width="600" controls>
  <source alt="video" src="https://github.com/LowDrod/c-dvd/raw/main/video.mp4" type="video/mp4"/>
</video>

## Build and run

### Linux

```console
$ sh ./build.sh
```

> ⚠️ If you compile with Clang, may not work properly!

### Windows & Mac

> TODO

## Key Bindings

| Shortcut                                       | Action        |
| ---------------------------------------------- | :------------ |
| `<kbd>`Ctrl `</kbd>` `<kbd>`C `</kbd>` | Stops the app |

## Options

```bash
$ ./screensaver -h
Usage: screensaver [OPTIONS]...

  -p [/path/to/file]     path to ascii file you wanna use
  -h                     show this help
  -s [number]            number of refresh per second
  -b                     makes the background change color
                         every time the ascii hit an edge
  -f                     makes the foreground change color
                         every time the ascii hit an edge
```
