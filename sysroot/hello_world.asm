.global _start

.text

_start:
# write(1, message, 13)
mov     $8, %eax                # system call 8 is write
mov     $1, %ebx                # file handle 1 is stdout
mov     $message, %ecx          # address of string to output
mov     $13, %edx               # number of bytes
int $0x80                       # invoke operating system to do the write

# exit(0)
mov     $5, %eax                # system call 5 is exit
xor     %ebx, %ebx              # we want return code 0
int $0x80                       # invoke operating system to exit

message:
.ascii  "Hello, world\n"
