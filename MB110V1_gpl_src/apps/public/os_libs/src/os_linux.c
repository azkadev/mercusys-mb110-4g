/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		os_linux.c
 * brief		
 * details	
 *
 * author	Wu Zhiqin
 * version	
 * date		04May11
 *
 *
 * history \arg	
 */

#if defined(__LINUX_OS_FC__)

#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdio.h> 
#include <time.h> 
#include <assert.h>
#include <linux/sched.h>

#include <os_lib.h>

/**************************************************************************************************/
/*                                      DEFINES                                                   */
/**************************************************************************************************/
#define MAX_TIMER_NUM	((SIGNUM_MAX) - (SIGNUM_DYNAMIC_MIN))


/* 
 * brief ARP proc path	
 */
#define _ARP_TABLE_NAME		"/proc/net/arp"


/**************************************************************************************************/
/*                                      TYPES                                                     */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      EXTERN_PROTOTYPES                                         */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      LOCAL_PROTOTYPES                                          */
/**************************************************************************************************/
static int os_timerInit(void);
static int os_timerCreate(int signum, sighandler handler, timer_t *tid);
static timer_t os_timerSet(timer_t timer_id, int nDelayMilliseconds, int nIntervalMilliseconds);

/**************************************************************************************************/
/*                                      VARIABLES                                                 */
/**************************************************************************************************/

/* 
 * brief for ftok id.
 */
static int queues = 0;

/* 
 * brief	for timer
 */
timer_t *l_timers = NULL;


/**************************************************************************************************/
/*                                      LOCAL_FUNCTIONS                                           */
/**************************************************************************************************/
/*
 * fn		static int os_timerInit(void)
 * brief	malloc memory for to contail all timers status
 */
static int os_timerInit(void)
{
	if (NULL == l_timers) /* first called */
	{
		l_timers = (timer_t*)malloc(sizeof(timer_t) * MAX_TIMER_NUM);
		if (NULL == l_timers)
		{
			printf("Failed to malloc for l_timers.\n");
			return -1;
		}
		memset((void*)l_timers, 0, (sizeof(timer_t) * MAX_TIMER_NUM));
	}

	return 0;
}

/*
 * fn		static int os_timerCreate(int signum, sighandler handler_func, timer_t *tid)
 * brief	creates a interval timer, which will deliver signal to the thread each interval,
 *			and register a function to the signal, then function will be excuted each interval
 *
 * param[in]	signum			signum of signal to deliver
 * param[in]	handler_func	function registered to signal
 * param[out]	tid				timer ID pointer
 *
 * return	result
 * retval	0		timer id created, saved in tid
 *			< 0		error
 *
 * note
 */
static int os_timerCreate(int signum, sighandler handler_func, timer_t *tid)
{
	struct sigevent se;
	int index = signum - SIGNUM_DYNAMIC_MIN;

	if (NULL == tid)
	{
		return -1;
	}
	if (signum < SIGNUM_DYNAMIC_MIN || signum > SIGNUM_MAX)
	{
		printf("tpCreateTimer: signum is too small\n");
		return -1;
	}

	if (os_timerInit())
		return -1;

	/* signal already registered? */
	if (l_timers[index])
	{
		printf("tpCreateTimer: signal %d has been used\n", signum);
		return -2;
	}

	memset (&se, 0, sizeof (se));

	signal(signum, handler_func);

	se.sigev_notify = SIGEV_SIGNAL;
	se.sigev_signo = signum;
	se.sigev_notify_function = handler_func;
	//se.sigev_value.sival_int = signum;
	//se.sigev_value.sival_ptr = (void *) &tid;

	if (timer_create(CLOCK_MONOTONIC, &se, tid) < 0)
	{
		perror("timer_creat");
		return -3;
	}

	return 0;
}

/*
 * fn		timer_t os_timerSet(timer_t timer_id, int nDelayMilliseconds, int nIntervalMilliseconds)
 * brief	start the created timer
 *
 * param[in]	timer_id	timer id of timer to start
 * param[in]	nDelayMilliseconds	time waiting before first run (millisecond)
 * param[in]	nIntervalMilliseconds	nIntervalMilliseconds after the first run (millisecond), if 0, will not run
 *							any more after first run. WARNING: if 0, the timer wil not
 *							NOT be deleted automatically, and can be set to run again
 * param[out]	N/A
 *
 * return	result of start timer
 * retval	timer id	start OK
 *			-1	start ERROR
 *
 * note		WARNING: see param[in]	nIntervalMilliseconds
 */
