#include <iostream>
using namespace std;

void usePtr1(int *p)
{
    *p = 5;
    cout << *p << endl;
    delete p;
    cout << *p << endl;
}

int main(int argc, char **argv)
{
    int *p = new int;
    //usePtr1(p);
    *p = 5;
    delete p;
    cout << *p << endl;
    return 0;
}