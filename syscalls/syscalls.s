.section __TEXT,__text
.globl __sys_write
__sys_write:
    movz x16, #0x2000, lsl #16
    movk x16, #0x0004, lsl #0
    svc 0x80
    ret

.globl __sys_read
__sys_read:
    movz x16, #0x2000, lsl #16
    movk x16, #0x0003, lsl #0
    svc 0x80
    ret
