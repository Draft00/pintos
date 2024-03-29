#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <string.h>

#define USER_VADDR_BOTTOM ((void *) 0x08048000) //все что выше этого физический адресс

struct lock filesys_lock;

struct process_file {
	struct file *file;
	int fd;
	struct list_elem elem;
};

int process_add_file (struct file *f);
struct file* process_get_file (int fd);

static void syscall_handler (struct intr_frame *);
void get_arg (struct intr_frame *f, int *arg, int n);
void check_valid_ptr (const void *vaddr);
void check_valid_buffer (void* buffer, unsigned size);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
	lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
int arg[3];

check_valid_ptr((const void*) f->esp);
int sys_num = *(int*) f->esp;
//int* args = (int*) f->esp + 1;

switch (sys_num)
{
    case SYS_HALT:
	halt();
	break;
    case SYS_EXIT:
	//exit(args[0]);
	get_arg(f, &arg[0], 1);
	exit(arg[0]);
	break;
    case SYS_EXEC:
	get_arg(f, &arg[0], 1);
	arg[0] = pagedir_get_page(thread_current()->pagedir, (const void *) arg[0]);
	//check_valid_ptr((const void *)args[0]);
	//args[0] = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);

/*
pagedir 
  Looks up the physical address that corresponds to user virtual
    address UADDR in PD.  Returns the kernel virtual address
   corresponding to that physical address, or a null pointer if
   UADDR is unmapped. 
*/
	if (!arg[0]) exit (-1);
	f->eax = exec((const char *) arg[0]); 
	break;
    case SYS_CLOSE:
	get_arg(f, &arg[0], 1);
	close(arg[0]);
	break;
    case SYS_FILESIZE:
	get_arg(f, &arg[0], 1);
	f->eax = filesize(arg[0]);
	break;
    case SYS_WAIT:
	get_arg(f, &arg[0], 1);
	f->eax = wait(arg[0]);
	break;
    case SYS_CREATE:
	get_arg(f, &arg[0], 2);
	//check_valid_ptr((const void *)args[0]);
	arg[0] = pagedir_get_page(thread_current()->pagedir, (const void *) arg[0]);
	if (!arg[0]) exit (-1);
	f->eax = create((const char *)arg[0], (unsigned) arg[1]);
	break;
    case SYS_REMOVE:
	get_arg(f, &arg[0], 1);
	//check_valid_ptr((const void *)args[0]);
	arg[0] = pagedir_get_page(thread_current()->pagedir, (const void *) arg[0]);
	if (!arg[0]) exit (-1);
	f->eax = remove((const char *) arg[0]);
	break;
    case SYS_OPEN:
	get_arg(f, &arg[0], 1);
	//check_valid_ptr((const void *)args[0]);
	arg[0] = pagedir_get_page(thread_current()->pagedir, (const void *) arg[0]);
	if (!arg[0]) exit (-1);
	f->eax = open((const char *) arg[0]);
	break; 		
   
    case SYS_READ:
	get_arg(f, &arg[0], 3);
	check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
	arg[1] = pagedir_get_page(thread_current()->pagedir, (const void *) arg[1]);
	if (!arg[1]) exit (-1);
	f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
	break;
    case SYS_WRITE:
	get_arg(f, &arg[0], 3);
	check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
	arg[1] = pagedir_get_page(thread_current()->pagedir, (const void *) arg[1]);
	if (!arg[1]) exit (-1);
	f->eax = write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
	break;
}


}

void halt (void)
{
	shutdown_power_off();
}

void exit (int status)
{
	struct thread *cur = thread_current();
	if (thread_alive(cur->parent)) cur->cp->status = status;
	printf ("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

pid_t exec (const char *cmd_line)
{
	pid_t pid = process_execute(cmd_line);
	struct child_process* cp = get_child_process(pid);
	while (cp->load == NOT_LOADED) barrier();
	if (cp->load == LOAD_FAIL) return -1; //in start_process() process.c
	return pid;
}

int wait (pid_t pid)
{
	return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
	lock_acquire(&filesys_lock);
	bool result = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return result;
}

bool remove (const char *file)
{
	lock_acquire(&filesys_lock);
	bool result = filesys_remove(file);
	lock_release(&filesys_lock);
	return result;
}

int open (const char *file)
{
	lock_acquire(&filesys_lock);
	struct file *f = filesys_open(file);
	if (!f)
	{
		lock_release(&filesys_lock);
		return -1;
	}
	int result = process_add_file(f);
	lock_release(&filesys_lock);
	return result;
}

int filesize (int fd)
{
	lock_acquire(&filesys_lock);

	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return -1;
	}
	int size = file_length(f);
	lock_release(&filesys_lock);
	return size;
}

int read (int fd, void *buffer, unsigned size)
{
	if (fd == STDIN_FILENO)
	{
		unsigned i;
		uint8_t* local_buffer = (uint8_t *) buffer;
		for (i = 0; i < size; i++) local_buffer[i] = input_getc();
		return size;
	}

	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return -1;
	}
	int bytes = file_read(f, buffer, size);
	lock_release(&filesys_lock);
	return bytes;
}

