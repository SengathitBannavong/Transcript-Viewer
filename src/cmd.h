#pragma once

void ExecuteCommand(const char *input,
                    int        *out_nav,
                    char       *out_filter,   /* kept for compat */
                    char       *out_msg,
                    int         msg_size);
