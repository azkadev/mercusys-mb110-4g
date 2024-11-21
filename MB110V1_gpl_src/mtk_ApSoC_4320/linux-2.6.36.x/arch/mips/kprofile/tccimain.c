/************************************************************************
 *
 *	Copyright (C) 2006 Trendchip Technologies, Corp.
 *	All Rights Reserved.
 *
 * Trendchip Confidential; Need to Know only.
 * Protected as an unpublished work.
 *
 * The computer program listings, specifications and documentation
 * herein are the property of Trendchip Technologies, Co. and shall
 * not be reproduced, copied, disclosed, or used in whole or in part
 * for any reason without the prior express written permission of
 * Trendchip Technologeis, Co.
 *
 *************************************************************************/
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <asm/addrspace.h>
//#include <asm/tc3162/tc3162.h>
/*remove nouse include file
#include <asm/tc3162/TCIfSetQuery_os.h>*/
//#include <asm/tc3162/ledcetrl.h>
//--#include <asm/tc3162/cmdparse.h>
#include "cmdparse.h"
//--#include "../../../version/tcversion.h"

#ifdef TCSUPPORT_DOWNSTREAM_QOS
#include <linux/netdevice.h>
extern unsigned short int voip_rx_port[VOIP_RX_PORT_NUM];
extern char downstream_qos_enable;
#endif


#ifdef CMD_API
#include "tcci.h"
#endif
#define MAX_ROOT_CMD	16

#define NARG		40

#define FALSE		0
#define TRUE		1

int cmd_reg_add(char *cmd_name, cmds_t *cmds_p);
int cmd_register(cmds_t *cmds_p);
int subcmd(const cmds_t tab[], int argc, char *argv[], void *p);
#ifdef CMD_API
int cosubcmd(const cmds_t tab[], int argc, char *argv[], void *p);
extern char adslinfobuf[ADSLBUFINFO];
#endif

//--extern int tcadslcmd_init(void);
//--extern void tcadslcmd_exit(void);
extern int tcsyscmd_init(void);
extern void tcsyscmd_exit(void);

/* Command lookup and branch tables */
cmds_t Cmds[MAX_ROOT_CMD];

int cmd_reg_add(char *cmd_name, cmds_t *cmds_p)
{
	return 0;
}

/*_____________________________________________________________________________
**      function name: cmd_register
**      descriptions:
**         Register a new ci-cmd into top ci-cmd tables.
**
**      parameters:
**             cmds_p: Specify you want to register ci-cmd
**
**      global:
**             Cmds
**
**      return:
**              Success:        0
**              Otherwise:     -1
**
**      call:
**   	None
**
**      revision:
**      1. Here 2008/8/13
**____________________________________________________________________________
*/
int
cmd_register(cmds_t *cmds_p)
{
	unsigned int i=0;
	for(i=0; i < MAX_ROOT_CMD; i++){
		if(Cmds[i].name == NULL){
			memcpy(&Cmds[i], cmds_p, sizeof(cmds_t));
			return 0;
		}
	}

	printk("Can't register ci-cmd, Cmds is full\n");
	return -1;
}/*end cmd_register*/

/*_____________________________________________________________________________
**      function name: cmd_unregister
**      descriptions:
**         Remove a new ci-cmd in the ci-cmd tables.
**
**      parameters:
**             name: command's name
**
**      global:
**             Cmds
**
**      return:
**              Success:        0
**              Otherwise:     -1
**
**      call:
**   	None
**
**      revision:
**      1. JasonLin 2008/9/23
**____________________________________________________________________________
*/
int
cmd_unregister(char *name)
{
    int i;
	for(i=0; i < MAX_ROOT_CMD; i++){
		if(strcmp(Cmds[i].name, name) == 0){
			memset(&Cmds[i], 0, sizeof(cmds_t));
			return 0;
		}
	}

	printk("Can't find %s ci-cmd\n", name);
	return -1;
}/*end cmd_unregister*/

