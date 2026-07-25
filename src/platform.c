#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ══════════════════════════════════════════════════════════════════════════
 *  Windows
 * ══════════════════════════════════════════════════════════════════════════ */
#if defined(_WIN32)

#include <windows.h>
#include <commdlg.h>

const char *TV_HomeDir(void)
{
    static char home[MAX_PATH];
    if (home[0]) return home;

    const char *profile = getenv("USERPROFILE");
    if (profile && profile[0]) {
        snprintf(home, sizeof home, "%s", profile);
        return home;
    }
    /* Domain accounts sometimes only expose the split form. */
    const char *drive = getenv("HOMEDRIVE");
    const char *path  = getenv("HOMEPATH");
    if (drive && path && drive[0] && path[0]) {
        snprintf(home, sizeof home, "%s%s", drive, path);
        return home;
    }
    snprintf(home, sizeof home, ".");
    return home;
}

/* Build a comdlg32 filter: "desc (*.ext)\0*.ext\0All files (*.*)\0*.*\0\0".
 * The embedded NULs mean this cannot be assembled with snprintf. */
static void win_build_filter(char *buf, size_t n,
                             const char *desc, const char *ext)
{
    size_t i = 0;
    int    w;

    w = snprintf(buf + i, n - i, "%s (*.%s)", desc, ext);
    if (w < 0 || (size_t)w >= n - i) { buf[0] = '\0'; buf[1] = '\0'; return; }
    i += (size_t)w + 1;                       /* step past the terminating NUL */

    w = snprintf(buf + i, n - i, "*.%s", ext);
    if (w < 0 || (size_t)w >= n - i) { buf[0] = '\0'; buf[1] = '\0'; return; }
    i += (size_t)w + 1;

    w = snprintf(buf + i, n - i, "All files (*.*)");
    if (w < 0 || (size_t)w >= n - i) { buf[0] = '\0'; buf[1] = '\0'; return; }
    i += (size_t)w + 1;

    w = snprintf(buf + i, n - i, "*.*");
    if (w < 0 || (size_t)w >= n - i) { buf[0] = '\0'; buf[1] = '\0'; return; }
    i += (size_t)w + 1;

    if (i < n) buf[i] = '\0';                 /* double NUL ends the list */
}

/* Seed the dialog: comdlg32 wants the filename in lpstrFile and the folder in
 * lpstrInitialDir, so split `suggest` on the last separator. */
static void win_seed(const char *suggest, char *file, size_t file_n,
                     char *dir, size_t dir_n)
{
    file[0] = '\0';
    dir[0]  = '\0';
    if (!suggest || !suggest[0]) return;

    const char *slash = strrchr(suggest, '\\');
    const char *fwd   = strrchr(suggest, '/');
    if (fwd && (!slash || fwd > slash)) slash = fwd;

    if (slash) {
        size_t dlen = (size_t)(slash - suggest);
        if (dlen >= dir_n) dlen = dir_n - 1;
        memcpy(dir, suggest, dlen);
        dir[dlen] = '\0';
        snprintf(file, file_n, "%s", slash + 1);
    } else {
        snprintf(file, file_n, "%s", suggest);
    }
}

static int win_dialog(int save, const char *title, const char *suggest,
                      const char *desc, const char *ext,
                      char *out, size_t n)
{
    char filter[256];
    char file[MAX_PATH];
    char dir[MAX_PATH];

    win_build_filter(filter, sizeof filter, desc, ext);
    win_seed(suggest, file, sizeof file, dir, sizeof dir);

    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize     = sizeof ofn;
    ofn.hwndOwner       = GetActiveWindow();
    ofn.lpstrFilter     = filter;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = file;
    ofn.nMaxFile        = sizeof file;
    ofn.lpstrTitle      = title;
    ofn.lpstrDefExt     = ext;
    ofn.lpstrInitialDir = dir[0] ? dir : NULL;
    /* OFN_NOCHANGEDIR is load-bearing: the app loads assets/ and writes
     * db_<user>.db through relative paths, so the dialog must leave the
     * process working directory alone. */
    ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
                (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

    BOOL ok = save ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);
    if (ok) {
        snprintf(out, n, "%s", file);
        return 1;
    }
    /* A zero extended error means the user simply closed the dialog. Anything
     * else is a real failure, so let the caller fall back to a default path. */
    return (CommDlgExtendedError() == 0) ? -1 : 0;
}

int TV_PickSavePath(const char *title, const char *suggest,
                    const char *desc, const char *ext,
                    char *out, size_t n)
{
    return win_dialog(1, title, suggest, desc, ext, out, n);
}

int TV_PickOpenPath(const char *title,
                    const char *desc, const char *ext,
                    char *out, size_t n)
{
    return win_dialog(0, title, NULL, desc, ext, out, n);
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Unix desktops (zenity / kdialog, best effort)
 * ══════════════════════════════════════════════════════════════════════════ */
#else

const char *TV_HomeDir(void)
{
    const char *home = getenv("HOME");
    return (home && home[0]) ? home : ".";
}

/* True if a program is available on PATH. */
static int tv_have(const char *prog)
{
    char cmd[128];
    snprintf(cmd, sizeof cmd, "command -v %s >/dev/null 2>&1", prog);
    return system(cmd) == 0;
}

/* Run a dialog command and capture its first stdout line (newline stripped).
 * Returns 1 if a non-empty path came back. */
static int tv_dialog(const char *cmd, char *out, size_t n)
{
    FILE *p = popen(cmd, "r");
    if (!p) return 0;
    out[0] = '\0';
    if (!fgets(out, (int)n, p)) { pclose(p); return 0; }
    int rc = pclose(p);
    size_t L = strlen(out);
    while (L && (out[L - 1] == '\n' || out[L - 1] == '\r')) out[--L] = '\0';
    return (rc == 0 && L > 0);
}

int TV_PickSavePath(const char *title, const char *suggest,
                    const char *desc, const char *ext,
                    char *out, size_t n)
{
    char cmd[1024];
    if (tv_have("zenity")) {
        snprintf(cmd, sizeof cmd,
            "zenity --file-selection --save --confirm-overwrite "
            "--title='%s' --filename='%s' --file-filter='%s | *.%s' 2>/dev/null",
            title, suggest ? suggest : "", desc, ext);
        return tv_dialog(cmd, out, n) ? 1 : -1;
    }
    if (tv_have("kdialog")) {
        snprintf(cmd, sizeof cmd,
            "kdialog --getsavefilename '%s' '*.%s|%s' --title '%s' 2>/dev/null",
            suggest ? suggest : ".", ext, desc, title);
        return tv_dialog(cmd, out, n) ? 1 : -1;
    }
    return 0;
}

int TV_PickOpenPath(const char *title,
                    const char *desc, const char *ext,
                    char *out, size_t n)
{
    char cmd[1024];
    if (tv_have("zenity")) {
        snprintf(cmd, sizeof cmd,
            "zenity --file-selection --title='%s' "
            "--file-filter='%s | *.%s' --file-filter='All files | *' 2>/dev/null",
            title, desc, ext);
        return tv_dialog(cmd, out, n) ? 1 : -1;
    }
    if (tv_have("kdialog")) {
        snprintf(cmd, sizeof cmd,
            "kdialog --getopenfilename . '*.%s|%s' --title '%s' 2>/dev/null",
            ext, desc, title);
        return tv_dialog(cmd, out, n) ? 1 : -1;
    }
    return 0;
}

#endif
