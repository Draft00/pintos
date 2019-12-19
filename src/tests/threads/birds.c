/*
  File for 'birds' task implementation.
*/


#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "devices/timer.h"

static struct semaphore sema_dish;
static struct semaphore sema_mom;
static bool mom_woke_up;
static int food_size, food_eaten;
static struct condition cond_dish;
static struct condition cond_mom;
//static bool mom_woke_up;
//static int food_size, food_eaten;

static void init(unsigned int dish_size)
{
    sema_init(&sema_dish, dish_size);
    sema_init(&sema_mom, 0);
    mom_woke_up = 0;
	food_eaten = 0;
    food_size = dish_size;
}

static void bird(void* arg)
{
    msg("bird created.");

	while (1)
	{
		while (food_eaten <= food_size)
			thread_yield();
					
		msg ("Mom woke up");
		food_eaten = 0;
		sema_up(&sema_mom);
		thread_yield();
		sema_down(&sema_mom);
	}
}

static void chick(void* arg)
{
    msg("chick %d created.", (int) arg);
    while (1)
    {
		if (food_eaten > food_size)
		{
			thread_yield();
			sema_down(&sema_mom);
			sema_up(&sema_mom);
		}
		
		food_eaten++;
		msg("Chick %d eating", (int)(arg));
		timer_sleep(15);
	}
}


void test_birds(unsigned int num_chicks, unsigned int dish_size)
{
  unsigned int i;
  init(dish_size);

  thread_create("bird", PRI_DEFAULT, &bird, NULL);

  for(i = 0; i < num_chicks; i++)
  {
    char name[32];
    snprintf(name, sizeof(name), "chick_%d", i + 1);
    thread_create(name, PRI_DEFAULT, &chick, (void*) (i+1) );
  }

  timer_msleep(5000);
  pass();
}
