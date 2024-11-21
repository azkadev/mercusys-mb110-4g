/* Automatically generated file. Do not edit. Edit wrapawk/wrapfunc.inp. */
#ifndef WRAPDEF_H
#define WRAPDEF_H
int (*next_open)(const char *pathname, int flags, int mode)=tmp_open;
FILE * (*next_fopen)(const char *pathname, const char *mode)=tmp_fopen;
FILE * (*next_freopen)(const char *pathname, const char *mode, FILE *stream)=tmp_freopen;


int (*NEXT_LSTAT_NOARG)LSTAT_ARG(int ver, const char *file_name, struct stat *buf)=TMP_LSTAT;
int (*NEXT_STAT_NOARG)STAT_ARG(int ver, const char *file_name, struct stat *buf)=TMP_STAT;
int (*NEXT_FSTAT_NOARG)FSTAT_ARG(int ver, int fd, struct stat *buf)=TMP_FSTAT;
#ifdef HAVE_FSTATAT
int (*NEXT_FSTATAT_NOARG)FSTATAT_ARG(int ver, int dir_fd, const char *path, struct stat *buf, int flags)=TMP_FSTATAT;
#endif /* HAVE_FSTATAT */

#ifdef STAT64_SUPPORT
int (*NEXT_LSTAT64_NOARG)LSTAT64_ARG(int ver, const char *file_name, struct stat64 *buf)=TMP_LSTAT64;
int (*NEXT_STAT64_NOARG)STAT64_ARG(int ver, const char *file_name, struct stat64 *buf)=TMP_STAT64;
int (*NEXT_FSTAT64_NOARG)FSTAT64_ARG(int ver, int fd, struct stat64 *buf)=TMP_FSTAT64;
#ifdef HAVE_FSTATAT
int (*NEXT_FSTATAT64_NOARG)FSTATAT64_ARG(int ver, int dir_fd, const char *path, struct stat64 *buf, int flags)=TMP_FSTATAT64;
#endif /* HAVE_FSTATAT */
#endif /* STAT64_SUPPORT */

int (*NEXT_MKNOD_NOARG)MKNOD_ARG(int ver, const char *pathname, mode_t mode, dev_t XMKNOD_FRTH_ARG dev)=TMP_MKNOD;

#ifdef HAVE_FSTATAT
#ifdef HAVE_MKNODAT
int (*NEXT_MKNODAT_NOARG)MKNODAT_ARG(int ver, int dir_fd, const char *pathname, mode_t mode, dev_t dev)=TMP_MKNODAT;
#endif /* HAVE_MKNODAT */
#endif /* HAVE_FSTATAT */


int (*next_chown)(const char *path, uid_t owner, gid_t group)=tmp_chown;
int (*next_lchown)(const char *path, uid_t owner, gid_t group)=tmp_lchown;
int (*next_fchown)(int fd, uid_t owner, gid_t group)=tmp_fchown;
int (*next_chmod)(const char *path, mode_t mode)=tmp_chmod;
int (*next_fchmod)(int fd, mode_t mode)=tmp_fchmod;
int (*next_mkdir)(const char *path, mode_t mode)=tmp_mkdir;
int (*next_unlink)(const char *pathname)=tmp_unlink;
int (*next_rmdir)(const char *pathname)=tmp_rmdir;
int (*next_remove)(const char *pathname)=tmp_remove;
int (*next_rename)(const char *oldpath, const char *newpath)=tmp_rename;

#ifdef FAKEROOT_FAKENET
pid_t (*next_fork)(void)=tmp_fork;
pid_t (*next_vfork)(void)=tmp_vfork;
int (*next_close)(int fd)=tmp_close;
int (*next_dup2)(int oldfd, int newfd)=tmp_dup2;
#endif /* FAKEROOT_FAKENET */


uid_t (*next_getuid)(void)=tmp_getuid;
gid_t (*next_getgid)(void)=tmp_getgid;
uid_t (*next_geteuid)(void)=tmp_geteuid;
gid_t (*next_getegid)(void)=tmp_getegid;
int (*next_setuid)(uid_t id)=tmp_setuid;
int (*next_setgid)(gid_t id)=tmp_setgid;
int (*next_seteuid)(uid_t id)=tmp_seteuid;
int (*next_setegid)(gid_t id)=tmp_setegid;
int (*next_setreuid)(SETREUID_ARG ruid, SETREUID_ARG euid)=tmp_setreuid;
int (*next_setregid)(SETREGID_ARG rgid, SETREGID_ARG egid)=tmp_setregid;
#ifdef HAVE_GETRESUID
int (*next_getresuid)(uid_t *ruid, uid_t *euid, uid_t *suid)=tmp_getresuid;
#endif /* HAVE_GETRESUID */
#ifdef HAVE_GETRESGID
int (*next_getresgid)(gid_t *rgid, gid_t *egid, gid_t *sgid)=tmp_getresgid;
#endif /* HAVE_GETRESGID */
#ifdef HAVE_SETRESUID
int (*next_setresuid)(uid_t ruid, uid_t euid, uid_t suid)=tmp_setresuid;
#endif /* HAVE_SETRESUID */
#ifdef HAVE_SETRESGID
int (*next_setresgid)(gid_t rgid, gid_t egid, gid_t sgid)=tmp_setresgid;
#endif /* HAVE_SETRESGID */
#ifdef HAVE_SETFSUID
uid_t (*next_setfsuid)(uid_t fsuid)=tmp_setfsuid;
#endif /* HAVE_SETFSUID */
#ifdef HAVE_SETFSGID
gid_t (*next_setfsgid)(gid_t fsgid)=tmp_setfsgid;
#endif /* HAVE_SETFSGID */
int (*next_initgroups)(const char *user, INITGROUPS_SECOND_ARG group)=tmp_initgroups;
int (*next_setgroups)(SETGROUPS_SIZE_TYPE size, const gid_t *list)=tmp_setgroups;

#ifdef HAVE_FSTATAT
#ifdef HAVE_FCHMODAT
int (*next_fchmodat)(int dir_fd, const char *path, mode_t mode, int flags)=tmp_fchmodat;
#endif /* HAVE_FCHMODAT */
#ifdef HAVE_FCHOWNAT
int (*next_fchownat)(int dir_fd, const char *path, uid_t owner, gid_t group, int flags)=tmp_fchownat;
#endif /* HAVE_FCHOWNAT */
#ifdef HAVE_MKDIRAT
int (*next_mkdirat)(int dir_fd, const char *pathname, mode_t mode)=tmp_mkdirat;
#endif /* HAVE_MKDIRAT */
#ifdef HAVE_OPENAT
int (*next_openat)(int dir_fd, const char *pathname, int flags)=tmp_openat;
#endif /* HAVE_OPENAT */
#ifdef HAVE_RENAMEAT
int (*next_renameat)(int olddir_fd, const char *oldpath, int newdir_fd, const char *newpath)=tmp_renameat;
#endif /* HAVE_RENAMEAT */
#ifdef HAVE_UNLINKAT
int (*next_unlinkat)(int dir_fd, const char *pathname, int flags)=tmp_unlinkat;
#endif /* HAVE_UNLINKAT */
#endif /* HAVE_FSTATAT */

#ifdef HAVE_SYS_ACL_H
int (*next_acl_set_fd)(int fd, acl_t acl)=tmp_acl_set_fd;
int (*next_acl_set_file)(const char *path_p, acl_type_t type, acl_t acl)=tmp_acl_set_file;
#endif /* HAVE_SYS_ACL_H */
#endif
