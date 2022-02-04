//
// Created by ZyWang on 2021/10/7.
//
#include <string.h>
#include <stdlib.h>

#include "parser.h"


/*
 * Split commands.
 *      Split cmds using " " and pass char** to next step.
 */
char ** split(char* cmd_raw){
    if(cmd_raw == NULL || strcmp(cmd_raw, "") == 0)
        return 0;

    char *saveptr1;
    char **argv = (char**)malloc(1000 * sizeof (char*));
    char *temp;
    int i;

    // make back-up of the whole commands
    cmd_whole = malloc(sizeof (char) * (strlen(cmd_raw) + 1));
    strcpy(cmd_whole, cmd_raw);
    cmd_whole[strlen(cmd_raw)] = '\0';

    if(argv)   memset(argv, '\0', 1000 * sizeof(char*));

    for(i = 0, temp = cmd_raw; ; temp = NULL, i++){
        char *token = strtok_r(temp, " ", &saveptr1);
        if (token == NULL)
            break;

        if(strcmp(token, "|") == 0)
            cmd_count += 1;

        argv[i] = malloc((strlen(token) + 1) * sizeof(char));
        strcpy(argv[i], token);
        argv[i][strlen(token)] = '\0';
        cmdlen++;
    }
    argv[cmdlen] = NULL;
    cmd_count += 1;

    return argv;
}


/*
 * Parsing the commands
 *      Turn commands into structures and test its validation.
 *      Return 1 if valid or 0 if invalid.
 */
int parser(struct Command **cmds, char** cmd_split, int len){
    if(cmd_split == NULL || *cmd_split == NULL || strlen(*cmd_split) == 0)
        return 0;

    // initialize global vars
    append = 0;
    int count = 0;

    int new_round = 1;   // mark start of each command "|"
    int cmd_valid = 1;  // return value of function cmd_test()
    int arg_valid = 1;  // return value of function arg_test()

    char** args;    // args [0] is cmd, [1, ...] are args
    int arg_count;

    while(*cmd_split){
        // a new command(just one, or splitted by "|")
        if(new_round == 1){
            if((cmd_valid && arg_valid) == 0 )
                return 0;

            arg_valid = 1;

            // get cmd
            args = (char**)malloc(sizeof (char *) * 50);
            args[0] = malloc(sizeof (char) * (strlen(*cmd_split) + 1));
            strcpy(args[0], *cmd_split);
            args[0][strlen(*cmd_split)] = '\0';

            // test cmd's validation
            // len mainly used for build-in commands to limit its length.
            cmd_split++;
            char** next = cmd_split;
            cmd_valid = cmd_test(args[0], next, len);
            arg_count = 1;
            count += 1;

            new_round = 0;
            continue;
        }


        if(strcmp(*cmd_split, "|") == 0){
            if(opt_redir_count != 0)    // in case we have output redirection before pipes end
                return 0;
            else{
                cmd_split++;
                if(*cmd_split == NULL || strlen(*cmd_split) == 0)      // in case commands end with "|"
                    return 0;

                // every time we encounter "|", we start a new command
                // so we store the current command into struct.
                new_round = 1;

                cmds[count-1] = malloc(sizeof (struct Command));
                cmds[count-1]->cid = count - 1;
                cmds[count-1]->cmd = cmd_whole;
                cmds[count-1]->argv = args;

                continue;
            }
        } else if (strcmp(*cmd_split, "<") == 0){
            if(ipt_redir_file == NULL && count == 1){   // make sure we only have input redirection in the first command.
                cmd_split++;
                if(*cmd_split != NULL && strlen(*cmd_split) != 0){  // "<" must followed by filename.
                    ipt_redir_file = malloc(sizeof (char) * (strlen(*cmd_split) + 1));
                    strcpy(ipt_redir_file, *cmd_split);
                    ipt_redir_file[strlen(*cmd_split)] = '\0';

                    ipt_redir_count += 1;
                    cmd_split++;

                    if (args_test(ipt_redir_file) == 0)  // invalid filename.
                        return 0;
                    if ( *cmd_split != NULL && args_test(*cmd_split) == 1)  // make sure "<" only followed by one filename or arg..
                        return 0;
                }
                else { return 0;}
            } else { return 0;}
        } else if (strcmp(*cmd_split, ">") == 0 || strcmp(*cmd_split, ">>") == 0){
            if(opt_redir_file == NULL){     // make sure we do not have output redirection before.
                if(strcmp(*cmd_split, ">>") == 0)
                    append = 1;
                cmd_split++;
                if(*cmd_split != NULL && strlen(*cmd_split) != 0){
                    opt_redir_file = malloc(sizeof (char) * (strlen(*cmd_split) + 1));
                    strcpy(opt_redir_file, *cmd_split);
                    opt_redir_file[strlen(*cmd_split)] = '\0';

                    opt_redir_count += 1;
                    cmd_split++;

                    if (args_test(opt_redir_file) == 0)
                        return 0;
                    if (*cmd_split != NULL && args_test(*cmd_split) == 1)
                        return 0;
                }
                else { return 0;}
            } else { return 0;}
        } else {  // token not belonging to any of the above situation, then it is one arg..
            args[arg_count] = malloc(sizeof (char) * (strlen(*cmd_split) + 1));
            strcpy(args[arg_count], *cmd_split);
            args[arg_count][strlen(*cmd_split)] = '\0';
            arg_valid = arg_valid && args_test(args[arg_count]);

            arg_count++;
            cmd_split++;
        }
    }

    // handle the first command(no pipe), or the last command(with pipe)
    cmds[count-1] = malloc(sizeof (struct Command));
    cmds[count-1]->cid = count - 1;
    cmds[count-1]->cmd = cmd_whole;
    cmds[count-1]->argv = args;

    cmds[count] = NULL;
    return cmd_valid && arg_valid;
}


/*
 * Test cmds' validation,
 *      Especially check for build-in commands
 *      Then check its name.
 */
int cmd_test(char* cmd, char** cmd_next, int len){
    if(strcmp(cmd, "cd") == 0 || strcmp(cmd, "fg") == 0){
        if(len != 2)
            return 0;

        char* next = malloc(sizeof (char) * (strlen(*cmd_next) + 1));
        strcpy(next, *cmd_next);
        next[strlen(*cmd_next)] = '\0';

        int ret;

        if(strcmp(cmd, "cd") == 0) ret = args_test(next);
        else ret = digit_test(next);

        free(next);
        return ret;
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "jobs") == 0){
        if(len == 1)    return 1;
        else    return 0;
    } else {
        return args_test(cmd);
    }
}


/*
 * Args validation test
 *      Check for invalid characters.
 */
int args_test(char* arg){
    if(arg == NULL || strlen(arg) == 0)
        return 1;

    for(long unsigned int i = 0; i<strlen(arg); i++){
        if(arg[i] != ' ' && arg[i] != '\t' && arg[i] != 62 && arg[i] != 60 && arg[i] != 124
           && arg[i] != 42 && arg[i] != 33 && arg[i] != 96 && arg[i] != 39 && arg[i] != 34)
            continue;
        else
            return 0;
    }
    return 1;
}



/*
 * digit test
 *      return true if and only if args are all digits.
 */
int digit_test(char* arg){
    if(arg == NULL || strlen(arg) == 0)
        return 0;

    for(long unsigned int i = 0; i<strlen(arg); i++){
        if(arg[i] >= 48 && arg[i] <= 57)
            continue;
        else
            return 0;
    }

    return 1;
}