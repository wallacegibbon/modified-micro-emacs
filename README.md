## Introduction

This is a tiny terminal editor modified from [uEmacs/PK 4.0][uemacs/pk],
I call it **M**dified Micro **E**macs (`me`).

The script engine is removed.  Concepts like `paragraph` and `word` are also
removed.

ANSI CSI control sequences is used directly.  We don't need extra libraries
(like `libncurses`) to run this program.

Meta prefixed key bindings are removed.  Ctrl is enough for everything.

UTF-8 support got removed and binary data is supported.

A keyboard remapper like [this one][key remapper] is recommended to map `Space`
to `Ctrl` when holding it.


## Build and Install

Build:

```sh
make -j
```

To build on non-POSIX systems: (not supported yet)

```sh
make -j PLATFORM_OBJS=nonposix.o
```

Install:

```sh
sudo make install
```

To change the installation path:

```sh
sudo make install BIN=/usr/local/bin
```


## Miscellaneous

We can make `me` the default `editor` of the system with `update-alternatives`:

To get the priority of `editor`:
```sh
update-alternatives --display editor
```

```sh
sudo update-alternatives --install /usr/bin/editor editor /usr/bin/me 100
```

If the priority 100 is still too low, set it manually:
```sh
sudo update-alternatives --set editor /usr/bin/me
```


## Debug

There is a program `showkey` for getting raw input from terminal in the same
way as `me`.

Build:

```sh
make showkey
```

Run:
```sh
./showkey
```

[uemacs/pk]: https://github.com/torvalds/uemacs
[key remapper]: https://github.com/wallacegibbon/simple-keyboard-remapper
