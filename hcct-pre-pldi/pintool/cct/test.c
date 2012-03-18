#include <stdio.h>

void f(), g(), h();

void f(){
    g(1);
    h();
    g(0);
}

void g(int x){
    if (x) h();
    else g(1);
}

void h(){
}

int main(){
    f();
    return 0;
}

