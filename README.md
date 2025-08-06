## Introduction

This is a tiny emacs-like editor modified from [uEmacs/PK 4.0][uemacs/pk],
I call it **M**dified Micro **E**macs (`me`).


## Goals

- Fix all bugs.
- Be tiny.

The script engine is removed.  Concepts like `paragraph` and `word` are also
removed.

ANSI CSI control sequences is used directly.  We don't need extra libraries
(like `libncurses`) to run this program.


## Build and Install

Build this program:

```sh
make -j
```

Install it:

```sh
sudo make install
```

To change the installation path:

```sh
sudo make install BIN_PATH=/usr/local/bin
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

There is a program `showkeys` for getting raw input from terminal,
which is useful for debugging.  Build it:

```sh
make showkeys
```

And run it:
```sh
./showkeys
```

[uemacs/pk]: https://github.com/torvalds/uemacs
