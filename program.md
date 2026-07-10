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
| `export [term\|type]` | `export term` | Export the transcript to a PDF. No argument = auto (by term if imported, else by type). See **Export to PDF** below. |
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

## Graduation Planner

Open from the **Planner** entry in the sidebar (below Dashboard). It forecasts the
honor tier you're heading toward and shows how to reach a tier you choose.

### Honor tiers (CPA on the 4.0 scale)

| Tier         | CPA band   |
|--------------|------------|
| Normal       | 2.0 – 2.49 |
| Good         | 2.5 – 3.19 |
| Excellent    | 3.2 – 3.59 |
| God of HUST  | 3.6 – 4.0  |

CPA below 2.0 is "Below classification" (not eligible to graduate).

> **Scale note:** grade → GPA uses the true HUST 4.0 scale — A and A+ both = 4.0,
> B+ 3.5, B 3.0, C+ 2.5, C 2.0, D+ 1.5, D 1.0, F/X 0.0. So the CPA can never exceed
> 4.0 and the honor bands line up exactly.

### What it shows

- **Projected honor** — your pass-only CPA classified into a tier ("if you keep your
  current average"), plus the range still reachable given the credits you have left
  (best case all-A, worst passing case all-D).
- **Graduate as** — pick a target tier. Tiers that are no longer mathematically
  reachable are dimmed and not selectable.
- **How far into <tier>** — once a reachable tier is chosen, pick the ambition:
  *Low* (just cross the floor, e.g. 2.50 for Good), *Medium* (mid-band), or
  *High* ("every possible" — the top of the band). The required average and
  per-subject target grades recompute to that CPA.
- **Verdict** — for the chosen target: "secured", the average grade you must hold
  across your remaining credits to reach it, or "out of reach" with the highest tier
  still possible. Remaining credits and failed (F) subjects assumed retaken count as
  the work ahead.
- **What to learn next** — every not-yet-passed subject, ranked failed-retakes-first,
  then by recommended term, then by credit. High-credit subjects are tagged
  "high impact"; each shows the target grade implied by your chosen tier.

## Export to PDF

Save a printable copy of your transcript as a real `.pdf` (no external tools or
libraries required — the writer is built in).

**How to trigger**

- **Settings → Export Transcript (PDF)** → choose **Export by Term** or
  **Export by Type**, or
- the command palette: **Ctrl + K**, then `export`, `export term`, or `export type`.

You choose how the grades are grouped:

| Choice | Layout |
|--------|--------|
| **By term** | One section per semester with its GPA and credits, listing *every attempt* (retakes and newly-added subjects are shown under the term they were taken in, never masked or merged). |
| **By type** | Grouped into the same subject-type sections as the dashboard (each with passed/total credits and its CPA), listing the courses you have grades for. |

**By Term needs data.** Grouping by term is only possible once you have imported
per-term grades from the portal. Until then the **Export by Term** button is
shown inactive (greyed out and not clickable), and the `export term` command
reports that no per-term data is available. **Export by Type** always works.

On desktop a native *Save as…* dialog appears (via `zenity`/`kdialog`), seeded
with `transcript_<user>.pdf` in your home folder. If no dialog tool is installed
the file is written to the working directory instead. On the web build the PDF
is generated and downloaded by the browser.

Every PDF starts with a header (student, date, scale) and a summary (CPA all /
passed, credits earned vs required, honour standing, graduation status) before
the grade sections.

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

