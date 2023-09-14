#pragma  once
#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include"cppjieba/Jieba.hpp"

#include<boost/algorithm/string.hpp>
namespace ns_util
{
  class FileUtil
  {
    public: 
      static bool ReadFile(const std::string & file_path,std::string *out)
      {
        std::ifstream in(file_path,std::ios::in);
        if(!in.is_open())
        {
          std::cerr<<"open file"<<file_path<<" error"<<std::endl;
          return false;
        }
        std::string line;
        //如何理解getline读到文件的结束？
        //getline的返回值是istream&,while 需要的是bool类型，本质是重载了强制类型转化
        // 比如你判断是一个对象，对象里面就进行了重载，重载强转得到bool值
        while(std::getline(in,line))
        {
          *out +=line;
        }
        in.close();
        return true;
      }

  };

   class StringUtil
   {
     //被外部直接使用的函数就得有static 
     public:  
     static void Split(const std::string& line,std::vector<std::string>* results,const std::string &sep )
        {
            boost::split(*results,line,boost::is_any_of(sep),boost::token_compress_on);

        }
    };

   //结巴分词的引入完成
  //dict 已经是创建好的软连接,在当前目录之下
      const char* const DICT_PATH = "./dict/jieba.dict.utf8";
      const char* const HMM_PATH = "./dict/hmm_model.utf8";
      const char* const USER_DICT_PATH = "./dict/user.dict.utf8";
      const char* const IDF_PATH = "./dict/idf.utf8";
      const char* const STOP_WORD_PATH = "./dict/stop_words.utf8";
   class JiebaUtil
   {
     private:
       static cppjieba::Jieba jieba;
     public:
       static void CutString(const std::string &src,std::vector<std::string >*out)
       {
         jieba.CutForSearch(src,*out);
       }

   };
      cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);
}




