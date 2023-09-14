// 设计测试用例
//编译的时候，会将这三行去掉，仅仅是为了方便我们设计测试用例
#ifndef COMPILER_ONLINE
#include "header.cpp"
#endif

void Test1()
{
    //定义临时对象来完成方法的调用
    bool ret = Solution().isPalindrome(121);
    if (ret)
    {
        std::cout << "Test1 ok!" << std::endl;
    }
    else
    {
        std::cout << "Test1 failed! input: 121, output expected true, actual false" << std::endl;
    }
}
void Test2()
{
    bool ret = Solution().isPalindrome(-10);
    if (!ret)
    {
        std::cout << "Test2 ok!" << std::endl;
    }
    else
    {
        std::cout << "Test2 failed! input: -10, output expected false, actual true" << std::endl;
    }
}
int main()
{
    Test1();
    Test2();
    return 0;
}
