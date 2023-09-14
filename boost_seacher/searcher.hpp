#pragma once
#include<algorithm>
#include"log.hpp"
#include<jsoncpp/json/json.h>
#include"index.hpp"
#include"Util.hpp"
namespace ns_searcher
{
  //处理重复文档ID的结构体
  struct InvertedElemPrint{
    uint64_t doc_id;
    int weight;
    std::vector<std::string>words;//存放相同ID下的分出来的不同的词，实现去重效果
    InvertedElemPrint():doc_id(0),weight(0)
    {}
  };
  class Searcher
  {
    private:
      ns_index::Index *index;//
    public:
      Searcher(){}
      ~Searcher(){}
    public:
      void InitSearcher(const std::string & input)
      {
        //1.首先要获取创建需要索引的对象,获得单例模式
        index = ns_index::Index::GetInstance();
        
       LOG(NORMAL,"获取index单例成功...");
       // std::cout<<"获取index单例成功。。。"<<std::endl;
        //2. 根据index创建索引
        index->BuildIndex(input);
       // std::cout<<"建立索引成功。。。"<<std::endl;
        LOG(NORMAL,"建立正排倒排索引成功...");
      }
      void Search(const std::string & query,std::string * json_string)
      {
        //1. 搜索的时候也要进行分词
        std::vector<std::string> words;
        ns_util::JiebaUtil::CutString(query,&words);
        //2. 触发：根据各个词进行Index查找,建立索引的时候忽略大小写，搜索也需要
        //将很多词的倒排拉链放到一起
        //ns_index::InvertedList inverted_list_all;//内部放的都是InvertedElem节点
        
        //声明一个不重复的倒排拉链的节点
        std::vector<InvertedElemPrint> inverted_list_all;

        //如果id都相同，都进入到一个打印函数中<doc_id,数组>
        std::unordered_map<uint64_t,InvertedElemPrint> tokens_map;

        for(std::string word:words)
        {
          boost::to_lower(word);
          //先获取倒排拉链
          ns_index::InvertedList * inverted_list =index->GetInvertedList(word);
          if(nullptr == inverted_list )
          {
            //当前Word没有倒排，没有正排
            continue;//继续检测下一个词
          }
          //vector的insert重载了插入迭代器(在哪里插，插的起始迭代器，尾巴)
          //获取所有词的倒排拉链
          //存在问题：搜的是一句话的话，切分的很多次对应一个文档ID，会出现重复的文档现象。
          //本来出一个就行，但出现了很多重复。你合并一下呗
          //inverted_list_all.insert(inverted_list_all.end(),inverted_list->begin(),inverted_list->end());
            
          //遍历拉链节点，doc_id相同的就重叠处理，然后权值相加，并把它们放到一个数组当中，整体处理
          for(const auto & elem:*inverted_list)
          {
            auto &item=tokens_map[elem.doc_id];//[],如果存在直接获取，不存在就新建
            //item 一定是doc_id 相同的print节点
            item.doc_id=elem.doc_id;
            item.weight+=elem.weight;

            item.words.push_back(elem.word);//vector中包含很多同一个句子里的词
          }
        }

        for(const auto&item:tokens_map)
        {
          inverted_list_all.push_back(std::move(item.second));
        }
        //3. 合并排序，汇总查找结果，按照相关性进行降序排序
        //设置回调函数用lambda表达式，你也可以自己写一个仿函数
        
       // std::sort(inverted_list_all.begin(),inverted_list_all.end(),\
           // [](const ns_index::InvertedElem & e1,const ns_index::InvertedElem &e2){
             // return e1.weight>e2.weight;
           // });
           //应该升级为打印之后去重的倒排拉链的节点 ,如何实现去重？就是打印结构体中id的重叠，和文档权值的相加
        std::sort(inverted_list_all.begin(),inverted_list_all.end(),\
            [](const InvertedElemPrint &e1,const InvertedElemPrint &e2){
              return e1.weight>e2.weight;
            }); 
           
           
        //4. 构建，根据查找结果返回一个json_string字符串
        Json::Value root;  
        for(auto & item : inverted_list_all)
          {
            //由id，获取到正排索引，得到DocInfo 结构体
            ns_index::DocInfo *doc =index->GetForwardList(item.doc_id);
            if(nullptr == doc)
            {
              continue;
            }
            //将获取到的结构体进行序列化转化为json_string(jsoncpp)
            Json::Value elem;
            elem["title"]=doc->title;
            //获取描述信息，item中也有关键字文档当中的
            elem["desc"]=GetDesc(doc->content,item.words[0]);//item是一个填充了很多docid相同的词的数组，选其中一个就行
            //是去标签之后的结果，我们要的是一部分内容
            elem["url"]=doc->url;
            //for test weight
            //elem["id"]=(int)item.doc_id;
            //elem["weight"]= item.weight;
            
            root.append(elem);//追加的是已经排序之后的序列
          }

          //Json::StyledWriter writer;
          //后更改选择fastwriter
        Json::FastWriter writer;
          *json_string=writer.write(root);
      }
      //获取摘要的功能，并不是文档去标签之后的结果，而是一部分，一部分中最好也有关键字word
      std::string GetDesc(const std::string & html_content,const std::string &word)
      {
        //找到Word在html_content中首次出现，然后往前50字节（到开头），往后100字节，截取
        //先找到首次出现
        const int prev_step=50;
        const int next_step=100;
       // std::size_t pos=html_content.find(word);
        //if(pos == std::string::npos)
       // {
          //return "None1";
      //  }
        auto iter =std::search(html_content.begin(),html_content.end(),word.begin(),word.end(),[](int a,int b){
            return (std::tolower(a)==std::tolower(b));
            });
        if(iter == html_content.end())
        {
          return "None1";
        }
        int pos= std::distance(html_content.begin(),iter);

        //获取start，end
        int start =0;
        int end =html_content.size()-1;
       //如果之前没有50字节，就从头开始
       //如果有50字节，就从倒数第50字节开始
       //加法全变成正整数
        if(pos>start+prev_step ) start = pos-prev_step;
        if(pos <end-next_step) end=pos+next_step;
        if(start>=end) return "None2";
        std::string desc = html_content.substr(start,end-start);
        desc+="...";
        return desc;

      }
  };
}
