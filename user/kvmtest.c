// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int
main(void)
{
  printf("kvmtest: start\n");
  if(checkvm()){
    printf("kvmtest: OK\n");
  } else {
    printf("kvmtest: FAIL Kernel is using global kernel pagetable\n");
  }
  exit(0);
}
