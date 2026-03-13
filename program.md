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
| `Ctrl + ,` | Open Asset Paths settings popup |

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
| `filter <text>` | `filter IT` | Filter current table rows by subject code/name text |
| `filter pass` | `filter pass` | Show only passed subjects in current table |
| `filter fail` | `filter fail` | Show only failed (studied but not pass) subjects |
| `filter noscore` | `filter noscore` | Show only subjects with no score (`X`) |
| `filter clear` | `filter clear` | Clear all table filters |
| `settings` | `settings` | Open Asset Paths settings popup |
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
| Click a row | Open the **Edit Score** popup for that subject |

### Edit Score Popup

| Key / Action | Effect |
|--------------|--------|
| Type digits or `.` | Enter a score value (max 6 characters per field) |
| `Tab` | Switch focus between the Midterm and Final fields |
| `Backspace` | Delete the last character in the active field |
| `Escape` | Close the popup without saving |
| Click a ratio button (`50/50`, `40/60`, `30/70`) | Select the midterm-to-final weight ratio |
| **Save** button | Validate that both scores are in the 0–10 range, write to DB, refresh display, show result toast |
| **Reset** button | Clear all scores for the subject, refresh display, show result toast |
| **Cancel** button | Close the popup without saving |

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

### Runtime path config

Settings are now edited in-app as structured forms/tables (no path editing in UI).

In **Ctrl + ,** settings popup:

| Tab | Editor type | Editable fields |
|-----|-------------|-----------------|
| `ui.cfg` | Form | `font_scale`, `target_fps` |
| `grad_config.cfg` | Table + row form | `mode`, `limit`, `group` per `type_id` row |
| `subjects.dat` | Section table + row form | `code`, `term`, `credit`, `name` per subject row |

Buttons:

| Button | Effect |
|--------|--------|
| `Save` | Save current editor file |
| `Save + Reload` | Save file and reload DB data immediately (`grad_config.cfg`, `subjects.dat`) |
| `Cancel` | Close settings popup |

### `assets/fonts.cfg` format

```
# one path per line — relative or absolute
Font/Space_Mono/SpaceMono-Bold.ttf
/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
```

