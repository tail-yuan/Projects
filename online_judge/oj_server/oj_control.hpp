#pragma once
#include <iostream>
#include <string>
#include <signal.h>
// #include "oj_model.hpp"
#include "oj_model2.hpp"
#include "../comm/log.hpp"
#include "../comm/util.hpp"
#include <mutex>
#include <algorithm>
#include "oj_view.hpp"
#include <fstream>
#include <vector>
#include "../comm/httplib.h"
#include <cassert>
#include <jsoncpp/json/json.h>
namespace ns_control
{
    using namespace ns_util;
    using namespace ns_log;
    using namespace ns_view;
    using namespace ns_model;
    using namespace httplib;

    // 提供服务的主机
    class Machine
    {
    public:
        std::string ip;  //编译服务的ip
        int port;        //编译服务的端口
        uint64_t load;   //编译服务的负载，需要锁进行保护，可能同时来很多主机访问服务
        std::mutex *mtx; //禁止拷贝，所以用拷贝指针的方式调用这把锁
    public:
        Machine() : ip(""), port(0), load(0), mtx(nullptr)
        {
        }
        ~Machine() {}

    public:
        //提升主机负载
        void IncLoad()
        {
            if (mtx)
                mtx->lock();
            load++;
            if (mtx)
                mtx->unlock();
        }
        //减少主机负载
        void DecLoad()
        {
            if (mtx)
                mtx->lock();
            load--;
            if (mtx)
                mtx->unlock();
        }
        //获取主机负载
        uint64_t Load()
        {
            uint64_t _load = 0;
            if (mtx)
                mtx->lock();
            _load = load;
            if (mtx)
                mtx->unlock();

            return _load;
        }
        void ResetLoad()
        {
            if (mtx)
                mtx->lock();
            load = 0;
            if (mtx)
                mtx->unlock();
        }
    };

    const std::string service_machine = "./conf/service_machine.conf";

    // 负载均衡板块
    class LoadBalance
    {
    private:
        std::vector<Machine> machines; //可以给我们提供编译服务的主机，下标对应主机编号
        //所有在线的主机id
        std::vector<int> online;
        // 所有离线的主机id
        std::vector<int> offline;
        // 保证数据安全
        std::mutex mtx;

    public:
        LoadBalance()
        {
            assert(LoadConf(service_machine));
            LOG(INFFO) << "加载 " << service_machine << "成功"
                       << "\n";
        }
        ~LoadBalance() {}

