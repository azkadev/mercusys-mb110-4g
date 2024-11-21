#ifndef __KPROFILE_HOOK_H
#define __KPROFILE_HOOK_H

extern void (*profilingSetupHook)(unsigned int tbl_size);
extern void (*profilingEventHook)(unsigned int cntr, unsigned int event, unsigned int count, 
						unsigned int kernel, unsigned int user, unsigned int exl);
extern void (*profilingStartHook)(void);
extern void (*profilingStopHook)(void);
extern void (*profilingLogHook)(unsigned int label, unsigned int usr_data);

static inline void kprofileSetup(unsigned int tbl_size)
{
	if (profilingSetupHook)
		profilingSetupHook(tbl_size);
}

static inline void kprofileEvent(unsigned int cntr, unsigned int event, unsigned int count, 
						unsigned int kernel, unsigned int user, unsigned int exl)
{
	if (profilingEventHook)
		profilingEventHook(cntr, event, count, kernel, user, exl);
}

static inline void kprofileStart(void)
{
	if (profilingStartHook)
		profilingStartHook();
}

static inline void kprofileStop(void)
{
	if (profilingStopHook)
		profilingStopHook();
}

static inline void kprofileLog(unsigned int label, unsigned int usr_data)
{
	if (profilingLogHook)
		profilingLogHook(label, usr_data);
}

#endif
