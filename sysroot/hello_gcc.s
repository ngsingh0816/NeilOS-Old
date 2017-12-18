.global main

.text

main:
// Print
mov $message, %eax
pushl %eax
call printf
addl $4, %esp

movl $0, %eax
ret

message:
.ascii  "Hello, world\n"