    public:
        bool LoadConf(const std::string &machine_conf)
        {
            std::ifstream in(machine_conf);
            if (!in.is_open())
            {
                LOG(FATAL) << "加载：" << machine_conf << "失败"
                           << "\n";
                return false;
            }
            std::string line;
            while (std::getline(in, line))
            {
                // :作为分隔符切分,两部分，ip+port
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, ":");
                if (tokens.size() != 2)
                {
                    LOG(WARNING) << "切分" << line << "失败"
                                 << "\n";
                    continue;
                }
                Machine m;
                m.ip = tokens[0];
                m.port = atoi(tokens[1].c_str());
                m.load = 0;
                m.mtx = new std::mutex();

                online.push_back(machines.size()); //先插入在线主机，0
                machines.push_back(m);             // size=1了
            }
            in.close();
            return true;
        }
        // 输出型参数，根据主机地址直接操控主机
        bool SmartChoice(int *id, Machine **m) //解引用是Machine* ，让外面通过地址得到主机
        {
            // 1. 使用选择好的主机，更新主机负载
            // 2. 我们需要可能离线该主机，online->offline
            mtx.lock();
            // 负载均衡算法
            int online_num = online.size();
            if (online_num == 0)
            {
                mtx.unlock();
                LOG(FATAL) << "所有的后端编译器已经全部离线"
                           << "\n";
                return false;
            }
            //遍历找到所有的负载最小的机器
            uint64_t min_load = machines[online[0]].Load();
            *id = online[0];
            *m = &machines[online[0]];
            for (int i = 0; i < online_num; i++)
            {
                uint64_t cur_load = machines[online[i]].Load();
                if (min_load > cur_load)
                {
                    min_load = cur_load;
                    *id = online[i];
                    *m = &machines[online[i]];
                }
            }

            mtx.unlock();
            return true;
        }
        void OfflineMachine(int which)
        {
            mtx.lock();
            for (auto iter = online.begin(); iter != online.end(); iter++)
            {
                if (*iter == which)
                {
                    //将负载清零之后，在删除
                    machines[which].ResetLoad();
                    online.erase(iter);
                    offline.push_back(which); //删除之后指向下一个位置，发生迭代器失效。所以用which不是*iter
                    break;                    //不用考虑迭代器失效的问题
                }
            }
            mtx.unlock();
        }
        void OnlineMachine()
        {
            // 统一上线的所有服务器,把offline的内容都移到online中
            mtx.lock();
            online.insert(online.end(), offline.begin(), offline.end());
            offline.erase(offline.begin(), offline.end());
            mtx.unlock();
            LOG(INFO) << "所有的主机上线了"
                      << "\n";
        }
        // fortest
        void ShowMachines()
        {
            mtx.lock();
            std::cout << "当前在线主机列表: ";
            for (auto &id : online)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;
            std::cout << "当前离线主机列表: ";
            for (auto &id : offline)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;
            mtx.unlock();
        }
    };

    // 核心执行业务控制器
    class Control
    {
    private:
        Model model_;
        View view_;                //提供网页渲染功能的
        LoadBalance load_balance_; //核心负载均衡器
    public:
        Control() {}
        ~Control() {}

    public:
        void RecoveryMachine()
        {
            load_balance_.OnlineMachine();
        }
        //根据获取到的题目数据构建网页，输出型参数
        bool AllQuestions(std::string *html)
        {
            bool ret = true;
            vector<struct Question> all;
            if (model_.GetAllQuestions(&all))
            {
                sort(all.begin(), all.end(), [](const struct Question &q1, const struct Question &q2) { //是字符串，不想要用ASCII码表进行排序
                    return atoi(q1.number.c_str()) < atoi(q2.number.c_str());
                });
                //获取题目信息成功到达all中，将所有的题目数据构建成网页
                view_.ExpandAllHtml(all, html);
            }
            else
            {
                *html = "获取题目失败";
                ret = false;
            }
            return ret;
        }

        bool Question(const string &number, string *html)
        {
            bool ret = true;
            struct Question q;
            if (model_.GetOneQuestion(number, &q))
            {
                // 获取指定题目信息成功，将所有的题目数据构建成网页
                view_.OneExpandHtml(q, html);
            }
            else
            {
                *html = "指定题目: " + number + " 不存在!";
                ret = false;
            }
            return ret;
        }

        // bool Question(const std::string &number, std::string *html)
        // {
        //     bool ret = true;
        //     struct Question q;
        //     if (model_.GetOneQuestion(number, &q))
        //     {
        //         //获取指定题目信息成功到达all中，将所有的题目数据构建成网页
        //         view_.ExpandOneHtml(q, html);
        //     }
        //     else
        //     {
        //         *html = "指定题目" + number + "不存在";
        //         ret = false;
        //     }
        //     return ret;
        // }

        void Judge(const std::string &number, const std::string in_json, std::string *out_json)
        {

            //用户提交上来的json_string中字段，id: code: input:
            // 根据题目编号直接拿到题目细节
            struct Question q;
            model_.GetOneQuestion(number, &q);
            // 1. in_json进行反序列化，得到题目id，得到用户提交的源代码
            Json::Reader reader;
            Json::Value in_value;
            reader.parse(in_json, in_value);
            std::string code = in_value["code"].asString();
            // 2. 重新拼接用户代码+测试用例代码形成完整代码
            Json::Value compile_value;
            compile_value["input"] = in_value["input"].asString();
            compile_value["code"] = code + "\n" + q.tail; //测试用例添加上
            compile_value["cpu_limit"] = q.cpu_limit;
            compile_value["mem_limit"] = q.mem_limit;
            Json::FastWriter writer;
            std::string compile_string = writer.write(compile_value);
            // 3. 选择负载最低的主机， 一直选择，直到主机可用，否则就是全挂
            while (true)
            {
                int id = 0;
                Machine *m = nullptr;
                if (!load_balance_.SmartChoice(&id, &m))
                {
                    break;
                }
                // 4. 发起http请求得到结果
                Client cli(m->ip, m->port);
                m->IncLoad();
                LOG(INFO) << " 选择主机成功, 主机id: " << id << " 详情: " << m->ip << ":" << m->port << " 当前主机的负载是: " << m->Load() << "\n";
                if (auto res = cli.Post("/compile_and_run", compile_string, "application/json;charset=utf-8"))
                {
                    // 5. 将结果赋值给out_json
                    if (res->status == 200)
                    {
                        *out_json = res->body;
                        m->DecLoad();
                        LOG(INFO) << "请求服务成功"
                                  << "\n";
                        break;
                    }
                    //错误码失败，接着选别的
                    m->DecLoad();
                }
                else
                {
                    //请求失败
                    LOG(ERROR) << " 当前请求的主机id: " << id << " 详情: " << m->ip << ":" << m->port << " 可能已经离线"
                               << "\n";
                    load_balance_.OfflineMachine(id);
                    load_balance_.ShowMachines(); // for test
                }
            }
        }
    };
}