static timer_t os_timerSet(timer_t timer_id, int nDelayMilliseconds, int nIntervalMilliseconds)
{
	struct itimerspec ts, ots;
	int sec, nsec;

	sec = nDelayMilliseconds / 1000;
	nsec = (nDelayMilliseconds - sec * 1000) * 1000 * 1000;
	ts.it_value.tv_sec = sec;
	ts.it_value.tv_nsec =  nsec;

	sec = nIntervalMilliseconds / 1000;
	nsec = (nIntervalMilliseconds - sec * 1000) * 1000 * 1000;
	ts.it_interval.tv_sec = sec;
	ts.it_interval.tv_nsec = nsec;

	/* we need a nDelayMilliseconds delay, so TIMER_ABSTIME cant be set -- lsz 081215
	 *if (timer_settime(timer_id, TIMER_ABSTIME, &ts, &ots) < 0) */

	if (timer_settime(timer_id, 0, &ts, &ots) < 0)
	{
		perror ("tpSetTimer");
		return (timer_t)(-1);
	}

	return timer_id;
}


/**************************************************************************************************/
/*                                      PUBLIC_FUNCTIONS                                          */
/**************************************************************************************************/

/**************************************************************************************************/
/*                                      GLOBAL_FUNCTIONS                                          */
/**************************************************************************************************/

/* 
 * fn		int os_semCreate(int initialCount, OS_SEM_OPTIONS opt, OS_SEM *pSem)
 * brief	create semaphore.
 * details	opt is for vxWorks OS. opt value may be SEM_Q_FIFO_K or SEM_Q_PRIORITY_K
 *
 * param [in]	initialCount - initial value for semaphore.
 * param [in]	opt	- SEM_Q_FIFO_K or SEM_Q_PRIORITY_K, used in vxWorks OS.
 * param[out]	pSem - to keep the semaphore ID.
 *
 * return	0 if success, -1 otherwise.	
 */
int os_semCreate(int initialCount, OS_SEM_OPTIONS opt, OS_SEM *pSem)
{
    int ret;

	assert(pSem != NULL);
	
    ret = sem_init(pSem, 0, initialCount);
    if (ret != 0)
    {
        switch(errno)
        {
            case EINVAL:
                printf("Semaphore initialization failed: value exceeds SEM_VALUE_MAX.");
                break;
            case ENOSYS:
                printf("Semaphore initialization failed: pshared is non-zero, but the system "
							"does  not  support  process shared semaphores (see sem_overview.)");
                break;
            default:
                printf("Semaphore initialization failed: unknown fault.");
                break;
        }
        return -1;
    }

    return 0;
}



/* 
 * fn		int os_semDestroy  (OS_SEM *pSem)
 * brief	delete semaphore.
 * details	
 *
 * param [in]	sem - semaphore ID to delete
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int os_semDestroy  (OS_SEM *pSem)
{
    if (0 != sem_destroy(pSem))
    {
        printf("Semaphore destroy failed.");
        return -1;
    }

    return 0;
}



/* 
 * fn		int os_semTake(OS_SEM *pSem, unsigned int timeout)
 * brief	take a semaphore
 * details	This routine performs the take operation on a specified semaphore. For vxWorks OS, 
 *			A timeout in ticks may be specified. If a task times out, os_semTake( ) will return 
 *			ERROR. Timeouts of WAIT_FOREVER (-1) and NO_WAIT (0) indicate to wait indefinitely or 
 *			not to wait at all. 
 *
 * param [in]	sem - semaphore ID to take 
 * param [in]	timeout - for vxWorks, timeout in ticks 
 * param[out]	
 *
 * return	0 if success, -1 otherwise.	
 */
int os_semTake(OS_SEM *pSem, unsigned int timeout)
{
    int ret;
    do
    {
        ret = sem_wait(pSem);
    } while (-1 == ret && EINTR == errno);

    if (ret)
    {
        perror("sem_wait");
        printf("Semaphore wait failed.");
    }

    return ret;
}



/* 
 * fn		int os_semGive(OS_SEM *pSem)
 * brief	give a semaphore
 * details	This routine performs the give operation on a specified semaphore.
 *
 * param [in]	sem - semaphore ID to give 
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int os_semGive(OS_SEM *pSem)
{
    if (0 != sem_post(pSem))
    {
		perror("sem_post");
        printf("Semaphore post failed.");
        return -1;
    }

    return 0;
}


/* 
 * fn		    int os_semVCreate(unsigned int key, int initialValue, OS_V_SEM *pSem) 
 * brief	    Create a new system v semaphore.for process synchronous.
 * details	    
 *
 * param [in]	key			 - a none zero(IPC_PRIVATE) key is associated with a unique 
 *							   semaphore set.
 * param [in]	initialValue - initial value of the system v semaphore. can be 0 or 1
 * param[out]	pSem		 - to keep the system v semaphore ID.
 *
 * return	    0 if success, -1 if failed
 * retval	    
 *
 * note		  	In Linux, this routine does not create a systen v semaphore set, it only create
 *				a system v semaphore.  
 */
int os_semVCreate(unsigned int key, int initialValue, OS_V_SEM *pSem)
{
	int flags;

	assert((pSem != NULL) && (initialValue > -1 && initialValue < 2));

	flags = IPC_CREAT | 0666;

	if ((*pSem = semget((key_t)key, 1, flags)) == -1)
	{	
		printf("semget failed, errno=%d", errno);

		return -1;
	}

	/*
	 * We are creating new semaphore, so initialize semaphore to 1
	 */
	if(semctl(*pSem, 0, SETVAL, initialValue) == -1)
	{
		printf("setctl setval 1 failed, errno=%d", errno);

		return -1;
	}

	return 0;
}



