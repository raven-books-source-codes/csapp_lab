0000000000400fce <func4>:
   400fce:   48 83 ec 08             sub    $0x8,%rsp
   400fd2:   89 d0                   mov    %edx,%eax
   400fd4:   29 f0                   sub    %esi,%eax            ; 第一次, eax=17; 第二次进入, 
   400fd6:   89 c1                   mov    %eax,%ecx            ; 第一次进入时, ecx = 14
   400fd8:   c1 e9 1f                shr    $0x1f,%ecx           ; 第一次进入时, ecx = 0
   400fdb:   01 c8                   add    %ecx,%ea
   400fdd:   d1 f8                   sar    %eax                 ; 第一次进入时, ecx = 7
   400fdf:   8d 0c 30                lea    (%rax,%rsi,1),%ecx   ; 第一次进入时, ecx = 7
   400fe2:   39 f9                   cmp    %edi,%ecx            ; ecx : edi
   400fe4:   7e 0c                   jle    400ff2 <func4+0x24>  ; ecx <= edi
   400fe6:   8d 51 ff                lea    -0x1(%rcx),%edx      ; 第一次进入,ecdx = 6
   400fe9:   e8 e0 ff ff ff          callq  400fce <func4>       ; 递归调用
   400fee:   01 c0                   add    %eax,%eax            
   400ff0:   eb 15                   jmp    401007 <func4+0x39>
   400ff2:   b8 00 00 00 00          mov    $0x0,%eax           ; eax  = 0 
   400ff7:   39 f9                   cmp    %edi,%ecx           ; ecx : edi
   400ff9:   7d 0c                   jge    401007 <func4+0x39> ; ecx >= edi 结合上面 ecx <=edi ==> ecx = edi, 最后一层应该在这里返回, 而前文已经推断得到ecx=7
   400ffb:   8d 71 01                lea    0x1(%rcx),%esi
   400ffe:   e8 cb ff ff ff          callq  400fce <func4>
   401003:   8d 44 00 01             lea    0x1(%rax,%rax,1),%eax    ; 最后一层返回eax时,eax 应该=0
   401007:   48 83 c4 08             add    $0x8,%rsp
   40100b:   c3                      retq
   
 000000000040100c <phase_4>:
   40100c:   48 83 ec 18             sub    $0x18,%rsp
   401010:   48 8d 4c 24 0c          lea    0xc(%rsp),%rcx
   401015:   48 8d 54 24 08          lea    0x8(%rsp),%rdx
   40101a:   be cf 25 40 00          mov    $0x4025cf,%esi
   40101f:   b8 00 00 00 00          mov    $0x0,%eax
   401024:   e8 c7 fb ff ff          callq  400bf0 <__isoc99_sscanf@plt>
   401029:   83 f8 02                cmp    $0x2,%eax            ; 需要输入2个参数
   40102c:   75 07                   jne    401035 <phase_4+0x29>
   40102e:   83 7c 24 08 0e          cmpl   $0xe,0x8(%rsp)
   401033:   76 05                   jbe    40103a <phase_4+0x2e>
   401035:   e8 00 04 00 00          callq  40143a <explode_bomb>
   40103a:   ba 0e 00 00 00          mov    $0xe,%edx            ; 构造函数参数3
   40103f:   be 00 00 00 00          mov    $0x0,%esi            ; 构造函数参数2
   401044:   8b 7c 24 08             mov    0x8(%rsp),%edi       ; 构造函数参数1 edi=输入的第一个参数
   401048:   e8 81 ff ff ff          callq  400fce <func4>       ; 进入func4函数
   40104d:   85 c0                   test   %eax,%eax            ; 这行和下一行,要求func4返回的参数一定等于0
   40104f:   75 07                   jne    401058 <phase_4+0x4c>
   401051:   83 7c 24 0c 00          cmpl   $0x0,0xc(%rsp)       ; 第二参数为0
   401056:   74 05                   je     40105d <phase_4+0x51>
   401058:   e8 dd 03 00 00          callq  40143a <explode_bomb>
   40105d:   48 83 c4 18             add    $0x18,%rsp
   401061:   c3                      retq
