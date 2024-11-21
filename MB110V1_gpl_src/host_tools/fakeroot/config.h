/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* for packed */
#define FAKEROOT_ATTR(x) __attribute__ ((x))

/* store path in the database instead of inode and device */
/* #undef FAKEROOT_DB_PATH */

/* use TCP instead of SysV IPC */
/* #undef FAKEROOT_FAKENET */

/* Define to 1 if the compiler understands __builtin_expect. */
#define HAVE_BUILTIN_EXPECT 1

/* Define to 1 if you have the declaration of `setenv', and to 0 if you don't.
   */
#define HAVE_DECL_SETENV 1

/* Define to 1 if you have the declaration of `unsetenv', and to 0 if you
   don't. */
#define HAVE_DECL_UNSETENV 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <endian.h> header file. */
#define HAVE_ENDIAN_H 1

/* Define to 1 if you have the `fchmodat' function. */
#define HAVE_FCHMODAT 1

/* Define to 1 if you have the `fchownat' function. */
#define HAVE_FCHOWNAT 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <features.h> header file. */
#define HAVE_FEATURES_H 1

/* Define to 1 if you have the `fstatat' function. */
#define HAVE_FSTATAT 1

/* Define to 1 if you have the `getresgid' function. */
#define HAVE_GETRESGID 1

/* Define to 1 if you have the `getresuid' function. */
#define HAVE_GETRESUID 1

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `dl' library (-ldl). */
#define HAVE_LIBDL 1

/* Define to 1 if you have the `pthread' library (-lpthread). */
/* #undef HAVE_LIBPTHREAD */

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkdirat' function. */
#define HAVE_MKDIRAT 1

/* Define to 1 if you have the `mknodat' function. */
#define HAVE_MKNODAT 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the `openat' function. */
#define HAVE_OPENAT 1

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `renameat' function. */
#define HAVE_RENAMEAT 1

/* have the semun union */
/* #undef HAVE_SEMUN_DEF */

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the `setfsgid' function. */
#define HAVE_SETFSGID 1

/* Define to 1 if you have the `setfsuid' function. */
#define HAVE_SETFSUID 1

/* Define to 1 if you have the `setresgid' function. */
#define HAVE_SETRESGID 1

/* Define to 1 if you have the `setresuid' function. */
#define HAVE_SETRESUID 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the <sys/acl.h> header file. */
/* #undef HAVE_SYS_ACL_H */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/feature_tests.h> header file. */
/* #undef HAVE_SYS_FEATURE_TESTS_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/sysmacros.h> header file. */
#define HAVE_SYS_SYSMACROS_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unlinkat' function. */
#define HAVE_UNLINKAT 1

/* second argument of initgroups */
#define INITGROUPS_SECOND_ARG gid_t

/* path to libc shared object */
#define LIBCPATH "/lib/libc.so.6"

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "fakeroot"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "schizo@debian.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "fakeroot"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "fakeroot 1.9.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "fakeroot"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.9.1"

/* type of readlink bufsize */
#define READLINK_BUFSIZE_TYPE size_t

/* type of readlink buf */
#define READLINK_BUF_TYPE char

/* type of readlink return value */
#define READLINK_RETVAL_TYPE ssize_t

/* type of setgroups size */
#define SETGROUPS_SIZE_TYPE size_t

/* argument of setregid */
#define SETREGID_ARG gid_t

/* argument of setreuid */
#define SETREUID_ARG gid_t

/* second argument of stat */
#define STAT_SECOND_ARG struct stat *

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* stat-struct conversion hackery */
/* #undef STUPID_ALPHA_HACK */

/* Version number of package */
#define VERSION "1.9.1"

/* Stuff.  */
#define WRAP_STAT __xstat
#define WRAP_STAT_QUOTE "__xstat"
#define TMP_STAT tmp___xstat
#define NEXT_STAT_NOARG next___xstat

#define WRAP_LSTAT_QUOTE "__lxstat"
#define WRAP_LSTAT __lxstat
#define TMP_LSTAT tmp___lxstat
#define NEXT_LSTAT_NOARG next___lxstat

#define WRAP_FSTAT_QUOTE "__fxstat"
#define WRAP_FSTAT __fxstat
#define TMP_FSTAT tmp___fxstat
#define NEXT_FSTAT_NOARG next___fxstat

#define WRAP_FSTATAT_QUOTE "__fxstatat"
#define WRAP_FSTATAT __fxstatat
#define TMP_FSTATAT tmp___fxstatat
#define NEXT_FSTATAT_NOARG next___fxstatat

#define WRAP_STAT64_QUOTE "__xstat64"
#define WRAP_STAT64 __xstat64
#define TMP_STAT64 tmp___xstat64
#define NEXT_STAT64_NOARG next___xstat64

#define WRAP_LSTAT64_QUOTE "__lxstat64"
#define WRAP_LSTAT64 __lxstat64
#define TMP_LSTAT64 tmp___lxstat64
#define NEXT_LSTAT64_NOARG next___lxstat64

#define WRAP_FSTAT64_QUOTE "__fxstat64"
#define WRAP_FSTAT64 __fxstat64
#define TMP_FSTAT64 tmp___fxstat64
#define NEXT_FSTAT64_NOARG next___fxstat64

#define WRAP_FSTATAT64_QUOTE "__fxstatat64"
#define WRAP_FSTATAT64 __fxstatat64
#define TMP_FSTATAT64 tmp___fxstatat64
#define NEXT_FSTATAT64_NOARG next___fxstatat64

#define WRAP_MKNOD_QUOTE "__xmknod"
#define WRAP_MKNOD __xmknod
#define TMP_MKNOD tmp___xmknod
#define NEXT_MKNOD_NOARG next___xmknod

#define WRAP_MKNODAT_QUOTE "__xmknodat"
#define WRAP_MKNODAT __xmknodat
#define TMP_MKNODAT tmp___xmknodat
#define NEXT_MKNODAT_NOARG next___xmknodat


/* fifth argument of __xmknodat */
#define XMKNODAT_FIFTH_ARG *

/* fourth argument of __xmknod */
#define XMKNOD_FRTH_ARG *

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

#if ! HAVE_BUILTIN_EXPECT
#define __builtin_expect(x, expected_value) (x)
#endif
