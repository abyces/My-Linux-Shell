//
// Created by ZyWang on 2021/10/7.
//

#ifndef NYUSH_PARSER_H
#define NYUSH_PARSER_H

#endif //NYUSH_PARSER_H
char* ipt_redir_file;
char* opt_redir_file;
int ipt_redir_count;
int opt_redir_count;
int append;
char* cmd_whole;
int cmd_count;
int cmdlen;

struct Command {
    int cid;
    char *cmd;
    char **argv;
};

char ** split(char* cmd_raw);
int parser(struct Command **cmds, char** cmd_split, int len);
int cmd_test(char* cmd, char** cmd_next, int len);
int args_test(char* arg);
int digit_test(char* arg);
