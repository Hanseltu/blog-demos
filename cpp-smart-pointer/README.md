# c++智能指针的用法

前言：最新看GitHub上的C++项目开源代码，很多都用到了诸如 `shared_ptr` 和 `unique_ptr` , 
之前在《C++ Primer Plus》里面看到过，但是没有太注意其在开源项目中的用法。在此总结记录一下，
本文将从以下几个方面进行总结

* 什么是智能指针？
* 为什么要使用智能指针？
* 智能指针如何使用？

## 什么是智能指针？

C/C++里面内存泄漏问题可能是程序员比较头疼的问题，智能指针主要用来解决C++中的内存泄漏问题。
智能指针的原理是，接受一个申请好的内存地址，构造一个保存在栈上的智能指针对象，当程序退出栈的作用域
范围后，由于栈上的变量自动被销毁，智能指针内部保存的内存也就被释放掉了（除非将智能指针保存起来）。
C++一共提供了四种智能指针：std::auto_ptr, std::shared_ptr, std::unique_ptr, std::weak_ptr，
，其中第一个auto_ptr是C++98提供的针对内存泄漏的解决方案，并在C++11中遗弃，后三个是C++11支持的智能
指针，在后续的版本中也在继续使用。注意使用时需添加头文件<memory>。

因为auto_ptr较老且已被遗弃，本文暂不考虑其使用方法，主要对后面三种方法进行说明。


## 为什么要使用智能指针？

那么为什么要使用智能指针呢，普通指针为什么不能很好的解决内存泄漏问题？先看下面的例子：

```c++
#include <iostream>
using namespace std;

void usePtr1(int *&p)
{
	*p = 5;
	cout << *p << endl;
	delete p;
}

int main(int argc, char **argv)
{
	int *p = new int;
	usePtr1(p);
	cout << *p << endl;
	return 0;
}
```

Output:

```
5
0
```

以上代码在 `main` 函数中 `new` 了块内存，但是在函数 `usePtr1` 调用后便释放了，`main` 函数
再次使用这块内存时讲得不到实际的值。我们可能有许多方案来解决这个问题，可以在主函数中调用完以后再对
这块内存进行删除，这样就不会出现这个问题了。以上考虑的只是一个简单的场景，当情况变得复杂时：如在主函数申请的内存被很多函数使用，主函数应该在什么
时候释放这块内存呢？程序员一不小心可能就会出错。（不释放堆上申请的内存都是对计算机耍流氓）

* 注：p指针存放在栈中，new出来的对象存放在堆上，p指针指向该对象，delete p 并不是删除P而是抹除p指向的堆上的对象。
delete p 后p指针其实还是存在的，但是为了不让p指向已经被抹除的对象，通常赋值NULL给p。

再看一个例子

```c++
void remodel(std::string & str)
{
    std::string * ps = new std::string(str);
    ...
    if (weird_thing())
        throw exception();
    str = *ps; 
    delete ps;
    return;
}
```

上面代码当出现异常时（weird_thing()返回true），delete将不被执行，因此将导致内存泄露。

这个时候，智能指针的用处就来了。

## 智能指针如何使用？

### shared_ptr 用法

* 初始化

可以通过构造函数、std::make_shared<T>辅助函数和reset方法来初始化shared_ptr：

```c++
#include "stdafx.h"
#include <iostream>
#include <future>
#include <thread>

using namespace std;
class Person
{
public:
    Person(int v) {
        value = v;
        std::cout << "Cons" <<value<< std::endl;
    }
    ~Person() {
        std::cout << "Des" <<value<< std::endl;
    }

    int value;

};

int main()
{
    std::shared_ptr<Person> p1(new Person(1));// Person(1)的引用计数为1

    std::shared_ptr<Person> p2 = std::make_shared<Person>(2);

    p1.reset(new Person(3));// 首先生成新对象，然后引用计数减1，引用计数为0，故析构Person(1)
                            // 最后将新对象的指针交给智能指针

    std::shared_ptr<Person> p3 = p1;//现在p1和p3同时指向Person(3)，Person(3)的引用计数为2

    p1.reset();//Person(3)的引用计数为1
    p3.reset();//Person(3)的引用计数为0，析构Person(3)
    return 0;
}
```

注意，不能将一个原始指针直接赋值给一个智能指针，如下所示，原因是一个是类，一个是指针。

`std::shared_ptr<int> p4 = new int(1);// error`

reset()包含两个操作。当智能指针中有值的时候，调用reset()会使引用计数减1.当调用reset（new xxx())重新赋值时，
智能指针首先是生成新对象，然后将就对象的引用计数减1（当然，如果发现引用计数为0时，则析构旧对象），
然后将新对象的指针交给智能指针保管。

* 获取原始指针

```c++
std::shared_ptr<int> p4(new int(5));
int *pInt = p4.get();
```

* 指定删除器

智能指针可以指定删除器，当智能指针的引用计数为0时，自动调用指定的删除器来释放内存。
std::shared_ptr可以指定删除器的一个原因是其默认删除器不支持数组对象，这一点需要注意。

