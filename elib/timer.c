#include "std.h"
#include "timer.h"

typedef enum {TimerDoing, TimerDel} TimerStatus;

typedef struct _TimerNode TimerNode_t;
struct _TimerNode {
	TimerStatus status;
	euint       interval;
	ePointer    args;
	euint       msec;
	eTimerFunc  func;

	euint		rest;
	TimerNode_t  *next;
};

static TimerNode_t *timer_head        = ENULL;
static TimerNode_t *add_timer_head    = ENULL;
#ifdef WIN32
static e_thread_mutex_t timer_lock = {0};
#else
static e_thread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void timer_del(TimerNode_t *timer)
{
	TimerNode_t **tt = &timer_head;

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
	TimerNode_t *timer;
	euint      msec;

	msec   = (remain + usec) / 1000 + sec * 1000;
	remain = (remain + usec) % 1000;

	timer = timer_head;
	while (timer) {
		if (timer->status == TimerDel) {
			TimerNode_t *t = timer;
			timer = t->next;
			timer_del(t);
		}
		else {
			timer->msec += msec;
			if (timer->msec >= timer->interval) {
				if ((timer->rest < timer->interval * 3) && (timer->msec >= timer->interval * 2))
					timer->rest += msec;
				else {
					timer->rest = 0;
					if (timer->func &&
							timer->func((eTimer)timer, timer->msec / timer->interval, timer->args) < 0)
						timer->status = TimerDel;
					timer->msec = timer->msec % timer->interval;
				}
			}
			timer = timer->next;
		}
	}
}

static void add_timer(void)
{
	e_thread_mutex_lock(&timer_lock);
	while (add_timer_head) {
		TimerNode_t *new = add_timer_head;
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
	TimerNode_t *timer = e_calloc(1, sizeof(TimerNode_t));

	timer->interval  = interval;
	timer->rest      = 0;
	timer->args      = args;
	timer->func      = func;
	timer->status    = TimerDoing;

	e_thread_mutex_lock(&timer_lock);
	timer->next      = add_timer_head;
	add_timer_head   = timer;
	e_thread_mutex_unlock(&timer_lock);

	return (eTimer)timer;
}

typedef struct {
	TimerStatus status;
	euint       interval;
	ePointer    args;
	euint       msec;
	eTimerFunc  func;

	euint       remain;
	euint       oldsec;
	euint       oldusec;
} ThreadTimer_t;

static ePointer thread_timer_handler(ePointer data)
{
	ThreadTimer_t *timer = (ThreadTimer_t *)data;

	struct timeval  tp;
	struct timezone tzp;

	euint sec, usec;
	euint newsec;
	euint newusec;

	gettimeofday(&tp, &tzp);

	timer->remain  = 0;
	timer->oldsec  = newsec  = tp.tv_sec;
	timer->oldusec = newusec = tp.tv_usec;

	do {
		if (newusec < timer->oldusec) {
			sec  = newsec - timer->oldsec - 1;
			usec = 1000000 - timer->oldusec + newusec;
		}
		else {
			sec  = newsec - timer->oldsec;
			usec = newusec - timer->oldusec;
		}

		timer->oldsec  = newsec;
		timer->oldusec = newusec;

		timer->msec  += (timer->remain + usec) / 1000 + sec * 1000;
		timer->remain = (timer->remain + usec) % 1000;

		if (timer->msec >= timer->interval) {
			if (timer->func((eTimer)timer, timer->msec / timer->interval, timer->args) < 0)
				timer->status = TimerDel;
			timer->msec = timer->msec % timer->interval;
		}
		else {
			usleep((timer->interval - timer->msec) * 1000 - timer->remain);
		}

		gettimeofday(&tp, &tzp);
		newsec  = tp.tv_sec;
		newusec = tp.tv_usec;

	} while (timer->status == TimerDoing);

	return (ePointer)data;
}

eTimer e_thread_timer_add(euint interval, eTimerFunc func, ePointer args)
{
	ThreadTimer_t *timer;
	e_thread_t pid;

	if (!func) return -1;

	timer = e_calloc(1, sizeof(ThreadTimer_t));
	timer->interval  = interval;
	timer->args      = args;
	timer->func      = func;
	timer->status    = TimerDoing;

	if (e_thread_create(&pid, thread_timer_handler, timer) < 0) {
		e_free(timer);
		return -1;
	}

	return (eTimer)timer;
}

void e_timer_del(eTimer timer)
{
	TimerNode_t *t = (TimerNode_t *)timer;
	t->status    = TimerDel;
}

void e_timer_init(void)
{
#ifdef WIN32
	e_thread_mutex_init(&timer_lock, NULL);
#endif
}
