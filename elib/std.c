#include <time.h>
#include <string.h>
#include <fcntl.h>
#include "std.h"
#include "elist.h"

#ifdef WIN32

echar *e_strcasestr(const echar *s1, const echar *s2)
{
	echar c, sc;
	size_t len;

	if ((c = *s2++) != 0) {
		c = tolower((euchar)c);
		len = e_strlen(s2);
		do {
			do {
				if ((sc = *s1++) == 0)
					return NULL;
			} while ((echar)tolower((euchar)sc) != c);
		} while (e_strncasecmp(s1, s2, len) != 0);
		s1--;
	}
	return (echar *)s1;
}

//static FILE _iob0 = __iob_func()[0];
//static FILE _iob1 = __iob_func()[1];
//static FILE _iob2 = __iob_func()[2];
#ifndef _WIN64
FILE _iob[3] = {0, 0, 0};
#endif
/*
int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	FILETIME ft;
	euint64 tmpres = 0;  
	static int tzflag = 0; 


	if (tp) {    
#ifdef _WIN32_WCE
		SYSTEMTIME st;
		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);
#else
		GetSystemTimeAsFileTime(&ft);
#endif

		tmpres |= ft.dwHighDateTime;   
		tmpres <<= 32; 
		tmpres |= ft.dwLowDateTime;
		tmpres /= 10;
		tmpres -= DELTA_EPOCH_IN_MICROSECS;

		tp->tv_sec = (long)(tmpres / 1000000UL); 
		tp->tv_usec = (long)(tmpres % 1000000UL); 
	}

	if (tzp) {   
		if (!tzflag){
#if !TSK_UNDER_WINDOWS_RT
			_tzset();
#endif
			tzflag++;  
		}   
		tzp->tz_minuteswest = _timezone / 60;
		tzp->tz_dsttime = _daylight;
	}

	return 0; 
}
*/
int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	edouble f;
	LARGE_INTEGER inter;
	static eullong QuadPart = 0;
	if (QuadPart == 0) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		QuadPart = freq.QuadPart;
	}
	QueryPerformanceCounter(&inter);
	f = inter.QuadPart / (edouble)QuadPart;

	tp->tv_sec = (long)f;
	tp->tv_usec = (long)((f -  tp->tv_sec) * 1000000);

	return 0;
}

eint e_thread_create(e_thread_t *thread, void *(*routine)(void *), ePointer arg)
{
	*thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)routine, arg, 0, NULL);
	if (*thread == 0)
		return -1;
	return 0;
}

eint e_thread_join(e_thread_t thread, void **retval)
{
	return -1;
}

eint e_thread_mutex_init(e_thread_mutex_t *mutex, const e_thread_mutexattr_t *mutexattr)
{
	mutex->handle = CreateMutex(NULL, FALSE, NULL);
	mutex->value  = 1;
	return 0;
}

eint e_thread_mutex_lock(e_thread_mutex_t *mutex)
{
	mutex->value--;
	WaitForSingleObject(mutex->handle, INFINITE);
	return 0;
}

eint e_thread_mutex_trylock(e_thread_mutex_t *mutex)
{
	return -1;
}

eint e_thread_mutex_unlock(e_thread_mutex_t *mutex)
{
	mutex->value++;
	ReleaseMutex(mutex->handle);
	return 0;
}

eint e_thread_mutex_destroy(e_thread_mutex_t *mutex)
{
	CloseHandle(mutex->handle);
	return 0;
}

eint e_thread_rwlock_init(e_thread_rwlock_t *rwlock, e_thread_rwlockattr_t *attr)
{
#ifdef _WIN64
	InitializeSRWLock(rwlock); 
#else
	*rwlock = CreateMutex(NULL, FALSE, NULL);
#endif
	return 0;
}

eint e_thread_rwlock_rdlock(e_thread_rwlock_t *rwlock)
{
#ifdef _WIN64
	AcquireSRWLockShared(rwlock);
#else
	WaitForSingleObject(*rwlock, INFINITE);
#endif
	return 0;
}

eint e_thread_rwlock_wrlock(e_thread_rwlock_t *rwlock)
{
#ifdef _WIN64
	AcquireSRWLockExclusive(rwlock);
#else
	WaitForSingleObject(*rwlock, INFINITE);
#endif
	return 0;
}

eint e_thread_rwlock_tryrdlock(e_thread_rwlock_t *rwlock)
{
	return -1;
}

eint e_thread_rwlock_trywrlock(e_thread_rwlock_t *rwlock)
{
	return -1;
}