/* Call a subcommand based on the first token in an already-parsed line */
int subcmd	(
	const cmds_t tab[], int argc, char *argv[], void *p
)
{
	register const cmds_t *cmdp;
	int found = 0;
	int i;

	/* Strip off first token and pass rest of line to subcommand */
	if (argc < 2) {
		if (argc < 1)
			printk("SUBCMD - Don't know what to do?\n");
		else
			goto print_out_cmds;
			//printk("\"%s\" - takes at least one argument\n",argv[0]);
		return -1;
	}
	argc--;
	argv++;
	for(cmdp = tab;cmdp->name != NULL;cmdp++){
		if(strncmp(argv[0],cmdp->name,strlen(argv[0])) == 0){
			found = 1;
			break;
		}
	}
	if(!found){
		char buf[66];

print_out_cmds:
		printk("valid subcommands:\n");
		memset(buf,' ',sizeof(buf));
		buf[64] = '\n';
		buf[65] = '\0';
		for(i=0,cmdp = tab;cmdp->name != NULL;cmdp++){
			strncpy(&buf[i*16],cmdp->name,strlen(cmdp->name));
			if(i == 3){
				printk(buf);
				memset(buf,' ',sizeof(buf));
				buf[64] = '\n';
				buf[65] = '\0';
			}
			i = (i+1)%4;
		}
		if(i != 0)
			printk(buf);
		return -1;
	}
	if(argc <= cmdp->argcmin){
		if(cmdp->argc_errmsg != NULL)
			printk("Usage: %s\n",cmdp->argc_errmsg);
		return -1;
	}
	if(cmdp->func == NULL)  /*Ryan_20091103*/
		return 0;
	return (*cmdp->func)(argc,argv,p);
}
#ifdef CMD_API
int cosubcmd  (
    const cmds_t tab[], int argc, char *argv[], void *p
)
{
	register const cmds_t *cmdp;
	int found = 0;
	int i, ret = 0;

	/* Strip off first token and pass rest of line to subcommand */
	if (argc < 2) {
		if (argc < 1)
			printk("SUBCMD - Don't know what to do?\n");
		else{
			char buf[66];

			printk("valid subcommands of %s:\n", argv[0]);
			memset(buf,' ',sizeof(buf));
			buf[64] = '\n';
			buf[65] = '\0';
			for(i=0,cmdp = tab;cmdp->name != NULL;cmdp++){
				strncpy(&buf[i*16],cmdp->name,strlen(cmdp->name));
				if(i == 3){
					printk(buf);
					memset(buf,' ',sizeof(buf));
					buf[64] = '\n';
					buf[65] = '\0';
				}
				i = (i+1)%4;
			}
			if(i != 0)
				printk(buf);
		}
		return -1;
	}
	for(i = 1; i < argc; i++){
		found = 0;
		for(cmdp = tab; cmdp->name != NULL; cmdp++){
			if(strncmp(argv[i],cmdp->name,strlen(argv[i])) == 0){
				found = 1;
				if((*cmdp->func)(argc-i,argv+i,p))
					ret = -1;
				break;
			}
		}
		if(!found){
			if(argv[i-1][0] != '-'){
				printk("invalid subcommand \"%s\" of command \"%s\"\r\n", argv[i], argv[0]);
				ret = -1;
			}
			else
				ret = 0;
		}
		if(ret)
			break;
	}
	return ret;
}
#endif

static char *
stringparse(char *line)
{
	register char *cp = line;
	unsigned long num;
	register char *cp_tmp;


	while ( *line != '\0' && *line != '\"' ) {
		if ( *line == '\\' ) {
			line++;
			switch ( *line++ ) {
			case 'n':
				*cp++ = '\n';
				break;
			case 't':
				*cp++ = '\t';
				break;
			case 'v':
				*cp++ = '\v';
				break;
			case 'b':
				*cp++ = '\b';
				break;
			case 'r':
				*cp++ = '\r';
				break;
			case 'f':
				*cp++ = '\f';
				break;
			case 'a':
				*cp++ = '\a';
				break;
			case '\\':
				*cp++ = '\\';
				break;
			case '\?':
				*cp++ = '\?';
				break;
			case '\'':
				*cp++ = '\'';
				break;
			case '\"':
				*cp++ = '\"';
				break;
			case 'x':
				cp_tmp = --line;
				num = simple_strtoul( cp_tmp, &line, 16 );
				*cp++ = (char) num;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				cp_tmp = --line;
				num = simple_strtoul( cp_tmp, &line, 8 );
				*cp++ = (char) num;
				break;
			case '\0':
				return NULL;
			default:
				*cp++ = *(line - 1);
				break;
			};
		} else {
			*cp++ = *line++;
		}
	}

	if ( *line == '\"' )
		line++; 	/* skip final quote */
	*cp = '\0';		/* terminate string */
	return line;
}

