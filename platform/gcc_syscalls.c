/**
 * Minimal newlib hooks for the bare-metal ARM GCC build.
 *
 * The firmware does not currently use a filesystem or dynamic allocation.
 * These definitions keep accidental I/O deterministic and bound a future
 * sbrk() heap to the area reserved by the linker script.
 */

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>

extern char __end__;
extern char __HeapLimit;

__attribute__((noreturn)) void _exit(int status)
{
    (void)status;
    __asm volatile ("cpsid i");
    for (;;)
    {
        __asm volatile ("wfi");
    }
}

int _close(int file)
{
    (void)file;
    errno = ENOSYS;
    return -1;
}

int _fstat(int file, struct stat *status)
{
    (void)file;
    if (status != NULL)
    {
        status->st_mode = S_IFCHR;
    }
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int offset, int origin)
{
    (void)file;
    (void)offset;
    (void)origin;
    errno = ENOSYS;
    return -1;
}

int _read(int file, char *buffer, int length)
{
    (void)file;
    (void)buffer;
    (void)length;
    errno = ENOSYS;
    return -1;
}

int _write(int file, char *buffer, int length)
{
    (void)file;
    (void)buffer;
    return length;
}

void *_sbrk(ptrdiff_t increment)
{
    static char *heapEnd = &__end__;
    char *previousHeapEnd = heapEnd;

    if ((increment > 0) && ((size_t)increment > (size_t)(&__HeapLimit - heapEnd)))
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    if ((increment < 0) && ((size_t)(-increment) > (size_t)(heapEnd - &__end__)))
    {
        errno = EINVAL;
        return (void *)-1;
    }

    heapEnd += increment;
    return previousHeapEnd;
}

int _kill(int processId, int signal)
{
    (void)processId;
    (void)signal;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}
