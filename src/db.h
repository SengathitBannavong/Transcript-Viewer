#pragma once

#include <sqlite3.h>
#include "struct_table.h"
#include <stddef.h>

#define MIN_PASS_SCORE 3.0f

const char *db_compute_letter_r(float mid, float final_, float rm, float rf);


int DB_Exists(const char *username);
int DB_Open(const char *username);
void db_exec(const char *sql);
void DB_CreateSchema(void);
void DB_SeedFromDat(void);
void DB_LoadScores_Test(void);
void DB_SeedGradRules(void);
void DB_LoadGradRules(void);
void DB_ValidateData(void);
void DB_ApplyMinPassRule(void);
void DB_ReloadData(void);
void DB_Query(Player *player);
int DB_UpdateScore(const char *code, float mid, float final_);
int DB_UpdateScoreRatio(const char *code, float mid, float final_, int ratio_sel);
int DB_ClearScore(const char *code);
int DB_SubjectExists(const char *code);
int DB_GetSubjectCredits(const char *code);
int DB_UpdateGradRule(int type_id, int mode, int limit_val, int group_id);
int DB_SaveGradConfig(const char *path);
void DB_Close(void);

void DB_PersistInit(void);
void DB_Persist(void);

#if defined(PLATFORM_WEB)
void DB_ExportDownload(const char *username);
void DB_ImportPick(const char *username);
int DB_ImportPoll(const char *username);
#else
int DB_PickSavePath(const char *suggest, char *outpath, size_t n);
int DB_PickOpenPath(char *outpath, size_t n);
int DB_ExportFile(const char *username, char *outpath, size_t n);
int DB_ImportFile(const char *username, const char *src);
int tv_have(const char *prog);
#endif
