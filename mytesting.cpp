#include <iostream>
#include <vector>
using namespace std;

class abc{
    public:
    char letter;
    int count;
    abc(char a, int b):letter(a), count(b){};
};

int main(){
    vector<abc> xyz;
    xyz.emplace_back('a', 10);
}