/* 
 * fn		    int os_semVGet(unsigned int key, OS_V_SEM *pSem) 
 * brief	    Get a existed system v semaphore referenced by key.
 * details	    
 *
 * param [in]	key	 - a none zero(IPC_PRIVATE) key is associated with a unique 
 *					   semaphore set.
 * param[out]	pSem - to keep the system v semaphore ID.
 *
 * return	    0 if success, -1 if failed	    
 */
int os_semVGet(unsigned int key, OS_V_SEM *pSem)
{
	int flags = IPC_CREAT | 0666;
	
	if (key == 0)
	{
		printf("use key IPC_PRIVATE(0) will aways create a new v semaphore.");
		return -1;
	}

	assert(pSem != NULL);

	if ((*pSem = semget((key_t)key, 0, flags)) == -1)
	{
		printf("v semaphore with key(%x) is not exitst, please create the semaphore.", key);
		return -1;
	}

	return 0;
}



/* 
 * fn		    int os_semVDestory(OS_V_SEM sem) 
 * brief	    Destory a system v semaphore.
 * details	    
 *
 * param [in]	sem - system v semaphore ID.
 * param[out]	
 *
 * return	    0 if success, -1 if failed		    
 */
int os_semVDestory(OS_V_SEM sem)
{
	if (sem == -1)
	{
		return 0;
	}

	if (semctl(sem, 0, IPC_RMID) < 0)
	{
		printf("delete v semaphore %d failed, errno=%d", sem, errno);
		return -1;
	}
	
	OS_DEBUG_PRINT("v semaphore %d deleted.", sem);

	return 0;
}



/* 
 * fn		    int os_semVTake(OS_V_SEM sem) 
 * brief	    Take a system v semaphore (P operate)
 * details	    
 *
 * param [in]	sem - system v semaphore ID.
 * param[out]	
 *
 * return	    0 if success, -1 if failed		    
 */
int os_semVTake(OS_V_SEM sem)
{
    int ret;
	struct sembuf semBuf;
	semBuf.sem_num = 0;
	semBuf.sem_op = -1;
	semBuf.sem_flg = SEM_UNDO;

    do
    {
    	ret = semop(sem, &semBuf, 1);
    } while (-1 == ret && EINTR == errno);
    if (ret)
    {
        printf("Take semaphore failed, errno=%d", errno);
        return -1;
    }

	return 0;
}



/* 
 * fn		    int os_semVGive(OS_V_SEM sem) 
 * brief	    Give a system v semaphore (V operate)
 * details	    
 *
 * param [in]	sem - system v semaphore ID.
 * param[out]	
 *
 * return	    0 if success, -1 if failed		    
 */
int os_semVGive(OS_V_SEM sem)
{
    int ret;
	struct sembuf semBuf;
	semBuf.sem_num = 0;
	semBuf.sem_op = 1;
	semBuf.sem_flg = SEM_UNDO;

    do
    {
    	ret = semop(sem, &semBuf, 1);
    } while (-1 == ret && EINTR == errno);
    if (ret)
    {
        printf("Give semaphore failed, errno=%d", errno);
        return -1;
    }

	return 0;
}



/* 
 * fn		int os_threadCreate(char * pName,  
 *								int priority, 
 *								int stack_size, 
 *								FUNCPTR pFunc, 
 *								void * pArg, 				
 *								OS_THREAD * pTid)
 * brief	create a Linux thread or spawn a vxWorks task
 * details	
 *
 * param [in]	pName - for vxWorks, name of new  task.
 * param [in]	priority - for vxWorks, priority of new task
 * param [in]	stackSize - for vxWorks, size (bytes) of stack needed plus name 
 * param [in]	pFunc - entry point of new task or thread.
 * param [in]	pArg - task args to pass to func 
 * param[out]	pTid - to keep the thread id or task id
 *
 * return	0 if success, -1 otherwise.	
 */