eint e_thread_rwlock_rdunlock(e_thread_rwlock_t *rwlock)
{
#ifdef _WIN64
	ReleaseSRWLockShared(rwlock);
#else
	ReleaseMutex(*rwlock);
#endif
	return 0;
}

eint e_thread_rwlock_wrunlock(e_thread_rwlock_t *rwlock)
{
#ifdef _WIN64
	ReleaseSRWLockExclusive(rwlock);
#else
	ReleaseMutex(*rwlock);
#endif
	return 0;
}

eint pthread_rwlock_destroy(e_thread_rwlock_t *rwlock)
{
	return -1;
}

eint e_thread_cond_init(e_thread_cond_t *cond, e_thread_condattr_t *attr)
{
	return -1;
}

eint e_thread_cond_broadcast(e_thread_cond_t *cond)
{
	return -1;
}

eint e_thread_cond_wait(e_thread_cond_t *cond, e_thread_mutex_t *mutex)
{
	return -1;
}

eint e_sem_init(e_sem_t *sem, euint value)
{
	sem->handle = CreateSemaphore(NULL, value, 1000, NULL);
	sem->value  = value;
	return 0;
}

eint e_sem_destroy(e_sem_t *sem)
{
	CloseHandle(sem->handle);
	return 0;
}

eint e_sem_post(e_sem_t *sem)
{
	ReleaseSemaphore(sem->handle, 1, NULL);
	sem->value++;
	return 0;
}

eint e_sem_wait(e_sem_t *sem)
{
	sem->value--;
	WaitForSingleObject(sem->handle, INFINITE);
	return 0;
}

eint e_sem_trywait(e_sem_t *sem)
{
	return -1;
}

eint e_sem_timedwait(e_sem_t *sem, const struct timespec *abs_timeout)
{
	return -1;
}

eint e_sem_getvalue(e_sem_t *sem, eint *val)
{
	*val = sem->value;
	return 0;
}

DIR *e_opendir(const echar *path)
{
	WIN32_FIND_DATA FindData;
	DIR *dir;
	echar buf[512];

	e_sprintf(buf, "%s\\*.*", path);

	if (!(dir = (DIR *)e_calloc(sizeof(DIR), 1)))
		return NULL;

	dir->hFind = FindFirstFile(buf, &FindData);
	if (dir->hFind == INVALID_HANDLE_VALUE) {
		e_free(dir);
		return NULL;
	}

	dir->dd_fd = 0;

	return dir;
}

struct dirent *e_readdir(DIR *dir)
{
	static struct dirent dirent;
	WIN32_FIND_DATA FileData;

	if (!FindNextFile(dir->hFind, &FileData))
		return 0;

	e_strncpy(dirent.d_name, FileData.cFileName, 256);
	dirent.d_reclen = e_strlen(dirent.d_name);
	dirent.d_reclen = FileData.nFileSizeLow;

	if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		dirent.d_type = DT_DIR;
	else if (FileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL)
		dirent.d_type = DT_REG;
	else
		dirent.d_type = DT_UNKNOWN;

	return &dirent;
}

eint e_closedir(DIR *dir)
{
	free(dir);
	return 0;
}

int e_open_file(const char *filename, int flags)
{
	int create = OPEN_EXISTING;
	int access = 0;
	int mode   = 0;

	if ((flags & 3) == O_RDONLY) {
		access = GENERIC_READ;
		mode = FILE_SHARE_READ;
	}
	else if ((flags & 3) == O_WRONLY) {
		access = GENERIC_WRITE;
		mode = FILE_SHARE_WRITE;
	}
	else if ((flags & 3) == O_RDWR) {
		access = GENERIC_READ | GENERIC_WRITE;
		mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}

	if (flags & O_CREAT) {
		if (flags & O_EXCL)
			create = CREATE_NEW;
		else
			create = OPEN_ALWAYS;
	}

	if (flags & O_TRUNC)
		create = TRUNCATE_EXISTING;

	return (int)CreateFile(filename,
	                      access,
	                      mode,
	                      NULL,
	                      create,
	                      FILE_ATTRIBUTE_NORMAL,
	                      NULL);
}

void e_close_file(int fd)
{
	if ((HANDLE)fd != INVALID_HANDLE_VALUE)
		CloseHandle((HANDLE)fd);
}