/* replace terminating end of line marker(s) with null */
void
rip(register char *s)
{
	register char *cp;

	if((cp = strchr(s,'\n')) != NULL)
		*cp = '\0';
	if((cp = strchr(s,'\r')) != NULL)
		*cp = '\0';
}

int  /*Rodney_20091112*/
cmdparse(
	const cmds_t *cmds, char *line, void *p
)
{
	//const cmds_t *cmds;
	const cmds_t *cmdp;
	char *argv[NARG],*cp;
	int argc;

	cmds = Cmds;
	/* Remove cr/lf */
	rip(line);

	for(argc = 0;argc < NARG;argc++)
		argv[argc] = NULL;

	for(argc = 0;argc < NARG;){
		register int qflag = FALSE;

		/* Skip leading white space */
		while(*line == ' ' || *line == '\t')
			line++;
		if(*line == '\0')
			break;
		/* return if comment character first non-white */
		if ( argc == 0  &&  *line == '#' )
			return 0;
		/* Check for quoted token */
		if(*line == '"'){
			line++;	/* Suppress quote */
			qflag = TRUE;
		}
		argv[argc++] = line;	/* Beginning of token */

		if(qflag){
			/* Find terminating delimiter */
			if((line = stringparse(line)) == NULL){
				return -1;
			}
		} else {
			/* Find space or tab. If not present,
			 * then we've already found the last
			 * token.
			 */
			if((cp = strchr(line,' ')) == NULL
			 && (cp = strchr(line,'\t')) == NULL){
				break;
			}
			*cp++ = '\0';
			line = cp;
		}
	}
	if (argc < 1) {		/* empty command line */
		argc = 1;
		argv[0] = "";
	}
	/* Look up command in table; prefix matches are OK */
	for(cmdp = cmds;cmdp->name != NULL;cmdp++){
		if(strncmp(argv[0],cmdp->name,strlen(argv[0])) == 0) {
			break;
		}
	}
	if(cmdp->name == NULL) {
		if(cmdp->argc_errmsg != NULL)
			printk("%s\n",cmdp->argc_errmsg);
		return -1;
	}
	if(argc < cmdp->argcmin) {
		/* Insufficient arguments */
		printk("Usage: %s\n",cmdp->argc_errmsg);
		return -1;
	}
	if(cmdp->func == NULL)
		return 0;
	return (*cmdp->func)(argc,argv,p);
}

#if 0
int
cmdparse(
	const cmds_t cmds[], char *line, void *p
)
{
	const cmds_t *cmdp;
	char *argv[NARG],*cp;
	int argc;

	/* Remove cr/lf */
	rip(line);

	for(argc = 0;argc < NARG;argc++)
		argv[argc] = NULL;

	for(argc = 0;argc < NARG;){
		register int qflag = FALSE;

		/* Skip leading white space */
		while(*line == ' ' || *line == '\t')
			line++;
		if(*line == '\0')
			break;
		/* return if comment character first non-white */
		if ( argc == 0  &&  *line == '#' )
			return 0;
		/* Check for quoted token */
		if(*line == '"'){
			line++;	/* Suppress quote */
			qflag = TRUE;
		}
		argv[argc++] = line;	/* Beginning of token */

		if(qflag){
			/* Find terminating delimiter */
			if((line = stringparse(line)) == NULL){
				return -1;
			}
		} else {
			/* Find space or tab. If not present,
			 * then we've already found the last
			 * token.
			 */
			if((cp = strchr(line,' ')) == NULL
			 && (cp = strchr(line,'\t')) == NULL){
				break;
			}
			*cp++ = '\0';
			line = cp;
		}
	}
	if (argc < 1) {		/* empty command line */
		argc = 1;
		argv[0] = "";
	}
	/* Look up command in table; prefix matches are OK */
	for(cmdp = cmds;cmdp->name != NULL;cmdp++){
		if(strncmp(argv[0],cmdp->name,strlen(argv[0])) == 0) {
			break;
		}
	}
	if(cmdp->name == NULL) {
		if(cmdp->argc_errmsg != NULL)
			printk("%s\n",cmdp->argc_errmsg);
		return -1;
	}
	if(argc < cmdp->argcmin) {
		/* Insufficient arguments */
		printk("Usage: %s\n",cmdp->argc_errmsg);
		return -1;
	}
	if(cmdp->func == NULL)
		return 0;
	return (*cmdp->func)(argc,argv,p);
}
#endif

