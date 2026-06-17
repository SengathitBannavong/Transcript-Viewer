#pragma once

/* ─── UI Strings ──────────────────────────────────────────────────────── */

/* Sidebar / Navigation */
#define STR_LOGO_TITLE            "Transcript"
#define STR_LOGO_SUBTITLE         "Viewer v1.0"
#define STR_LOGO_BADGE            "TC"
#define STR_NAV_DASHBOARD         "Dashboard"
#define STR_NAV_PLANNER           "Planner"
#define STR_NAV_SETTINGS          "Settings"
#define STR_NAV_P_BADGE           "P"
#define STR_NAV_S_BADGE           "S"
#define STR_NAV_WARN_BADGE        "!"
#define STR_SECTION_SUBJECT_TYPES "SUBJECT TYPES"

/* Database IO Messages */
#define STR_DB_EXPORT_BTN         "Export .db"
#define STR_DB_IMPORT_BTN         "Import .db"
#define STR_DB_RELOAD_BTN         "Reload Config"
#define STR_DB_EXPORT_WEB_MSG     "Downloading db_%s.db"
#define STR_DB_EXPORT_SUCCESS     "Exported to %.220s"
#define STR_DB_EXPORT_CANCEL      "Export cancelled"
#define STR_DB_EXPORT_FAIL        "Export failed"
#define STR_DB_IMPORT_SUCCESS     "Imported %.220s"
#define STR_DB_IMPORT_FAIL        "Import failed: not a valid .db"
#define STR_DB_IMPORT_CANCEL      "Import cancelled"
#define STR_DB_IMPORT_NO_DIALOG   "No file dialog found - drag a .db onto the window"
#define STR_DB_RELOAD_SUCCESS     "Reload OK: all data valid (was %d warnings)"
#define STR_DB_RELOAD_WARN        "Reload done. %d warning%s remain"

/* User Profile */
#define STR_USER_ROLE             "Student"
#define STR_USER_EMPTY            "--"

/* Table Columns */
#define STR_TBL_HDR_NUM           "#"
#define STR_TBL_HDR_NAME          "SUBJECT NAME"
#define STR_TBL_HDR_CODE          "CODE"
#define STR_TBL_HDR_GRADE         "GR"
#define STR_TBL_HDR_MID           "MID"
#define STR_TBL_HDR_FIN           "FIN"
#define STR_TBL_HDR_PASS          "PASS"
#define STR_TBL_HDR_CREDIT        "CR"
#define STR_TBL_HDR_TERM          "SEM"

/* UI Glyphs (single-character markers) */
#define STR_INPUT_CURSOR          "|"
#define STR_INPUT_PLACEHOLDER     "-"
#define STR_INPUT_BLANK           " "
#define STR_CMD_PROMPT_ICON       ">"
#define STR_CMD_BADGE_K           "K"
#define STR_TOAST_ARROW           "->"

/* Table Rows & Grades */
#define STR_ROW_EDIT              "Edit >"
#define STR_GRADE_X               "X"
#define STR_GRADE_PLUS            "+"
#define STR_GRADE_MINUS           "-"
#define STR_PASS_YES              "YES"
#define STR_PASS_NO               "NO"
#define STR_PASS_EMPTY            "--"

/* Header Metrics */
#define STR_HDR_META_FORMAT       "%s   %d subjects   %d / %d section credits"
#define STR_HDR_CPA_LABEL         "CUMULATIVE GPA"
#define STR_HDR_CPA_MAX           "/ 4.00"
#define STR_HDR_STANDING          "STANDING"
#define STR_HDR_READY             "READY"
#define STR_HDR_NOT_READY         "NOT READY"
#define STR_HDR_CREDITS_FORMAT    "%d / %d cr"
#define STR_CTRL_K_HINT           "Ctrl+K"

/* Stat Ledger Labels */
#define STR_STAT_CREDITS_PASSED   "CREDITS PASSED"
#define STR_STAT_CREDITS_FAILED   "CREDITS FAILED"
#define STR_STAT_CPA_PASS_ONLY    "CPA (PASS ONLY)"
#define STR_STAT_TOTAL_CREDITS    "TOTAL CREDITS"
#define STR_STAT_THIS_SECTION     "THIS SECTION"

/* Academic Alerts & Warnings */
#define STR_ALERT_CAUTION         "Caution"
#define STR_ALERT_WARNING         "Warning"
#define STR_ALERT_CRITICAL        "Critical"
#define STR_ALERT_BANNER_FORMAT   "Academic Alert  [Level %d - %s]  %d studied-but-failed credits"
#define STR_WARN_BANNER_TITLE     "Data file validation errors:"
#define STR_WARN_BANNER_ITEM      "  * %s"

