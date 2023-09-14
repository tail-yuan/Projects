#pragma once
// 根据题目列表文件，加载所有的额题目信息到内存中
// model主要和数据进行交互，对外提供访问数据的接口
#include <iostream>
#include "../comm/util.hpp"
#include <string>
#include "../comm/log.hpp"
#include <unordered_map>
#include <vector>
#include <assert.h>
#include <fstream>
namespace ns_model
{
    using namespace std;
    using namespace ns_log;
    using namespace ns_util;
    struct Question
    {
        std::string number;
        std::string title;
        std::string star; //难度
        int cpu_limit;    //时间要求
        int mem_limit;
        std::string desc;   //题目的描述
        std::string header; //题目预设代码
        std::string tail;   //题目的测试用例，需要和header拼接
    };
    const std::string questions_list = "./questions/questions.list";
    const std::string questions_path = "./questions/"; //题库路径，根据同名访问文件夹
    
    class Model
    {
    private:
        // 题号映射题目细节
        unordered_map<std::string, Question> questions;

    public:
        Model()
        {
            assert(LoadQuestionList(questions_list));
        }
        // 加载题目列表
        bool LoadQuestionList(const std::string &question_list)
        {
            // 加载配置文件：questions+题目编号文件

            ifstream in(question_list);
            if (!in.is_open())
            {
                LOG(FATAL)<<"加载题库失败,请检查是否存在题库文件"<<"\n";
                return false;
            }
            //读取文件，按行配置，按行读取
            std::string line;
            while (std::getline(in, line))
            {
                //字符串切分
                vector<string> tokens;
                StringUtil::SplitString(line, &tokens, " ");
                if (tokens.size() != 5)
                {
                    LOG(WARNING)<<"请检查加载的部分题目文件格式"<<"\n";
                    continue; //继续找下一个，这个不要了
                }
                Question q;
                q.number = tokens[0];
                q.title = tokens[1];
                q.star = tokens[2];
                q.cpu_limit = atoi(tokens[3].c_str());
                q.mem_limit = atoi(tokens[4].c_str());

                std::string path = questions_path;
                path += q.number;
                path += "/";

                //文件读取，来拼接字符串ReadFile()
                FileUtil::ReadFile(path + "desc.txt", &(q.desc), true);
                FileUtil::ReadFile(path + "header.cpp", &(q.header), true);
                FileUtil::ReadFile(path + "tail.cpp", &(q.tail), true);

                questions.insert({q.number, q}); //插入到map中,注意格式{}
            }
            LOG(INFO)<<"加载题目成功"<<"\n";
            in.close();
            return true;
        }
        bool GetAllQuestions(vector<Question> *out)
        {
            if (questions.size() == 0)
            {
                LOG(ERROR)<<"用户获取题库失败"<<"\n";
                return false;
            }
            for (const auto &q : questions)
            {
                out->push_back(q.second); // first : key second: 题目信息
            }
            return true;
        }
        // 根据索引得到题目
        bool GetOneQuestion(const std::string &number, Question *q)
        {
            const auto& iter = questions.find(number);
            if(iter == questions.end()){
                LOG(ERROR) << "用户获取题目失败, 题目编号: " << number << "\n";
                return false;
            }
            (*q) = iter->second;
            return true;
        }
        
        // bool GetOneQuestion(const std::string &number, Question *q)
        // {
        //     const auto &iter = questions.find(number);
        //     if (iter == questions.end())
        //     {
        //         LOG(ERROR)<<"用户获取当题失败"<<"\n";
        //         return false;
        //     }
        //     (*q) = iter->second;
        //     return true;
        // }
        ~Model() {}
    };
}