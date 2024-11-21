/* This is a generated file, don't edit */

#define NUM_APPLETS 64

const char applet_names[] ALIGN1 = ""
"[" "\0"
"[[" "\0"
"arping" "\0"
"ash" "\0"
"basename" "\0"
"blkid" "\0"
"brctl" "\0"
"cat" "\0"
"chmod" "\0"
"cp" "\0"
"cut" "\0"
"date" "\0"
"df" "\0"
"echo" "\0"
"egrep" "\0"
"env" "\0"
"fgrep" "\0"
"find" "\0"
"free" "\0"
"getty" "\0"
"grep" "\0"
"gzip" "\0"
"halt" "\0"
"head" "\0"
"ifconfig" "\0"
"init" "\0"
"insmod" "\0"
"ipcrm" "\0"
"ipcs" "\0"
"kill" "\0"
"killall" "\0"
"linuxrc" "\0"
"ln" "\0"
"logger" "\0"
"login" "\0"
"ls" "\0"
"lsmod" "\0"
"mkdir" "\0"
"mount" "\0"
"netstat" "\0"
"pidof" "\0"
"ping" "\0"
"ping6" "\0"
"poweroff" "\0"
"ps" "\0"
"reboot" "\0"
"rm" "\0"
"rmmod" "\0"
"route" "\0"
"sed" "\0"
"sh" "\0"
"sleep" "\0"
"tail" "\0"
"tar" "\0"
"taskset" "\0"
"telnet" "\0"
"telnetd" "\0"
"test" "\0"
"tftp" "\0"
"touch" "\0"
"tr" "\0"
"umount" "\0"
"uname" "\0"
"vconfig" "\0"
;

#define APPLET_NO_arping 2
#define APPLET_NO_ash 3
#define APPLET_NO_basename 4
#define APPLET_NO_blkid 5
#define APPLET_NO_brctl 6
#define APPLET_NO_cat 7
#define APPLET_NO_chmod 8
#define APPLET_NO_cp 9
#define APPLET_NO_cut 10
#define APPLET_NO_date 11
#define APPLET_NO_df 12
#define APPLET_NO_echo 13
#define APPLET_NO_egrep 14
#define APPLET_NO_env 15
#define APPLET_NO_fgrep 16
#define APPLET_NO_find 17
#define APPLET_NO_free 18
#define APPLET_NO_getty 19
#define APPLET_NO_grep 20
#define APPLET_NO_gzip 21
#define APPLET_NO_halt 22
#define APPLET_NO_head 23
#define APPLET_NO_ifconfig 24
#define APPLET_NO_init 25
#define APPLET_NO_insmod 26
#define APPLET_NO_ipcrm 27
#define APPLET_NO_ipcs 28
#define APPLET_NO_kill 29
#define APPLET_NO_killall 30
#define APPLET_NO_linuxrc 31
#define APPLET_NO_ln 32
#define APPLET_NO_logger 33
#define APPLET_NO_login 34
#define APPLET_NO_ls 35
#define APPLET_NO_lsmod 36
#define APPLET_NO_mkdir 37
#define APPLET_NO_mount 38
#define APPLET_NO_netstat 39
#define APPLET_NO_pidof 40
#define APPLET_NO_ping 41
#define APPLET_NO_ping6 42
#define APPLET_NO_poweroff 43
#define APPLET_NO_ps 44
#define APPLET_NO_reboot 45
#define APPLET_NO_rm 46
#define APPLET_NO_rmmod 47
#define APPLET_NO_route 48
#define APPLET_NO_sed 49
#define APPLET_NO_sh 50
#define APPLET_NO_sleep 51
#define APPLET_NO_tail 52
#define APPLET_NO_tar 53
#define APPLET_NO_taskset 54
#define APPLET_NO_telnet 55
#define APPLET_NO_telnetd 56
#define APPLET_NO_test 57
#define APPLET_NO_tftp 58
#define APPLET_NO_touch 59
#define APPLET_NO_tr 60
#define APPLET_NO_umount 61
#define APPLET_NO_uname 62
#define APPLET_NO_vconfig 63

#ifndef SKIP_applet_main
int (*const applet_main[])(int argc, char **argv) = {
test_main,
test_main,
arping_main,
ash_main,
basename_main,
blkid_main,
brctl_main,
cat_main,
chmod_main,
cp_main,
cut_main,
date_main,
df_main,
echo_main,
grep_main,
env_main,
grep_main,
find_main,
free_main,
getty_main,
grep_main,
gzip_main,
halt_main,
head_main,
ifconfig_main,
init_main,
insmod_main,
ipcrm_main,
ipcs_main,
kill_main,
kill_main,
init_main,
ln_main,
logger_main,
login_main,
ls_main,
lsmod_main,
mkdir_main,
mount_main,
netstat_main,
pidof_main,
ping_main,
ping6_main,
halt_main,
ps_main,
halt_main,
rm_main,
rmmod_main,
route_main,
sed_main,
ash_main,
sleep_main,
tail_main,
tar_main,
taskset_main,
telnet_main,
telnetd_main,
test_main,
tftp_main,
touch_main,
tr_main,
umount_main,
uname_main,
vconfig_main,
};
#endif

const uint16_t applet_nameofs[] ALIGN2 = {
0x0000,
0x0002,
0x0005,
0x000c,
0x0010,
0x0019,
0x001f,
0x0025,
0x0029,
0x002f,
0x0032,
0x0036,
0x003b,
0x003e,
0x0043,
0x0049,
0x004d,
0x0053,
0x0058,
0x005d,
0x0063,
0x0068,
0x006d,
0x0072,
0x0077,
0x0080,
0x0085,
0x008c,
0x0092,
0x0097,
0x009c,
0x00a4,
0x00ac,
0x00af,
0x00b6,
0x00bc,
0x00bf,
0x00c5,
0x00cb,
0x00d1,
0x00d9,
0x00df,
0x00e4,
0x00ea,
0x00f3,
0x00f6,
0x00fd,
0x0100,
0x0106,
0x010c,
0x0110,
0x0113,
0x0119,
0x011e,
0x0122,
0x012a,
0x0131,
0x0139,
0x013e,
0x0143,
0x0149,
0x014c,
0x0153,
0x0159,
};

