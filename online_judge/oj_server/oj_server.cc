#include<iostream>
#include<signal.h>
#include "../comm/httplib.h"
#include"oj_control.hpp"
using namespace ns_control;
using namespace httplib;

static Control *ctrl_ptr=nullptr;
//当收到信号之后就重新上线所有服务器
void Recovery(int sigo)
{
    ctrl_ptr->RecoveryMachine();
}
int main()
{
    signal(SIGQUIT,Recovery);
    // 用户请求路由服务功能
    Server svr;
    ns_control::Control ctrl;
    ctrl_ptr=&ctrl;

    // 获取所有的题目
    svr.Get("/all_questions",[&ctrl](const Request&req,Response &resp){
        //resp.set_content("这是所有题目的列表","text/plain;charset=utf-8");
        //返回一张包含有所有题目的.html的网页,根据Ctrl获取model信息
        std::string html;
        ctrl.AllQuestions(&html);
        resp.set_content(html,"text/html;charset=utf-8");
    });

    // 根据题目编号获取内容，拼接代码组成网页
    // 正则匹配,question/(/d+))
    // R"()",保持字符串的原貌，不做相关的转义
    svr.Get(R"(/question/(\d+))", [&ctrl](const Request &req, Response &resp){
        std::string number = req.matches[1];//1号内容存放的就是内个题号;%d
        std::string html;
        ctrl.Question(number, &html);
        resp.set_content(html, "text/html; charset=utf-8");
    });
    
    // svr.Get("/hello",[&ctrl](const Request&req,Response &resp){
    //     //std::string number=req.matches[1];//通过字段获取
    //     resp.set_content("这是指定的一道题","text/plain;charset=utf-8");
    //     // std::string html;
    //     // ctrl.Question(number,&html);
    //     // resp.set_content(html,"text/html;charset=utf-8");

    // });

    // 用户提交代码使用判题功能（1. 每道题的测试用例2. compile_and_run）
    svr.Post(R"(/judge/(\d+))",[&ctrl](const Request&req,Response &resp){
        std::string number=req.matches[1];//通过字段获取
        std::string result_json;
        ctrl.Judge(number,req.body,&result_json);
        //resp.set_content("这是指定的判题"+ number,"text/plain;charset=utf-8");
        resp.set_content(result_json,"application/json;charset=utf-8");


    });
    
    svr.set_base_dir("./wwwroot");//设置默认首页是wwwroot目录下
    svr.listen("0.0.0.0",8080);
    return 0;
}