> 需要注意的问题：
> * 不要用一个原始指针初始化多个shared_ptr，原因在于，会造成二次销毁，如下所示：
> ```c++
> int *p5 = new int;
> std::shared_ptr<int> p6(p5);> 
> std::shared_ptr<int> p7(p5);// logic error
>```
> * 不要在函数实参中创建shared_ptr。因为C++的函数参数的计算顺序在不同的编译器下是不同的。
>   正确的做法是先创建好，然后再传入。
>  `function(shared_ptr<int>(new int), g());`
> * 禁止通过shared_from_this()返回this指针，这样做可能也会造成二次析构。
> * 避免循环引用。智能指针最大的一个陷阱是循环引用，循环引用会导致内存泄漏。解决方法是AStruct或BStruct改为weak_ptr。

### unique_ptr 用法

unique_ptr，是用于取代c++98的auto_ptr的产物,在c++98的时候还没有移动语义(move semantics)的支持，
因此对于auto_ptr的控制权转移的实现没有核心元素的支持,但是还是实现了auto_ptr的移动语义，
这样带来的一些问题是拷贝构造函数和复制操作重载函数不够完美,具体体现就是把auto_ptr作为函数参数，
传进去的时候控制权转移，转移到函数参数，当函数返回的时候并没有一个控制权移交的过程，所以过了函数调用则原先的auto_ptr已经失效了。
在c++11当中有了移动语义，使用move()把unique_ptr传入函数，这样你就知道原先的unique_ptr已经失效了。
移动语义本身就说明了这样的问题，比较坑爹的是标准描述是说对于move之后使用原来的内容是未定义行为，并非抛出异常，所以还是要靠人肉遵守游戏规则。
再一个，auto_ptr不支持传入deleter，所以只能支持单对象(delete object)，而unique_ptr对数组类型有偏特化重载，并且还做了相应的优化，比如用[]访问相应元素等。

unique_ptr 是一个独享所有权的智能指针，它提供了严格意义上的所有权，包括：

* 拥有它指向的对象

* 无法进行复制构造，无法进行复制赋值操作。即无法使两个unique_ptr指向同一个对象。但是可以进行移动构造和移动赋值操作

* 保存指向某个对象的指针，当它本身被删除释放的时候，会使用给定的删除器释放它指向的对象

 unique_ptr 可以实现如下功能：

* 为动态申请的内存提供异常安全

* 讲动态申请的内存所有权传递给某函数

* 从某个函数返回动态申请内存的所有权

* 在容器中保存指针

* auto_ptr 应该具有的功能

使用例子

```c++
unique_ptr<Test> fun()
{
    return unique_ptr<Test>(new Test("789"));
}
int main()
{
    unique_ptr<Test> ptest(new Test("123"));
    unique_ptr<Test> ptest2(new Test("456"));
    ptest->print();
    ptest2 = std::move(ptest);//不能直接ptest2 = ptest
    if(ptest == NULL)cout<<"ptest = NULL\n";
    Test* p = ptest2.release();
    p->print();
    ptest.reset(p);
    ptest->print();
    ptest2 = fun(); //这里可以用=，因为使用了移动构造函数
    ptest2->print();
    return 0;
}
```

### weak_ptr 用法

weak_ptr是用来解决shared_ptr相互引用时的死锁问题，如果说两个shared_ptr相互引用，那么这两个指针的引用计数永远不可能下降为0，
资源永远不会释放。它是对对象的一种弱引用，不会增加对象的引用计数，和shared_ptr之间可以相互转化，shared_ptr可以直接赋值给它，
它可以通过调用lock函数来获得shared_ptr。

用法如下：

```c++

class B;
class A
{
public:
    shared_ptr<B> pb_;
    ~A()
    {
        cout<<"A delete\n";
    }
};
class B
{
public:
    shared_ptr<A> pa_;
    ~B()
    {
        cout<<"B delete\n";
    }
};
 
void fun()
{
    shared_ptr<B> pb(new B());
    shared_ptr<A> pa(new A());
    pb->pa_ = pa;
    pa->pb_ = pb;
    cout<<pb.use_count()<<endl;
    cout<<pa.use_count()<<endl;
}
 
int main()
{
    fun();
    return 0;
}
```

output：

```
2
2
```

可以看到fun函数中pa ，pb之间互相引用，两个资源的引用计数为2，当要跳出函数时，智能指针pa，pb析构时两个资源引用计数会减一，
但是两者引用计数还是为1，导致跳出函数时资源没有被释放（A B的析构函数没有被调用），如果把其中一个改为weak_ptr就可以了，
我们把类A里面的shared_ptr<B> pb_; 改为weak_ptr<B> pb_; 运行结果如下，这样的话，资源B的引用开始就只有1，当pb析构时，
B的计数变为0，B得到释放，B释放的同时也会使A的计数减一，同时pa析构时使A的计数减一，那么A的计数为0，A得到释放。

output:

```
1
2
B delete
A delete
```

注意的是我们不能通过weak_ptr直接访问对象的方法，比如B对象中有一个方法print(),我们不能这样访问，pa->pb_->print(); 
英文pb_是一个weak_ptr，应该先把它转化为shared_ptr,如：shared_ptr<B> p = pa->pb_.lock(); p->print();

参考：

https://www.cnblogs.com/jiayayao/p/6128877.html

https://blog.csdn.net/snail_hunan/article/details/43086189

https://www.cnblogs.com/tenosdoit/p/3456704.html