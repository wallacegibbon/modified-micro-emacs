## Introduction

This is a tiny emacs-like editor modified from [uEmacs/PK 4.0][uemacs/pk],
I call it **M**dified Micro **E**macs (`me`).


## Goal

- Be more compatible with GNU emacs.
- Be tiny. (by removing unecessary parts like the script engine)


## Build

The default build works on all ANSI-compatible terminals.  It do not rely on
`termcap` or `terminfo`.

We can also build it with `termcap` or `terminfo` to support more terminals:

```sh
make TERMCAP=1
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

There is a program for getting raw input from terminal, which is useful for
debugging.  Build it like this:

```sh
make showkeys
```


[uemacs/pk]: https://github.com/torvalds/uemacs
