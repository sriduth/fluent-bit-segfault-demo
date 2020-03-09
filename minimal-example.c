#include<stdio.h>
#include<sys/socket.h> 

/**
my_fn is compiled into the following:

f3 0f 1e fa             endbr64
55                      push   %rbp
48 89 e5                mov    %rsp,%rbp
90                      nop
5d                      pop    %rbp
c3                      retq
*/
static void my_fn() { return; }

static void (*my_function)() = 0;

__attribute__((section(".text")))
static const unsigned char page[4096] = {
					 0xf3, 0x0f, 0x1e, 0xfa,
					 0x55,         
					 0x48, 0x89, 0xe5,
					 0x90,
					 0x5d,
					 0xc3
};

int main(int argc , char *argv[])
{
  my_function = (void (*)())page;
  my_function();
  return 0;
}