static char user_space_flag[8]="0\n";
void setUserSpaceFlag(int num)
{
    sprintf(user_space_flag, "%d\n", num);
}

static int tcci_cmd_read_proc(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len;

	len = sprintf(page, "%s", user_space_flag);

	len -= off;
	*start = page + off;

	if (len > count)
		len = count;
	else
		*eof = 1;

	if (len < 0)
		len = 0;

    sprintf(user_space_flag, "0\n");

	return len;
}

DEFINE_MUTEX(tcci_mutex);

static int tcci_cmd_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[160];

	mutex_lock(&tcci_mutex);
	if (count > sizeof(val_string) - 1) {
		mutex_unlock(&tcci_mutex);
		return -EINVAL;
	}

	if (copy_from_user(val_string, buffer, count)) {
		mutex_unlock(&tcci_mutex);
		return -EFAULT;
	}

	val_string[count] = '\0';
	//	cmdparse(Cmds, val_string, NULL);
	// cmdparse(val_string, NULL);
	#ifdef CMD_API
	memset(adslinfobuf,0,sizeof(adslinfobuf));
	#endif
	cmdparse(NULL, val_string, NULL);  /*Rodney_20091112*/

	mutex_unlock(&tcci_mutex);

	return count;
}

DEFINE_MUTEX(dbg_msg_mutex);
static int dbg_msg_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data)
{
	char val_string[128];

	mutex_lock(&dbg_msg_mutex);
	if (count > sizeof(val_string) - 1) {
		mutex_unlock(&dbg_msg_mutex);
		return -EINVAL;
	}

	if (copy_from_user(val_string, buffer, count)) {
		mutex_unlock(&dbg_msg_mutex);
		return -EFAULT;
	}

	val_string[count] = '\0';
	//	cmdparse(Cmds, val_string, NULL);
	printk(val_string);
//	cmdparse(val_string, NULL);

	mutex_unlock(&dbg_msg_mutex);

	return count;
}
#ifdef TCSUPPORT_DOWNSTREAM_QOS
static int prio_enable_write_proc(struct file *file, const char *buffer,
	unsigned long count, void *data){
	int val;
	char val_string[8];

	if (count > sizeof(val_string) - 1)
		return -EINVAL;
	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;
	val_string[count] = '\0';
	val = simple_strtoul(val_string, NULL, 10);
	if (val == 0){
		//printk("Disable Downstream Qos\n");
		downstream_qos_enable = 0;
	}
	else
	{	
		//printk("Enable Downstream Qos\n");
		downstream_qos_enable = 1;
		
	}	
	return count;
		
}
static int prio_enable_read_proc(char *buf, char **start, off_t off, int count,
    int *eof, void *data){

	int len;

	len = sprintf( buf, "%d\n", downstream_qos_enable);

	len -= off;
	*start = buf + off;

	if (len > count)
		len = count;
	else
		*eof = 1;

	if (len < 0)
		len = 0;
	
return len;

#if 0
	if(downstream_qos_enable == 0){
		printk("Downstream Qos is Disabled\n");
	}else if(downstream_qos_enable == 1){
		printk("Downstream Qos is Enabled\n");
	}	

	return 0;
#endif	
}	

