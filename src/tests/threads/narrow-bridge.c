
/* File for 'narrow_bridge' task implementation.  
   SPbSTU, IBKS, 2017 */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "narrow-bridge.h"

#define cross 2
#define n 100
int num_emerge_left = 0, num_norm_left = 0, num_emerge_right = 0, num_norm_right = 0, on_bridge = 0, passed_left = 0, passed_right = 0, first = 0;
enum car_direction cur_dir;
struct semaphore norm_left;
struct semaphore norm_right;
struct semaphore emerge_left;
struct semaphore emerge_right;
struct list left_norm_list;
struct list right_norm_list;
struct list left_emerge_list;
struct list right_emerge_list;
struct car
{
  int num;
  struct thread *holder;
  enum car_priority prio;
  enum car_direction dir;
  struct list_elem spare;
};

// Called before test. Can initialize some synchronization objects.
void narrow_bridge_init(void)
{
  int i;
  sema_init(&norm_left, 0);
  sema_init(&norm_right, 0);
  sema_init(&emerge_left, 0);
  sema_init(&emerge_right, 0);
  list_init(&left_norm_list);
  list_init(&right_norm_list);
  list_init(&left_emerge_list);
  list_init(&right_emerge_list);
}

void arrive_bridge(enum car_priority prio, enum car_direction dir)
{
  struct thread *cur = thread_current();
  struct car *c;
  c = palloc_get_page (PAL_ZERO);
  c->holder = cur;
  c->prio = prio;
  c->dir = dir;
  if (dir == dir_right)
    if (prio == car_normal)
    {
      list_push_back(&right_norm_list, &c->spare);
      c->num = num_norm_right;
      num_norm_right++;
    }
    else
    {
      list_push_back(&right_emerge_list, &c->spare);
      c->num = num_emerge_right;
      num_emerge_right++;
    }
  else
    if (prio == car_normal)
    {
      list_push_back(&left_norm_list, &c->spare);
      c->num = num_norm_left;
      num_norm_left++;
    }
    else
    {
      list_push_back(&left_emerge_list, &c->spare);
      c->num = num_emerge_left;
      num_emerge_left++;
    }
  if (first == 0)
  {
    cur_dir = dir;
    first = 1;
  }
  int check = 0;
  if (cur_dir != dir)
  {
    if (dir == dir_right)
      if (prio == car_normal)
        sema_down(&norm_right);
      else
        sema_down(&emerge_right);
    else
      if (prio == car_normal)
        sema_down(&norm_left);
      else
        sema_down(&emerge_left);
    check = 1;
  }
  if (check == 0 && dir == dir_right && num_emerge_right != 0 && prio == car_normal)
  {
    sema_down(&norm_right);
    check = 1;
  }
  if (check == 0 && dir == dir_left && num_emerge_left != 0 && prio == car_normal)
  {
    sema_down(&norm_left);
    check = 1;
  }
  if (check == 0 && on_bridge >= cross && dir == dir_right && prio == car_normal)
  {
    sema_down(&norm_right);
    check = 1;
  }
  if (check == 0 && on_bridge >= cross && dir == dir_left && prio == car_normal)
  {
    sema_down(&norm_left);
    check = 1;
  }
  if (check == 0 && on_bridge >= cross && dir == dir_right && prio == car_emergency)
  {
    sema_down(&emerge_right);
    check = 1;
  }
  if (check == 0 && on_bridge >= cross && dir == dir_left && prio == car_emergency)
  {
    sema_down(&emerge_left);
    check = 1;
  }
  on_bridge++;
  if (dir == dir_right && prio == car_emergency) 
	num_emerge_right--;
  if (dir == dir_right && prio == car_normal) 
	num_norm_right--;
  if (dir == dir_left && prio == car_emergency) 
	num_emerge_left--;
  if (dir == dir_left && prio == car_normal) 
	num_norm_left--;
}

void exit_bridge(enum car_priority prio, enum car_direction dir)
{
  int check = 0;
  
  if (check == 0 && dir == dir_right && num_emerge_right)
  {
    sema_up(&emerge_right);
    check = 1;
  }
  if (check == 0 && dir == dir_left && num_emerge_left)
  {
    sema_up(&emerge_left);
    check = 1;
  }
  if (check == 0 && dir == dir_right && num_emerge_right == 0 && num_emerge_left)
  {
    if (on_bridge == 1)
      sema_up(&emerge_left);
    check = 1;
  }
  if (check == 0 && dir == dir_left && num_emerge_left == 0 && num_emerge_right)
  {
    if (on_bridge == 1)
      sema_up(&emerge_right);
    check = 1;
  }
  if (check == 0 && dir == dir_right && num_norm_right)
  {
    sema_up(&norm_right);
    check = 1;
  }
  if (check == 0 && dir == dir_left && num_norm_left)
  {
    sema_up(&norm_left);
    check = 1;
  }
  if (check == 0 && dir == dir_right && num_norm_right == 0 && num_norm_left)
  {
    if (on_bridge == 1)
      sema_up(&norm_left);
    check = 1;
  }
  if (check == 0 && dir == dir_left && num_norm_left == 0 && num_norm_right)
  {
    if (on_bridge == 1)
      sema_up(&norm_right);
    check = 1;
  }
  on_bridge--;
}