int write (int fd, const void *buffer, unsigned size)
{
	if (fd == STDOUT_FILENO)
	{
		putbuf(buffer, size);
		return size;
	}
	lock_acquire(&filesys_lock);
	struct file *f = process_get_file(fd);
	if (!f)
	{
		lock_release(&filesys_lock);
		return -1;
	}
	int bytes = file_write(f, buffer, size);
	lock_release(&filesys_lock);
	return bytes;
}

void close (int fd)
{
	lock_acquire(&filesys_lock);
	process_close_file(fd);
	lock_release(&filesys_lock);
}

void check_valid_ptr (const void *vaddr)
{
	if (!is_user_vaddr(vaddr) || vaddr < USER_VADDR_BOTTOM || !pagedir_get_page(thread_current()->pagedir, vaddr))
		exit(-1);
}

/*
is_user_vaddr проверяет принадлежит ли указанный адрес адресному пространству пользовательских 
программ, pagedir_get_page — проверяет выделена ли страница под указанный виртуальный адрес пользовательской 
программы и возвращает ее
*/

int process_add_file (struct file *f)
{
	struct process_file *pf = malloc(sizeof(struct process_file));
	pf->file = f;
	pf->fd = thread_current()->fd;
	thread_current()->fd++;
	list_push_back(&thread_current()->file_list, &pf->elem);
	return pf->fd;
}

struct file* process_get_file (int fd)
{
	struct thread *t = thread_current();
	struct list_elem *e;

	for (e = list_begin (&t->file_list); e != list_end (&t->file_list); e = list_next (e))
	{
		struct process_file *pf = list_entry (e, struct process_file, elem);
		if (fd == pf->fd)
			return pf->file;
	}
	return NULL;
}

void process_close_file (int fd)
{
struct thread *t = thread_current();
struct list_elem *next, *e = list_begin(&t->file_list);

while (e != list_end (&t->file_list))
{
	next = list_next(e);
	struct process_file *pf = list_entry (e, struct process_file, elem);
	if (fd == pf->fd || fd == CLOSE_ALL)
	{
		file_close(pf->file);
		list_remove(&pf->elem);
		free(pf);
		if (fd != CLOSE_ALL)
		      return;
	}
	e = next;
}
}

struct child_process* get_child_process (int pid)
{
	struct thread *t = thread_current();
	struct list_elem *e;

	for (e = list_begin (&t->children_list); e != list_end (&t->children_list); e = list_next (e))
	{
		struct child_process *cp = list_entry (e, struct child_process, elem);
		if (pid == cp->pid)
			return cp;
	}
	return NULL;
}

void remove_child_process (struct child_process *cp)
{
	list_remove(&cp->elem);
	free(cp);
}

/*
void remove_child_processes (void)
{
	struct thread *t = thread_current();
	struct list_elem *e;
	

	if (!list_empty(&t->children_list))
	{for (e = list_begin (&t->children_list); e != list_end (&t->children_list); e = list_next (e))
        {
		struct child_process *cp = list_entry (e, struct child_process, elem);
		list_remove(&cp->elem);
		free(cp);
        }}


}

*/

void remove_child_processes (void)
{
	struct thread *t = thread_current();
	struct list_elem *next, *e = list_begin(&t->children_list);

	while (e != list_end (&t->children_list))
	{
		next = list_next(e);
		struct child_process *cp = list_entry (e, struct child_process, elem);
		list_remove(&cp->elem);
		free(cp);
		e = next;
	}
}

void get_arg (struct intr_frame *f, int *arg, int n)
{
	int i;
	int *ptr;
	for (i = 0; i < n; i++)
	{
		ptr = (int *) f->esp + i + 1;
		check_valid_ptr((const void *) ptr);
		arg[i] = *ptr;
	}
}

void check_valid_buffer (void* buffer, unsigned size)
{
	unsigned i;
	char* local_buffer = (char *) buffer;
	for (i = 0; i < size; i++)
	{
		check_valid_ptr((const void*) local_buffer);
		local_buffer++;
	}
}
