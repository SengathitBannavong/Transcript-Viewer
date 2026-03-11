#pragma once

/* ─── Data model ─────────────────────────────────────────────────────── */
typedef struct {
    int         id;
    const char *name;
    const char *department;
    const char *role;
    int         salary;
    int         status;   /* 0=Active  1=Inactive  2=On Leave */
    const char *hireDate;
    const char *initials;
} Employee;

static Employee gEmployees[] = {
    { 1, "Alice Johnson",  "Engineering",  "Senior Developer",   132000, 0, "2019-03-15", "AJ"},
    { 2, "Bob Martinez",   "Marketing",    "Brand Manager",       94000, 0, "2020-07-22", "BM"},
    { 3, "Carol White",    "HR",           "HR Specialist",       72000, 0, "2021-01-08", "CW"},
    { 4, "David Chen",     "Engineering",  "DevOps Engineer",    118000, 0, "2018-11-03", "DC"},
    { 5, "Elsa Nguyen",    "Design",       "UX Designer",         88000, 2, "2020-04-17", "EN"},
    { 6, "Frank Lee",      "Finance",      "Financial Analyst",   95000, 0, "2017-06-30", "FL"},
    { 7, "Grace Kim",      "Engineering",  "Frontend Developer", 105000, 0, "2021-09-12", "GK"},
    { 8, "Henry Brown",    "Sales",        "Account Executive",   82000, 1, "2019-05-25", "HB"},
    { 9, "Iris Patel",     "Design",       "Product Designer",    91000, 0, "2022-02-14", "IP"},
    {10, "James Wilson",   "Engineering",  "Backend Developer",  115000, 0, "2020-08-19", "JW"},
    {11, "Karen Davis",    "Marketing",    "Content Strategist",  78000, 0, "2021-11-03", "KD"},
    {12, "Liam Thompson",  "Finance",      "CFO",                195000, 0, "2015-01-20", "LT"},
    {13, "Maya Rodriguez", "HR",           "HR Director",        135000, 0, "2016-04-07", "MR"},
    {14, "Nathan Clark",   "Sales",        "Sales Director",     158000, 0, "2017-09-14", "NC"},
    {15, "Olivia Scott",   "Engineering",  "QA Engineer",         85000, 2, "2022-06-01", "OS"},
    {16, "Paul Harris",    "Engineering",  "Tech Lead",          145000, 0, "2018-03-22", "PH"},
    {17, "Quinn Adams",    "Marketing",    "Social Media Mgr",    69000, 1, "2023-01-15", "QA"},
    {18, "Rachel Turner",  "Design",       "Creative Director",  128000, 0, "2019-07-08", "RT"},
    {19, "Sam Walker",     "Finance",      "Accountant",          75000, 0, "2020-10-29", "SW"},
    {20, "Tina Morgan",    "Engineering",  "Mobile Developer",   110000, 0, "2021-04-18", "TM"},
};

#define EMP_COUNT ((int)(sizeof(gEmployees) / sizeof(gEmployees[0])))