/* Empty Table Placeholder */
#define STR_EMPTY_CATEGORY        "No subjects in this category."

/* Table Footer Metrics */
#define STR_TBL_FOOTER_FORMAT     "Showing %d subjects  |  %d passed  |  %d/%d credits  |  CPA %.2f"
#define STR_TBL_FOOTER_PASSED_CR  "Total passed credits: %d"

/* Dashboard */
#define STR_DASH_OVERVIEW_FORMAT  "Overview for %s  |  %d total subjects"
#define STR_DASH_STANDING_LABEL   "ACADEMIC STANDING"
#define STR_DASH_GRAD_READY       "Ready to graduate"
#define STR_DASH_GRAD_PROGRESS    "In progress"
#define STR_DASH_PROG_FORMAT      "%d of %d required credits  (%d%%)"
#define STR_DASH_HONOR_FORMAT     "Projected honor: %s  (CPA %.2f)"
#define STR_DASH_GRADE_DIST       "Grade Distribution"
#define STR_DASH_CPA_GAUGE        "CPA Gauge"
#define STR_DASH_CPA_FORMAT       "CPA  %.3f / 4.00"
#define STR_DASH_CREDITS_BY_TYPE  "Credits by Type"
#define STR_DASH_TYPE_BAR_FORMAT  "%d/%d cr"
#define STR_DASH_GRADED_SUBJECTS  "%d graded subjects"

/* Graduation Planner */
#define STR_PLAN_TITLE            "Graduation Planner"
#define STR_PLAN_SUBTITLE         "Where you're headed, and how to reach the honor you want"
#define STR_PLAN_PROJECTED_LABEL  "PROJECTED HONOR  (if you keep your current average)"
#define STR_PLAN_STATS_FORMAT     "Pass-only CPA %.2f   ·   %d of %d required credits   ·   %d remaining"
#define STR_PLAN_REACHABLE_FORMAT "Still reachable: %s  to  %s"
#define STR_PLAN_FINAL_STANDING   "All required credits earned - this is your final standing."
#define STR_PLAN_GRADUATE_AS      "GRADUATE AS"
#define STR_PLAN_CPA_MIN_FORMAT   "CPA %.1f+"
#define STR_PLAN_OUT_OF_REACH     "out of reach"
#define STR_PLAN_FLEX_LABEL       "HOW FAR INTO %s"
#define STR_PLAN_FLEX_FORMAT      "%s  (CPA %.2f)"
#define STR_PLAN_VERDICT_NONE     "Pick a target above to see the average and the per-subject grades you need."
#define STR_PLAN_VERDICT_SECURE_R "%s is secured - just pass your remaining %d credits."
#define STR_PLAN_VERDICT_SECURE_L "%s is already locked in."
#define STR_PLAN_VERDICT_REACH    "To graduate %s: average at least %.2f (about %c%s) across your %d remaining credits."
#define STR_PLAN_VERDICT_IMPOSS   "%s is out of reach now - the highest you can still earn is %s."
#define STR_PLAN_LIST_LABEL       "WHAT TO LEARN NEXT  ·  %d subjects left  (failed retakes first, then by term)"
#define STR_PLAN_LIST_EMPTY       "Nothing left - every subject is passed. Congratulations!"
#define STR_PLAN_ROW_TERM_FORMAT  "T%u"
#define STR_PLAN_ROW_CR_FORMAT    "%ucr"
#define STR_PLAN_HIGH_IMPACT      "high impact"
#define STR_PLAN_OPTIONAL         "optional"
#define STR_PLAN_TAG_RETAKE       "Retake"
#define STR_PLAN_TAG_NOT_STARTED  "Not started"
#define STR_PLAN_TAG_PASS         "Pass"

