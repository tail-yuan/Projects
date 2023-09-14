#pragma  once
#include<mutex>
#include<iostream>
#include<vector>
#include<fstream>
#include"Util.hpp"
#include"log.hpp"
#include<unordered_map>

namespace ns_index
{
  struct DocInfo
  {
    std::string title;    //文档标题
    std::string content;  //文档对应的去标签之后的内容
    std::string url;      //官网文档url
    uint64_t doc_id;      //文档的id，后续按文档获取内容，并且建立倒排索引的时候需要他
  };
  //构建倒排索引，一个Word对应很多文档信息(ID，权重，关键字),每个文档看成一个结构体节点，组成相关联的拉链
  struct InvertedElem
  {
      uint64_t doc_id;
      int weight;
      std::string word;
  };

  //倒排拉链的形式
  typedef std::vector<InvertedElem> InvertedList;

  class Index
  {
    private:
      //正排索引用数组的形式，简单的将文档信息(DocInfo)填充进数组就可以了，对应的id是有用的。
      std::vector<DocInfo> forward_index;
      //倒排索引一定是一个关键字对应一组文档信息[关键字，ID，网址]即一个拉链
      std::unordered_map <std::string,InvertedList> inverted_index;
    private:
      Index(){}
      Index(const Index& )=delete;
      Index& operator=(const Index&)= delete;

      static Index* instance;
      static std::mutex mtx;
    public:
      ~Index(){}
    public:
      //获取单例的函数，可能会有线程安全，所以要进行加锁操作
     static  Index* GetInstance()
      {
        if(nullptr == instance)//两层判断，减少很多加锁和申请锁的过程
        {
              mtx.lock();
             if(nullptr == instance)//再判断一遍之前没创建过单例
              {
                 instance =new Index();
              }
        }
        mtx.unlock();
        return instance;
      }

    //获取映射信息的函数
      //1. 正排索引的
      DocInfo* GetForwardList(const uint64_t doc_id)
      {
        if(doc_id >= forward_index.size())
        {
          std::cerr<<"doc_id out of range.error"<<std::endl;
          return nullptr;
        }
          return &forward_index[doc_id];
      }
      //2. 倒排索引的，每查找一个关键字，对应返回的就是一个拉链信息集
      InvertedList* GetInvertedList(const std::string & word)
      {
        auto iter = inverted_index.find(word);
        if(iter == inverted_index.end())
        {
          std::cout<<"Word is not conclued,have no InvertedList"<<std::endl;
          return nullptr;
        }

        return &(iter->second);
      }
  //创建映射关系的
      //构建索引.需要将文档传过来
      bool BuildIndex(const std::string &input)
      {
        //1. 读取文件
        std::ifstream in(input,std::ios::in | std::ios::binary);
        if(!in.is_open())
        {
          std::cerr<<"open file failed!"<<std::endl;
          return false;
        }
        //2. 按行读取文件
        std::string line;
        int count=0;
        while(std::getline(in,line))
        {
          //构建正排索引，返回值是文档信息(DocInfo )
          DocInfo* doc= BuildForwardIndex(line);
          if(nullptr == doc)
          {
            std::cerr<<"build "<<line<<"index failed!"<<std::endl;
            continue;//一个建立失败没关系，继续整就完了
          }
          //建立倒排索引,是根据Word信息，也就是在doc中
          BuildInvertedIndex(*doc);
          count++;
          if(count%50==0)
          {
         //   std::cout<<"当前建立的索引文档为.."<<count <<std::endl;
              LOG(NORMAL,"当前建立的索引文档为："+ std::to_string(count));
          }
        }
        return true;
      }
    private:
      DocInfo* BuildForwardIndex(std::string & line)
      {
        //将文档切分成三个有用信息的三个字符串
        //1.解析line，字符串切分
        std::vector<std::string> results;
        std::string sep ="\3";
        ns_util::StringUtil::Split(line,&results,sep);
        if(results.size()!=3)
        {
          //三部分缺少一个
          return nullptr;
        }
        //2 .将字符串三部分进行填充
        DocInfo doc;
        doc.title=results[0];
        doc.content=results[1];
        doc.url=results[2];
        doc.doc_id=forward_index.size();//先保存id再插入内容。反过来的话，下标就是插入之后数组大小-1
        //3. 填充到vector<doc>中
        forward_index.push_back(std::move(doc));
        //返回最后一个，也就是最新的一个元素的地址
        return &forward_index.back();
      }
      bool BuildInvertedIndex(const DocInfo & doc )
      {
        struct word_cnt
        {
          int title_cnt;
          int content_cnt;
          word_cnt():title_cnt(0),content_cnt(0)
          {}
        };
        //1. 对标题的内容进行分词
        std::vector<std::string> title_words;
        ns_util::JiebaUtil::CutString(doc.title,&title_words);
        std::unordered_map <std::string ,word_cnt> word_map;
        //对标题进行词频统计
        for(auto s :title_words)
        {
          boost::to_lower(s);//大小写都用小写来进行词频统计,算法上用小写，但是用户的文档并不作出修改
          word_map[s].title_cnt++;

        }
        //对文档内容进行分词
        std::vector<std::string> content_words;
        ns_util::JiebaUtil::CutString(doc.content,&content_words);
        //对文档内容进行词频统计
        for(auto s :content_words)
        {
          boost::to_lower(s);
          word_map[s].content_cnt++;
        }
#define X 10
#define Y 1

        for(auto & word_pair: word_map)
        {
          InvertedElem item;
          item.doc_id=doc.doc_id;
          item.word=word_pair.first;
          item.weight=X*word_pair.second.title_cnt+ Y* word_pair.second.content_cnt;
          InvertedList & inverted_list = inverted_index[word_pair.first];//map中添加string
          inverted_list.push_back(std::move(item));//在string对应位置加上item节点信息
        }

        return true;

      }

  };
  //静态函数要在外面加一层初始化
  Index* Index::instance =nullptr;//构架单例模式
  std::mutex Index::mtx;
}

