int os_threadCreate(char * pName,  
					int priority, 
					int stackSize, 
					FUNCPTR pFunc, 
					void * pArg, 				
					OS_THREAD * pTid)
{
    int ret = -1;
	int policy = SCHED_NORMAL;
    pthread_attr_t thread_attr;
	OS_PARAMS params;
	assert((pFunc != NULL) && (pTid != NULL));

    //init thread attr
    if (pthread_attr_init(&thread_attr) == 0) 
    {
        OS_DEBUG_PRINT( "pthread_attr_init success.");

        //set thread attr
        if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) == 0) 
        {
            OS_DEBUG_PRINT("pthread_attr_setdetachstate success.");

			/* set stack size */
			if (stackSize < PTHREAD_STACK_MIN)
			{
				stackSize = PTHREAD_STACK_MIN;
			}
			pthread_attr_setstacksize(&thread_attr, stackSize);
			/* set priority for special thread */
			if ((priority & PTHREAD_SCHED_FIFO) != 0)
			{
				priority &= ~PTHREAD_SCHED_FIFO;
				policy = SCHED_FIFO;
			}
			if ((priority & PTHREAD_SCHED_RR) != 0)
			{
				priority &= ~PTHREAD_SCHED_RR;
				policy = SCHED_RR;
			}
			pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
			if (priority != SCHED_NORMAL)
			{
				struct sched_param sch_param;
				if (priority < sched_get_priority_min(policy) || priority > sched_get_priority_max(policy))
				{
					priority = sched_get_priority_min(policy);
				}
				pthread_attr_setschedpolicy(&thread_attr, policy);
				sch_param.sched_priority = priority;
				pthread_attr_setschedparam(&thread_attr, &sch_param);
			}
            //Creat thread
            memset(&params, 0, sizeof(OS_PARAMS));
			if (pName != NULL)
			{
				strncpy(params.name, pName, OS_NAME_LEN - 1);
			}
			params.arg = pArg;
			params.priority = priority;
			if ( sem_init(&params.sem, 0, 0) )
			{
				printf("sem for thread %s created failed!\n", params.name);
				return -1;
			}
            if (pthread_create(pTid, &thread_attr, (void *)pFunc, (void *)&params) == 0)
            {
				/* params is a local variable,we MUST wait until the thread does not use it any more */
		        while (sem_wait(&params.sem) == -1 && EINTR == errno);
                OS_DEBUG_PRINT("Init thread %s successful.", pName);
                ret = 0;
            } 
            else 
            {
                switch(errno) 
                {
                    case EAGAIN:
                        printf("pthread_create(%s) failed: too much thread numbers.", pName);
                        break;
                    case EINVAL:
                        printf("pthread_create(%s) failed: thread id illegality.",pName);
                        break;
                    default :
                        printf("pthread_create(%s) failed: unknown.",pName);
                        break;
                }
            }
        }
        else
        {
            printf("pthread_attr_setdetachstate Failed.");
        }

        (void)pthread_attr_destroy(&thread_attr);
    }
    else
    {
        printf("pthread_attr_init failed .");
    }

    return ret;
}

/*
 * fn		int os_threadCreateTR143(char * pName,
 *								int priority,
 *								int stack_size,
 *								FUNCPTR pFunc,
 *								void * pArg,
 *								OS_THREAD * pTid)
 * brief	create a Linux thread or spawn a vxWorks task
 * details
 *
 * param [in]	pName - for vxWorks, name of new  task.
 * param [in]	priority - for vxWorks, priority of new task
 * param [in]	stackSize - for vxWorks, size (bytes) of stack needed plus name
 * param [in]	pFunc - entry point of new task or thread.
 * param [in]	pArg - task args to pass to func
 * param[out]	pTid - to keep the thread id or task id
 *
 * return	0 if success, -1 otherwise.
 */
int os_threadCreateTR143(char * pName,
					int priority,
					int stackSize,
					FUNCPTR pFunc,
					void * pArg,
					OS_THREAD * pTid)
{
    int ret = -1;
	int policy = SCHED_NORMAL;
    pthread_attr_t thread_attr;
	OS_PARAMS params;
	assert((pFunc != NULL) && (pTid != NULL));

    //init thread attr
    if (pthread_attr_init(&thread_attr) == 0)
    {
        OS_DEBUG_PRINT( "pthread_attr_init success.");

        //set thread attr
        if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) == 0)
        {
            OS_DEBUG_PRINT("pthread_attr_setdetachstate success.");

			/* set stack size */
			if (stackSize < PTHREAD_STACK_MIN)
			{
				stackSize = PTHREAD_STACK_MIN;
			}
			pthread_attr_setstacksize(&thread_attr, stackSize);
			/* set priority for special thread */
			if ((priority & PTHREAD_SCHED_FIFO) != 0)
			{
				priority &= ~PTHREAD_SCHED_FIFO;
				policy = SCHED_FIFO;
			}
			if ((priority & PTHREAD_SCHED_RR) != 0)
			{
				priority &= ~PTHREAD_SCHED_RR;
				policy = SCHED_RR;
			}
			pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
			if (priority != SCHED_NORMAL)
			{
				struct sched_param sch_param;
				if (priority < sched_get_priority_min(policy) || priority > sched_get_priority_max(policy))
				{
					priority = sched_get_priority_min(policy);
				}
				pthread_attr_setschedpolicy(&thread_attr, policy);
				sch_param.sched_priority = priority;
				pthread_attr_setschedparam(&thread_attr, &sch_param);
			}
            //Creat thread
            memset(&params, 0, sizeof(OS_PARAMS));
			if (pName != NULL)
			{
				strncpy(params.name, pName, OS_NAME_LEN - 1);
			}

			params.arg = pArg;
			params.priority = priority;
			//if ( sem_init(&params.sem, 0, 0) )
			//{
			//	printf("sem for thread %s created failed!\n", params.name);
			//	return -1;
			//}

            if (pthread_create(pTid, &thread_attr, (void *)pFunc, (void *)&params) == 0)
            {
				/* params is a local variable,we MUST wait until the thread does not use it any more */
	        	//while (sem_wait(&params.sem) == -1 && EINTR == errno);
                OS_DEBUG_PRINT("Init thread %s successful.", pName);
                ret = 0;
            }
            else
            {
                switch(errno)
                {
                    case EAGAIN:
                        printf("pthread_create(%s) failed: too much thread numbers.", pName);
                        break;
                    case EINVAL:
                        printf("pthread_create(%s) failed: thread id illegality.",pName);
                        break;
                    default :
                        printf("pthread_create(%s) failed: unknown.",pName);
                        break;
                }
            }
        }
        else
        {
            printf("pthread_attr_setdetachstate Failed.");
        }

        (void)pthread_attr_destroy(&thread_attr);
    }
    else
    {
        printf("pthread_attr_init failed .");
    }

    return ret;
}

