/***************************************************************************

  timer.c

  Functions needed to generate timing and synchronization between several
  CPUs.

  Changes 2/27/99:
  	- added some rounding to the sorting of timers so that two timers
  		allocated to go off at the same time will go off in the order
  		they were allocated, without concern for floating point rounding
  		errors (thanks Juergen!)
  	- fixed a bug where the base_time was not updated when a CPU was
  		suspended, making subsequent calls to get_relative_time() return an
  		incorrect time (thanks Nicola!)
  	- changed suspended CPUs so that they don't eat their timeslice until
  		all other CPUs have used up theirs; this allows a slave CPU to
  		trigger a higher priority CPU in the middle of the timeslice
  	- added the ability to call timer_reset() on a oneshot or pulse timer
  		from within that timer's callback; in this case, the timer won't
  		get removed (oneshot) or won't get reprimed (pulse)

  Changes 12/17/99 (HJB):
	- added overclocking factor and functions to set/get it at runtime.

  Changes 12/23/99 (HJB):
	- added burn() function pointer to tell CPU cores when we want to
	  burn cycles, because the cores might need to adjust internal
	  counters or timers.

***************************************************************************/

#include "cpuintrf.h"
#include "driver.h"
#include "timer.h"


#define MAX_TIMERS 256


#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



/*-------------------------------------------------
	internal timer structure
-------------------------------------------------*/

typedef struct timer_entry
{
	struct timer_entry *next;
	struct timer_entry *prev;
	void (*callback)(int);
	int callback_param;
	int tag;
	UINT8 enabled;
	UINT8 temporary;
	double period;
	double start;
	double expire;
} timer_entry;



/*-------------------------------------------------
	global variables
-------------------------------------------------*/

/* conversion constants */
double cycles_to_sec[MAX_CPU];
double sec_to_cycles[MAX_CPU];

/* list of active timers */
static timer_entry timers[MAX_TIMERS];
static timer_entry *timer_head;
static timer_entry *timer_free_head;
static timer_entry *timer_free_tail;

/* other internal states */
static double global_offset;
static timer_entry *callback_timer;
static int callback_timer_modified;



/*-------------------------------------------------
	get_relative_time - return the current time
	relative to the global_offset
-------------------------------------------------*/

INLINE double get_relative_time(void)
{
	int activecpu = cpu_getactivecpu();
	return (activecpu >= 0) ? cpunum_get_localtime(activecpu) : 0;
}



/*-------------------------------------------------
	timer_new - allocate a new timer
-------------------------------------------------*/

INLINE timer_entry *timer_new(void)
{
	timer_entry *timer;

	/* remove an empty entry */
	if (!timer_free_head)
		return NULL;
	timer = timer_free_head;
	timer_free_head = timer->next;
	if (!timer_free_head)
		timer_free_tail = NULL;

	return timer;
}



/*-------------------------------------------------
	timer_list_insert - insert a new timer into
	the list at the appropriate location
-------------------------------------------------*/

INLINE void timer_list_insert(timer_entry *timer)
{
	double expire = timer->enabled ? timer->expire : TIME_NEVER;
	timer_entry *t, *lt = NULL;

	/* sanity checks for the debug build */
	#ifdef MAME_DEBUG
	{
		int tnum = 0;

		/* loop over the timer list */
		for (t = timer_head; t; t = t->next, tnum++)
		{
			if (t == timer)
				printf("This timer is already inserted in the list!\n");
			if (tnum == MAX_TIMERS-1)
				printf("Timer list is full!\n");
		}
	}
	#endif

	/* loop over the timer list */
	for (t = timer_head; t; lt = t, t = t->next)
	{
		/* if the current list entry expires after us, we should be inserted before it */
		/* note that due to floating point rounding, we need to allow a bit of slop here */
		/* because two equal entries -- within rounding precision -- need to sort in */
		/* the order they were inserted into the list */
		if ((t->expire - expire) > TIME_IN_NSEC(1))
		{
			/* link the new guy in before the current list entry */
			timer->prev = t->prev;
			timer->next = t;

			if (t->prev)
				t->prev->next = timer;
			else
				timer_head = timer;
			t->prev = timer;
			return;
		}
	}

	/* need to insert after the last one */
	if (lt)
		lt->next = timer;
	else
		timer_head = timer;
	timer->prev = lt;
	timer->next = NULL;
}



/*-------------------------------------------------
	timer_list_remove - remove a timer from the
	linked list
-------------------------------------------------*/