/* Edit Popup */
#define STR_EDIT_ERR_BOTH_SCORES  "Enter both midterm and final."
#define STR_EDIT_ERR_RANGE        "Scores must be between 0 and 10."
#define STR_EDIT_ERR_NOT_FOUND    "Subject %s not found."
#define STR_EDIT_SUCCESS_FORMAT   "Saved %s: mid=%.2f  final=%.2f  ratio=%s"
#define STR_EDIT_CODE_FORMAT      "Code: %s"
#define STR_EDIT_LABEL_AVERAGE    "AVERAGE"
#define STR_EDIT_STATUS_PASS      "PASS"
#define STR_EDIT_STATUS_FAIL      "FAIL"
#define STR_EDIT_STATUS_ENTER     "enter scores"
#define STR_EDIT_LABEL_MIDTERM    "Midterm  (0 - 10)"
#define STR_EDIT_LABEL_FINAL      "Final  (0 - 10)"
#define STR_EDIT_INPUT_FORMAT     "%s "
#define STR_EDIT_INPUT_CURS_FORMAT "%s|"
#define STR_EDIT_HINT_KEYS        "Tab  switch field   ·   Enter  save"
#define STR_EDIT_LABEL_RATIO      "Ratio:"
#define STR_EDIT_PLAN_TITLE       "WHAT YOU NEED ON THE FINAL"
#define STR_EDIT_PLAN_PROMPT      "Enter your midterm score to see what you need."
#define STR_EDIT_PLAN_AUTO_F      "Midterm %.1f is below %.0f - automatic F."
#define STR_EDIT_PLAN_UNPASSABLE  "No final score can pass this subject."
#define STR_EDIT_PLAN_NEED_FORMAT "need  %.1f  on the final"
#define STR_EDIT_PLAN_IMPOSSIBLE  "Even a perfect final can't pass this subject."
#define STR_EDIT_SAVE_BTN         "Save"
#define STR_EDIT_RESET_BTN        "Reset"
#define STR_EDIT_RESET_SUCCESS    "Reset score for %s"
#define STR_EDIT_CANCEL_BTN       "Cancel"

/* Name Input Screen */
#define STR_NAME_INPUT_TITLE      "Transcript Viewer"
#define STR_NAME_INPUT_SUBTITLE   "Enter your username to continue"
#define STR_NAME_INPUT_LABEL      "Username  (max 25 chars)"
#define STR_NAME_INPUT_DB_FOUND   "db_%s.db  found | load existing data"
#define STR_NAME_INPUT_DB_NEW     "db_%s.db  will be created"
#define STR_NAME_INPUT_PROMPT     "Type your username and press Enter"
#define STR_NAME_INPUT_ENTER_CFM  "Enter  Confirm"
#define STR_NAME_INPUT_ESC_QUIT   "ESC  Quit"

/* Settings Page */
#define STR_SETTINGS_TITLE        "Settings"
#define STR_SETTINGS_SUBTITLE     "Adjust display options - changes apply instantly"

/* Stepper control glyphs */
#define STR_STEP_DEC              "-"
#define STR_STEP_INC              "+"

/* ui.cfg card — interactive display settings */
#define STR_CFG_UI_TITLE          "Display  (ui.cfg)"
#define STR_CFG_UI_PATH           "Saved to  assets/ui.cfg"
#define STR_CFG_UI_FONT_LABEL     "Text size"
#define STR_CFG_UI_FONT_VAL       "%.1fx"
#define STR_CFG_UI_FONT_D         "font_scale - 1.0 = base, 1.4 = default, 2.5 = very large."
#define STR_CFG_UI_FPS_LABEL      "Frame rate"
#define STR_CFG_UI_FPS_VAL        "%d FPS"
#define STR_CFG_UI_FPS_D          "target_fps - 60 to 240 frames per second."
#define STR_CFG_UI_SAVED          "Saved to ui.cfg - applied instantly"

/* grad_config.cfg card — graduation rules, editable per subject type */
#define STR_CFG_GRAD_TITLE        "Graduation rules  (grad_config.cfg)"
#define STR_CFG_GRAD_PATH         "Saved to  assets/grad_config.cfg"
#define STR_CFG_GRAD_BODY         "Set how each subject type counts toward graduation. Changes are saved and applied right away."
#define STR_CFG_GRAD_ID_FMT       "id %d"
#define STR_CFG_GRAD_MODE_CAP     "Rule"
#define STR_CFG_GRAD_LIMIT_CAP    "Limit"
#define STR_CFG_GRAD_GROUP_CAP    "Group"
#define STR_GRAD_MODE_ALL         "All credits"
#define STR_GRAD_MODE_FIXED       "Fixed credits"
#define STR_GRAD_MODE_COUNT       "Subject count"
#define STR_CFG_GRAD_SAVED        "Saved to grad_config.cfg - applied"
#define STR_CFG_GRAD_RELOAD_BTN   "Reload from file"

/* Comment-preservation warning (when editing the files by hand) */
#define STR_SETTINGS_WARN_TITLE   "EDITING A .cfg FILE BY HAND?"
#define STR_SETTINGS_WARN_BODY    "Change only the values and keep every comment line (the lines starting with #) - they document what each option means. Do not delete them."

/* Command Palette */
#define STR_CMD_TITLE             "Command Palette"
#define STR_CMD_PROMPT            "please type \"help\" to see all command"
#define STR_CMD_HINT_ENTER        "Enter for Execute"
#define STR_CMD_HINT_BACKSPACE    "Backspace for Delete"
#define STR_CMD_HINT_ESC          "ESC for Stop Program"