/*
 * fn		void os_threadSetNamePrioFinal(OS_PARAMS *pArg)
 * brief	set name and priority for thread,and notify the caller to continue
 *
 * param[in]	pArg : inlcudes the thread name,priority or other arguments the thread required
 *
 * note		the thread MUST NOT use the argument after 100ms or this function has been called
 */
void os_threadSetNamePrioFinal(OS_PARAMS *pArg)
{
	prctl(PR_SET_NAME,	pArg->name, 0, 0, 0);
	if (PTHREAD_PRIO_OTHER == pArg->priority)
		setpriority(PRIO_PROCESS, syscall(__NR_gettid), 0);
	sem_post(&pArg->sem);
}

/* 
 * fn		int os_threadExit(OS_THREAD tid)
 * brief	exit a thread. 
 * details	in vxWorks, this routine do nothing.
 *
 * param [in]	tid - thread id or task id.
 * param[out]	
 *
 * return	0 if success, -1 otherwise.	
 */
int os_threadExit(OS_THREAD tid)
{
    pthread_exit(NULL);

    return 0;
}



/* 
 * fn		int	os_threadDelete(OS_THREAD tid)
 * brief	cancel a thread or delete a task
 * details	
 *
 * param [in]	tid - thread id to cancel or task ID of task to delete 
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int	os_threadDelete(OS_THREAD tid)
{
	if (0 != pthread_cancel (tid))
	{
		printf("thread delete failed .");
		return -1;
	}

	return 0;
}


/* 
 * fn		    OS_THREAD os_getTid() 
 * brief	    get current thread's ID
 * details	    
 *
 * return	    current thread's ID	    
 */
OS_THREAD os_getTid()
{
	return pthread_self();
}



/* 
 * fn		int os_queueCreate  (int maxMsg, int msgSize, OS_MSG_OPTIONS opt, OS_MSGQ *pQid)
 * brief	create a message queue
 * details	
 *
 * param [in]	maxMsg - for vxWorks, max messages that can be queued 
 * param [in]	msgSize - for vxWorks, max bytes in a message 
 * param [in]	opt - for vxWorks, message queue options 
 * param[out]	pQid - to keep the queue id.
 *
 * return	0 if success, -1 otherwise.		
 */
int os_queueCreate  (int maxMsg, int msgSize, OS_MSG_OPTIONS opt, OS_MSGQ *pQid)
{
    int flag;
	key_t key;

	assert(pQid != NULL);

	if (-1 == (key = ftok("/etc/services", queues++)))
	{
		printf("ftok error: no key generated");
        return -1;
	}
	
    flag = IPC_CREAT | 0666;
    *pQid = msgget(key, flag);
	
    if (*pQid == -1)
	{
        printf("msgget failed.");
        return -1;
    }
    return 0;
}



/* 
 * fn		int os_queueDelete  (OS_MSGQ qid)
 * brief	delete a message queue
 * details	
 *
 * param [in]	qid - message queue to delete 
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int os_queueDelete  (OS_MSGQ qid)
{
    if (-1 == msgctl(qid, IPC_RMID, NULL))
    {
       printf("msgget failed.");
       return -1;
    }

    return 0;
}



/* 
 * fn		int os_queueReceive(OS_MSGQ qid, long type, int timeout, int bufSize, char *pBuf)
 * brief	receive data from a message queue
 * details	This routine receives a message from the message queue qid. The received message is 
 *			copied into the specified buffer, which is bufSize in length. If the message is 
 *			longer than bufSize, the remainder of the message is discarded .
 *
 * param [in]	qid - message queue from which to receive  
 * param [in]	type - for Linux, message type to revice.
 * param [in]	timeout - for vxWorks, ticks to wait
 * param [in]	bufSize - size of buffer 
 * param[out]	pBuf - buffer to receive message 
 *
 * return	The number of bytes copied to buffer, or -1	
 */
