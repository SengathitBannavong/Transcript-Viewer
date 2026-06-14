# How SQLite works in the WebAssembly build

This document explains how the same SQLite-backed code that runs natively on
Linux/Windows also runs — and **persists data** — in the browser via
WebAssembly (wasm). If you only care about the desktop build, none of this
applies: there SQLite is just a normal library writing a normal file to disk.

> TL;DR — In the browser there is no disk. We compile SQLite to wasm, give it a
> *virtual* filesystem, and back that filesystem with the browser's IndexedDB so
> the `.db` file survives page reloads. Each visitor's data lives in **their own
> browser**, not on the server.

---

## 1. The problem: there is no filesystem in a browser

Natively, `DB_Open()` calls `sqlite3_open("db_<user>.db", ...)` and SQLite
reads/writes a real file in the working directory. The OS handles persistence.

In the browser:

- There is no real filesystem. JavaScript/wasm cannot `fopen()` a path on the
  user's disk.
- Firebase Hosting (or any static host) only **serves files to** the browser —
  it never receives or stores user data. Hosting is a read-only CDN.

So two things have to be solved separately:

1. **Make SQLite run at all** — it expects a C filesystem (`open`, `read`,
   `write`, `lseek`, `fsync`).
2. **Make the data survive a reload** — wasm memory is wiped every time the page
   is closed or refreshed.

---

## 2. Making SQLite *run*: compile it to wasm

SQLite is just C, so we compile its amalgamation (`sqlite3.c`) with emscripten
instead of gcc:

```make
# Makefile — desktop links the system library; web compiles SQLite itself
$(SQLITE_WEB_OBJ): sqlite3.c sqlite3.h
	$(EMCC) -Os -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o sqlite3.web.o
```

- On **desktop Linux** we link the OS package (`-lsqlite3`).
- On **web** there is no system SQLite, so we build it from source — exactly
  like the existing Windows path already does.

emscripten provides a POSIX-like libc, including a virtual filesystem (the
emscripten **FS** API). SQLite's file calls (`open`, `read`, `write`, …) are
transparently serviced by that virtual FS. SQLite does not know or care that
it's running in a browser.

### Which virtual filesystem?

emscripten has several FS backends. The two that matter here:

| Backend  | Lives in            | Survives reload? |
|----------|---------------------|------------------|
| **MEMFS** | wasm linear memory (RAM) | ❌ wiped on reload |
| **IDBFS** | the browser's IndexedDB  | ✅ if you flush it |

By default everything is MEMFS. That's enough for SQLite to *work*, but the
database would vanish the instant the user refreshed. That's where step 2 comes
in.

---

## 3. Making data *persist*: IDBFS + IndexedDB

We mount an **IDBFS** filesystem at `/persist` and store the database there:

```
/persist/db_<user>.db      ← the SQLite file
```

IDBFS is special: it keeps a working copy in memory (fast, synchronous — which
SQLite requires) **and** can mirror that copy into the browser's IndexedDB
(durable). You move data between the two with `FS.syncfs()`:

- `FS.syncfs(true,  cb)` — **load**: IndexedDB → in-memory FS (do this at startup)
- `FS.syncfs(false, cb)` — **flush**: in-memory FS → IndexedDB (do this after writes)

Both directions are **asynchronous**, which matters below.

All of this is wrapped in `src/db.h` and compiled only for the web
(`#if defined(PLATFORM_WEB)`); on desktop the same functions are empty inline
no-ops.

### 3.1 Startup — load before opening

`DB_PersistInit()` runs once at startup, **before** any `DB_Open()`:

```c
EM_ASM({
    FS.mkdir('/persist');
    FS.mount(IDBFS, {}, '/persist');
    Module.__dbSyncDone = 0;
    FS.syncfs(true, function (err) { Module.__dbSyncDone = 1; });  // load
});
while (!EM_ASM_INT({ return Module.__dbSyncDone; }))
    emscripten_sleep(30);   // ASYNCIFY: block until the async load finishes
```

The load is async, but `DB_Open()` needs the file to already be present. We
bridge that gap with **ASYNCIFY** (an emscripten link flag, `-s ASYNCIFY`):
`emscripten_sleep()` yields to the browser and resumes when the load callback
has fired. So by the time the user types their name and the DB opens, any
previously-saved database is already on the virtual FS.

`db_path()` returns the `/persist/...` path on web so `DB_Open()` /
`DB_Exists()` look in the right place:

```c
#if defined(PLATFORM_WEB)
    snprintf(buf, bufsz, "/persist/db_%s.db", username);
#else
    snprintf(buf, bufsz, "db_%s.db", username);
#endif
```

