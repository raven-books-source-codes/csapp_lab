00000000004010f4 <phase_6>:
  4010f4:	push   %r14
  4010f6:	push   %r13
  4010f8:	push   %r12
  4010fa:	push   %rbp
  4010fb:	push   %rbx
  4010fc:	sub    $0x50,%rsp
  401100:	mov    %rsp,%r13
  401103:	mov    %rsp,%rsi
  -----------------1.输入字段数是否为6检验 start--------------------------
  401106:	callq  40145c <read_six_numbers> ; 读取6个字段
  40110b:	mov    %rsp,%r14
  40110e:	mov    $0x0,%r12d
  -----------------2.输入的6个字段不能重复检验 start---------------------
  401114:	mov    %r13,%rbp
  401117:	mov    0x0(%r13),%eax
  40111b:	sub    $0x1,%eax                ; eax = eax -1
  40111e:	cmp    $0x5,%eax                ; eax : 5
  401121:	jbe    401128 <phase_6+0x34>    ; 相等,则通过, 这里是检验是否输入的是6个字段
  401123:	callq  40143a <explode_bomb>
  -----------------1.输入字段数是否为6检验 end--------------------------
  401128:	add    $0x1,%r12d
  40112c:	cmp    $0x6,%r12d
  401130:	je     401153 <phase_6+0x5f>
  401132:	mov    %r12d,%ebx
  401135:	movslq %ebx,%rax               ; rax作为输入数组的索引下标
  401138:	mov    (%rsp,%rax,4),%eax      ; 取出第rax个数,并存放在eax中
  40113b:	cmp    %eax,0x0(%rbp)          ; rbp其实就是外部循环的被用来比较的数(设为x_i),eax是内部循环找到的数(设为x_j)  j = i+1 到 5 ,i=0 到5
  40113e:	jne    401145 <phase_6+0x51>
  401140:	callq  40143a <explode_bomb>
  401145:	add    $0x1,%ebx               ; 内部循环+1
  401148:	cmp    $0x5,%ebx
  40114b:	jle    401135 <phase_6+0x41>
  40114d:	add    $0x4,%r13               ; 外部循环+1
  401151:	jmp    401114 <phase_6+0x20>
  -----------------2.输入的6个字段不能重复检验 end---------------------
  -----------------3. 所有数组元素x_{i} = 7 - x{i} start--------------
  401153:	lea    0x18(%rsp),%rsi         ; 指针走到数组头
  401158:	mov    %r14,%rax
  40115b:	mov    $0x7,%ecx               ; 7
  401160:	mov    %ecx,%edx
  401162:	sub    (%rax),%edx             ; x_i = 7 - x_i
  401164:	mov    %edx,(%rax)
  401166:	add    $0x4,%rax               ; 指针+1
  40116a:	cmp    %rsi,%rax               ; 是否走到数组尾
  40116d:	jne    401160 <phase_6+0x6c>
  40116f:	mov    $0x0,%esi
  401174:	jmp    401197 <phase_6+0xa3>   ; 这里直接产生了代码的跨越, 所以先调到 0x401197地址去看看
  -----------------3. 所有数组元素x_{i} = 7 - x{i} end--------------
  -----------------4. 链表节点处理代码段(将链表节点地址移动到stack的内存中) start-----
  401176:	mov    0x8(%rdx),%rdx
  40117a:	add    $0x1,%eax
  40117d:	cmp    %ecx,%eax
  40117f:	jne    401176 <phase_6+0x82>   ; 找到链表中相应的元素.链表node结构中有一个类似id的字段, 目前的数组元素就是用来找到对应的id, id的范围是1-6
  401181:	jmp    401188 <phase_6+0x94>
  401183:	mov    $0x6032d0,%edx          ; 本题的关键, 查看0x6032d0内存地址空间, 你会发现这里有一个nodex的段, 可以猜想是某种数据结构, 链表/tree/图等, 通过更多的x打印,会发现大概率是链表结构
  401188:	mov    %rdx,0x20(%rsp,%rsi,2)   ; 将链表的node地址,移动到stack空间去
  40118d:	add    $0x4,%rsi                ; rsi指针+1
  401191:	cmp    $0x18,%rsi               ; 是否走到数组尾
  401195:	je     4011ab <phase_6+0xb7>
  401197:	mov    (%rsp,%rsi,1),%ecx
  40119a:	cmp    $0x1,%ecx               ; 映射后的数组元素是否<=1
  40119d:	jle    401183 <phase_6+0x8f>
  40119f:	mov    $0x1,%eax               ;
  4011a4:	mov    $0x6032d0,%edx          ; 链表头节点地址
  4011a9:	jmp    401176 <phase_6+0x82>   ; 跳转到链表节点处理代码段
  -----------------4. 链表节点处理代码段(将链表节点地址移动到stack的内存中) end-----
  -----------------5. 更新链表next指针(按照映射后的数组元素连接) start-------------
  4011ab:	mov    0x20(%rsp),%rbx         ; 走到这里,说明0x6032d0内存地址中的所有链表node地址,都已经移动到了$rsp+0x20上的内存空间
  4011b0:	lea    0x28(%rsp),%rax
  4011b5:	lea    0x50(%rsp),%rsi
  4011ba:	mov    %rbx,%rcx
  4011bd:	mov    (%rax),%rdx
  4011c0:	mov    %rdx,0x8(%rcx)          ; 更新next指针
  4011c4:	add    $0x8,%rax               ; 下一个链表node
  4011c8:	cmp    %rsi,%rax               ; 是否移动到了最后一个node
  4011cb:	je     4011d2 <phase_6+0xde>
  4011cd:	mov    %rdx,%rcx
  4011d0:	jmp    4011bd <phase_6+0xc9>
  -----------------5. 更新链表next指针(按照映射后的数组元素连接) end-------------
  4011d2:	movq   $0x0,0x8(%rdx)          ; 走到这里,说明链表已经重新连接成功
  4011d9:
  4011da:	mov    $0x5,%ebp
  -----------------6. 验证链表是否是递减序列 start------------------------------
  4011df:	mov    0x8(%rbx),%rax
  4011e3:	mov    (%rax),%eax
  4011e5:	cmp    %eax,(%rbx)
  4011e7:	jge    4011ee <phase_6+0xfa>   ; 上一个node value >下一个node value才能通过,所以要求的递减序列
  4011e9:	callq  40143a <explode_bomb>
  4011ee:	mov    0x8(%rbx),%rbx          ; 指向下一个node
  4011f2:	sub    $0x1,%ebp               ; ebp = ebp-1 , ebp从5减到0
  4011f5:	jne    4011df <phase_6+0xeb>
  -----------------6. 验证链表是否是递减序列 end------------------------------
  4011f7:	add    $0x50,%rsp
  4011fb:	pop    %rbx
  4011fc:	pop    %rbp
  4011fd:	pop    %r12
  4011ff:	pop    %r13
  401201:	pop    %r14
  401203:	retq   

