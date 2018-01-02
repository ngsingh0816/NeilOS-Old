/*
 * Rather than create a case for each number of arguments, we simplify
 * and use one macro for up to three arguments; the system calls should
 * ignore the other registers, and they're caller-saved anyway.
 */
#define DO_CALL(name,number)   \
.globl name                   ;\
name:                         ;\
pushl %ebx                    ;\
pushl %ecx                    ;\
pushl %edi                    ;\
pushl %esi                    ;\
movl $number, %eax            ;\
movl 20(%esp), %ebx           ;\
movl 24(%esp), %ecx           ;\
movl 28(%esp), %edx           ;\
movl 32(%esp), %esi           ;\
int	$0x80                     ;\
popl %esi                     ;\
popl %edi                     ;\
popl %ecx                     ;\
popl %ebx                     ;\
ret

DO_CALL(sys_fork, 0)
DO_CALL(sys_execve, 1)
DO_CALL(sys_getpid, 2)
DO_CALL(sys_getppid, 3)
DO_CALL(sys_waitpid, 4)
DO_CALL(sys_exit, 5)
DO_CALL(sys_open, 6)
DO_CALL(sys_read, 7)
DO_CALL(sys_write, 8)
DO_CALL(sys_llseek, 9)
DO_CALL(sys_truncate, 10)
DO_CALL(sys_stat, 11)
DO_CALL(sys_close, 12)
DO_CALL(sys_isatty, 13)
DO_CALL(sys_pipe, 14)
DO_CALL(sys_fcntl, 15)
DO_CALL(sys_mkdir, 16)
DO_CALL(sys_link, 17)
DO_CALL(sys_unlink, 18)
DO_CALL(sys_readdir, 19)
DO_CALL(sys_utime, 20)
DO_CALL(sys_brk, 21)
DO_CALL(sys_sbrk, 22)
DO_CALL(sys_dup, 23)
DO_CALL(sys_dup2, 24)
DO_CALL(sys_times, 25)
DO_CALL(sys_gettimeofday, 26)
DO_CALL(sys_kill, 27)
DO_CALL(sys_sigaction, 28)
DO_CALL(sys_sigsetmask, 29)
DO_CALL(sys_siggetmask, 30)
DO_CALL(sys_sigprocmask, 31)
DO_CALL(sys_sigsuspend, 32)
DO_CALL(sys_alarm, 33)
DO_CALL(sys_sleep, 34)
DO_CALL(sys_sched_yield, 35)
DO_CALL(sys_sysconf, 36)
DO_CALL(sys_fpathconf, 37)
DO_CALL(sys_getwd, 38)
DO_CALL(sys_chdir, 39)
DO_CALL(sys_ioctl, 40)
DO_CALL(sys_thread_create, 41)
DO_CALL(sys_gettid, 42)
DO_CALL(sys_thread_wait, 43)
DO_CALL(sys_thread_exit, 44)
