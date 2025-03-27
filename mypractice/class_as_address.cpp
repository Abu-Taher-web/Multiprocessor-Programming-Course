#include <iostream>
#include <memory>  // Required for smart pointers
using namespace std;

class myclass{
    public:
    int a;
    int b;
    myclass(int x, int y):a(x), b(y){};
};

myclass func(int a, int b){
    return myclass(a, b);
}

int main() {
    myclass x = func(3, 4);
    cout<<x<<endl;
}
