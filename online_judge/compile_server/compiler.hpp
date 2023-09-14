#pragma once
#include<iostream>
#include<unistd.h>
#include<sys/wait.h>
#include "../comm/util.hpp"
#include<fcntl.h>
#include"../comm/log.hpp"

namespace ns_compiler
{
    using namespace ns_util;
    using namespace ns_log;

    class Compiler
    {
        public:
        Compiler(){}
        ~Compiler(){}
        //输入参数，编译的文件名，编译成功和失败返回值bool 
        // 传来文件名1234，不需要后缀和路径，内部自行拼接后缀和路径
        //临时文件都放在temp文件夹中，需要一下三种文件
        // 1234 -> ./temp/1234.cpp
        // 1234 -> ./temp/1234.exe
        // 1234 -> ./temp/1234.stderr
        static bool Compile(const std::string & file_name)
        {
            pid_t res= fork();
            if(res<0)
            {
                LOG(ERROR)<<"内部错误，创建子进程失败"<<"\n";
                return false;//错误不再打印cout,而是引进日志功能
            }
            else if(res==0)//调用其他程序g++完成编译功能
            {
                // 进行程序替换之前，打开错误文件
                //我有读取写入权限，其他人有读取权限0644
                umask(0);//使得不受平台影响权限设置
                int _stderr = open(PathUtil::CompilerError(file_name).c_str(), O_CREAT | O_WRONLY,0644);
                if(_stderr <0)
                {
                    LOG(WARNING)<<"没有成功形成stderr文件"<<"\n";
                    exit(1);
                }
                //重定向标准错误到_stderr,本来是2号（标准错误）的显示器文件信息转打印到_stderr上
                dup2(_stderr,2);

                //程序替换并不影响进程文件描述符表
                //进程替换，g++ -o .exe .cpp -std=c++11
                //选择带名称的不带不路径的
                // 我要执行谁（g++）,我想怎么执行（后续代码）
                execlp("g++","g++","-o",PathUtil::Exe(file_name).c_str(),\
                PathUtil::Src(file_name).c_str(),"-D","COMPILER_ONLINE","-std=c++11",nullptr/*可变参数列表命令行参数传完了最后一个是NULL结尾*/);
                    //条件编译，有了COMPILER_ONLINE就会再拼接代码时，将头文件去掉
                LOG(ERROR)<<"启动G++失败可能是参数错误"<<"\n";
                exit(2);
            }
            else
            {
                //父进程得等待子进程的编译结果
                waitpid(res,nullptr,0);
                //编译是否成功，有没有形成可执行程序
                if(FileUtil::IsFileExits(PathUtil::Exe(file_name)))
                {
                    LOG(INFO)<<PathUtil::Src(file_name)<<"编译成功"<<"\n";
                    return true;
                }
            }
            LOG(ERROR)<<"编译失败，没有形成可执行程序"<<"\n";
            //编译出错，我想要信息
            return false;
        } 
    };
}