int os_queueReceive(OS_MSGQ qid, long type, int timeout, int bufSize, char *pBuf)
{
    int ret = 0;
    int flag = 0;
    struct msgbuf {
        long mtype;
        char *text;
    }msg; 

	assert(pBuf != NULL);
	
	msg.text = pBuf;

    ret = msgrcv(qid, &msg, bufSize, type, flag); 

    if (ret == -1) {
        printf("msgrcv failed.\n");
        return -1;
    }
    strcpy(pBuf, msg.text);
    
    return ret;
}



/* 
 * fn		int os_queueSend(OS_MSGQ qid, long type, int timeout, int priority, char *pBuf)
 * brief	send data to a message queue
 * details	
 *
 * param [in]	qid - message queue on which to send
 * param [in]	type - for Linux, message type to send.
 * param [in]	timeout - for vxWorks, ticks to wait
 * param [in]	priority - for vxWorks,  MSG_PRI_NORMAL or MSG_PRI_URGENT 
 * param [in]	pBuf - message to send 
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int os_queueSend(OS_MSGQ qid, long type, int timeout, int priority, char *pBuf)
{
    int ret = 0;
    struct msgbuf {
        long mtype;
        char *text;
    }msg;

	assert(pBuf != NULL);

    msg.mtype = type;
    msg.text = pBuf;
    
    ret = msgsnd(qid, &msg, strlen(msg.text) + 1, 0);
    if(ret == -1) {
        printf("msgsend failed.");
        return -1;
    }
    
    return 0;
}



/* 
 * fn		int os_mutexCreate (OS_MUTEX *pMutex)
 * brief	create a mutex
 * details	
 *
 * param [in]	
 * param[out]	pMutex - to keep the mutex id.
 *
 * return	0 if success, -1 otherwise.		
 */
int os_mutexCreate (OS_MUTEX *pMutex)
{
	assert(pMutex != NULL);
	
    if (0 != pthread_mutex_init(pMutex,NULL))
    {
        printf("pthread mutex init failed.");
        return -1;
    }

    return 0;
}



/* 
 * fn		int os_mutexDestroy (OS_MUTEX *pMutex)
 * brief	delete a mutex
 * details	
 *
 * param [in]	pMutex - mutex to delete
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int os_mutexDestroy (OS_MUTEX *pMutex)
{
	assert(pMutex != NULL);
	
    if (0 != pthread_mutex_destroy(pMutex))
    {
        printf("pthread mutex destroy failed.");
        return -1;
    }

    return 0;
}



/* 
 * fn		int os_mutexLock(OS_MUTEX *pMutex)
 * brief	take a mutex
 * details	
 *
 * param [in]	pMutex - the mutex to take.
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int os_mutexLock(OS_MUTEX *pMutex)
{
	assert(pMutex != NULL);
	
    return pthread_mutex_lock(pMutex);
}



/* 
 * fn		int os_mutexUnlock(OS_MUTEX *pMutex)
 * brief	give a mutex
 * details	
 *
 * param [in]	pMutex - the mutex to give
 * param[out]	
 *
 * return	0 if success, -1 otherwise.	
 */
int os_mutexUnlock(OS_MUTEX *pMutex)
{
	assert(pMutex != NULL);
	
    return pthread_mutex_unlock(pMutex);
}



/* 
 * fn		int	os_shmGet(int key, size_t size, int shmFlg, void** ppShmAddr)
 * brief	Create the shared memory region.
 * details	For Linux, this routine is used to create a shared memory region. For vxWorks, this 
 *			routine is used to malloc a buffer.
 *
 * param [in]	key - key to get the shared memory region.
 * param [in]	size - shared memory size.
 * param [in]	shmFlg - flag to get the shared memory region.
 *
 * return	0 if success, -1 otherwise.	
 *
 * note		if key == 0, it will always create a new shared memory.
 */
int	os_shmGet(int key, size_t size, int shmFlg)
{
	return shmget((key_t)key, size, shmFlg);
}



/* 
 * fn		void *os_shmAt(int shmId, const void* pShmAddr, int shmFlg)
 * brief	attach to the shared memory.
 * details	For Linux, this routine, attach to the shared memory shmId, and return the address that
 *			the process attach to. For vxWorks, this routine just increase the counter of the shared
 *			memory.
 *
 * param [in]	shmId - for Linux, shared memory id.
 * param [in]	pShmAddr - In Linux, this address may be NULL or the fixed address that you want to
 *						   attath to. In vxWorks, this address is the address that return in 
 *						   os_shmGet.
 * param [in]	shmFlg - for Linux, attach flag.
 * param[out]	
 *
 * return	0 if success, -1 otherwise.	
 */
void *os_shmAt(int shmId, const void* pShmAddr, int shmFlg)
{
	return shmat(shmId, pShmAddr, shmFlg);
}



