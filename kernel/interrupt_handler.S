.global lapic_intr_handler
lapic_intr_handler:
  push %rax
  push %rbx
  push %rcx
  push %rdx
  push %rdi
  push %rsi
  push %rbp
  push %r8
  push %r9
  push %r10
  push %r11
  push %r12
  push %r13
  push %r14
  push %r15
  mov %rsp, %rdi
  call lapic_intr_handler_internal
  mov 0x0(%rax)	, %rsp
  mov 0x8(%rax), %rbx
  mov %rbx, %cr3
  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rbp
  pop %rsi
  pop %rdi
  pop %rdx
  pop %rcx
  pop %rbx
  pop %rax
  iretq

.global syscall_handler
syscall_handler:
  push %rbx
  push %rcx
  push %rdx
  push %rdi
  push %rsi
  push %rbp
  push %r8
  push %r9
  push %r10
  push %r11
  push %r12
  push %r13
  push %r14
  push %r15
  call syscall_handler_internal
  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rbp
  pop %rsi
  pop %rdi
  pop %rdx
  pop %rcx
  pop %rbx
  iretq

.global page_fault_handler
page_fault_handler:
  push %rax
  push %rbx
  push %rcx
  push %rdx
  push %rdi
  push %rsi
  push %rbp
  push %r8
  push %r9
  push %r10
  push %r11
  push %r12
  push %r13
  push %r14
  push %r15
  mov %rsp, %rdi
  call page_fault_handler_internal
  pop %r15
  pop %r14
  pop %r13
  pop %r12
  pop %r11
  pop %r10
  pop %r9
  pop %r8
  pop %rbp
  pop %rsi
  pop %rdi
  pop %rdx
  pop %rcx
  pop %rbx
  pop %rax
  add $0x8, %rsp
  iretq

.global end_of_schedule


