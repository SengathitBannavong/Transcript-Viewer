#pragma once

/*
 * pdf_export.h — export the current player's transcript to a real .pdf file.
 *
 * No external dependencies: a small standard-14-font PDF writer lives in
 * pdf_export.c. The caller picks how the grades are grouped:
 *
 *   • PDF_GROUP_TERM — one section per semester, every attempt listed under the
 *     term it was taken in (retakes / newly-added subjects included, nothing
 *     masked or merged). Needs imported per-term data — see PDF_TermsAvailable().
 *   • PDF_GROUP_TYPE — subject-type sections, the same breakdown shown on the
 *     dashboard. Always available.
 *   • PDF_GROUP_AUTO — by term when per-term data exists, else by type.
 *
 * A PDF_GROUP_TERM request with no per-term data falls back to by-type.
 *
 * Writes "transcript_<user>.pdf" next to the database. On success returns true
 * and fills out_path with the file path and msg with a short status line; on
 * failure returns false and msg holds the reason. Any of out_path/msg may be
 * NULL.
 */

#include <stdbool.h>

typedef enum {
    PDF_GROUP_AUTO = 0,
    PDF_GROUP_TERM = 1,
    PDF_GROUP_TYPE = 2,
} PdfGroupMode;

/* True when a By-Term export is possible (per-term attempts have been imported).
 * Use this to enable/disable the "By Term" control. */
bool PDF_TermsAvailable(void);

bool PDF_ExportTranscript(PdfGroupMode mode,
                          char *out_path, int path_sz, char *msg, int msg_sz);
