## Introduction

This is a tiny emacs-like editor modified from [uEmacs/PK 4.0][uemacs/pk],
I call it **M**dified Micro **E**macs (`me`).


## Goals

- Fix all bugs.
- Be tiny.

The script engine is removed.  Concepts like `paragraph` and `word` are also
removed.


## Build

The default build uses ANSI control sequences directly.  We don't need extra
libraries besides `libc` to run this program.

To support non-ANSI terminals, build this program with `terminfo`.

```sh
make USE_TERMCAP=1
```

> The deprecated `termcap` is supported, too.  But we need to change the
> Makefile to use `termcap`.


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
