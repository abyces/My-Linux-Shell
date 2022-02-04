//
// Created by ZyWang on 2021/10/7.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include "buildin.h"
#include "process.h"
#include "parser.h"

/*
 * func cd
 */
int func_cd(char** argv){
    if( argv[1][0] == '/'){
        if(chdir(argv[1]) == -1){
            fprintf(stderr, "Error: invalid directory\n");
            return -1;
        }
    } else {
        char *dir = (char*)malloc(sizeof (char) * 300);
        if(dir) memset(dir, '\0', sizeof (char) * 300);
        strcpy(dir, getcwd(NULL, 0));
        strcat(dir, "/");
        strcat(dir, argv[1]);
        strcat(dir, "\0");
        if( chdir(dir) == -1){
            fprintf(stderr, "Error: invalid directory\n");
            return -1;
        }
        free(dir);
    }
    return 0;
}


/*
 * func jobs
 *      List all jobs created by this shell with its shell pid (not real pid in sys)
 */
int func_jobs(){
    struct Process *ptr = head->next;
    while(ptr != NULL){
        char* outstr = (char*)malloc(1000 * sizeof (char));
        if(outstr)  memset(outstr, '\0', 1000 * sizeof(char));
        char sh_pid_buffer[20];

        strcat(outstr, "[");
        sprintf(sh_pid_buffer, "%d", ptr->sh_pid);
        strcat(outstr, sh_pid_buffer);
        strcat(outstr, "] ");
        strcat(outstr, ptr->cmd);
        strcat(outstr, "\n");
        strcat(outstr, "\0");

        printf("%s", outstr);

        fflush(stdout);
        free(outstr);
        ptr = ptr->next;
    }
    return 0;
}


/*
 * func fg
 *      Given shell pid, wake the job up to foreground using SIGCONT.
 *      Handle signals like Ctrl-Z or Ctrl-C and update list.
 */
int func_fg(char** argv){
    int sh_pid = atoi(argv[1]);
    struct Process *proc = proc_search_sh_pid(sh_pid);
    if(proc) {
        tcsetpgrp(STDIN_FILENO, proc->pgid);
        tcsetpgrp(STDOUT_FILENO, proc->pgid);

        kill(-proc->pgid, SIGCONT);

        siginfo_t siginfo;
        while(waitid(P_PGID, proc->pgid, &siginfo, WEXITED | WSTOPPED) != -1){
            if(siginfo.si_code == CLD_STOPPED){
                kill(-proc->pgid, SIGSTOP);
                break;
            } else if (siginfo.si_code == CLD_EXITED ) {
                if (proc_search_sh_pid(sh_pid) != NULL) proc_delete_pid(sh_pid);
                if (siginfo.si_status != 0){
                    kill(-proc->pgid, SIGKILL);
                    break;
                }
            } else if (siginfo.si_code == CLD_KILLED || siginfo.si_code == CLD_DUMPED){
                if (proc_search_sh_pid(sh_pid) != NULL) proc_delete_pid(sh_pid);
                kill(-proc->pgid, SIGKILL);
                break;
            }
        }

        tcsetpgrp(STDIN_FILENO, getpgid(init_pid));
        tcsetpgrp(STDOUT_FILENO, getpgid(init_pid));

        return 1;
    } else {
        fprintf(stderr, "Error: invalid job\n");
        return -1;
    }
}


/*
 * func exit
 *      Search process list to see if the shell can exit or not
 */
int func_exit(){
    int ret = proc_no_zombie();
    if(ret == 0) {
        return -2;
    } else {
        fprintf(stderr, "Error: there are suspended jobs\n");
        return -1;
    }
}
