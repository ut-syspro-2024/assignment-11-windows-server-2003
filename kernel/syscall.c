#include "syscall.h"
#include "util.h"

unsigned long long syscall_puts(char* str) {
  puts(str);
  return 0;
}

unsigned long long syscall_handler_internal(unsigned long long syscall_id,
    unsigned long long arg1, unsigned long long arg2, unsigned long long arg3) {
  unsigned long long ret;

  switch (syscall_id) {
    case SYSCALL_PUTS:
      ret = syscall_puts((char *)arg1);
      break;
    default:
      puts("unknown syscall: ");
	  puth(syscall_id, 4);
	  puts("\n");
      break;
  }

  return ret;
}