eMapHandle e_map_open_file(const char *filename, int flags)
{
	eFileMap *hmap = calloc(sizeof(eFileMap), 1);
	int create = OPEN_EXISTING;
	int access = 0;
	int mode   = 0;

	if ((flags & 3) == O_RDONLY) {
		access = GENERIC_READ;
		mode = FILE_SHARE_READ;
	}
	else if ((flags & 3) == O_WRONLY) {
		access = GENERIC_WRITE;
		mode = FILE_SHARE_WRITE;
	}
	else if ((flags & 3) == O_RDWR) {
		access = GENERIC_READ | GENERIC_WRITE;
		mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	}

	if (flags & O_CREAT) {
		if (flags & O_EXCL)
			create = CREATE_NEW;
		else
			create = OPEN_ALWAYS;
	}

	if (flags & O_TRUNC)
		create = TRUNCATE_EXISTING;

	hmap->fd = CreateFile(filename,
	                      access,
	                      mode,
	                      NULL,
	                      create,
	                      FILE_ATTRIBUTE_NORMAL,
	                      NULL);
	return (eMapHandle)hmap;
}

void e_map_close(eMapHandle hmap)
{
	eFileMap *fm = (eFileMap *)hmap;
	CloseHandle(fm->fd);
}

int e_map_read_file(eMapHandle hmap, void *buf, int size)
{
	int ret = 0;
	eFileMap *fm = (eFileMap *)hmap;
	if (!ReadFile(fm->fd, buf, size, &ret, NULL))
		return -1;
	return ret;
}

int e_map_write_file(eMapHandle hmap, void *buf, int size)
{
	int ret = 0;
	eFileMap *fm = (eFileMap *)hmap;
	if (!WriteFile(fm->fd, buf, size, &ret, NULL))
		return -1;
	return ret;
}

int e_map_seek(eMapHandle hmap, int offset, int whence)
{
	eFileMap *fm = (eFileMap *)hmap;
	return SetFilePointer(fm->fd,offset, NULL, whence);
}

int e_map_get_size(eMapHandle hmap)
{
	eFileMap *fm = (eFileMap *)hmap;
	return GetFileSize(fm->fd, NULL);
}

void *e_mmap(void *addr, size_t size, int prot, int flags, eMapHandle hmap, off_t offset)
{
	eFileMap *fm = (eFileMap *)hmap;
	int access = 0;
	int page;
	
	if (prot & 1) {
		access |= FILE_MAP_READ;
		page = PAGE_READONLY;
	}

	if (prot & 2) {
		access |= FILE_MAP_WRITE;
		page = PAGE_READWRITE;
	}
		
	if (prot & 4)
		access |= FILE_MAP_EXECUTE;

	fm->map = CreateFileMapping(fm->fd,
			NULL,
			page,
			0,
			size,
			NULL);

	return MapViewOfFile(fm->map,
			access,
			offset,
			0,
			0);
}

int e_munmap(void *addr, size_t length)
{
	return UnmapViewOfFile(addr);
}

int e_socket_init(void)
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
		return -1;
	return 0;
}

int e_socket_close(eSocket_t fd)
{
	return closesocket(fd);
}

#else

#include <sys/stat.h>

echar *e_strcasestr(const echar *s1, const echar *s2)
{
	return (echar *)strcasestr((const char *)s1, (const char *)s2);
}

eint e_thread_create(e_thread_t *thread, void *(*routine)(void *), ePointer arg)
{
	return pthread_create(thread, NULL, routine, arg);
}

eint e_thread_join(e_thread_t thread, void **retval)
{
	return pthread_join(thread, retval);
}

eint e_thread_mutex_init(e_thread_mutex_t *mutex, const e_thread_mutexattr_t *mutexattr)
{
	return pthread_mutex_init(mutex, mutexattr);
}

eint e_thread_mutex_lock(e_thread_mutex_t *mutex)
{
	return pthread_mutex_lock(mutex);
}

eint e_thread_mutex_trylock(e_thread_mutex_t *mutex)
{
	return pthread_mutex_trylock(mutex);
}

eint e_thread_mutex_unlock(e_thread_mutex_t *mutex)
{
	return pthread_mutex_unlock(mutex);
}

eint e_thread_mutex_destroy(e_thread_mutex_t *mutex)
{
	return pthread_mutex_destroy(mutex);
}

eint e_thread_rwlock_rdlock(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_rdlock(rwlock);
}

eint e_thread_rwlock_wrlock(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_wrlock(rwlock);
}

eint e_thread_rwlock_tryrdlock(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_tryrdlock(rwlock);
}

eint e_thread_rwlock_trywrlock(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_trywrlock(rwlock);
}

eint e_thread_rwlock_unlock(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_unlock(rwlock);
}

