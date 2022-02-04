//
// Created by ZyWang on 2021/10/7.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "process.h"



void proc_insert_tail(struct Process *proc){
    if (proc_search_pgid(proc->pgid) == NULL){
        struct Process *ptr = (struct Process *)malloc(sizeof(struct Process));
        if(ptr)  memset(ptr, '\0', sizeof(struct Process));
        // pid, ppid, pgid
        ptr->pid = proc->pid; ptr->ppid = proc->ppid; ptr->pgid = proc->pgid;
        // sh_id, status
        ptr->sh_pid = proc->sh_pid; ptr->status = proc->status;
        // cmd
        ptr->cmd = proc->cmd;

        // insert.
        ptr->pre = tail;
        ptr->next = NULL;
        tail->next = ptr;
        tail = tail->next;

        proc_count += 1;
    }
}


void proc_delete_pid(int sh_pid){
    struct Process *p1 = head;
    while(p1 != NULL){
        if(p1->sh_pid == sh_pid){
            p1->pre->next = p1->next;
            if(p1->next != NULL) {
                p1->next->pre = p1->pre;
            }else {
                tail = p1->pre;
                tail->next = NULL;
            }
            free(p1);
            break;
        }
        p1 = p1->next;
    }
}


void proc_printinfo(int sh_pid){
    struct Process *proc = proc_search_sh_pid(sh_pid);
    if (proc != NULL)
        printf("-----------\n real pid: %d,\n ppid: %d,\n pgid: %d,\n nyush_pid: %d,\n status: %d,\n cmd: %s\n-----------\n",
               proc->pid, proc->ppid, proc->pgid, proc->sh_pid, proc->status, proc->cmd);
    else
        fprintf(stderr, "Error: no such process\n");
}


struct Process *proc_search_sh_pid(int sh_pid){
    struct Process *p1 = head;
    while(p1 != NULL){
        if(p1->sh_pid == sh_pid){
            return p1;
        }
        p1 = p1->next;
    }

    return NULL;
}


struct Process *proc_search_pgid(int pgid){
    struct Process *p1 = head;
    while(p1 != NULL){
        if(p1->pgid == pgid){
            return p1;
        }
        p1 = p1->next;
    }

    return NULL;
}


int proc_no_zombie(){
    struct Process *ptr = head->next;
    int cnt = 0;
    while(ptr != NULL){
        if(ptr->status == SUSPEND || ptr->status == RUNNING)
            return -1;
        ptr = ptr->next;
        cnt += 1;
    }
    return cnt;
}
