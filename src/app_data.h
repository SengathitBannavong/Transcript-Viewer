#pragma once

/*
 * app_data.h — application data model
 *
 * The actual data lives in the SQLite in-memory database (db.h).
 * gPlayer is the in-RAM mirror populated by DB_Query().
 */

#include "struct_table.h"

/* Single global player record — filled by DB_Query() */
static Player gPlayer;

/* Human-readable names — populated at runtime from DB (subject_types table)   */
/* which itself was seeded from the [N] Name headers in assets/subjects.dat     */
static char gTypeName[sizeSubjectType][64];

/* Graduation rules per type — loaded from DB by DB_LoadGradRules()             */
static GradRule gGradRules[sizeSubjectType];

/* Data validation warnings — filled by DB_ValidateData() on every DB open      */
/* Each warning is a NUL-terminated string packed end-to-end; gDataWarnCount     */
/* holds the number of warnings.  gDataWarningsBuf is the raw storage.           */
#define DATA_WARN_BUF 2048
static char gDataWarningsBuf[DATA_WARN_BUF];
static int  gDataWarnCount;   /* number of \0-terminated strings in buf          */
