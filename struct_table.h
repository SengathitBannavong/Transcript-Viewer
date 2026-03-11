#ifndef STRUCT_TABLE_H
#define STRUCT_TABLE_H

#define MAXSIZENAME 256
#define MAXSIZEID 26
#define MAX_COLUMNS 10
#define MAX_COLUMN_WIDTH 56

typedef struct Subject_Node {
    char name[MAXSIZENAME];
    char ID[MAXSIZEID];
    char score_letter;
    float score_number_mid;
    float  score_number_final;
    unsigned int status_pass : 1;
    unsigned int status_ever_been_study: 2;
    unsigned int credit : 4;
    unsigned int term_recomment_to_studie: 4;
    struct Subject_Node *next;
} Subject_Node;

typedef struct _Subject_Type {
    int Total_Subject;
    unsigned int count_passSubject;
    unsigned int count_passCredit; // ADD NEW
    int Total_Credit;
    char nameoftype[MAXSIZENAME];
    Subject_Node* head;
    Subject_Node* tail;
} Subject_Type;

typedef enum {
    co_so_nganh,
    dai_cuong,
    the_thao,
    ly_luat_chinh_tri,
    tu_chon,
    thuc_tap,
    modunI,
    modunII,
    modunIII,
    modunIV,
    modunV,
    do_an_tot_nghiep,
    sizeSubjectType
} index_subject_type;

typedef struct Player {
    char name_player[MAXSIZENAME];
    int ToTal_credit_pass; // ADD NEW
    int ToTal_credit_npass;
    Subject_Type numofSubjectType[sizeSubjectType];
    unsigned int status_can_grauate : 1;
    unsigned int status_alert : 2;
} Player;

#endif