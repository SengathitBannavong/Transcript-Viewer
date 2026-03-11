# Clay DB Viewer — Key Bind Guide

## Global Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl + K` | Open / close the Command Palette |

---

## Command Palette

> Open the palette first with **Ctrl + K**, then use the keys below.

| Key | Action |
|-----|--------|
| Any printable key (`A`–`Z`, `0`–`9`, symbols…) | Type into the command input |
| `Backspace` | Delete the last character (hold to repeat) |
| `Enter` | Submit the command and close the palette |
| `Escape` | Dismiss the palette without submitting |

### Commands (parsed by [easy-args](https://github.com/gouwsxander/easy-args))

| Command | Example | Effect |
|---------|---------|--------|
| `nav -n <N>` | `nav -n 3` | Switch sidebar to item N (0=Dashboard, 1=Employees, 2=Reports, 3=Analytics, 4=Settings) |
| `filter -d <dept>` | `filter -d Engineering` | Filter the employee table to show only that department |
| `clear` | `clear` | Remove the active department filter |
| `help` | `help` | Show available commands in the toast |

Arguments use flag syntax as defined by easyargs:
- `-n` takes a required integer value (0–4).
- `-d` takes a required string value (department name, case-sensitive).

### What happens after submitting

- The palette closes immediately.
- A **result toast** appears at the top-centre showing the command result or an error message.
- The toast stays visible for **5 seconds**, then fades out over the last second.

---

## Mouse

| Action | Effect |
|--------|--------|
| Scroll wheel | Scroll the employee table vertically |
| Hover over a table row | Row highlights in a brighter colour |

---

## Window

| Action | Effect |
|--------|--------|
| Drag window edge / corner | Resize the window (layout reflows automatically) |
