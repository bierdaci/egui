#include "std.h"
#include "timer.h"

typedef enum {TimerDoing, TimerDel} TimerStatus;

typedef struct _TimerData TimerData;
struct _TimerData {
	TimerStatus status;
	euint       interval;
	euint       msec;
	ePointer    args;
	eTimerFunc  func;
	TimerData  *next;
};

static TimerData *timer_head        = ENULL;
static TimerData *add_timer_head    = ENULL;
#ifdef WIN32
static e_thread_mutex_t timer_lock = {0};
#else
static e_thread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void timer_del(TimerData *timer)
{
	TimerData **tt = &timer_head;

	while (*tt) {
		if (*tt == timer) {
			*tt = timer->next;
			e_free(timer);
			break;
		}
		tt = &(*tt)->next;
	}
	return;
}

static void dispense_usec(euint sec, euint usec)
{
	static euint remain = 0;
	TimerData *timer;
	euint      msec;

	msec   = (remain + usec) / 1000 + sec * 1000;
	remain = (remain + usec) % 1000;

	timer = timer_head;
	while (timer) {
		if (timer->status == TimerDel) {
			TimerData *t = timer;
			timer = t->next;
			timer_del(t);
		}
		else {
			timer->msec += msec;
			if (timer->msec >= timer->interval) {
				if (timer->func &&
						timer->func((eTimer)timer, timer->msec / timer->interval, timer->args) < 0)
					timer->status = TimerDel;
				timer->msec = timer->msec % timer->interval;
			}
			timer = timer->next;
		}
	}
}

static void add_timer(void)
{
	e_thread_mutex_lock(&timer_lock);
	while (add_timer_head) {
		TimerData *new = add_timer_head;
		add_timer_head = add_timer_head->next;
		new->next      = timer_head;
		timer_head     = new;
	}
	e_thread_mutex_unlock(&timer_lock);
}

void e_timer_loop(void)
{
    struct timeval tp;
    struct timezone tzp;
    static euint secbase = 0, oldsec;
	static euint oldusec;
	euint sec, usec;
    euint newsec;
	euint newusec;

	add_timer();

	gettimeofday(&tp, &tzp);

	if (!secbase) {
		oldsec  = newsec  = secbase = tp.tv_sec;
		oldusec = newusec = tp.tv_usec;
	}
	else {
		newsec  = tp.tv_sec;
		newusec = tp.tv_usec;
	}

	if (newusec < oldusec) {
		sec  = newsec - oldsec - 1;
		usec = 1000000 - oldusec + newusec;
	}
	else {
		sec  = newsec - oldsec;
		usec = newusec - oldusec;
	}

	dispense_usec(sec, usec);

	oldsec  = newsec;
	oldusec = newusec;
}

eTimer e_timer_add(euint interval, eTimerFunc func, ePointer args)
{
	TimerData *timer = e_calloc(1, sizeof(TimerData));

	timer->interval  = interval;
	timer->args      = args;
	timer->func      = func;
	timer->status    = TimerDoing;

	e_thread_mutex_lock(&timer_lock);
	timer->next      = add_timer_head;
	add_timer_head   = timer;
	e_thread_mutex_unlock(&timer_lock);

	return (eTimer)timer;
}

void e_timer_del(eTimer timer)
{
	TimerData *t = (TimerData *)timer;
	t->status    = TimerDel;
}

void e_timer_init(void)
{
#ifdef WIN32
	e_thread_mutex_init(&timer_lock, NULL);
#endif
}