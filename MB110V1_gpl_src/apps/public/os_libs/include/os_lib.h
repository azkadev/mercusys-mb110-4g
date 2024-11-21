/*  Copyright(c) 2009-2011 Shenzhen TP-LINK Technologies Co.Ltd.
 *
 * file		os_lib.h
 * brief	This file is for different OS
 * details	
 *
 * author	Wu Zhiqin
 * version	1.0.0
 * date		04May11
 *
 *
 * history \arg	1.0.0, 04May11, Wu Zhiqin    Create.
 */

#ifndef __OS_LIB_H__
#define __OS_LIB_H__

/* #include */
#if defined(__LINUX_OS_FC__)
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>

typedef pthread_t       OS_THREAD;
typedef sem_t           OS_SEM;
typedef int				OS_V_SEM;
typedef int             OS_MSGQ;
typedef pthread_mutex_t OS_MUTEX;
#define OS_NAME_LEN     16
typedef struct
{
	void *arg;
	char name[OS_NAME_LEN];
	int  priority;
	sem_t sem; /* used by thread to notify the caller that it will not use the argument any more */
} OS_PARAMS;

#define PTHREAD_SCHED_FIFO       (1 << 31)
#define PTHREAD_SCHED_RR         (1 << 30)
#define PTHREAD_PRIO_HIGH        60
#define PTHREAD_PRIO_MED         50
#define PTHREAD_PRIO_LOW         40
#define PTHREAD_PRIO_OTHER       0

#define WAIT_FOREVER    -1
#define NO_WAIT         0

#define MSG_PRI_NORMAL	0
#define MSG_PRI_URGENT	1

#define	SIGNUM_MIN		SIGRTMIN
#define	SIGNUM_ADVSEC	(SIGNUM_MIN + 0)
#define SIGNUM_SYSLOG	(SIGNUM_MIN + 1)
#define SIGNUM_RESTORE	(SIGNUM_MIN + 2)

#define SIGNUM_DYNAMIC_MIN (SIGNUM_MIN + 8)		// For tpPeriodRun

#define	SIGNUM_MAX		SIGRTMAX

typedef void (* sighandler)();

typedef void  * (*FUNCPTR)(void *);

char * strptime(const char *pBuf, const char *pFrmt, struct tm *pTime);

#elif defined(__VXWORKS_OS_FC__)
#include <vxWorks.h>
#include <taskLib.h>
#include <msgQLib.h>
#include <semLib.h>
#include <semSmLib.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <inetLib.h>
#include <errno.h>
#include <arpLib.h>

typedef int             OS_THREAD;
typedef SEM_ID          OS_SEM;
typedef SEM_ID			OS_V_SEM;
typedef MSG_Q_ID        OS_MSGQ;
typedef SEM_ID          OS_MUTEX;

/* typedef unsigned int    socklen_t; */


#define	OS_MEM_HEADER_LENGTH			4

#ifndef DEFAULT_THREAD_PRIORITY_K
#define DEFAULT_THREAD_PRIORITY_K       101
#endif

#ifndef DEFAULT_THREAD_STACK_SIZE_K
#define DEFAULT_THREAD_STACK_SIZE_K     5000
#endif

#define MAX_MSG                         30

/* 
 * brief Test for read permission. 
 */
#define	R_OK							4

/* 
 * brief Test for write permission. 
 */
#define	W_OK							2

/* 
 * brief Test for execute permission. 
 */
#define	X_OK							1

/* 
 * brief Test for existence.
 */
#define	F_OK							0

/* 
 * brief for IPC key
 */
#define IPC_PRIVATE	0

/* 
 * brief create if key is nonexistent 	
 */
#define IPC_CREAT						00001000 

/* 
 * brief fail if key exists 
 */
#define IPC_EXCL						00002000 

/* 
 * fn		int strcasecmp(const char *s1, const char *s2)
 * brief	如果库里没有strcasecmp则使用自定义的strcasecmp.	
 *
 * param[in]	s1			字符串1
 * param[in]	s2			字符串2
 *
 * return		int	
 * retval		0  	两字符串相同(不区分大小写)
 *				<0	字符串1 < 字符串2
 *				>0  字符串1 > 字符串2
 * note		
 */
int strcasecmp(const char *s1, const char *s2);

int strncasecmp(const char *s1, const char *s2, int n);

int access(char *pName, int mode);

#endif

char * strptime(const char *pBuf, const char *pFrmt, struct tm *pTime);


#if 0
#define OS_DEBUG_PRINT(_fmt, arg...) printf(_fmt, ##arg)
#else
#define OS_DEBUG_PRINT(_fmt, arg...)
#endif

/* Typedefs common to all ports */

