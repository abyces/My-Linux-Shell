//
// Created by ZyWang on 2021/10/7.
//
#ifndef NYUSH_PROCESS_H
#define NYUSH_PROCESS_H

#endif //NYUSH_PROCESS_H

// define
#define KILLED 0
#define RUNNING 1
#define SUSPEND 2

pid_t init_pid;  // shell pid

struct Process *head;
struct Process *tail;

int proc_count;

struct Process {
    pid_t pid;      //real pid in sys
    pid_t ppid;     //parent pid
    pid_t pgid;     //gourp id

    int sh_pid;     //pid in this shell
    int status;

    char *cmd;      //whole cmd

    struct Process *pre;
    struct Process *next;
};

// process
void proc_insert_tail(struct Process *proc);
void proc_delete_pid(int sh_pid);
void proc_printinfo(int sh_pid);
int proc_no_zombie();
struct Process *proc_search_sh_pid(int sh_pid);
struct Process *proc_search_pgid(int pgid);