/* 
 * fn		int	os_shmDt(const void *pShmAddr)
 * brief	deattach the shared memory.
 * details	
 *
 * param [in]	pShmAddr - address of the shared memory.
 * param[out]	
 *
 * return	0 if success, -1 otherwise.		
 */
int	os_shmDt(const void *pShmAddr)
{
	return shmdt(pShmAddr);
}



/* 
 * fn		int os_shmDel(int shmId, void *pShmAddr)
 * brief	delete the shared memory.
 * details	In Linux, this routine delete the shared memory, if the attach number of the shared is 
 *			less than 2, otherwise just deattch the memory. In vxWorks, this routine free the shared 
 *			memory buffer, if the attach number of the shared is less than 2, otherwise juct deattch
 *			the memory.
 *
 * param [in]	shmId - for Linux.
 * param [in]	pShmAddr - shared memory addredd.
 * param[out]	
 *
 * return	0 if success, -1 otherwise.			
 */
int os_shmDel(int shmId, void *pShmAddr)
{
	struct shmid_ds shmbuf;

	/*
	* stat the shared memory to see how many processes are attached.
	*/
	memset(&shmbuf, 0, sizeof(shmbuf));
	if (shmctl(shmId, IPC_STAT, &shmbuf) < 0)
	{
		OS_DEBUG_PRINT("shmId = %d, shmAddr = 0x%p", shmId, pShmAddr);
		printf("shmctl IPC_STAT failed");
		return -1;
	}
	else
	{
		OS_DEBUG_PRINT("nattached=%d", (int)shmbuf.shm_nattch);
	}

	if (shmbuf.shm_nattch > 1 && pShmAddr != NULL)
	{
		/* other proceeses are still attached, just detach myself and return now. */
		if (shmdt(pShmAddr) != 0)
		{
			printf("shmdt of shmAddr=0x%p failed", pShmAddr);
		}
		else
		{
			OS_DEBUG_PRINT("detached shmAddr=0x%p", pShmAddr);
		}

		return 0;
	}

	if (shmdt(pShmAddr) != 0)
	{
		printf("shmdt of shmAddr=0x%p failed", pShmAddr);
	}
	else
	{
		OS_DEBUG_PRINT("detached shmAddr=0x%p", pShmAddr);
	}

	memset(&shmbuf, 0, sizeof(shmbuf));
	if (shmctl(shmId, IPC_RMID, &shmbuf) < 0)
	{
		printf("shm destory of shmId=%d failed.", shmId);
	}
	else
	{
		OS_DEBUG_PRINT("shared mem (shmId=%d) destroyed.", shmId);
	}

	return 0;
}



/* 
 * fn		    int os_inet_aton(const char * pString, struct in_addr * pInetAddr) 
 * brief	    convert a network address from dot notation, store in a structure
 * details	    
 *
 * param [in]	pString - string containing address, dot notation
 * param[out]	pInetAddr - struct in which to store address
 *
 * return	    0 if success, -1, otherwise.	    
 */
int os_inet_aton(const char * pString, struct in_addr * pInetAddr)
{
	if (!inet_aton(pString, pInetAddr))	
	{
		return -1;
	}
	else
	{
		return 0;
	}
}


/* 
 * fn		int os_taskCreate(char * pName,  
 *							  int priority, 
 *							  int stackSize, 
 *							  FUNCPTR pFunc, 
 *							  void * pArg)
 * brief	create a Linux process or spawn a vxWorks task
 * details	
 *
 * param [in]	pName - for vxWorks, name of new  task.
 * param [in]	priority - for vxWorks, priority of new task
 * param [in]	stackSize - for vxWorks, size (bytes) of stack needed plus name 
 * param [in]	pFunc - entry point of new task or thread.
 * param [in]	pArg - task args to pass to func 
 *
 * return	0 if success, -1 otherwise.		
 */
int os_taskCreate(char *pName,  
				  int priority, 
				  int stackSize, 
				  FUNCPTR pFunc, 
				  void * pArg)
{
	system(pName);
	
	return 0;
}

/* 
 * fn 		int os_getMacByIp(char *pIpAddr, char *pMacStr)
 * brief 	Get mac addr by ip in system ARP table.
 *
 * param [in] pIpAddr Internet address of target;
 * param[out] pMacAddr where to return the H/W address string("00:e0:ec:69:35:d4");
 *
 * return 0 if success, -1 if error.
 *
 * note		Only for IPv4
 */