typedef enum _OS_SEM_OPTIONS
{
    SEM_Q_FIFO_K = 0,
    SEM_Q_PRIORITY_K
} OS_SEM_OPTIONS;

typedef enum _OS_MSG_OPTIONS
{
    MSG_Q_FIFO_K = 0,
    MSG_Q_PRIORITY_K = 1,
    MSG_Q_EVENTSEND_ERR_NOTIFY_K
}OS_MSG_OPTIONS;

#ifdef __cplusplus
extern "C" {
#endif


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
int os_semCreate(int initialCount, OS_SEM_OPTIONS opt, OS_SEM *pSem);



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
int os_semDestroy  (OS_SEM *pSem);



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
int os_semTake(OS_SEM *pSem, unsigned int timeout);



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
int os_semGive(OS_SEM *pSem);



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
int os_semVCreate(unsigned int key, int initialValue, OS_V_SEM *pSem);



/* 
 * fn		    int os_semVGet(int key, OS_V_SEM *pSem) 
 * brief	    Get a existed system v semaphore referenced by key.
 * details	    
 *
 * param [in]	key	 - a none zero(IPC_PRIVATE) key is associated with a unique 
 *					   semaphore set.
 * param[out]	pSem - to keep the system v semaphore ID.
 *
 * return	    0 if success, -1 if failed	    
 */
int os_semVGet(unsigned int key, OS_V_SEM *pSem);



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
int os_semVDestory(OS_V_SEM sem);



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
int os_semVTake(OS_V_SEM sem);



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
int os_semVGive(OS_V_SEM sem);


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
					OS_THREAD * pTid);

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
					OS_THREAD * pTid);


/*
 * fn		void os_threadSetNamePrioFinal(OS_PARAMS *pArg)
 * brief	set name and priority for thread,and notify the caller to continue
 *
 * param[in]	pArg : inlcudes the thread name,priority or other arguments the thread required
 *
 * note		the thread MUST NOT use the argument after 100ms or this function has been called
 */
void os_threadSetNamePrioFinal(OS_PARAMS *pArg);

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
int os_threadExit(OS_THREAD tid);



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
int os_threadDelete(OS_THREAD tid);



/* 
 * fn		    OS_THREAD os_getTid() 
 * brief	    get current thread's ID
 * details	    
 *
 * return	    current thread's ID	    
 */
OS_THREAD os_getTid();



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
int os_queueCreate  (int maxMsg, int msgSize, OS_MSG_OPTIONS opt, OS_MSGQ *pQid);



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
int os_queueDelete(OS_MSGQ qid);



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
int os_queueReceive (OS_MSGQ qid, long type, int timeout, int bufsize, char *pBuf);



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
int os_queueSend (OS_MSGQ qid, long type, int timeout, int priority, char *pBuf);



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
int os_mutexCreate (OS_MUTEX *pMutex);



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
int os_mutexDestroy (OS_MUTEX *pMutex);



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
int os_mutexLock(OS_MUTEX *pMutex);



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
int os_mutexUnlock(OS_MUTEX *pMutex);



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
int os_shmGet(int key, size_t size, int shmFlg);



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
void *os_shmAt(int shmId, const void* pShmAddr, int shmFlg);



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
int	os_shmDt(const void *pShmAddr);



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
int os_shmDel(int shmId, void *pShmAddr);



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
int os_inet_aton(const char * pString, struct in_addr * pInetAddr);



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
				  void * pArg);


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
int os_getMacByIp(char *pIpAddr, char *pMacStr);


#if defined(__LINUX_OS_FC__)

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
int		os_timerDelete(timer_t timer_id);

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
timer_t		os_timerDelayRun(sighandler func, int nDelaySeconds);

/* 
 * fn		timer_t os_timerPeriodRun(sighandler func, int nDelayMilliseconds, int nIntervalMilliseconds)
 * brief	run a function repeatedly with appointed first wait and interval 
 *
 * param[in]	func		function to run repeatedly
 * param[in]	nDelayMilliseconds		time to wait before the first run (millisecond)
 * param[in]	nIntervalMilliseconds	interval time between each running after the first run (millisecond)
 * param[out]	N/A
 *
 * return	result of create and start the timer
 * retval	timer id		all OK
 *			other	ERROR ocurred
 *
 * note		
 */
timer_t		os_timerPeriodRun(sighandler func, int nDelayMilliseconds, int nIntervalMilliseconds);

#endif	/* __LINUX_OS_FC__ */

/* 
 * fn		int os_getSysUpTime(unsgined int *upTime)
 * brief	Get syetem up time	
 * param[out]	upTime - return system up time	
 *
 * return	0 means OK;-1 means error	
 */
int os_getSysUpTime(unsigned int *upTime);


#ifdef __cplusplus
}
#endif

#endif /* __OS_LIB_H__ */