eint e_thread_rwlock_rdunlock(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_unlock(rwlock);
}

eint e_thread_rwlock_wrunlock(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_unlock(rwlock);
}

eint e_thread_rwlock_destroy(e_thread_rwlock_t *rwlock)
{
	return pthread_rwlock_destroy(rwlock);
}

eint e_thread_rwlock_init(e_thread_rwlock_t *rwlock, e_thread_rwlockattr_t *attr)
{
	return pthread_rwlock_init(rwlock, attr);
}

eint e_thread_cond_init(e_thread_cond_t *cond, e_thread_condattr_t *attr)
{
	return e_thread_cond_init(cond, attr);
}

eint e_thread_cond_broadcast(e_thread_cond_t *cond)
{
	return pthread_cond_broadcast(cond);
}

eint e_thread_cond_wait(e_thread_cond_t *cond, e_thread_mutex_t *mutex)
{
	return pthread_cond_wait(cond, mutex);
}

eint e_sem_init(e_sem_t *sem, euint value)
{
	return sem_init(sem, 0, value);
}

eint e_sem_destroy(e_sem_t *sem)
{
	return sem_destroy(sem);
}

eint e_sem_post(e_sem_t *sem)
{
	return sem_post(sem);
}

eint e_sem_wait(e_sem_t *sem)
{
	return sem_wait(sem);
}

eint e_sem_trywait(e_sem_t *sem)
{
	return sem_trywait(sem);
}

eint e_sem_timedwait(e_sem_t *sem, const struct timespec *abs_timeout)
{
	return sem_timedwait(sem, abs_timeout);
}

eint e_sem_getvalue(e_sem_t *sem, eint *val)
{
	return sem_getvalue(sem, val);
}

int e_open_file(const char *filename, int flags)
{
	return open(filename, flags, S_IWUSR | S_IRUSR);
}

void e_close_file(int fd)
{
	close(fd);
}

eMapHandle e_map_open_file(const char *name, int flags)
{
	return open(name, flags, S_IWUSR | S_IRUSR);
}

void e_map_close(eMapHandle fd)
{
	close(fd);
}

int e_map_read_file(eMapHandle hmap, void *buf, int size)
{
	return read(hmap, buf, size);
}

int e_map_write_file(eMapHandle hmap, void *buf, int size)
{
	return write(hmap, buf, size);
}

int e_map_seek(eMapHandle hmap, int offset, int whence)
{
	return lseek(hmap, offset, whence);
}

int e_map_get_size(eMapHandle hmap)
{
	struct stat st;
	if (fstat(hmap, &st) < 0)
		return -1;
	return st.st_size;
}

void *e_mmap(void *addr, size_t length, int prot, int flags, eMapHandle fd, off_t offset)
{
	return mmap(addr, length, prot, flags, fd, offset);
}

int e_munmap(void *addr, size_t length)
{
	return munmap(addr, length);
}

int e_socket_init(void)
{
	return 1;
}

int e_socket_close(eSocket_t fd)
{
	return close(fd);
}

#endif

echar** e_strsplit(const echar *str, const echar *delimiter, eint max)
{
	elist_t list = {sizeof(echar *), 0, NULL, NULL};

	echar **str_array, *s;
	euint n = 0;
	const echar *remain;
	echar *new;

	e_return_val_if_fail(str != NULL, NULL);
	e_return_val_if_fail(delimiter != NULL, NULL);
	e_return_val_if_fail(delimiter[0] != '\0', NULL);

	if (max < 1)
		max = 0x7fff;

	remain = str;
	s = e_strstr(remain, delimiter);
	if (s) {
		eint delimiter_len = e_strlen(delimiter);

		while (--max && s) {
			eint   len;     

			len = s - remain;
			new = e_malloc(len + 1);
			e_strncpy(new, remain, len);
			new[len] = 0;
			e_list_add_data_to_head(&list, &new);
			n++;
			remain = s + delimiter_len;
			s = e_strstr(remain, delimiter);
		}
	}

	if (*str) {
		new = e_strdup(remain);
		n++;
		e_list_add_data_to_head(&list, &new);
	}

	str_array = e_calloc(sizeof(echar *), n + 1);

	str_array[n--] = NULL;
	while (list.head) {
		e_list_out_data_from_head(&list, &str_array[n--]);
	}

	return str_array;
}

void e_strfreev(echar **str_array)
{
	if (str_array) {
		int i;
		for(i = 0; str_array[i]; i++)
			e_free(str_array[i]);
		e_free(str_array);
	}
}