int os_getMacByIp(char *pIpAddr, char *pMacStr)
{
#define OS_BUFLEN_256      256
#define OS_IP_STRING_LEN	16
#define OS_MAC_STRING_LEN	18

	FILE *fp = NULL;
	char buff[OS_BUFLEN_256] = {0};
	int mark = 0;
	char *p = NULL, *q = NULL;
	int len = 0;

	fp =  fopen(_ARP_TABLE_NAME, "r");
	if (NULL == fp)
	{
		perror("getMacByIp");
		
		memset(pMacStr, 0 , OS_MAC_STRING_LEN);
		printf("Open arp proc error when get %s ip's mac\n", pIpAddr);

		return -1;
	}
	
	/* append whitespace to the IP string, otherwise, if we search "192.168.1.1" in 
	 * a string which contains "192.168.1.1xx", it return FOUND but not correct.
	 */
	len = strlen(pIpAddr);
	if (len < OS_IP_STRING_LEN - 1)
	{
		pIpAddr[len] = ' ';
		pIpAddr[len+1] = 0;
	}

	while (fgets(buff, OS_BUFLEN_256, fp) != NULL)
	{
		if ((p = strstr(buff, pIpAddr)) != NULL)
		{
			q = p;
			while (++q && q < buff + OS_BUFLEN_256 - 1)
			{
				if (*q == ' ' && mark == 0)
				{
					p = q + 1;
				}
				
				if (*q == ':')
				{
					*q = '-';
					mark = 1;
				}
				
				if (*q == ' ' && mark == 1)
				{
					strncpy(pMacStr, p, OS_MAC_STRING_LEN - 1);
					pMacStr[OS_MAC_STRING_LEN - 1] = 0;

					/* Restoration pIpAddr */
					if (len < OS_IP_STRING_LEN - 1)
					{
						pIpAddr[len] = 0;
					}
					fclose(fp);
					
					return 0;
				}
			}
		}
	};

	/* Restoration pIpAddr */
	if (len < OS_IP_STRING_LEN - 1)
	{
		pIpAddr[len] = 0;
	}
	fclose(fp);

#undef OS_BUFLEN_256
#undef OS_IP_STRING_LEN
#undef OS_MAC_STRING_LEN

	return -1;
}

/* 
 * fn		int os_timerDelete(timer_t timer_id)
 * brief	delete a created timer
 *
 * param[in]	timer_id	timer id of timer to delete
 * param[out]	N/A
 *
 * return	result of delete
 * retval	0		delete OK
 *			other	delete ERROR
 *
 * note		
 */
int os_timerDelete(timer_t timer_id)
{
	int i;

	if (os_timerInit())
		return -1;

	for (i = 0; i < MAX_TIMER_NUM; i ++)
	{
		if (l_timers[i] == timer_id)
		{
			l_timers[i] = 0;
			return timer_delete(timer_id);
		}
	}

	printf("delete timer id %d failed: not found\n", (int)timer_id);
	return -1;
}

/* 
 * fn		timer_t os_timerDelayRun(sighandler func, int nDelaySeconds)
 * brief	just run a function after a period of time
 *
 * param[in]	func	function to run later]
 * param[in]	nDelaySeconds	time to wait before running the function (second)
 * param[out]	N/A
 *
 * return	result of create and start the timer
 * retval	timer id		all OK
 *			other	ERROR ocurred
 *
 * note		
 */
timer_t os_timerDelayRun(sighandler func, int nDelaySeconds)
{
	return os_timerPeriodRun(func, nDelaySeconds * 1000, 0);
}

/* 
 * fn		timer_t os_timerPeriodRun(sighandler func, int nDelayMilliseconds, int nIntervalMilliseconds)
 * brief	run a function repeatedly with appointed first wait and nIntervalMilliseconds
 *
 * param[in]	func		function to run repeatedly
 * param[in]	nDelayMilliseconds	time to wait before the first run (millisecond)
 * param[in]	nIntervalMilliseconds	nIntervalMilliseconds time between each running after the first run (millisecond)
 * param[out]	N/A
 *
 * return	result of create and start the timer
 * retval	timer id	all OK
 *			other	ERROR ocurred
 *
 * note		
 */
timer_t os_timerPeriodRun(sighandler func, int nDelayMilliseconds, int nIntervalMilliseconds)
{
	int sig;

	if (os_timerInit())
		return (timer_t)-1;

	for (sig = SIGNUM_DYNAMIC_MIN; sig < SIGNUM_MAX; sig++)
	{
		if (!l_timers[sig - SIGNUM_DYNAMIC_MIN])
		{
			if (os_timerCreate(sig, func, &l_timers[sig - SIGNUM_DYNAMIC_MIN]) < 0)
			{
				printf("Call tpCreateTimer failed\n");
				return (timer_t)-1;
			}
			
			return os_timerSet(l_timers[sig - SIGNUM_DYNAMIC_MIN], nDelayMilliseconds, nIntervalMilliseconds);
		}
	}

	return (timer_t)-1;
}


/* 
 * fn		int os_getSysUpTime(unsigned int *upTime)
 * brief	Get syetem up time	
 * param[out]	upTime - return system up time	
 *
 * return	0 means OK;-1 means error	
 */
int os_getSysUpTime(unsigned int *upTime)
{
	struct sysinfo info;

	if (-1 == sysinfo(&info))
	{
		perror("Get system up time error:");
		
		*upTime = 0;
		return -1;
	}
	
	/* NOTICE:info.uptime is "long" data type */
	*upTime = (unsigned int)info.uptime;

	return 0;
}


#endif  /* __LINUX_OS_FC__ */