static int voip_port_write_proc(struct file *file, const char *buffer,
    unsigned long count, void *data){
	int i;
	char val_string[32];
	unsigned int tmp[4];

	if (count > sizeof(val_string) - 1)
		return -EINVAL;

	if (copy_from_user(val_string, buffer, count))
		return -EFAULT;

	val_string[count] = '\0';

	if (sscanf(val_string, "%lu %lu %lu %lu", &tmp[0], &tmp[1], &tmp[2], &tmp[3]) < VOIP_RX_PORT_NUM) {
		printk("usage: <rxport0> <rxport1> <rxport2> <rxport3> \n");
		return count;
	}
	for(i=0;i<VOIP_RX_PORT_NUM;i++){
		if(tmp[i] > 65536){
			printk("port value exceed 65535\n");
			return count;
		}
	}

	printk("Support port number : %d\nvalue ",VOIP_RX_PORT_NUM);
	for(i=0;i<VOIP_RX_PORT_NUM;i++){
		voip_rx_port[i] = (unsigned short int)tmp[i];
		printk("%u ",voip_rx_port[i]);
	}
	printk("\n");
	return count;

}
static int voip_port_read_proc(char *buf, char **start, off_t off, int count,
    int *eof, void *data){
	int i;

	printk("Support port number : %d\nvalue ",VOIP_RX_PORT_NUM);
	for(i=0;i<VOIP_RX_PORT_NUM;i++){
		printk("%u ",voip_rx_port[i]);
	}
	printk("\n");

	return 0;
}
#endif

static int __init tccicmd_init(void)
{
	struct proc_dir_entry *procRegDir=NULL;
	struct proc_dir_entry *tcci_cmd_proc=NULL;
	struct proc_dir_entry *dbg_msg_proc=NULL;
	//printk("TC3162 CLI Command 0.1\n");
	//--printk("%s\n",MODULE_VERSION_TCCICMD);
	/*Init cmd*/
	memset(Cmds, 0, (sizeof(cmds_t)*MAX_ROOT_CMD));

	if (procRegDir == NULL)
		procRegDir = proc_mkdir("kprofile", NULL);

	/* adsl commands */
	if((tcci_cmd_proc = create_proc_entry("tcci_cmd", 0, procRegDir))) {
		tcci_cmd_proc->read_proc = tcci_cmd_read_proc;
		tcci_cmd_proc->write_proc = tcci_cmd_write_proc;
	}

	/* debug message */
	if((dbg_msg_proc = create_proc_entry("dbg_msg", 0, procRegDir))) {
		dbg_msg_proc->read_proc = tcci_cmd_read_proc;
		dbg_msg_proc->write_proc = dbg_msg_write_proc;
	}

#ifdef TCSUPPORT_DOWNSTREAM_QOS
	/*Prio Enable Switch. shnwind 20110418.*/
	if((tcci_cmd_proc = create_proc_entry("Prio_Enable", 0, procRegDir))){
		tcci_cmd_proc->read_proc = prio_enable_read_proc;
		tcci_cmd_proc->write_proc = prio_enable_write_proc;
	}
	/*Write Voip rx port for rtp.*/
	if((tcci_cmd_proc = create_proc_entry("Prio_Voip_Rxport", 0, procRegDir))) {
		tcci_cmd_proc->read_proc = voip_port_read_proc;
		tcci_cmd_proc->write_proc = voip_port_write_proc;
	}
#endif

	//--tcadslcmd_init();
	tcsyscmd_init();
	return 0;
}

static void __exit tccicmd_exit(void)
{
	printk("TC3162 CLI Command 0.1 exit!\n");

	remove_proc_entry("tc3162/tcci_cmd", NULL);
	remove_proc_entry("tc3162/dbg_msg", NULL);
#ifdef TCSUPPORT_DOWNSTREAM_QOS
	remove_proc_entry("tc3162/Prio_Enable", NULL);
	remove_proc_entry("tc3162/Prio_Voip_Rxport", NULL);
#endif	

	//--tcadslcmd_exit();
	tcsyscmd_exit();
}

EXPORT_SYMBOL(cmdparse);
EXPORT_SYMBOL(subcmd);
EXPORT_SYMBOL(Cmds);
EXPORT_SYMBOL(cmd_reg_add);
EXPORT_SYMBOL(cmd_register);
EXPORT_SYMBOL(cmd_unregister);
EXPORT_SYMBOL(setUserSpaceFlag);

module_init (tccicmd_init);
module_exit (tccicmd_exit);
