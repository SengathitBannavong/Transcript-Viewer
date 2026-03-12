# assets/ Setup Guide

All files in this folder can be edited with any plain-text editor.
**No recompile is needed** — changes take effect the next time the program starts
(unless noted otherwise).

---

## ui.cfg  —  Display settings

```
font_scale 2.0
target_fps 124
```

| Key | Type | Default | Range | Effect |
|---|---|---|---|---|
| `font_scale` | float | `1.8` | `0.1` – `10.0` | Multiplies every font size in the UI. `1.0` = smallest readable, `1.4` = compact, `1.8` = comfortable, `2.0` = large monitor |
| `target_fps` | integer | `60` | `60` – `240` | Frame-rate cap. Set to `60` to save CPU, `144` or `240` for high-refresh monitors |

**Syntax rules**
- One key–value pair per line: `key value`
- Lines starting with `#` are comments and are ignored
- Unknown keys are silently skipped

---

## fonts.cfg  —  Font search list

```
Font/Space_Mono/SpaceMono-Bold.ttf
Font/Quicksand/static/Quicksand-Regular.ttf
Font/Outfit/static/Outfit-Regular.ttf
```

- Each line is a path to a `.ttf` font file (relative to the program's working directory or absolute).
- Paths are tried **top to bottom**; the first file that loads successfully is used.
- Lines starting with `#` are comments.
- **At least one valid line is required.** If no font loads, the program shows an error and exits.

### Common system font paths

| OS | Example path |
|---|---|
| Linux | `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf` |
| Linux | `/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf` |
| Windows | `C:/Windows/Fonts/consola.ttf` |
| Windows | `C:/Windows/Fonts/arial.ttf` |

---

## grad_config.cfg  —  Graduation credit requirements

Controls how many credits / subjects are required to graduate for each subject type.
Edit this to adapt the program to a **different major or university curriculum**.

### Format

```
type_id   mode   limit_val   group_id
```

| Column | Meaning |
|---|---|
| `type_id` | Which subject type (see table below) |
| `mode` | How the requirement is counted (see table below) |
| `limit_val` | Credit / subject count target (only used for modes 1 and 2) |
| `group_id` | `0` = standalone; `≥1` = "pick best of group" (see below) |

### type_id values

| ID | Name |
|---|---|
| 1 | Co So Nganh (Major Foundation) |
| 2 | Dai Cuong (General Education) |
| 3 | The Thao (Physical Education) |
| 4 | Ly Luat Chinh Tri (Political Theory) |
| 5 | Tu Chon (Supplementary knowledge) |
| 6–11 | Modul I – VI (Specialisation Modules) |
| 12 | Thuc Tap (Internship) |
| 13 | Do An Tot Nghiep (Thesis / Capstone) |

### mode values

| Mode | Name | Meaning | When to use `limit_val` |
|---|---|---|---|
| `0` | `total_credit` | Must pass **all** credits defined in `subjects.dat` for this type | Set to `0` (ignored) |
| `1` | `fixed` | Must pass exactly `limit_val` credits | Set the required credits |
| `2` | `subject_count` | Must pass `limit_val` **subjects** (not credits) | Set the required subject count |

### group_id — "pick best of group"

When several modules share the same `group_id ≥ 1`, the student only needs to
fully complete **one** of them (the one with the most credits passed counts).
Set `group_id = 0` to require that module independently.

**Example — current config:**
```
6   0   0   1   # modunI   ─┐
7   0   0   1   # modunII   ├─ student only needs 1 of these 3
8   0   0   1   # modunIII ─┘
9   0   0   2   # modunIV  ─┐ student only needs 1 of these 2
10  0   0   2   # modunV   ─┘
```

### After editing `grad_config.cfg`

The updated rules are loaded the next time you **log in**.  
If you already have a database open, run the `reload` command in the palette
(`Ctrl+K` → `reload`) to pick up the changes immediately.

---

## subjects.dat  —  Curriculum subject list

Defines every subject in the curriculum: code, term, credits, and display name,
grouped by subject type.

### Format

```
[type_id] Section Name
CODE   TERM   CREDIT   Subject Name
...
```

**Example:**
```
[1] Co So Nganh
IT2000   1   3   Introduction to IT and Communications
IT3011   2   2   Data Structures and Algorithms
```

| Field | Rules |
|---|---|
| `CODE` | Unique subject identifier, no spaces, max 15 chars |
| `TERM` | Integer ≥ 1 — which academic term the subject appears in |
| `CREDIT` | Integer ≥ 1 |
| `Subject Name` | Free text; rest of line after tab / spaces |

### When to edit

- To **add a subject**: append a new line under the correct `[type_id]` section.
- To **remove a subject**: delete its line.
- To **change a credit value or term**: edit the number in that column.

> **Important:** After changing `subjects.dat`, delete the user's `db_<name>.db`
> file (or run `reload` in the command palette) so the database is reseeded with
> the new data.

---

## tabletest.txt  —  Optional score import (test user only)

Pre-fills scores for the username **test** when a new database is created for that user.  
Not used for any other username.  You can edit this file to change the test dataset
without affecting real user databases.

### Format

```
CODE   midterm_score   final_score
```

**Example:**
```
IT2000   8.5   9.0
IT3011   7.0   6.5
```

Scores are floats in the range `0.0` – `10.0`.

---

## Summary — Which file to edit for common tasks

| Task | File to edit |
|---|---|
| Change font size or frame rate | `ui.cfg` |
| Use a different font | `fonts.cfg` |
| Change required credits to graduate | `grad_config.cfg` |
| Add / remove / rename subjects | `subjects.dat` |
| Change test-user sample scores | `tabletest.txt` |