INLINE void timer_list_remove(timer_entry *timer)
{
	/* sanity checks for the debug build */
	#ifdef MAME_DEBUG
	{
		timer_entry *t;
		int tnum = 0;

		/* loop over the timer list */
		for (t = timer_head; t && t != timer; t = t->next, tnum++) ;
		if (t == NULL)
			printf ("timer not found in list");
	}
	#endif

	/* remove it from the list */
	if (timer->prev)
		timer->prev->next = timer->next;
	else
		timer_head = timer->next;
	if (timer->next)
		timer->next->prev = timer->prev;
}



/*-------------------------------------------------
	timer_init - initialize the timer system
-------------------------------------------------*/

void timer_init(void)
{
	int i;

	/* we need to wait until the first call to timer_cyclestorun before using real CPU times */
	global_offset = 0.0;
	callback_timer = NULL;
	callback_timer_modified = 0;

	/* reset the timers */
	memset(timers, 0, sizeof(timers));

	/* initialize the lists */
	timer_head = NULL;
	timer_free_head = &timers[0];
	for (i = 0; i < MAX_TIMERS-1; i++)
	{
		timers[i].tag = -1;
		timers[i].next = &timers[i+1];
	}
	timers[MAX_TIMERS-1].next = NULL;
	timer_free_tail = &timers[MAX_TIMERS-1];
}



/*-------------------------------------------------
	timer_free - remove all timers on the current
	resource tag
-------------------------------------------------*/

void timer_free(void)
{
	int tag = get_resource_tag();
	timer_entry *timer, *next;

	/* scan the list */
	for (timer = timer_head; timer != NULL; timer = next)
	{
		/* prefetch the next timer in case we remove this one */
		next = timer->next;

		/* if this tag matches, remove it */
		if (timer->tag == tag)
			timer_remove(timer);
	}
}



/*-------------------------------------------------
	timer_time_until_next_timer - return the
	amount of time until the next timer fires
-------------------------------------------------*/

double timer_time_until_next_timer(void)
{
	double time = get_relative_time();
	return timer_head->expire - time;
}



/*-------------------------------------------------
	timer_adjust_global_time - adjust the global
	time; this is also where we fire the timers
-------------------------------------------------*/

void timer_adjust_global_time(double delta)
{
	timer_entry *timer;
	
	/* add the delta to the global offset */
	global_offset += delta;
	
	/* scan the list and adjust the times */
	for (timer = timer_head; timer != NULL; timer = timer->next)
	{
		timer->start -= delta;
		timer->expire -= delta;
	}
	
	LOG(("timer_adjust_global_time: delta=%.9f head->expire=%.9f\n", delta, timer_head->expire));
	
	/* now process any timers that are overdue */
	while (timer_head->expire < TIME_IN_NSEC(1))
	{
		int was_enabled = timer_head->enabled;

		/* if this is a one-shot timer, disable it now */
		timer = timer_head;
		if (timer->period == 0)
			timer->enabled = 0;

		/* set the global state of which callback we're in */
		callback_timer_modified = 0;
		callback_timer = timer;

		/* call the callback */
		if (was_enabled && timer->callback)
		{
			LOG(("Timer %08X fired (expire=%.9f)\n", (UINT32)timer, timer->expire));
			profiler_mark(PROFILER_TIMER_CALLBACK);
			(*timer->callback)(timer->callback_param);
			profiler_mark(PROFILER_END);
		}

		/* clear the callback timer global */
		callback_timer = NULL;

		/* reset or remove the timer, but only if it wasn't modified during the callback */
		if (!callback_timer_modified)
		{
			/* if the timer is temporary, remove it now */
			if (timer->temporary)
				timer_remove(timer);

			/* otherwise, reschedule it */
			else
			{
				timer->start = timer->expire;
				timer->expire += timer->period;

				timer_list_remove(timer);
				timer_list_insert(timer);
			}
		}
	}
}



/*-------------------------------------------------
	timer_alloc - allocate a permament timer that
	isn't primed yet
-------------------------------------------------*/

void *timer_alloc(void (*callback)(int))
{
	double time = get_relative_time();
	timer_entry *timer = timer_new();

	/* fail if we can't allocate a new entry */
	if (!timer)
		return NULL;

	/* fill in the record */
	timer->callback = callback;
	timer->callback_param = 0;
	timer->enabled = 0;
	timer->temporary = 0;
	timer->tag = get_resource_tag();
	timer->period = 0;

	/* compute the time of the next firing and insert into the list */
	timer->start = time;
	timer->expire = TIME_NEVER;
	timer_list_insert(timer);

	/* return a handle */
	return timer;
}



