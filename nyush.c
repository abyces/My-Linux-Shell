#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include "parser.h"
#include "process.h"
#include "buildin.h"

// global vars
int **pipes;
int cur_group_lead;     // leader process id of cur_process_group
int cur_group_id;       // group id of cur_process_group
int proc_count = 0;     // nums of process created

// function declarations
int exec_pipe(struct Command**, int num_cmds);
int exec_controller(struct Command *cmd, int num_cmds);
int forks(char *func_dir, struct Command*, int num_cmds);
void waits(char* cmdline);




int main() {
    init_pid = getpid();
    setpgrp();
    tcsetpgrp(STDIN_FILENO, getpgid(init_pid));
    tcsetpgrp(STDOUT_FILENO, getpgid(init_pid));

    struct Process sh = {init_pid, getppid(), getpgid(init_pid), 0, RUNNING,  "shell", NULL, NULL};
    head = &sh;
    head->next = NULL;
    tail = head;
    proc_count += 1;

    while(1){
        int ret = 0;

        cmdlen = 0;
        cmd_count = 0;

        ipt_redir_file = NULL;
        opt_redir_file = NULL;
        ipt_redir_count = 0;
        opt_redir_count = 0;

        size_t len = 1000;
        char *str_input = (char *)malloc(len * sizeof(char));

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        signal(SIGSTOP, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        printf("[nyush %s]$ ", basename(getcwd(NULL, 0)));
        fflush(stdout);
        if (getline(&str_input, &len, stdin) == EOF) {
            printf("\n");
            exit(-1);
        }

        if(str_input == NULL || strlen(str_input) == 0 || strcmp(str_input, "\n") == 0) {
            free(str_input);
            continue;
        } else {
            str_input[strlen(str_input) - 1] = '\0';
            char** cmd_split = split(str_input);

            struct Command** commands = (struct Command**)malloc(sizeof (struct Command*) * (cmd_count + 1));
            if(commands) memset(commands, 0, sizeof (struct Command*) * (cmd_count + 1));

            if(parser(commands, cmd_split, cmdlen) == 0)
                fprintf(stderr, "Error: invalid command\n");
            else{
                struct Command** cmds = (struct Command**)malloc(sizeof (struct Command*) * (cmd_count + 1) );
                int i;
                for(i = 0; i<cmd_count; i++) {
                    struct Command cmd = {commands[i]->cid, commands[i]->cmd, commands[i]->argv};
                    cmds[i] = malloc(sizeof (struct Command));
                    cmds[i]->cid = cmd.cid; cmds[i]->cmd = cmd.cmd; cmds[i]->argv = cmd.argv;
                }
                cmds[i] = NULL;
                ret = exec_pipe(cmds, cmd_count);
                for(int j = 0; j<cmd_count + 1; j++)
                    free(cmds[j]);
                free(cmds);
            }
            for(int i = 0; i<(cmd_count + 1); i++)
                free(commands[i]);
            free(commands);
        }
        if(ret == -2)
            break;

        free(str_input);
    }

}


/*
 * If necessary, initialize and close pipes.
 */
int exec_pipe(struct Command** cmds, int num_cmds){
    int ret = 0;

    // initialize pipe
    pipes = (int **)malloc(sizeof(int *) * num_cmds);
    for(int i = 0; i<num_cmds - 1; i++){
        pipes[i] = (int*) malloc(sizeof (int) * 2);
        if(pipe(pipes[i]) < 0)
            return -1;
    }

    for(int i = 0; i<num_cmds; i++){
        ret = exec_controller(cmds[i], num_cmds);
        if (ret == -1) {    // exe_c returns -1 when failed to fork, or invalid program
            if(i != 0) { kill(-cur_group_id, SIGKILL);}    // we just kill other process in pipe
            break;
        }
    }

    // close pipe
    for(int i = 0; i<num_cmds - 1; i++){
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if(num_cmds > 1 || (num_cmds == 1 && strcmp(cmds[0]->argv[0], "cd") != 0 && strcmp(cmds[0]->argv[0], "fg") != 0 && strcmp(cmds[0]->argv[0], "jobs") != 0 && strcmp(cmds[0]->argv[0], "exit") != 0)){
        char* cmdline = malloc(sizeof (char) * (strlen(cmds[0]->cmd)) + 1);
        strcpy(cmdline, cmds[0]->cmd);
        cmdline[strlen(cmds[0]->cmd)] = '\0';
        waits(cmdline);
    }

    free(pipes);
    return ret;
}


/*
 *  Execute build-in functions based on cmd name.
 *  Run function with relative path ./
 *  Search for and run functions with absolute path in /bin/ or /usr/bin/
 */
int exec_controller(struct Command *cmd, int num_cmds){
    int ret;

    if (strcmp(cmd->argv[0], "cd") == 0){
        ret = func_cd(cmd->argv);
    } else if (strcmp(cmd->argv[0], "jobs") == 0){
        ret = func_jobs();
    } else if (strcmp(cmd->argv[0], "fg") == 0){
        ret = func_fg(cmd->argv);
    } else if (strcmp(cmd->argv[0], "exit") == 0){
        ret = func_exit();
    } else if (cmd->argv[0][0] != '\0' && cmd->argv[0][1] != '\0' && ((cmd->argv[0][0] == '.' && cmd->argv[0][1] == '/') || (cmd->argv[0][0] == '/'))){
        if(access(cmd->argv[0], X_OK) == -1){
            fprintf(stderr, "Error: invalid program\n");
            ret = -1;
        } else {
            ret = forks(cmd->argv[0], cmd, num_cmds);
        }
    }  else {
        char func_dir1[50] = "/bin/";
        char func_dir2[50] = "/usr/bin/";
        strcat(func_dir1, cmd->argv[0]);
        if(access(func_dir1, X_OK) == -1 ){
            strcat(func_dir2, cmd->argv[0]);
            if(access(func_dir2, X_OK) == -1){
                fprintf(stderr, "Error: invalid program\n");
                ret = -1;
            } else{
                ret = forks(func_dir2, cmd, num_cmds);
            }
        } else{
            ret = forks(func_dir1, cmd, num_cmds);
        }
    }
    return ret;
}


/*
 * Last step before exec*.
 *      Handle I/O redirection.
 *      Fork child process, and build pipes between processes.
 *      Control process groups, foreground/background.
 *      Handle Ctrl-Z / Ctrl-t for some child processes.
 *      Add suspend jobs to list.
 */
int forks(char *func_dir, struct Command* cmd, int num_cmds){
    int pid = fork();

    if (pid == 0){
        signal(SIGTSTP, SIG_DFL);   // enable signals
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGSTOP, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);

        int mypid = getpid();
        if (cmd->cid == 0){
            setpgid(mypid, mypid);
            cur_group_lead = mypid;
            cur_group_id = getpgid(mypid);
        } else { setpgid(mypid, cur_group_id); }

        // IO redirection
        if(cmd->cid == 0 && ipt_redir_file != NULL){
            int infile;
            if( (infile = open(ipt_redir_file, O_RDONLY)) < 0 ){
                fprintf(stderr, "Error: invalid file\n");
                exit(-1);
            } else {
                dup2(infile, STDIN_FILENO);
                close(infile);
            }
        }
        if (cmd->cid == cmd_count - 1 && opt_redir_file != NULL) {
            if(append == 0) {
                int outfile = open(opt_redir_file, O_CREAT | O_RDWR | O_TRUNC,  0644);
                dup2(outfile, STDOUT_FILENO);
                close(outfile);
            } else {
                int outfile = open(opt_redir_file, O_CREAT | O_RDWR | O_APPEND, 0644);
                dup2(outfile, STDOUT_FILENO);
                close(outfile);
            }
        }

        // Pipes
        if (num_cmds > 1) {
            if(cmd->cid == 0){
                dup2(pipes[0][1], STDOUT_FILENO);
            } else if (cmd->cid == num_cmds - 1){
                dup2(pipes[cmd->cid - 1][0], STDIN_FILENO);
            } else {
                dup2(pipes[cmd->cid - 1][0], STDIN_FILENO);
                dup2(pipes[cmd->cid][1], STDOUT_FILENO);
            }
            for(int i = 0; i<num_cmds - 1; i++){
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
        }   // No Pipes

        execv(func_dir, cmd->argv);
        exit(-1);
    } else {
        if (cmd->cid == 0){
            setpgid(pid, pid);
            cur_group_lead = pid;
            cur_group_id = getpgid(pid);

            tcsetpgrp(STDIN_FILENO, cur_group_id);
            tcsetpgrp(STDOUT_FILENO, cur_group_id);

        } else { setpgid(pid, cur_group_id); }
    }

    return pid;
}



void waits(char* cmdline){
    siginfo_t siginfo;
    while(waitid(P_PGID, cur_group_id, &siginfo, WEXITED | WSTOPPED) != -1){
        if(siginfo.si_code == CLD_STOPPED){
            struct Process ch = {cur_group_lead, getpid(), cur_group_id, proc_count, SUSPEND, cmdline, NULL, NULL};
            proc_insert_tail(&ch);
            kill(-cur_group_id, SIGSTOP);
            break;
        } else if (siginfo.si_code == CLD_EXITED){
            if( siginfo.si_status == 0) {
                continue;
            } else {
                if (proc_search_sh_pid(proc_count) != NULL) proc_delete_pid(proc_count);
                kill(-cur_group_id, SIGKILL);
                break;
            }
        } else if (siginfo.si_code == CLD_KILLED || siginfo.si_code == CLD_DUMPED){
            if (proc_search_sh_pid(proc_count) != NULL) proc_delete_pid(proc_count);
            kill(-cur_group_id, SIGKILL);
            break;
        }
    }
    tcsetpgrp(STDIN_FILENO, getpgid(init_pid));
    tcsetpgrp(STDOUT_FILENO, getpgid(init_pid));
}
