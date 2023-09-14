#pragma once
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<sys/time.h>
#include<sys/resource.h>

#include "../comm/log.hpp"
#include "../comm/util.hpp"
namespace ns_runner
{
    using namespace ns_log;
    using namespace ns_util;
    class Runner
    {
    public:
        Runner() {}
        ~Runner() {}

    public:
    static void SetProcLimit(int _cpu,int _mem)
    {
        //设置进程占用资源大小的接口
        // 设置CPU时长
        struct rlimit cpu_rlimit;
        cpu_rlimit.rlim_max=RLIM_INFINITY;//硬约束设置成无上限
        cpu_rlimit.rlim_cur=_cpu;//软约束只要不超硬约束就行
        setrlimit(RLIMIT_CPU,&cpu_rlimit);
        // 设置内存大小（KB）
        struct rlimit mem_rlimit;
        mem_rlimit.rlim_max=RLIM_INFINITY;
        mem_rlimit.rlim_cur=_mem*1024;
        setrlimit(RLIMIT_AS,&mem_rlimit);
    }
        // 指明文件名即可，不需要路径和后缀
        //返回如果>0,程序异常收到了信号，返回值就是对应的信号编号
        // 返回值==0，正常跑完，结果都放到了临时文件中，结果是什么不关心
        // 返回值<0,内部错误（打开文件，创建子进程失败）
        //CPU最大资源上限，可以使用的内存大小
        static int Run(const std::string &file_name,int cpu_limit,int mem_limit)
        {

            //知晓可执行程序名,放到临时文件中保存运行结果
            std::string _execute = PathUtil::Exe(file_name);
            std::string _stdin = PathUtil::Stdin(file_name);
            std::string _stdout = PathUtil::Stdout(file_name);
            std::string _stderr = PathUtil::Stderr(file_name);

            //将文件打开，可以被子进程继承
            umask(0);
            int _in_fd = open(_stdin.c_str(), O_CREAT | O_RDONLY, 0644);
            int _out_fd = open(_stdout.c_str(), O_CREAT | O_WRONLY, 0644);
            int _err_fd = open(_stderr.c_str(), O_CREAT | O_WRONLY, 0644);

            //有一个打开文件失败，运行时信息无法获取
            if (_in_fd < 0 || _out_fd < 0 || _err_fd < 0)
            {
                LOG(ERROR)<<"运行时打开标准文件失败"<<"\n";
                return -1;
            }

            //只考虑是否正确运行完毕
            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR)<<"运行时创建子进程失败"<<"\n";
                close(_in_fd);
                close(_out_fd);
                close(_err_fd);
                return -2; //代表创建子进程失败
            }
            else if (pid == 0)
            {
                //子进程执行，数据都重定向到三个文件当中
                dup2(_in_fd, 0);
                dup2(_out_fd, 1);
                dup2(_err_fd, 2);
                //CPU和内存大小的限制
                SetProcLimit(cpu_limit,mem_limit);
                //带路径的可执行文件
                // 我要执行谁，想在命令行上如何执行（全路径的指明也可以执行）
                execl(_execute.c_str(), _execute.c_str(), nullptr);
                exit(1);
            }
            else
            {
                close(_in_fd);
                close(_out_fd);
                close(_err_fd);
                //不关心退出码，是关心是否是异常
                int status = 0;
                waitpid(pid, &status, 0); //阻塞等待
                LOG(INFO)<<"运行完毕"<<(status & 0x7F)<<"\n";
                
                return status & 0x7F;
            }
        }
    };
}