/*-------------------------------------------------
	timer_adjust - adjust the time when this
	timer will fire, and whether or not it will
	fire periodically
-------------------------------------------------*/

void timer_adjust(void *which, double duration, int param, double period)
{
	double time = get_relative_time();
	timer_entry *timer = which;

	/* if this is the callback timer, mark it modified */
	if (timer == callback_timer)
		callback_timer_modified = 1;

	/* compute the time of the next firing and insert into the list */
	timer->callback_param = param;
	timer->enabled = 1;

	/* set the start and expire times */
	timer->start = time;
	timer->expire = time + duration;
	timer->period = period;

	/* remove and re-insert the timer in its new order */
	timer_list_remove(timer);
	timer_list_insert(timer);
	
	/* if this was inserted as the head, abort the current timeslice and resync */
LOG(("timer_adjust %08X to expire @ %.9f\n", (UINT32)which, timer->expire));
	if (timer == timer_head && cpu_getexecutingcpu() >= 0)
		activecpu_abort_timeslice();
}



/*-------------------------------------------------
	timer_pulse - allocate a pulse timer, which
	repeatedly calls the callback using the given
	period
-------------------------------------------------*/

void timer_pulse(double period, int param, void (*callback)(int))
{
	void *timer = timer_alloc(callback);

	/* fail if we can't allocate */
	if (!timer)
		return;

	/* adjust to our liking */
	timer_adjust(timer, period, param, period);
}



/*-------------------------------------------------
	timer_set - allocate a one-shot timer, which
	calls the callback after the given duration
-------------------------------------------------*/

void timer_set(double duration, int param, void (*callback)(int))
{
	timer_entry *timer = timer_alloc(callback);

	/* fail if we can't allocate */
	if (!timer)
		return;

	/* mark the timer temporary */
	timer->temporary = 1;

	/* adjust to our liking */
	timer_adjust(timer, duration, param, 0);
}



/*-------------------------------------------------
	timer_reset - reset the timing on a timer
-------------------------------------------------*/

void timer_reset(void *which, double duration)
{
	timer_entry *timer = which;

	/* adjust the timer */
	timer_adjust(timer, duration, timer->callback_param, timer->period);
}



/*-------------------------------------------------
	timer_remove - remove a timer from the system
-------------------------------------------------*/

void timer_remove(void *which)
{
	timer_entry *timer = which;

	/* error if this is an inactive timer */
	if (timer->tag == -1)
	{
		logerror("timer_remove: removed an inactive timer!\n");
		return;
	}

	/* remove it from the list */
	timer_list_remove(timer);

	/* mark it as dead */
	timer->tag = -1;

	/* free it up by adding it back to the free list */
	if (timer_free_tail)
		timer_free_tail->next = timer;
	else
		timer_free_head = timer;
	timer->next = NULL;
	timer_free_tail = timer;
}



/*-------------------------------------------------
	timer_enable - enable/disable a timer
-------------------------------------------------*/

int timer_enable(void *which, int enable)
{
	timer_entry *timer = which;
	int old;

	/* set the enable flag */
	old = timer->enabled;
	timer->enabled = enable;

	/* remove the timer and insert back into the list */
	timer_list_remove(timer);
	timer_list_insert(timer);

	return old;
}



/*-------------------------------------------------
	timer_timeelapsed - return the time since the
	last trigger
-------------------------------------------------*/

double timer_timeelapsed(void *which)
{
	double time = get_relative_time();
	timer_entry *timer = which;

	return time - timer->start;
}



/*-------------------------------------------------
	timer_timeleft - return the time until the
	next trigger
-------------------------------------------------*/

double timer_timeleft(void *which)
{
	double time = get_relative_time();
	timer_entry *timer = which;

	return timer->expire - time;
}



/*-------------------------------------------------
	timer_get_time - return the current time
-------------------------------------------------*/

double timer_get_time(void)
{
	return global_offset + get_relative_time();
}



/*-------------------------------------------------
	timer_starttime - return the time when this
	timer started counting
-------------------------------------------------*/

double timer_starttime(void *which)
{
	timer_entry *timer = which;
	return global_offset + timer->start;
}



/*-------------------------------------------------
	timer_firetime - return the time when this
	timer will fire next
-------------------------------------------------*/

double timer_firetime(void *which)
{
	timer_entry *timer = which;
	return global_offset + timer->expire;
}
