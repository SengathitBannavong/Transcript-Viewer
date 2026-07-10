/*
 * pdf_export.c — dependency-free PDF export of the current transcript.
 *
 * A tiny PDF-1.4 writer (standard-14 fonts, no embedding) plus a composer that
 * lays the transcript out BY TERM when per-term attempts exist, else BY TYPE.
 * See pdf_export.h for the public contract.
 */

#include "pdf_export.h"
#include "app_data.h"
#include "db.h"
#include "score_logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>   /* strcasecmp */
#include <stdarg.h>
#include <stdint.h>
#include <time.h>

#if defined(PLATFORM_WEB)
#include <emscripten.h>
#endif

/* ── Page geometry (A4, PostScript points) ──────────────────────────────── */
#define PDF_PAGE_W    595.0f
#define PDF_PAGE_H    842.0f
#define PDF_MARGIN_X   50.0f
#define PDF_TOP_Y     800.0f
#define PDF_BOTTOM_Y   52.0f
#define PDF_MAX_LINES  4096
#define PDF_MAX_PAGES   200

/* Font ids used both in the resource dict (/F1../F3) and per-line. */
enum { F_REG = 1, F_BOLD = 2, F_MONO = 3 };

/* Palette (print-friendly, opaque RGB). Used only as whole 3-arg groups —
 * never inside a ?: (the commas would mis-parse as separate arguments). */
#define COL_INK   30,  30,  30
#define COL_GRAY 110, 110, 110
#define COL_HDR   40,  58, 110      /* section headings — muted navy */

typedef struct {
    char          text[240];
    unsigned char font;         /* F_REG / F_BOLD / F_MONO   */
    unsigned char r, g, b;
    float         size;
    float         gap;          /* extra space above the line */
} PdfLine;

typedef struct {
    PdfLine lines[PDF_MAX_LINES];
    int     n;
} PdfDoc;

/* ── Unicode → ASCII (strip Vietnamese / Latin diacritics) ──────────────────
 * The standard-14 fonts only speak single-byte encodings, so transliterate to
 * keep imported Vietnamese course names legible instead of garbled. */
static char uni_to_ascii(uint32_t cp)
{
    if (cp < 0x80) return (char)cp;

    switch (cp) {                                   /* Latin-1 Supplement */
        case 0xC0: case 0xC1: case 0xC2: case 0xC3:
        case 0xC4: case 0xC5:                 return 'A';
        case 0xC7:                            return 'C';
        case 0xC8: case 0xC9: case 0xCA: case 0xCB: return 'E';
        case 0xCC: case 0xCD: case 0xCE: case 0xCF: return 'I';
        case 0xD0:                            return 'D';
        case 0xD1:                            return 'N';
        case 0xD2: case 0xD3: case 0xD4: case 0xD5:
        case 0xD6: case 0xD8:                 return 'O';
        case 0xD9: case 0xDA: case 0xDB: case 0xDC: return 'U';
        case 0xDD:                            return 'Y';
        case 0xE0: case 0xE1: case 0xE2: case 0xE3:
        case 0xE4: case 0xE5:                 return 'a';
        case 0xE7:                            return 'c';
        case 0xE8: case 0xE9: case 0xEA: case 0xEB: return 'e';
        case 0xEC: case 0xED: case 0xEE: case 0xEF: return 'i';
        case 0xF0:                            return 'd';
        case 0xF1:                            return 'n';
        case 0xF2: case 0xF3: case 0xF4: case 0xF5:
        case 0xF6: case 0xF8:                 return 'o';
        case 0xF9: case 0xFA: case 0xFB: case 0xFC: return 'u';
        case 0xFD: case 0xFF:                 return 'y';
    }
    switch (cp) {                                   /* Latin Extended-A  */
        case 0x100: case 0x102: return 'A';  case 0x101: case 0x103: return 'a';
        case 0x110:             return 'D';  case 0x111:             return 'd';
        case 0x128:             return 'I';  case 0x129:             return 'i';
        case 0x168:             return 'U';  case 0x169:             return 'u';
        case 0x1A0:             return 'O';  case 0x1A1:             return 'o';
        case 0x1AF:             return 'U';  case 0x1B0:             return 'u';
    }
    /* Latin Extended Additional — Vietnamese tone marks. In this block the
     * uppercase form is the even codepoint, lowercase the odd one. */
    if (cp >= 0x1EA0 && cp <= 0x1EF9) {
        char base;
        if      (cp <= 0x1EB7) base = 'a';
        else if (cp <= 0x1EC7) base = 'e';
        else if (cp <= 0x1ECB) base = 'i';
        else if (cp <= 0x1EE3) base = 'o';
        else if (cp <= 0x1EF1) base = 'u';
        else                   base = 'y';
        return (cp & 1) ? base : (char)(base - 'a' + 'A');
    }
    return 0;                                       /* no sensible mapping */
}

