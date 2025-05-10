#include <stdio.h>
void add(int* a){
    *a = *a+100;
}

int main(){
    int x[5] = {11, 12, 13, 14, 15};

    printf("%p\n", x); // 15
    printf("%p\n", &x);
    printf("%p\n", &x[0]);
    printf("%d\n", *x);
    printf("%d\n", x[0]);

}