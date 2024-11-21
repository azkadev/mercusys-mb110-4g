#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>

void (*profilingSetupHook)(unsigned int tbl_size) = NULL;
void (*profilingEventHook)(unsigned int cntr, unsigned int event, unsigned int count, 
						unsigned int kernel, unsigned int user, unsigned int exl) = NULL;
void (*profilingStartHook)(void) = NULL;
void (*profilingStopHook)(void) = NULL;
void (*profilingLogHook)(unsigned int label, unsigned int usr_data) = NULL;

EXPORT_SYMBOL(profilingSetupHook);
EXPORT_SYMBOL(profilingEventHook);
EXPORT_SYMBOL(profilingStartHook);
EXPORT_SYMBOL(profilingStopHook);
EXPORT_SYMBOL(profilingLogHook);

