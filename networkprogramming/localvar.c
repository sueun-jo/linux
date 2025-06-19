#include <stdio.h>

int a; //전역이니까 0으로 초기화가 되고 BSS영역

int main (void){
    int b;
    
    printf( "%d %d",a, b);
    return 0;

}