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
