#pragma once
#include "compiler.hpp"
#include "runner.hpp"
#include "../comm/log.hpp"
#include "../comm/util.hpp"
#include <jsoncpp/json/json.h>
#include <signal.h>
#include<unistd.h>
namespace ns_compile_run
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_compiler;
    using namespace ns_runner;
    class CompileAndRun
    {
    public:
        // code>0,收到信号导致异常崩溃
        // code<0,整个过程非运行报错，下面那些
        // code=0，真个过程全部完成
        static std::string CodeToDesc(int code,const std::string &file_name )
        {
            std::string desc;
            switch (code)
            {
            case 0:
                desc = "编译运行成功";
                break;
            case -1:
                desc = "提交代码为空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3:
                FileUtil::ReadFile(PathUtil::CompilerError(file_name),&desc,true);
                break;
             case SIGABRT: // 6
                desc = "内存超过范围";
                break;
            case SIGXCPU://24
                desc = "CPU使用超时";
                break;
            case SIGFPE: // 8
                desc = "浮点数溢出";//除0错误
                break;
            case SIGSEGV: // 11
                desc = "段错误";
                break;
            default:
                desc = "未知错误"+ std::to_string(code);
                break;
            }
            return desc;
        }
        static void RemoveTempFile(const std::string &file_name)
        {
            //临时文件的个数时不确定的，因为错误原因不同导致的
            std::string _src=PathUtil::Src(file_name);
            //在temp路径下存在才删除
            if(FileUtil::IsFileExits(_src))unlink(_src.c_str());
            std::string complier_error =PathUtil::CompilerError(file_name);
            if(FileUtil::IsFileExits(complier_error)) unlink(complier_error.c_str());

            std::string _execute = PathUtil::Exe(file_name);
            if(FileUtil::IsFileExits(_execute)) unlink(_execute.c_str());

            std::string _stdin = PathUtil::Stdin(file_name);
            if(FileUtil::IsFileExits(_stdin)) unlink(_stdin.c_str());

            std::string _stdout = PathUtil::Stdout(file_name);
            if(FileUtil::IsFileExits(_stdout)) unlink(_stdout.c_str());

            std::string _stderr = PathUtil::Stderr(file_name);
            if(FileUtil::IsFileExits(_stderr)) unlink(_stderr.c_str());
        }
        // 输入：
        // input：用户给自己提交的代码对应的输入，scanf接受的
        // code:用户提交的代码
        // cpulimit
        // memlimit
        // 输出：
        // status：状态码
        // reason：请求结果
        // 我的程序运行完的结果以及运行完的错误结果
        static void Start(const std::string &in_json, std::string *out_json)
        {
            // 将字符串解析成key-val值
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value);//解析谁，解析到哪里

            std::string code = in_value["code"].asString();
            std::string input = in_value["input"].asString();

            int cpu_limit = in_value["cpu_limit"].asInt();
            int mem_limit = in_value["mem_limit"].asInt();

            int status_code = 0;
            Json::Value out_value;
            std::string file_name;
            int run_result = 0;

            if (code.size() == 0)
            {
                // 差错处理

                //"用户提交的代码是空的";
                status_code = -1;
                goto END;
            }
            //形成唯一文件名，没有目录和后缀
            // 毫秒级时间戳+原子性递增的唯一值，来保证唯一性
            file_name = FileUtil::UniqFileName();

            //添加后缀，形成源文件到temp路径下的code中
            // 写入文件失败
            if (!FileUtil::WriteFile(PathUtil::Src(file_name), code)) //形成临时src源文件

            {
                // "提交的代码发生了位置错误";
                status_code = -2;
                goto END;
            }
            //编译并运行
            if (!Compiler::Compile(file_name))
            {
                // 编译失败
                status_code = -3;
                goto END;
            }

            run_result = Runner::Run(file_name, cpu_limit, mem_limit);
            if (run_result < 0) //内部错误
            {
                status_code = -2;
            }
            else if (run_result > 0)
            {
                //运行崩溃
                status_code = run_result;
            }
            else
            {
                //运行成功
                status_code = 0;
            }

        //将差错处理统一处理
        END:
            //根据status_code返回序列化outValue
            out_value["status"] = status_code;
            out_value["reason"] = CodeToDesc(status_code,file_name); //错误码描述
            if (status_code == 0)
            {
                // 整个过程全部成功
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name),&_stdout,true);
                out_value["stdout"] = _stdout;

                std::string _stderr;
                FileUtil::ReadFile(PathUtil::Stderr(file_name),&_stderr,true);
                out_value["stderr"] = _stderr;
            }
            //序列化
            Json::StyledWriter writer;
            *out_json = writer.write(out_value);

            RemoveTempFile(file_name);
        }
    };
}