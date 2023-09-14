#pragma once
#include<iostream>
#include<string>
#include"util.hpp"
namespace ns_log
{
    using namespace ns_util;
    // 日志等级
    // 字符常量设置等级
    enum{
        INFO,//就是整数
        DEBUG,
        WARNING,
        ERROR,
        FATAL
    };
    // 打印日志
    // 输出流存在缓冲区
    //加快函数跳转，设置成内联函数
    // Log()<<"message"所以开放式日志得是ostream
    inline std::ostream & Log(const std::string & level,const std::string&file_name,int line)
    {
        // 添加日志等级
        std::string message="[";
        message+=level;
        message+="]";
        // 添加报错文件名称
        message+="[";
        message+=file_name;
        message+="]";
        // 添加报错行
        message+="[";
        message+=std::to_string(line);//整数转字符串
        message+="]";
        // 添加日志时间戳->获取时间戳
        message+="[";
        message+=TimeUtil::GetTimeStamp();
        message+="]";

        // cout 本质内部是包含缓冲区的
        std::cout<<message;//不引进endl进行刷新缓冲区，便于后续追加错误信息
        return std::cout;
    }
    //只想传level
    // 开放式日志
    // LOG<<"message"<<"\n"
    // 只需要使用LOG就可以

    #define LOG(level) Log(#level,__FILE__,__LINE__) //将宏参带#可以将宏名称也就是日志等级以字符串的形式进行打印
    // 使用时只用LOG(1)，使用宏替换的方式。因为是默认插到相应的位置，所以所处的文件和行号就不用显示的传了。
}