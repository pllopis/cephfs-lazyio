/*
  LD_PRELOAD hack to enable Lazy I/O when opening files on CephFS.
  Only the file paths that match the prefix defined by the environment variable
  LAZYIO_CEPHFS_PREFIX will be opened in Lazy I/O mode.
  Optionally, also define the environment variable LAZYIO_LOG where each 
  open and ioctl will be logged to. The PID will be appended to this log file path,
  meaning it can play nice when running with multiple instances (e.g. MPI).

  Author: pablo.llopis@cern.ch
*/
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/ioctl.h>

#define CEPH_IOCTL_MAGIC 0x97

/*
 * CEPH_IOC_LAZYIO - relax consistency
 *
 * Normally Ceph switches to synchronous IO when multiple clients have
 * the file open (and or more for write).  Reads and writes bypass the
 * page cache and go directly to the OSD.  Setting this flag on a file
 * descriptor will allow buffered IO for this file in cases where the
 * application knows it won't interfere with other nodes (or doesn't
 * care).
 */
#define CEPH_IOC_LAZYIO _IO(CEPH_IOCTL_MAGIC, 4)


static void log(const char *fmt, ...)
{
	static int (*open_orig)(const char *, int, mode_t);
	static int fd = -1;
	char filename[PATH_MAX];
	va_list ap;

	if (fd == -1) {
		const char *logpath;
		open_orig = (int (*)(const char*, int, mode_t))dlsym(RTLD_NEXT, "open");
		logpath = getenv("LAZYIO_LOG");
		if (logpath != NULL) {
			snprintf(filename, PATH_MAX, "%s.%d", logpath, getpid());
			fd = open_orig(filename, O_WRONLY|O_APPEND|O_CREAT, 0666);
			if (fd < 0) {
				fprintf(stderr, "WARN: Could not open log file: %s\n", filename);
			}
		}
	}

	if (fd) {
		va_start(ap, fmt);
		vdprintf(fd, fmt, ap);
		va_end(ap);
	}
}


int enable_lazy(const char *filename, int fd) {
	char absolute_filepath[PATH_MAX];

	char *prefix = getenv("LAZYIO_CEPHFS_PREFIX");
	if (!prefix)
		return 0;
	
	char *filepath = realpath(filename, absolute_filepath);

	if (filepath && (strncmp(prefix, filepath, strlen(prefix)) == 0)) {
		int lazy_ret = ioctl(fd, CEPH_IOC_LAZYIO);
		log("ceph_ioctl_lazyio(%d, %d) -> %d\n", fd, CEPH_IOC_LAZYIO, lazy_ret);
		return lazy_ret;
	}
	return 0;
}

/* Multiple open() syscalls are possible, depending on the system:
 * open()
 * open64()
 * __open()
 * __open64()
 * Override all of them equally
 */
int open(const char *filename, int flags, ...)
{
	static int (*open_orig)(const char *, int, mode_t);
	int fd;
	va_list ap;
	mode_t mode;

	if (!open_orig) {
		open_orig = (int (*)(const char*, int, mode_t))dlsym(RTLD_NEXT, "open");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	fd = open_orig(filename, flags, mode);
	log("open(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, fd);
	if (fd >= 0) {
		enable_lazy(filename, fd);
	}

	return fd;
}

int open64(const char *filename, int flags, ...)
{
	static int (*open64_orig)(const char *, int, mode_t);
	int fd;
	va_list ap;
	mode_t mode;

	if (!open64_orig) {
		open64_orig = (int (*)(const char*, int, mode_t))dlsym(RTLD_NEXT, "open64");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	fd = open64_orig(filename, flags, mode);
	log("open64(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, fd);
	if (fd >= 0) {
		enable_lazy(filename, fd);
	}

	return fd;
}


int __open(const char *filename, int flags, ...)
{
	static int (*__open_orig)(const char *, int, mode_t);
	int fd;
	va_list ap;
	mode_t mode;

	if (!__open_orig) {
		__open_orig = (int(*)(const char*, int, mode_t))dlsym(RTLD_NEXT, "__open");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	fd = __open_orig(filename, flags, mode);
	log("__open(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, fd);
	if (fd >= 0) {
		enable_lazy(filename, fd);
	}
	return fd;
}

int __open64(const char *filename, int flags, ...)
{
	static int (*__open64_orig)(const char *, int, mode_t);
	int fd;
	va_list ap;
	mode_t mode;

	if (!__open64_orig) {
		__open64_orig = (int (*)(const char*, int, mode_t))dlsym(RTLD_NEXT, "__open64");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	fd = __open64_orig(filename, flags, mode);
	log("__open64(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, fd);
	if (fd >= 0) {
		enable_lazy(filename, fd);
	}

	return fd;
}

