#include <iostream>
namespace abc{
    int x = 10;
    int add_number(int a, int b){
        return a+b;
    }
}
namespace abc{
    int y = 20;
    int mul_number(int a, int b){
        return a*b;
    }
}
int main(){
    std::cout<<"starting of the code"<<std::endl;
    std::cout<<abc::mul_number(10,20)<<std::endl;
    std::cout<<"x: "<<abc::x<<" y: "<<abc::y<<std::endl;
    std::cout<<"End of the code"<<std::endl;
}