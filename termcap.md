We can use `infocmp` command to list all terminfo names on our system.

```sh
infocmp -1 | sed 's/[,\\]//g' | awk '{print $1}'
```

# Legacy `termcap` to Modern `terminfo` Mapping


| `termcap` | `terminfo` | Description              |
| --------- | ---------- | ------------------------ |
| `al`      | `il1`      | Insert 1 line            |
| `bc`      | `cub1`     | Backspace one column     |
| `bt`      | `cbt`      | Back tab                 |
| `cd`      | `ed`       | Clear to end of screen   |
| `ce`      | `el`       | Clear to end of line     |
| `cl`      | `clear`    | Clear screen             |
| `cm`      | `cup`      | Cursor move              |
| `cr`      | `cr`       | Carriage return          |
| `cs`      | `csr`      | Set scrolling region     |
| `dc`      | `dch1`     | Delete 1 character       |
| `dl`      | `dl1`      | Delete 1 line            |
| `dm`      | `smir`     | Enter insert mode        |
| `do`      | `cud1`     | Cursor down one line     |
| `ei`      | `rmir`     | Exit insert mode         |
| `ho`      | `home`     | Move cursor to home      |
| `ic`      | `ich1`     | Insert 1 character       |
| `im`      | `ich`      | Insert mode (start)      |
| `ip`      | `ip`       | Insert padding           |
| `kd`      | `kcud1`    | Down arrow key           |
| `kl`      | `kcub1`    | Left arrow key           |
| `kr`      | `kcuf1`    | Right arrow key          |
| `ku`      | `kcuu1`    | Up arrow key             |
| `le`      | `cub1`     | Cursor left one column   |
| `ll`      | `ll`       | Cursor to lower-left     |
| `md`      | `bold`     | Begin bold mode          |
| `me`      | `sgr0`     | Turn off all attributes  |
| `mr`      | `rev`      | Reverse video mode       |
| `nd`      | `cuf1`     | Cursor right one column  |
| `se`      | `rmso`     | End standout mode        |
| `so`      | `smso`     | Begin standout mode      |
| `ta`      | `ht`       | Horizontal tab           |
| `te`      | `rmcup`    | Exit alternate screen    |
| `ti`      | `smcup`    | Enter alternate screen   |
| `ue`      | `rmul`     | End underline mode       |
| `us`      | `smul`     | Begin underline mode     |
| `up`      | `cuu1`     | Cursor up one line       |
| `vb`      | `flash`    | Visible bell             |
| `vs`      | `smcvvis`  | Make cursor very visible |
| `ve`      | `cnorm`    | Make cursor normal       |
| `vi`      | `civis`    | Make cursor invisible    |


# Ordered `terminfo` Names

## Cursor Movement

| Name   | Description                  |
| ------ | ---------------------------- |
| `cup`  | Move cursor to (row, col)    |
| `home` | Cursor to home position      |
| `ll`   | Cursor to lower-left         |
| `hpa`  | Horizontal position absolute |
| `vpa`  | Vertical position absolute   |
| `cud1` | Cursor down 1 line           |
| `cuu1` | Cursor up 1 line             |
| `cuf1` | Cursor forward 1 char        |
| `cub1` | Cursor back 1 char           |
| `cuf`  | Cursor forward n chars       |
| `cub`  | Cursor back n chars          |
| `cuu`  | Cursor up n lines            |
| `cud`  | Cursor down n lines          |


## Screen/Line Manipulation

| Name    | Description                   |
| ------- | ----------------------------- |
| `clear` | Clear screen                  |
| `el`    | Clear to end of line          |
| `el1`   | Clear to beginning of line    |
| `ed`    | Clear to end of screen        |
| `il1`   | Insert 1 line                 |
| `dl1`   | Delete 1 line                 |
| `dch1`  | Delete 1 character            |
| `ech`   | Erase n characters            |
| `smcup` | Enter alternate screen buffer |
| `rmcup` | Exit alternate screen buffer  |


## Text Attributes

| Name    | Description                           |
| ------- | ------------------------------------- |
| `bold`  | Begin bold text                       |
| `dim`   | Begin dim text                        |
| `smul`  | Begin underline                       |
| `rmul`  | End underline                         |
| `rev`   | Reverse video                         |
| `blink` | Begin blinking text                   |
| `invis` | Begin invisible text                  |
| `sgr0`  | Reset all attributes                  |
| `sgr`   | Set graphic rendition (combined form) |


## Colors

| Name    | Description                         |
| ------- | ----------------------------------- |
| `setaf` | Set ANSI foreground color (07/255)  |
| `setab` | Set ANSI background color (07/255)  |
| `op`    | Reset foreground/background colors  |


## Key Mappings

| Name    | Key          |
| ------- | ------------ |
| `kcuu1` | Up Arrow     |
| `kcud1` | Down Arrow   |
| `kcub1` | Left Arrow   |
| `kcuf1` | Right Arrow  |
| `kbs`   | Backspace    |
| `kent`  | Enter/Return |
| `kich1` | Insert       |
| `kdch1` | Delete       |
| `khome` | Home         |
| `kend`  | End          |
| `knp`   | Page Down    |
| `kpp`   | Page Up      |


## Mouse (if supported)

| Name    | Description      |
| ------- | ---------------- |
| `kmous` | Mouse events key |
| `smkx`  | Enable keypad    |
| `rmkx`  | Disable keypad   |
