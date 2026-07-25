#ifndef TV_PLATFORM_H
#define TV_PLATFORM_H

#include <stddef.h>

/* ── Platform abstraction ──────────────────────────────────────────────────
 * Thin wrapper over the handful of things that genuinely differ between
 * Windows and the Unix desktops: where the user's files live, and how to ask
 * them to pick a path. Everything else in the app is portable C.
 * ─────────────────────────────────────────────────────────────────────── */

#if defined(_WIN32)
#define TV_PATH_SEP '\\'
#else
#define TV_PATH_SEP '/'
#endif

/* Absolute path to the user's home / profile directory, or "." when the
 * environment does not name one. Never returns NULL; the result points at
 * static storage and stays valid for the life of the process. */
const char *TV_HomeDir(void);

/* Native "save as" / "open" dialogs.
 *
 * Returns:
 *    1  the user picked a path (written to out)
 *    0  no dialog is available on this system — the caller should fall back
 *       to a non-interactive default path
 *   -1  a dialog appeared and the user cancelled
 *
 * `desc`/`ext` describe the file-type filter, e.g. "PDF document" / "pdf".
 * `suggest` is a full path used to seed the filename and starting folder.
 *
 * Neither call changes the process working directory: the app resolves
 * assets/, Font/ and db_<user>.db relative to it, so a dialog that wandered
 * off would break the next asset load. */
int TV_PickSavePath(const char *title, const char *suggest,
                    const char *desc, const char *ext,
                    char *out, size_t n);

int TV_PickOpenPath(const char *title,
                    const char *desc, const char *ext,
                    char *out, size_t n);

#endif /* TV_PLATFORM_H */