### 3.2 After every write — flush

A SQLite write only changes the **in-memory** FS. To make it durable we call
`DB_Persist()` (`FS.syncfs(false, …)`) after each mutation:

- `DB_UpdateScoreRatio()` — after a score edit
- `DB_ReloadData()` — after re-seeding from the asset files
- `InitPlayerDB()` — right after a brand-new DB is seeded
- `DB_Close()` — on shutdown

The flush is fire-and-forget (we don't block on it); IndexedDB writes complete
in the background.

### 3.3 Journal mode: why DELETE on web

Natively the DB uses `PRAGMA journal_mode=WAL`. WAL keeps recent changes in
sidecar files (`*.db-wal`, `*.db-shm`), which complicates "persist the database"
— you'd have to sync several files and the shared-memory file doesn't translate
cleanly to IDBFS. So on web we switch to a rollback journal:

```c
#if defined(PLATFORM_WEB)
    db_exec("PRAGMA journal_mode=DELETE;");   // all data stays in the one .db file
#else
    db_exec("PRAGMA journal_mode=WAL;");
#endif
```

With `DELETE`, once a transaction commits, everything lives in the single
`db_<user>.db` file — so one `FS.syncfs(false)` persists the whole database
cleanly.

---

## 4. What this does and does NOT give you

✅ The database survives reloads, navigations, and closing the tab.
✅ It works on any static host (Firebase Hosting, GitHub Pages, Vercel, …) with
   **no backend** — persistence is 100% client-side.

❌ It is **per-browser, per-device**. Your phone and your laptop have separate
   databases. Two browsers on the same machine have separate databases.
❌ Clearing site data / "Delete cookies and site data" / private-browsing windows
   will wipe it.
❌ It is **not** a cloud backup. Firebase Hosting never sees the data.

```
   Browser ──(downloads static files)──► Firebase Hosting (CDN, read-only)
      │
      └─ writes /persist/db_<user>.db ─► IndexedDB  (THIS browser, THIS device)
```

---

## 5. Moving data between devices: Export / Import

Because the storage is local, the web build adds two sidebar buttons (web-only)
so users can move a database around as a real file:

- **Export .db** — `DB_ExportDownload()` reads the file out of the virtual FS
  (`FS.readFile`) and triggers a normal browser download (`db_<user>.db`).
- **Import .db** — `DB_ImportPick()` opens a file picker, writes the chosen file
  into the FS (`FS.writeFile`), and the next frame `DB_ImportPoll()` reopens the
  database from it and flushes it into IndexedDB so it persists too.

Import is asynchronous (the file dialog returns later), so completion is polled
once per frame in the main loop rather than blocked on.

### Want real cloud sync?

Hosting can't do it, but Firebase's **data** products can. The lightest option
that keeps the SQLite design intact is **Firebase Storage**: upload the bytes of
`db_<user>.db` (the same bytes `Export` downloads) and `getBytes()` them back on
another device. That's an additive feature — none of the above changes.

---

## 6. File / function reference

| Concern | Where |
|--------|-------|
| Compile SQLite to wasm | `Makefile` → `sqlite3.web.o` rule |
| Web link flags (`ASYNCIFY`, `FORCE_FILESYSTEM`, `-lidbfs.js`) | `Makefile` → `WEB_LDFLAGS` |
| Mount + initial load | `DB_PersistInit()` in `src/db.h`, called from `main()` |
| Flush helper | `DB_Persist()` in `src/db.h` |
| Web DB path (`/persist/...`) | `db_path()` in `src/db.h` |
| Journal mode switch | `DB_Open()` in `src/db.h` |
| Export / Import | `DB_ExportDownload()`, `DB_ImportPick()`, `DB_ImportPoll()` in `src/db.h`; buttons in `src/ui.c`; poll in `src/main.c` |

---

## 7. Gotchas worth remembering

- **`EM_ASM` and commas.** Inside `EM_ASM({...})`, the C preprocessor only treats
  commas inside `()` as protected — **not** commas inside `{}`. A top-level
  `var a = x, b = y;` will be mis-parsed as extra macro arguments. Use separate
  statements.
- **Synchronous SQLite, asynchronous storage.** SQLite's file I/O is synchronous;
  `FS.syncfs` is not. The initial load is the only place we must wait — handled
  by ASYNCIFY + `emscripten_sleep`. Flushes are fire-and-forget.
- **Don't expect WAL semantics on web.** We deliberately use `journal_mode=DELETE`
  there (see §3.3).
- **A fresh seed is persisted immediately** (`InitPlayerDB` → `DB_Persist`), so a
  new user who never edits anything still keeps their database after a reload.
