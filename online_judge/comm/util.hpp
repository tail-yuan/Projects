#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include<fstream>
#include <sys/stat.h>
#include<vector>
#include<atomic>
#include<boost/algorithm/string.hpp>
#include <unistd.h>
#include <sys/time.h>
//提供公共工具
namespace ns_util
{
    const std::string temp_path = "./temp/";

    class TimeUtil
    {
    public:
        static std::string GetTimeStamp()
        {
            // C->time函数
            //输出型参数timeval，所以需要传入
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec);
        }
        // 获得毫秒级别时间戳
        static std::string GetTimeMs()
        {
            struct timeval _time;
            // 输出型参数，所以需要传入timeval结构
            gettimeofday(&_time,nullptr);//第二个是时区，默认是nullptr就行

            return std::to_string(_time.tv_sec*1000+_time.tv_usec/1000);
        }
    };

    class PathUtil //添加文件后缀和路径
    {
    public:
        //添加后缀
        static std::string AddSuffix(const std::string &file_name, const std::string &suffix)
        {
            std::string path_name = temp_path;
            path_name += file_name;
            path_name += suffix;
            return path_name;
        }
        //编译时需要的临时文件
        // 构建源文件路径 + 后缀的完整文件名
        // 1234 -> ./temp/1234.cpp
        static std::string Src(const std::string &file_name)
        {
            return AddSuffix(file_name, ".cpp");
        }
        // 构建可执行程序的路径名+后缀
        // 1234 -> ./temp/1234.exe
        static std::string Exe(const std::string &file_name)
        {
            return AddSuffix(file_name, ".exe");
        }
        // 构建程序对应的标准错误完整路径名+后缀名
        // 1234 -> ./temp/1234.compile_err
        static std::string CompilerError(const std::string &file_name)
        {
            return AddSuffix(file_name, ".compile_err");
        }

        // 运行时需要的临时文件
        static std::string Stderr(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stderr");
        }
        static std::string Stdin(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stdin");
        }
        static std::string Stdout(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stdout");
        }
    };
    class FileUtil
    {
    public:
        static bool IsFileExits(const std::string &path_name)
        {
            //获取文件属性成功与否
            struct stat st;
            if (stat(path_name.c_str(), &st) == 0)
            {
                //获取文件属性成功
                return true;
            }
            return false;
        }
        static bool WriteFile(const std::string &target, const std::string&code)
        {
            std::ofstream out(target);//默认是输出
            if(!out.is_open())
            {
                return false;
            }
            out.write(code.c_str(),code.size());
            out.close();
            return true;
        }
        //形成唯一的文件名，毫秒值时间戳和原子性递增唯一值
        static std::string UniqFileName()
        {
            static std::atomic_uint id(0);
            id++;
            std::string ms=TimeUtil::GetTimeMs();
            std::string uniq_id =std::to_string(id);
            return ms+"_"+ uniq_id;

        }
        static bool ReadFile(const std::string &target,std::string * content, bool keep=false) //读取编译错误的文件，返回内容给用户
        {
            (*content).clear();
            // std::ifstream in(target,std::ios::in);
            std::ifstream in(target);
            if(!in.is_open())
            {
                return false;

            }
            std::string line;
            // 不保存行分隔符，有些时候需要保留\n,传参时传入是否保留\n的Keep参数
            // 内部重载了强制类型转换，返回值变成bool
            while(std::getline(in,line))//从流里面按行读取到line，
            {
                (*content)+=line;
                (*content) += (keep ? "\n":"");
            }
            in.close();
            return true;

        }
    };
    class StringUtil
    {
        public:
        // 切分字符串，目标切分字符串，切分之后的部分放到vector中(输出型)，指定的分隔符
        static void SplitString(std::string& str, std::vector<std::string>* target,std::string sep)
        {
            //boost split方法
             boost::split((*target),str,boost::is_any_of(sep),boost::algorithm::token_compress_on);

        }
    };

}