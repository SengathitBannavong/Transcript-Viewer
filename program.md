# Transcript Viewer — Key Bind & Config Guide

## Startup

When the program launches, a **name-input screen** appears.

| Key | Action |
|-----|--------|
| Any printable key (`A`–`Z`, `0`–`9`, `_`, `-`, …) | Type your username (max 25 chars) |
| `Backspace` | Delete the last character (hold to repeat) |
| `Enter` | Confirm username and open (or create) `db_<name>.db` |
| `Escape` | Quit the program |

- If `db_<name>.db` already exists it is opened and your saved scores are loaded.
- If it is new, the database is created and seeded from `assets/subjects.dat`.
- If the username is **jerry** and `assets/tablejerry.txt` exists, scores are auto-imported on first create.

---

## Global Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl + K` | Open / close the Command Palette |

---

## Command Palette

Open with **Ctrl + K**, then type a command and press **Enter**.

| Key | Action |
|-----|--------|
| Any printable key | Type into the command input |
| `Backspace` | Delete the last character (hold to repeat) |
| `Enter` | Execute the command and close the palette |
| `Escape` | Dismiss without executing |

### Commands

| Command | Example | Effect |
|---------|---------|--------|
| `type <N>` | `type 3` | Switch sidebar to subject-type N (0–11) |
| `score <CODE> <mid> <fin>` | `score IT2000 7.5 8.0` | Set midterm and final scores for a subject (0.0–10.0); grade letter and pass/fail are recalculated automatically |
| `clear <CODE>` | `clear IT2000` | Reset a subject score back to X / 0.0 |
| `logout` | `logout` | Close the current database and return to the name-input screen |
| `help` | `help` | Show available commands in the result toast |

### After submitting

- The palette closes and a **result toast** appears at the top-centre.
- The toast is visible for **5 seconds**, then fades out over the last second.

---

## Mouse

| Action | Effect |
|--------|--------|
| Scroll wheel | Scroll the subject table vertically |
| Click a sidebar item | Switch to that subject-type section |
| Hover over a row | Row highlights |

---

## Window

| Action | Effect |
|--------|--------|
| Drag edge / corner | Resize (layout reflows automatically) |

---

## Config Files (`assets/`)

| File | Purpose |
|------|---------|
| `fonts.cfg` | Font search list — one path per line, tried in order. `#` = comment. Edit to use a different font without recompiling. |
| `ui.cfg` | UI display settings. Edit and restart to apply. |
| `subjects.dat` | Curriculum data (105 subjects, 12 types). Seeded into a new DB on first login. |
| `tablejerry.txt` | Optional score import for username **jerry** (auto-loaded on new DB only). |

### `assets/ui.cfg` keys

| Key | Default | Description |
|-----|---------|-------------|
| `font_scale` | `1.8` | Multiplier for all font sizes. `1.0` = base, `1.4` = original, `1.8` = larger. Change and restart. |

### `assets/fonts.cfg` format

```
# one path per line — relative or absolute
Font/Space_Mono/SpaceMono-Bold.ttf
/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
```

