#pragma once
// mysql版本
// model主要和数据进行交互，对外提供访问数据的接口
#include <iostream>
#include "../comm/util.hpp"
#include <string>
#include "../comm/log.hpp"
#include <unordered_map>
#include <vector>
#include <assert.h>
#include <fstream>

#include"include/mysql.h"
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
    
    class Model
    {
        const std::string oj_questions="questions";
        const std::string host="127.0.0.1";
        const std::string user="root";
        const std::string passwd="yuanwei0710@honey.666";
        const std::string db="oj";
        const int port=3306;

    public:
        Model()
        {}
        bool QueryMysql(const string &sql,vector<Question> *out)//如果只有一个题目数组大小就是1
        {
            MYSQL *my=mysql_init(nullptr);

            if(nullptr==mysql_real_connect(my,host.c_str(),user.c_str(),passwd.c_str(),db.c_str(),port,nullptr,0))
            {
                LOG(FATAL)<<"连接数据库失败！"<<"\n";
                return false;
            }
            //一定要设置编码格式，默认是拉丁，会出现乱码
            mysql_set_character_set(my,"utf8");

            LOG(INFO)<<"连接数据库成功！"<<std::endl;
            // 执行SQL语句
            if(0!=mysql_query(my,sql.c_str()))
            {
                LOG(WARNING)<<sql<<"exectuate error"<<"\n";
                return false;
            }
            // 提取结果
            MYSQL_RES *res=mysql_store_result(my);

            // 分析结果
            int rows=mysql_num_rows(res);
            int cols=mysql_num_fields(res);

            for(int i=0;i<rows;i++)
            {
                MYSQL_ROW row=mysql_fetch_row(res);
                struct Question q;
                q.number=row[0];
                q.title=row[1];
                q.star=row[2];
                q.desc=row[3];
                q.header=row[4];
                q.tail=row[5];
                q.cpu_limit=atoi(row[6]);
                q.mem_limit=atoi(row[7]);
                out->push_back(q);
            }
            // 释放结果空间
            free(res);
            //关闭mysql连接
            mysql_close(my);
            return true;
        }
        bool GetAllQuestions(vector<Question> *out)
        {
           std::string sql="select * from ";
           sql += oj_questions;
           return QueryMysql(sql,out);
        }
        // 根据索引得到题目
        bool GetOneQuestion(const std::string &number, Question *q)
        {
            bool res=false;
            std::string sql="select * from ";
            sql += oj_questions;
            sql += " where id=";
            sql+=number;
            vector<Question>result;
            if(QueryMysql(sql,&result))
            {
                //选择一道题
                if (result.size()==1) 
                {
                    *q=result[0];
                    res=true;
                }
            }
           return res;
        }
        ~Model() {}
    };
}