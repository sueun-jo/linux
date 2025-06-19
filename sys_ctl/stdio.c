#include <stdio.h>

int main(void)
{
  char str[BUFSIZ], out[BUFSIZ];
  scanf("%s", str);
  printf("1 : %s\n", str);
  sprintf(out, "2 : %s ", str);
  perror(out);
  return 0;
}
