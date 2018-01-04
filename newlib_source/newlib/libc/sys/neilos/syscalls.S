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
pushl %edi					  ;\
movl $number, %eax            ;\
movl 24(%esp), %ebx           ;\
movl 28(%esp), %ecx           ;\
movl 32(%esp), %edx           ;\
movl 36(%esp), %esi           ;\
movl 40(%esp), %edi			  ;\
int	$0x80                     ;\
popl %edi					  ;\
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
DO_CALL(sys_select, 16)
DO_CALL(sys_mkdir, 17)
DO_CALL(sys_link, 18)
DO_CALL(sys_unlink, 19)
DO_CALL(sys_readdir, 20)
DO_CALL(sys_utime, 21)
DO_CALL(sys_brk, 22)
DO_CALL(sys_sbrk, 23)
DO_CALL(sys_dup, 24)
DO_CALL(sys_dup2, 25)
DO_CALL(sys_times, 26)
DO_CALL(sys_gettimeofday, 27)
DO_CALL(sys_kill, 28)
DO_CALL(sys_sigaction, 29)
DO_CALL(sys_sigsetmask, 30)
DO_CALL(sys_siggetmask, 31)
DO_CALL(sys_sigprocmask, 32)
DO_CALL(sys_sigsuspend, 33)
DO_CALL(sys_alarm, 34)
DO_CALL(sys_nanosleep, 35)
DO_CALL(sys_sched_yield, 36)
DO_CALL(sys_sysconf, 37)
DO_CALL(sys_fpathconf, 38)
DO_CALL(sys_getwd, 39)
DO_CALL(sys_chdir, 40)
DO_CALL(sys_ioctl, 41)
DO_CALL(sys_thread_create, 42)
DO_CALL(sys_gettid, 43)
DO_CALL(sys_thread_wait, 44)
DO_CALL(sys_thread_exit, 45)
