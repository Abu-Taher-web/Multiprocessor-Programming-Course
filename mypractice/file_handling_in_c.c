#include <stdlib.h>
#include <stdio.h>

int main(){
    int a = 10;
    int* p = &a;
    printf("%lu", sizeof(*p));
    

}