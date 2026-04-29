#### 二维数组名为什么不能直接赋值给二级指针

二维数组名是指向数组的指针，而二级指针是指向指针的指针，两者不是同一类型

#### c++内存布局

![img](https://pica.zhimg.com/v2-bf4b7a692d98b1c46694ca8fc74bc158_r.jpg)

栈：保存函数局部变量、参数和返回值

堆：动态分配内存，用户手动申请和释放

bss段：（Block Started by Symbol）存放未初始化的全局变量和静态变量（以及初始化为0的）

data段：存放已初始化且初始化不为0的全局变量和静态变量

text段：程序代码在内存中的映射，存放函数体的二进制代码

#### 在成员函数中调用delete this会出现什么问题？

​	在类对象的内存空间中，只有数据成员和虚函数表指针，类的成员函数单独放在代码段中。在调用成员函数时，隐含传递一个this指针，让成员函数知道当前是哪个对象在调它。

​	调用delete this时，类对象的内存空间被释放。在delete this之后进行的其他操作，只要不涉及this指针，都能够正常运行。一旦涉及到this指针，会出现不可预期的问题。

#### 在析构函数中调用delete this会发生什么？

​	delete this会去调用对象析构函数，而析构函数又调用delete this，形成无限递归。

#### 构造函数可以是虚函数吗？

如果构造函数是虚函数，那么调用构造函数需要通过vptr找到vtable。但在构造函数执行完成之前，vptr还没有指向正确的vtable。这意味着你必须先运行构造函数才能使用虚函数机制，但虚构造函数却要求你先有虚函数机制才能运行。

#### 哪些函数不能是虚函数

1. 构造函数
2. 友元函数：只有类的成员函数才能是虚函数
3. static成员函数：虚函数的调用需要this指针，而static成员函数无this指针
4. 内联函数：内联函数是没有地址的，编译的时候直接展开了
5. 成员函数模板：c++ 编译器在解析一个类的时候就要确定虚函数表的大小，如果允许一个虚函数是模板函数，那么compiler就需要在parse这个类之前扫描所有的代码，找出这个模板成员函数的调用（实例化），然后才能确定vtable的大小，而显然这是不可行的。模板成员函数在调用的时候才会创建

#### 构造函数或者析构函数中调用虚函数会发生什么？

首先不应该在构造函数和析构函数中调用虚函数

在构造函数中调用虚函数，创建派生类对象时，先执行基类的构造函数，在去执行派生类的构造函数。在执行基类构造函数中的虚函数时，派生类没有被构造出来，编译器会认为是基类对象在调用虚函数，执行的是基类的虚函数，达不到动态绑定的效果

在析构函数中调用虚函数时，先执行派生类的析构函数，在去执行基类的析构函数。在执行基类析构函数的虚函数时，派生类的内容已经被析构，编译器会认为是基类对象在调用虚函数，执行的是基类的虚函数

```c++
class Base
{
public:
    Base(){ func(); }
    virtual void func(){cout << "Base" << endl;}
    virtual void xu(){cout << "~Base" << endl;}
    ~Base(){xu();}
};

class Derive : public Base
{
public:
    Derive()
    {}
    virtual void func(){cout << "Derive" << endl;}
    virtual void xu(){cout << "~Derive" << endl;}
    ~Derive(){}
};

int main()
{
    Derive d;
    // 执行结果
    //  Base
    // ~Base
}
```

#### static的作用是什么？

1. 修饰普通变量时，改变其生命周期，由栈区延长为整个程序运行期间。若已初始化，则存放在.data段，否则存放在.bss段
2. 修饰类成员变量时，该成员变量属于类，而不属于具体哪个对象
3. 修饰普通函数时，限制作用域为当前文件
4. 修饰成员函数时，该函数属于类，而不属于具体哪个对象。无this指针，只能访问静态成员函数和成员变量

#### 为什么要进行内存对齐？

为了提高数据读取效率。若没有内存对齐，读取一个4字节的int时，存放位置可能是3 - 6字节，但是cpu取数据是以4字节为单位的，这时候需要取两次数据。而进行内存对齐后，存放位置为4 -7字节，只需取一次数据就行。有些处理器严格要求内存对齐，若没有内存对齐直接报错。

#### 什么是虚函数，它的作用是什么？

在类的成员函数中，用virtual修饰的函数即为虚函数。当子类继承父类时，可通过重写虚函数是实现多态的必要条件

#### 虚函数的底层实现机制是什么？

当类中存在虚函数时，编译器会为该类生成虚函数表，表中存放虚函数的地址。对象中会存在虚函数表指针，指向类的虚函数表。子类会继承父类的虚函数表，当子类重写某个虚函数时，将重写后的虚函数地址覆盖继承而来的虚函数地址。当通过基类指针或引用调用虚函数时，基类指针指向父类就执行父类的虚函数，指向子类就执行子类的虚函数，以此实现多态

#### 动态绑定和静态绑定的区别是什么？

动态绑程序运行时确定调用函数，静态绑定在编译期间就确定调用函数

#### std::map是怎么实现的？它的特性和复杂度是多少？



#### std::map和std::unordered_map的区别是什么，为什么设计这两个不同的数据结构？各自的使用场景是什么？



#### std::vector细节：

- 访问元素的方式（[] 与 .at()）有什么区别？

- 如何释放 vector 占用的多余内存？

- reserve（预留空间）、resize和 clear 这三个函数的区别是什么？

  把 capacity 至少扩充到 n。若 n > 当前 capacity 则扩容；若 n < 当前 capacity 则不缩容（强制保留），不影响现有元素。

  强制将元素数量改为 n。

  1. 若 n > 旧 size：新增元素并初始化（默认 0，也可以用其他值初始化）。

  2. 若 n < 旧 size：删除多出的元素。

  3. 若 n > 旧 capacity：自动扩容。

  把所有元素析构，size 归零，但保留底层数组（capacity 不变）。内存不会还给系统。

#### explicit关键字



#### static_cast和dynamic_cast

static_cast：**编译期**转换，用于**良性、安全、明确**的类型转换，没有运行时检查，**不保证安全**

​	用于基本数据类型之间的转换

​	void*和其他类型指针之间的转换

​	子类对象的指针转换成父类对象指针

​	以上三个都是隐式转换，最好把所有隐式转换都用static_cast代替

dynamic_cast：**运行期**转换，**主要用于多态类型的安全向下转换**，必须要有**虚函数**，否则不能用

​	只用于对象的指针和引用，主要用于执行“安全的向下转型”，因为downcast是不安全的

​	dynamic_cast是唯一一个在运行时处理的，因为我们转换后的指针如果请求一块无效的内存的话是会报错的，但是用该强转后会返回null，即转换成功会返回引用或者指针，失败返回null。如果一个引用类型执行了类型转换并且这个转换是不可能的，运行时一个 `bad_cast`的异常类型会被抛出。

#### move，移动构造函数和拷贝构造函数的区别，emplace_back和push_back的区别



#### struct和class的区别？

struct 和 class 在 C++ 中唯一区别是：struct 默认访问权限和默认继承权限都是 public，class 默认都是 private。其余功能完全相同。

#### 虚表存在哪个地方?

虚表（vtable）属于类，存在 **只读数据段**，虚表在**编译期就生成好了**

#### 进程地址空间，除了堆、栈，还有这些分区

​	      代码段（.text）：存放程序的二进制代码、函数指令，只读、可执行

​		  只读数据段（.rodata）：存放常量：字符串常量、`const` 全局 / 静态常量，虚表也放在这里

​          数据段（.data）：存放已初始化的全局变量、静态变量

​		  BSS 段（.bss）：存放未初始化的全局变量、静态变量程序运行前自动清 0

​          堆（heap）：`new/malloc` 分配，向上生长

​          栈（stack）：局部变量、函数调用、返回地址，向下生长

#### C++为什么需要完美转发 std::forward

```c++
void f(const string &s)
{ cout << "left " << s << endl; }

void f(string &&s)
{ cout << "right " << s << endl; }

void g(const string &s)
{ f(s); }

void g(string &&s)	// 原因在于右值引用本质上是左值，是可以取地址的即 f(s) 中s是左值
{ f(s); }

int main()
{
    g(string("hello1"));	// 结果是：left hello1
    string s1("hello2");
    g(s1);					// 结果是：left hello2

    return 0;
}
// 我们希望 g(string("hello1")) 调用的应该是 f(string &&s)，g(s1) 调用的应该是 f(const string &s)
// 但实际上调用的都是 f(const string &s)


// 为了达到我们想要的效果，需要进行类型转换
void f(const string &s)
{ cout << "left " << s << endl; }

void f(string &&s)
{ cout << "right " << s << endl; }

void g(const string &s)
{ f(static_cast<const string &>s); }	// 调用的是f(const string &s)

void g(string &&s)	
{ f(static_cast<string &&>s); }	// 调用的是f(string &&s)


// 但实际上不会去使用static_cast进行类型转换，而是使用std::forward
void f(const string &s)
{ cout << "left " << s << endl; }

void f(string &&s)
{ cout << "right " << s << endl; }

void g(const string &s)
{ f(std::forward<const string &>s); }	// 调用的是f(const string &s)

void g(string &&s)	
{ f(std::forward<string &&>s); }	// 调用的是f(string &&s)


// 如果函数g()有n个参数，需要有2^n个重载函数
g(const string &s1, string &&s2)
{
    f(std::forward<const string &>s1);
    f(std::forward<string &&>s1);
}

// 考虑写成模板(以一个参数为例)
template<class T>
void g(T&& v)	// 这里不是右值引用，而是万能引用。如果传入实参是左值，那么T就被推导为实参引用类型T&
{				// 如果是右值，那么T就被推导为 实参的非引用类型T
    f(std::forward<T>(v));
}
```



#### 如何定义一个只能在堆上（栈上）生成对象的类

```c++
class A	// 堆上创建
{
public:
    // 工厂函数：唯一创建入口
    static A* create()
    {
        return new A();
    }

    // 提供销毁接口
    void destory()
    {
        delete this;
    }

protected:
    A(){};  // 外部不能栈上创建
    ~A(){}; // 外部不能 delete
};
```

```c++
class B	// 栈上创建
{
public:
    B(){};
    ~B(){};
    void* operator new(size_t size) = delete;
    void operator delete(void *b) = delete;   // 重载了new就需要重载delete
private:
};
```

#### 构造函数析构函数是否能抛出异常



#### std::make_shared()与std::shared_ptr()的区别

**`std::shared_ptr<T>(new T())`：** 进行**两次**独立的内存分配。（为对象 `T` 分配一次内存。为**控制块**（存储引用计数、弱引用计数等）分配一次内存。）

![img](https://i-blog.csdnimg.cn/blog_migrate/6f9b0fdf4c2cb105d68a117842c32276.png)

![img](https://i-blog.csdnimg.cn/blog_migrate/2454639141fbba29601631486118ccb8.png)

内存分配的动作, 可以一次性完成. 这减少了内存分配的次数, 而内存分配是代价很高的操作。

`make_shared` 只分配一次内存, 这看起来很好. 减少了内存分配的开销. 问题来了, `weak_ptr` 会保持控制块(强引用, 以及弱引用的信息)的生命周期 , 而因此连带着保持了对象分配的内存, 只有最后一个 `weak_ptr` 离开作用域时, 内存才会被释放. 原本强引用减为 0 时就可以释放的内存, 现在变为了强引用, 若引用都减为 0 时才能释放, 意外的延迟了内存释放的时间. 这对于内存要求高的场景来说, 是一个需要注意的问题. 

#### 什么不能被继承

构造函数、析构函数、赋值运算符=、final关键字、构造函数和析构函数在private作用域内

#### shared_ptr是线程安全的吗？

同一个shared_ptr被多个线程“读”是安全的。2、同一个shared_ptr被多个线程“写”是不安全的。3、共享引用计数的不同的shared_ptr被多个线程”写“ 是安全的

#### 为什么需要继承std::enable_shared_from_this<T>?

```
class Foo {
public:
    void func() {
        std::shared_ptr<Foo> p(this); // 危险！
    }
};

如果对象本来已经被别的 shared_ptr 管理：
auto sp = std::make_shared<Foo>();
sp 一个控制块（引用计数器A）
p 一个新控制块（引用计数器B）
```

enable_shared_from_this它让对象内部知道：“我已经被哪个 shared_ptr 控制块管理着”。调用：shared_from_this()，得到的是：和原 shared_ptr 共用控制块的新 shared_ptr

