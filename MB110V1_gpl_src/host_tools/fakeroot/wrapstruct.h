/* Automatically generated file. Do not edit. Edit wrapawk/wrapfunc.inp. */
#ifndef WRAPSTRUCT_H
#define WRAPSTRUCT_H
struct next_wrap_st next_wrap[]= {
  {(void(*))&next_open, "open"},
  {(void(*))&next_fopen, "fopen"},
  {(void(*))&next_freopen, "freopen"},


  {(void(*))&NEXT_LSTAT_NOARG, WRAP_LSTAT_QUOTE},
  {(void(*))&NEXT_STAT_NOARG, WRAP_STAT_QUOTE},
  {(void(*))&NEXT_FSTAT_NOARG, WRAP_FSTAT_QUOTE},
#ifdef HAVE_FSTATAT
  {(void(*))&NEXT_FSTATAT_NOARG, WRAP_FSTATAT_QUOTE},
#endif /* HAVE_FSTATAT */

#ifdef STAT64_SUPPORT
  {(void(*))&NEXT_LSTAT64_NOARG, WRAP_LSTAT64_QUOTE},
  {(void(*))&NEXT_STAT64_NOARG, WRAP_STAT64_QUOTE},
  {(void(*))&NEXT_FSTAT64_NOARG, WRAP_FSTAT64_QUOTE},
#ifdef HAVE_FSTATAT
  {(void(*))&NEXT_FSTATAT64_NOARG, WRAP_FSTATAT64_QUOTE},
#endif /* HAVE_FSTATAT */
#endif /* STAT64_SUPPORT */

  {(void(*))&NEXT_MKNOD_NOARG, WRAP_MKNOD_QUOTE},

#ifdef HAVE_FSTATAT
#ifdef HAVE_MKNODAT
  {(void(*))&NEXT_MKNODAT_NOARG, WRAP_MKNODAT_QUOTE},
#endif /* HAVE_MKNODAT */
#endif /* HAVE_FSTATAT */


  {(void(*))&next_chown, "chown"},
  {(void(*))&next_lchown, "lchown"},
  {(void(*))&next_fchown, "fchown"},
  {(void(*))&next_chmod, "chmod"},
  {(void(*))&next_fchmod, "fchmod"},
  {(void(*))&next_mkdir, "mkdir"},
  {(void(*))&next_unlink, "unlink"},
  {(void(*))&next_rmdir, "rmdir"},
  {(void(*))&next_remove, "remove"},
  {(void(*))&next_rename, "rename"},

#ifdef FAKEROOT_FAKENET
  {(void(*))&next_fork, "fork"},
  {(void(*))&next_vfork, "vfork"},
  {(void(*))&next_close, "close"},
  {(void(*))&next_dup2, "dup2"},
#endif /* FAKEROOT_FAKENET */


  {(void(*))&next_getuid, "getuid"},
  {(void(*))&next_getgid, "getgid"},
  {(void(*))&next_geteuid, "geteuid"},
  {(void(*))&next_getegid, "getegid"},
  {(void(*))&next_setuid, "setuid"},
  {(void(*))&next_setgid, "setgid"},
  {(void(*))&next_seteuid, "seteuid"},
  {(void(*))&next_setegid, "setegid"},
  {(void(*))&next_setreuid, "setreuid"},
  {(void(*))&next_setregid, "setregid"},
#ifdef HAVE_GETRESUID
  {(void(*))&next_getresuid, "getresuid"},
#endif /* HAVE_GETRESUID */
#ifdef HAVE_GETRESGID
  {(void(*))&next_getresgid, "getresgid"},
#endif /* HAVE_GETRESGID */
#ifdef HAVE_SETRESUID
  {(void(*))&next_setresuid, "setresuid"},
#endif /* HAVE_SETRESUID */
#ifdef HAVE_SETRESGID
  {(void(*))&next_setresgid, "setresgid"},
#endif /* HAVE_SETRESGID */
#ifdef HAVE_SETFSUID
  {(void(*))&next_setfsuid, "setfsuid"},
#endif /* HAVE_SETFSUID */
#ifdef HAVE_SETFSGID
  {(void(*))&next_setfsgid, "setfsgid"},
#endif /* HAVE_SETFSGID */
  {(void(*))&next_initgroups, "initgroups"},
  {(void(*))&next_setgroups, "setgroups"},

#ifdef HAVE_FSTATAT
#ifdef HAVE_FCHMODAT
  {(void(*))&next_fchmodat, "fchmodat"},
#endif /* HAVE_FCHMODAT */
#ifdef HAVE_FCHOWNAT
  {(void(*))&next_fchownat, "fchownat"},
#endif /* HAVE_FCHOWNAT */
#ifdef HAVE_MKDIRAT
  {(void(*))&next_mkdirat, "mkdirat"},
#endif /* HAVE_MKDIRAT */
#ifdef HAVE_OPENAT
  {(void(*))&next_openat, "openat"},
#endif /* HAVE_OPENAT */
#ifdef HAVE_RENAMEAT
  {(void(*))&next_renameat, "renameat"},
#endif /* HAVE_RENAMEAT */
#ifdef HAVE_UNLINKAT
  {(void(*))&next_unlinkat, "unlinkat"},
#endif /* HAVE_UNLINKAT */
#endif /* HAVE_FSTATAT */

#ifdef HAVE_SYS_ACL_H
  {(void(*))&next_acl_set_fd, "acl_set_fd"},
  {(void(*))&next_acl_set_file, "acl_set_file"},
#endif /* HAVE_SYS_ACL_H */
  {NULL, NULL},
};
#endif
