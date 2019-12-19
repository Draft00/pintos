#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

#define CLOSE_ALL -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL -1 

struct child_process {
	int pid;
	int load;
	bool wait;
	bool exit;
	int status;
	struct list_elem elem;
};


void remove_child_processes (void);
struct child_process* get_child_process (int pid);
void remove_child_process (struct child_process *cp);

void process_close_file (int fd);

void syscall_init (void);

#endif /* userprog/syscall.h */