/* Decode UTF-8 `src` to printable ASCII in `dst` (always NUL-terminated). */
static void ascii_sanitize(const char *src, char *dst, int dstsz)
{
    const unsigned char *s = (const unsigned char *)src;
    int di = 0;
    while (*s && di < dstsz - 1) {
        unsigned char c = *s;
        uint32_t cp; int adv;
        if      (c < 0x80)          { cp = c;        adv = 1; }
        else if ((c >> 5) == 0x6)   { cp = c & 0x1F; adv = 2; }
        else if ((c >> 4) == 0xE)   { cp = c & 0x0F; adv = 3; }
        else if ((c >> 3) == 0x1E)  { cp = c & 0x07; adv = 4; }
        else                        { cp = 0;        adv = 1; }
        for (int k = 1; k < adv; k++) {
            if ((s[k] & 0xC0) != 0x80) { adv = 1; cp = 0; break; }
            cp = (cp << 6) | (s[k] & 0x3F);
        }
        s += adv;
        char a = uni_to_ascii(cp);
        if (a == 0)                a = '?';
        if (a < 0x20 || a > 0x7E)  a = ' ';         /* tabs/controls → space */
        dst[di++] = a;
    }
    dst[di] = '\0';
}

/* Append one formatted, sanitized line to the document. */
static void pdf_add(PdfDoc *d, int font, float size, float gap,
                    unsigned char r, unsigned char g, unsigned char b,
                    const char *fmt, ...)
{
    if (d->n >= PDF_MAX_LINES) return;
    char raw[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(raw, sizeof raw, fmt, ap);
    va_end(ap);

    PdfLine *ln = &d->lines[d->n++];
    ascii_sanitize(raw, ln->text, sizeof ln->text);
    ln->font = (unsigned char)font;
    ln->size = size;
    ln->gap  = gap;
    ln->r = r; ln->g = g; ln->b = b;
}

/* ── Growable string buffer for page content streams ────────────────────── */
static void sb_append(char **buf, int *pos, int *cap, const char *fmt, ...)
{
    for (;;) {
        va_list ap;
        va_start(ap, fmt);
        int avail = *cap - *pos;
        int need  = vsnprintf(*buf + *pos, (size_t)(avail > 0 ? avail : 0), fmt, ap);
        va_end(ap);
        if (need >= 0 && need < avail) { *pos += need; return; }
        int ncap = *cap > 0 ? *cap * 2 : 4096;
        while (ncap <= *pos + (need > 0 ? need : 0)) ncap *= 2;
        char *nb = realloc(*buf, (size_t)ncap);
        if (!nb) return;                             /* drop on OOM */
        *buf = nb; *cap = ncap;
    }
}

/* ── PDF file emitter ───────────────────────────────────────────────────── */
static bool pdf_write(const PdfDoc *d, FILE *f)
{
    static int   line_pg[PDF_MAX_LINES];
    static float line_y [PDF_MAX_LINES];

    /* 1. Paginate: assign every line a page index and a baseline y. */
    int   pages = 1, used = 0;
    float y = PDF_TOP_Y;
    for (int i = 0; i < d->n; i++) {
        const PdfLine *ln = &d->lines[i];
        float lead = ln->size * 1.32f;
        float ny   = y - ln->gap;
        if (ny - lead < PDF_BOTTOM_Y) {              /* need a fresh page */
            if (pages >= PDF_MAX_PAGES) break;       /* truncate, stay valid */
            pages++;
            ny = PDF_TOP_Y - ln->gap;
        }
        ny -= lead;
        line_pg[i] = pages - 1;
        line_y[i]  = ny;
        y = ny;
        used = i + 1;
    }

    /* 2. Build each page's content stream. */
    char **content = calloc((size_t)pages, sizeof *content);
    int   *clen    = calloc((size_t)pages, sizeof *clen);
    if (!content || !clen) { free(content); free(clen); return false; }

    for (int p = 0; p < pages; p++) {
        int cap = 0, pos = 0;
        char *buf = NULL;
        for (int i = 0; i < used; i++) {
            if (line_pg[i] != p) continue;
            const PdfLine *ln = &d->lines[i];
            char esc[512]; int ei = 0;
            for (const char *t = ln->text; *t && ei < (int)sizeof esc - 2; t++) {
                if (*t == '(' || *t == ')' || *t == '\\') esc[ei++] = '\\';
                esc[ei++] = *t;
            }
            esc[ei] = '\0';
            sb_append(&buf, &pos, &cap,
                "%.3f %.3f %.3f rg\nBT /F%d %.2f Tf %.2f %.2f Td (%s) Tj ET\n",
                ln->r / 255.0, ln->g / 255.0, ln->b / 255.0,
                (int)ln->font, ln->size, PDF_MARGIN_X, line_y[i], esc);
        }
        if (!buf) { buf = calloc(1, 1); }            /* empty page guard */
        content[p] = buf;
        clen[p]    = pos;
    }

    /* 3. Emit objects, tracking byte offsets for the xref table.
     *    1 Catalog · 2 Pages · 3-5 Fonts · 6.. Pages · then Content streams. */
    int  page_obj0    = 6;
    int  content_obj0 = 6 + pages;
    int  total        = 5 + 2 * pages;
    long off[6 + 2 * PDF_MAX_PAGES];

    static const char hdr[] = "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";
    fwrite(hdr, 1, sizeof hdr - 1, f);

    off[1] = ftell(f);
    fprintf(f, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");

    off[2] = ftell(f);
    fprintf(f, "2 0 obj\n<< /Type /Pages /Count %d /Kids [", pages);
    for (int p = 0; p < pages; p++) fprintf(f, "%d 0 R ", page_obj0 + p);
    fprintf(f, "] >>\nendobj\n");

    off[3] = ftell(f);
    fprintf(f, "3 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica "
               "/Encoding /WinAnsiEncoding >>\nendobj\n");
    off[4] = ftell(f);
    fprintf(f, "4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold "
               "/Encoding /WinAnsiEncoding >>\nendobj\n");
    off[5] = ftell(f);
    fprintf(f, "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Courier "
               "/Encoding /WinAnsiEncoding >>\nendobj\n");

    for (int p = 0; p < pages; p++) {
        off[page_obj0 + p] = ftell(f);
        fprintf(f,
            "%d 0 obj\n<< /Type /Page /Parent 2 0 R "
            "/MediaBox [0 0 %d %d] "
            "/Resources << /Font << /F1 3 0 R /F2 4 0 R /F3 5 0 R >> >> "
            "/Contents %d 0 R >>\nendobj\n",
            page_obj0 + p, (int)PDF_PAGE_W, (int)PDF_PAGE_H, content_obj0 + p);
    }
    for (int p = 0; p < pages; p++) {
        off[content_obj0 + p] = ftell(f);
        fprintf(f, "%d 0 obj\n<< /Length %d >>\nstream\n",
                content_obj0 + p, clen[p]);
        fwrite(content[p], 1, (size_t)clen[p], f);
        fprintf(f, "endstream\nendobj\n");
    }

    /* 4. Cross-reference table + trailer. */
    long xref = ftell(f);
    fprintf(f, "xref\n0 %d\n", total + 1);
    fprintf(f, "0000000000 65535 f \n");
    for (int i = 1; i <= total; i++)
        fprintf(f, "%010ld 00000 n \n", off[i]);
    fprintf(f, "trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n",
            total + 1, xref);

    for (int p = 0; p < pages; p++) free(content[p]);
    free(content);
    free(clen);
    return ferror(f) == 0;
}

/* ── Transcript composition ─────────────────────────────────────────────── */

/* Shared monospace column header for course tables. */
static void add_course_header(PdfDoc *d)
{
    pdf_add(d, F_MONO, 9, 10, COL_HDR,
            "%-9s %-46s %3s %6s %6s  %-3s",
            "CODE", "COURSE", "CR", "MID", "FIN", "GR");
}

/* One monospace course row. mid/final render as "-" when unset (portal import
 * supplies only the letter). failed rows are tinted red. */
static void add_course_row(PdfDoc *d, const char *code, const char *name,
                           int credit, float mid, float fin,
                           const char *letter, bool failed)
{
    char nm[200], ms[8], fs[8];
    ascii_sanitize(name, nm, sizeof nm);
    if (strlen(nm) > 46) { nm[45] = '.'; nm[44] = '.'; nm[46] = '\0'; }
    if (mid > 0.001f) snprintf(ms, sizeof ms, "%.1f", mid); else strcpy(ms, "-");
    if (fin > 0.001f) snprintf(fs, sizeof fs, "%.1f", fin); else strcpy(fs, "-");

    unsigned char r = 30, g = 30, b = 30;            /* ink */
    if (failed) { r = 170; g = 45; b = 40; }         /* fail red */
    pdf_add(d, F_MONO, 9, 1, r, g, b,
            "%-9s %-46s %3d %6s %6s  %-3s",
            code, nm, credit, ms, fs, letter);
}

/* Fill `d` with the header/summary + either the by-term or by-type body.
 * `requested` selects the grouping; AUTO (and a TERM request with no per-term
 * data) resolve to whichever layout is possible. Returns the layout actually
 * used (PDF_GROUP_TERM or PDF_GROUP_TYPE). */
static PdfGroupMode compose_transcript(PdfDoc *d, PdfGroupMode requested)
{
    Player *p    = &gApp.player;
    const char *user = gApp.user_name[0] ? gApp.user_name : "student";

    /* Resolve the effective layout. By-term needs imported attempts. */
    bool have_terms = (gApp.attempt_count > 0);
    PdfGroupMode mode = requested;
    if (mode == PDF_GROUP_AUTO) mode = have_terms ? PDF_GROUP_TERM : PDF_GROUP_TYPE;
    if (mode == PDF_GROUP_TERM && !have_terms) mode = PDF_GROUP_TYPE;

    char datebuf[40];
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (lt) strftime(datebuf, sizeof datebuf, "%Y-%m-%d %H:%M", lt);
    else    snprintf(datebuf, sizeof datebuf, "unknown");

    float cpa_all  = calc_cpa(p, 0);
    float cpa_pass = calc_cpa(p, 1);
    int   eff      = calc_effective_credits(p);
    int   req      = calc_required_credits(p);
    bool  grad_ok  = p->status_can_grauate;
    HonorTier tier = honor_tier(cpa_pass);

    /* ── Title + identity ── */
    pdf_add(d, F_BOLD, 22, 0,  COL_INK,  "Academic Transcript");
    pdf_add(d, F_REG,  11, 8,  COL_GRAY, "Student: %s", user);
    pdf_add(d, F_REG,   9, 2,  COL_GRAY, "Generated: %s    |    Scale: HUST 4.0",
            datebuf);

    /* ── Summary block ── */
    pdf_add(d, F_BOLD, 12, 16, COL_INK, "Summary");
    pdf_add(d, F_MONO, 10, 5,  COL_INK, "CPA (all attempts) : %.2f / 4.00", cpa_all);
    pdf_add(d, F_MONO, 10, 2,  COL_INK, "CPA (passed only)  : %.2f / 4.00", cpa_pass);
    pdf_add(d, F_MONO, 10, 2,  COL_INK, "Credits earned     : %d of %d required",
            eff, req);
    pdf_add(d, F_MONO, 10, 2,  COL_INK, "Honour standing    : %s", honor_name(tier));
    {
        unsigned char r = 170, g = 45, b = 40;       /* not-eligible red */
        if (grad_ok) { r = 40; g = 110; b = 70; }    /* eligible green   */
        pdf_add(d, F_MONO, 10, 2, r, g, b,
                "Graduation status  : %s",
                grad_ok ? "Eligible" : "Not yet eligible");
    }

    /* ── Body: the resolved grouping ── */
    if (mode == PDF_GROUP_TERM) {
        AttemptRow *A = gApp.attempts;
        int n = gApp.attempt_count;

        pdf_add(d, F_BOLD, 15, 22, COL_INK, "Grades by Term");
        pdf_add(d, F_REG,   9, 3,  COL_GRAY,
                "Every attempt is listed under the term it was taken in "
                "(retakes and newly-added subjects included).");

        for (int i = 0; i < n; ) {
            int term = A[i].term;
            int j = i;
            float sumPts = 0.f; int sumCr = 0;
            while (j < n && A[j].term == term) {
                if (A[j].credit > 0) {
                    int plus = (A[j].letter[1] == '+');
                    sumPts += score_to_gpa(A[j].letter[0], plus) * (float)A[j].credit;
                    sumCr  += A[j].credit;
                }
                j++;
            }
            float gpa = (sumCr > 0) ? sumPts / (float)sumCr : 0.f;
            int year = term / 10, sem = term % 10;
            const char *semlbl = (sem == 1) ? "HK1" : (sem == 2) ? "HK2"
                               : (sem == 3) ? "HK he" : "HK?";

            pdf_add(d, F_BOLD, 12, 16, COL_HDR,
                    "%d-%d  %-6s      GPA %.2f      %d cr",
                    year, year + 1, semlbl, gpa, sumCr);
            add_course_header(d);
            for (int k = i; k < j; k++) {
                bool failed = (A[k].letter[0] == 'F');
                add_course_row(d, A[k].code, A[k].name, A[k].credit,
                               A[k].mid, A[k].final_, A[k].letter, failed);
            }
            i = j;
        }
        return PDF_GROUP_TERM;
    }

    /* By subject type. */
    pdf_add(d, F_BOLD, 15, 22, COL_INK, "Grades by Type");
    pdf_add(d, F_REG,   9, 3,  COL_GRAY,
            "Grades grouped by subject type.");

    int any = 0;
    for (int t = 1; t < sizeSubjectType; t++) {
        Subject_Type *st = &p->numofSubjectType[t];
        if (!st->head) continue;

        /* Only sections that actually hold a studied grade. */
        int studied = 0;
        for (Subject_Node *node = st->head; node; node = node->next)
            if (node->score_letter != 'X') { studied = 1; break; }
        if (!studied) continue;
        any = 1;

        /* Show progress against the graduation requirement for this type
         * (the rule-resolved limit), not the raw sum of every subject's
         * credits — mirrors the dashboard's "This section" figure. For
         * subject-count types (e.g. sport) the figures are subject counts. */
        float cpaT     = calc_cpa_type(p, t, 0);
        int   req_pass = _sl_resolve_pass(p, t);
        int   req_lim  = _sl_resolve_limit(p, t);
        const char *unit =
            (gApp.grad_rules[t].mode == GRAD_SUBJECT_COUNT) ? "subjects" : "cr";
        pdf_add(d, F_BOLD, 12, 16, COL_HDR,
                "%s      passed %d/%d %s      CPA %.2f",
                gApp.type_name[t], req_pass, req_lim, unit, cpaT);
        add_course_header(d);
        for (Subject_Node *node = st->head; node; node = node->next) {
            if (node->score_letter == 'X') continue;   /* not taken yet */
            int  plus   = (node->status_ever_been_study & 2) != 0;
            bool failed = (node->status_pass == 0);
            char letter[4];
            snprintf(letter, sizeof letter, "%c%s",
                     node->score_letter, plus ? "+" : "");
            add_course_row(d, node->ID, node->name, (int)node->credit,
                           node->score_number_mid, node->score_number_final,
                           letter, failed);
        }
    }
    if (!any)
        pdf_add(d, F_REG, 11, 18, COL_GRAY,
                "No grades recorded yet. Enter scores or import a portal "
                "transcript, then export again.");
    return PDF_GROUP_TYPE;
}

/* ── Simulation report composition ──────────────────────────────────────── */

/* Locate a subject node by its code across every category; sets *type_out. */
static Subject_Node *sim_find_subject(Player *p, const char *code, int *type_out)
{
    for (int t = 1; t < sizeSubjectType; t++)
        for (Subject_Node *n = p->numofSubjectType[t].head; n; n = n->next)
            if (strcmp(n->ID, code) == 0) {
                if (type_out) *type_out = t;
                return n;
            }
    return NULL;
}

/* Fill `d` with a report of the committed sandbox overrides: overall CPA
 * before/after (both modes), the list of changed courses, and per-category
 * before/after for the categories a change actually moved. */
static void compose_simulation(PdfDoc *d)
{
    Player *p        = &gApp.player;
    const char *user = gApp.user_name[0] ? gApp.user_name : "student";

    char datebuf[40];
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (lt) strftime(datebuf, sizeof datebuf, "%Y-%m-%d %H:%M", lt);
    else    snprintf(datebuf, sizeof datebuf, "unknown");

    pdf_add(d, F_BOLD, 22, 0,  COL_INK,  "CPA Simulation Report");
    pdf_add(d, F_REG,  11, 8,  COL_GRAY, "Student: %s", user);
    pdf_add(d, F_REG,   9, 2,  COL_GRAY, "Generated: %s    |    Scale: HUST 4.0", datebuf);
    pdf_add(d, F_REG,   9, 2,  COL_GRAY,
            "Hypothetical only - your stored grades are unchanged.");

    int nov = gApp.sandbox_override_count;
    if (nov <= 0) {
        pdf_add(d, F_REG, 12, 20, COL_INK,
                "No simulated changes are active. Open the Planner, adjust grades "
                "in the sandbox, click Apply & Recalculate, then export again.");
        return;
    }

    /* ── Overall CPA, both modes ── */
    QpCategory a0 = calc_qp_overall(p, 0, 0), s0 = calc_qp_overall(p, 0, 1);
    QpCategory a1 = calc_qp_overall(p, 1, 0), s1 = calc_qp_overall(p, 1, 1);

    pdf_add(d, F_BOLD, 12, 16, COL_INK, "Overall CPA");
    pdf_add(d, F_MONO, 10, 5,  COL_INK,
            "All attempts : %.2f  ->  %.2f   (%+.2f)", a0.cpa, s0.cpa, s0.cpa - a0.cpa);
    pdf_add(d, F_MONO, 10, 2,  COL_INK,
            "Passed only  : %.2f  ->  %.2f   (%+.2f)", a1.cpa, s1.cpa, s1.cpa - a1.cpa);

    /* ── Changed courses ── */
    pdf_add(d, F_BOLD, 12, 16, COL_INK, "Simulated Changes (%d)", nov);
    pdf_add(d, F_MONO,  9, 10, COL_HDR,
            "%-9s %-40s %3s  %-5s %-5s", "CODE", "COURSE", "CR", "NOW", "SIM");
    for (int i = 0; i < nov; i++) {
        SandboxOverride *ov = &gApp.sandbox_overrides[i];
        int t = 0;
        Subject_Node *n = sim_find_subject(p, ov->subject_id, &t);
        char sim_g[5];
        snprintf(sim_g, sizeof sim_g, "%c%s", ov->grade_letter, ov->plus ? "+" : "");
        if (n) {
            int  plus = (n->status_ever_been_study & 2) != 0;
            char now_g[5], nm[200];
            if (n->score_letter == 'X') snprintf(now_g, sizeof now_g, "-");
            else snprintf(now_g, sizeof now_g, "%c%s", n->score_letter, plus ? "+" : "");
            ascii_sanitize(n->name, nm, sizeof nm);
            if (strlen(nm) > 39) { nm[38] = '.'; nm[37] = '.'; nm[39] = '\0'; }
            pdf_add(d, F_MONO, 9, 2, COL_INK, "%-9s %-40s %3d  %-5s %-5s",
                    ov->subject_id, nm, (int)n->credit, now_g, sim_g);
        } else {
            pdf_add(d, F_MONO, 9, 2, COL_INK, "%-9s %-40s %3s  %-5s %-5s",
                    ov->subject_id, "(unknown course)", "-", "-", sim_g);
        }
    }

    /* ── Per-category before/after (only categories a change moved) ── */
    pdf_add(d, F_BOLD, 12, 16, COL_INK, "By Category (all attempts)");
    pdf_add(d, F_MONO,  9, 10, COL_HDR,
            "%-28s %7s %7s %9s", "CATEGORY", "CPA", "SIM", "d QP");
    int shown = 0;
    for (int t = 1; t < sizeSubjectType; t++) {
        if (gApp.type_name[t][0] == 0 ||
            p->numofSubjectType[t].Total_Subject == 0) continue;
        QpCategory a = calc_qp_type(p, t, 0, 0), s = calc_qp_type(p, t, 0, 1);
        if (a.cpa == s.cpa && a.qp == s.qp) continue;      /* unchanged */
        char nm[40];
        ascii_sanitize(gApp.type_name[t], nm, sizeof nm);
        if (strlen(nm) > 27) { nm[26] = '.'; nm[25] = '.'; nm[27] = '\0'; }
        pdf_add(d, F_MONO, 9, 2, COL_INK, "%-28s %7.2f %7.2f %+9.1f",
                nm, a.cpa, s.cpa, s.qp - a.qp);
        shown++;
    }
    if (!shown)
        pdf_add(d, F_REG, 9, 6, COL_GRAY,
                "No category CPA moved (changes offset within a category).");
}

/* ── Optional native "save as" dialog (best effort) ─────────────────────── */
#if !defined(PLATFORM_WEB)
/* Run a shell dialog and capture its first stdout line. 1 = a path came back. */
static int pdf_dialog(const char *cmd, char *out, size_t n)
{
    FILE *pp = popen(cmd, "r");
    if (!pp) return 0;
    out[0] = '\0';
    if (!fgets(out, (int)n, pp)) { pclose(pp); return 0; }
    int rc = pclose(pp);
    size_t L = strlen(out);
    while (L && (out[L-1] == '\n' || out[L-1] == '\r')) out[--L] = '\0';
    return (rc == 0 && L > 0);
}

/* Ask the desktop where to save. Returns 1 = picked (dst filled),
 * 0 = no dialog tool available, -1 = a dialog ran but the user cancelled. */
static int pdf_pick_path(const char *suggest, char *dst, size_t n)
{
    char cmd[1024];
    if (tv_have("zenity")) {
        snprintf(cmd, sizeof cmd,
            "zenity --file-selection --save --confirm-overwrite "
            "--title='Export transcript to PDF' --filename='%s' "
            "--file-filter='PDF | *.pdf' 2>/dev/null", suggest);
        return pdf_dialog(cmd, dst, n) ? 1 : -1;
    }
    if (tv_have("kdialog")) {
        snprintf(cmd, sizeof cmd,
            "kdialog --getsavefilename '%s' '*.pdf|PDF document' 2>/dev/null",
            suggest);
        return pdf_dialog(cmd, dst, n) ? 1 : -1;
    }
    return 0;
}
#endif

/* Ensure `path` ends in ".pdf". */
static void ensure_pdf_ext(char *path, int sz)
{
    size_t L = strlen(path);
    if (L >= 4 && strcasecmp(path + L - 4, ".pdf") == 0) return;
    snprintf(path + L, (size_t)sz - L, ".pdf");
}

bool PDF_TermsAvailable(void)
{
    return gApp.attempt_count > 0;
}

bool PDF_ExportTranscript(PdfGroupMode mode,
                          char *out_path, int path_sz, char *msg, int msg_sz)
{
    const char *user = gApp.user_name[0] ? gApp.user_name : "student";
    char path[512];

#if defined(PLATFORM_WEB)
    snprintf(path, sizeof path, "/persist/transcript_%s.pdf", user);
#else
    const char *home = getenv("HOME");
    char suggest[512];
    snprintf(suggest, sizeof suggest, "%s/transcript_%s.pdf",
             home ? home : ".", user);
    int picked = pdf_pick_path(suggest, path, sizeof path);
    if (picked == -1) {                              /* user cancelled */
        if (msg) snprintf(msg, msg_sz, "Export cancelled.");
        return false;
    }
    if (picked == 0)                                 /* headless fallback */
        snprintf(path, sizeof path, "transcript_%s.pdf", user);
    ensure_pdf_ext(path, sizeof path);
#endif

    PdfDoc *doc = calloc(1, sizeof *doc);
    if (!doc) {
        if (msg) snprintf(msg, msg_sz, "Export failed: out of memory.");
        return false;
    }
    PdfGroupMode used = compose_transcript(doc, mode);

    FILE *f = fopen(path, "wb");
    if (!f) {
        free(doc);
        if (msg) snprintf(msg, msg_sz, "Export failed: cannot write %s", path);
        return false;
    }
    bool ok = pdf_write(doc, f);
    if (fclose(f) != 0) ok = false;
    free(doc);

    if (!ok) {
        if (msg) snprintf(msg, msg_sz, "Export failed while writing the PDF.");
        return false;
    }

#if defined(PLATFORM_WEB)
    DB_Persist();                                    /* flush /persist to IDB */
    /* Hand the file to the browser as a download. */
    EM_ASM({
        var p = UTF8ToString($0), name = UTF8ToString($1);
        try {
            var data = FS.readFile(p);
            var blob = new Blob([data], { type: 'application/pdf' });
            var url  = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url; a.download = name;
            document.body.appendChild(a); a.click(); document.body.removeChild(a);
            setTimeout(function () { URL.revokeObjectURL(url); }, 4000);
        } catch (e) { console.error('PDF download failed', e); }
    }, path, user);
#endif

    if (out_path) snprintf(out_path, path_sz, "%s", path);
    if (msg) snprintf(msg, msg_sz, "Exported %s transcript to %s",
                      used == PDF_GROUP_TERM ? "by-term" : "by-type", path);
    return true;
}

bool PDF_ExportSimulation(char *out_path, int path_sz, char *msg, int msg_sz)
{
    const char *user = gApp.user_name[0] ? gApp.user_name : "student";
    char path[512];

#if defined(PLATFORM_WEB)
    snprintf(path, sizeof path, "/persist/simulation_%s.pdf", user);
#else
    const char *home = getenv("HOME");
    char suggest[512];
    snprintf(suggest, sizeof suggest, "%s/simulation_%s.pdf",
             home ? home : ".", user);
    int picked = pdf_pick_path(suggest, path, sizeof path);
    if (picked == -1) {                              /* user cancelled */
        if (msg) snprintf(msg, msg_sz, "Export cancelled.");
        return false;
    }
    if (picked == 0)                                 /* headless fallback */
        snprintf(path, sizeof path, "simulation_%s.pdf", user);
    ensure_pdf_ext(path, sizeof path);
#endif

    PdfDoc *doc = calloc(1, sizeof *doc);
    if (!doc) {
        if (msg) snprintf(msg, msg_sz, "Export failed: out of memory.");
        return false;
    }
    compose_simulation(doc);

    FILE *f = fopen(path, "wb");
    if (!f) {
        free(doc);
        if (msg) snprintf(msg, msg_sz, "Export failed: cannot write %s", path);
        return false;
    }
    bool ok = pdf_write(doc, f);
    if (fclose(f) != 0) ok = false;
    free(doc);

    if (!ok) {
        if (msg) snprintf(msg, msg_sz, "Export failed while writing the PDF.");
        return false;
    }

#if defined(PLATFORM_WEB)
    DB_Persist();                                    /* flush /persist to IDB */
    EM_ASM({
        var p = UTF8ToString($0), name = UTF8ToString($1);
        try {
            var data = FS.readFile(p);
            var blob = new Blob([data], { type: 'application/pdf' });
            var url  = URL.createObjectURL(blob);
            var a = document.createElement('a');
            a.href = url; a.download = name;
            document.body.appendChild(a); a.click(); document.body.removeChild(a);
            setTimeout(function () { URL.revokeObjectURL(url); }, 4000);
        } catch (e) { console.error('PDF download failed', e); }
    }, path, user);
#endif

    if (out_path) snprintf(out_path, path_sz, "%s", path);
    if (msg) snprintf(msg, msg_sz,
                      gApp.sandbox_override_count > 0
                          ? "Exported simulation to %s"
                          : "Exported simulation (no changes active) to %s", path);
    return true;
}
