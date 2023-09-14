#include<iostream>
#include<string>
#include<vector>

#include<boost/filesystem.hpp>
#include"Util.hpp"

//是一个目录，下面放的就是html的所有网页
const std::string src_path="data/input";
const std::string output ="data/raw_html/raw.txt";
typedef struct DocInfo
{
  std::string title;
  std::string content;
  std::string url;//文档在官网中的url
}DocInfo_t;

bool EnumFile(const std::string &src_path, std::vector<std::string>* file_list);
bool ParseHtml(const std::vector<std::string >&file_list,std::vector<DocInfo_t>* results);
bool SaveHtml(const std::vector<DocInfo_t>& result,const std::string& output);

int main()
{
  std::vector<std::string>file_list;
  //递归式的吧每个html文件名带路径保存到file_list中，方便后续一个文件进行读取
  if(! EnumFile(src_path,&file_list))
  {
    std::cerr<<"EnumFile error"<<std::endl;
    return 1;
  }
  //第二步，按照file_list进行读取每个文件的内容，并进行解析
  std::vector<DocInfo_t> result;
  if(!ParseHtml(file_list,&result))
  {
    
    std::cerr<<"parse html error"<<std::endl;
    return 2;
  }
  //第三步，将解析之后的文件内容装填进output（raw.txt）中，以\3为分隔符
  if(!SaveHtml(result,output))
  {
    std::cerr<<"save html error"<<std::endl;
    return 3;
  }

   return 0;

}

bool EnumFile(const std::string &src_path, std::vector<std::string>* file_list)
{
  namespace fs= boost::filesystem;//命名空间使用
  fs::path root_path(src_path);//从哪里开始找
  //1. 判断路径是否存在
  if(!fs::exists(root_path))
  {
    std::cerr<<src_path<<"not exits"<<std::endl;
    return false;
  }
  //定义一个空的迭代器，用来进行判断递归是否结束
  fs::recursive_directory_iterator end;
  for(fs::recursive_directory_iterator iter(root_path);iter!=end;iter++)
  {
    //判断文件是否是普通文件
    if(!fs::is_regular_file(*iter))
    {
      continue;//如果不是的话，就不添加就行了，往下遍历
    }
    //判断文件路径后缀是否是.html
    if(iter->path().extension() != ".html")
    {
      continue;

    }
    //std::cout<<"debug: "<<iter->path().string()<<std::endl;
    //将当前路径下合法的以.html结束的普通网页文件插入到file_list
    file_list->push_back(iter->path().string());//获取到的最开始是路径对象
  }
  return true;
}
static bool ParseTitle(const std::string & file,std::string * title)
{
  std::size_t begin = file.find("<title>");
  if(begin==std::string ::npos)
  {
    return false;
  }
  std::size_t end=file.find("</title>");
  if(end==std::string::npos)
  {
    return false;
  }
  begin+=std::string("<title>").size();//后加的不知道是否是忘记添加了
  if(begin> end)
  {
    return false;
  }
  //将中间字符串添加，左闭右开区间
  *title =file.substr(begin,end-begin);
  return true;
}
static bool ParseContent(const std::string & file ,std::string * content)
{
  //去标签需要基于一个简单的状态机
  enum status
  {
    LABLE,
    CONTENT
  };

  enum status s = LABLE;
  for(char c :file)
  {
     switch(s)
     {
       case LABLE:
         if(c=='>') s=CONTENT;//如果遇到>,说明是标签的结尾。后面就是正文，或者是下一个标签的起始
         break;
       case CONTENT:
         if(c=='<') s=LABLE;
         else
         {
           if(c=='\n') c=' ';//不想保留原始数据中的\n,因为想用\n作为html解析之后的文本分隔符
           content->push_back(c);
         }
         break;
       default:
         break;
     }
  }
  return true;
}
static bool ParseUrl(const std::string & file_path,std::string * url)
{
  std::string url_head="https://www.boost.org/doc/libs/1_79_0/doc/html";
  //在自己原目录下截取掉前面的部分，剩下的组成网址的尾巴
  std::string url_tail=file_path.substr(src_path.size());//全局路径就是data/input
  *url=url_head+url_tail;
  return true;
}

static void  ShowDoc(const DocInfo_t& doc)
{
  std::cout<<"title: "<<doc.title<<std::endl;
  std::cout<<"content: "<<doc.content<<std::endl;
  std::cout<<"url: "<<doc.url<<std::endl;

}
bool ParseHtml(const std::vector<std::string >&file_list,std::vector<DocInfo_t>* results)
{
  for(const std::string &file:file_list)
  {
    //读取文件
    std::string result;
    if(!ns_util::FileUtil::ReadFile(file,&result))
    {
      continue;
    }
    //2.将每一个string 类型分割成结构体中的三部分
    DocInfo_t doc;
    if(!ParseTitle(result,&doc.title))
    {
      continue;
    }
    if(!ParseContent(result,&doc.content))
    {
      continue;
    }
    if(!ParseUrl(file,&doc.url))
    {
      continue;
    }
    //done 完成了解析任务，当前文档的相关内容都保存在了doc当中
    results->push_back(std::move(doc));//资源转移，右值，避免过多拷贝空间浪费
  
    //for test
    //ShowDoc(doc);
    // break;
  }
  return true;
}
bool SaveHtml(const std::vector<DocInfo_t>& result,const std::string& output)
{
#define SEP '\3'
  std::ofstream out(output,std::ios::out | std::ios::binary);
  if(!out.is_open())
  {
      std::cout<<"open files"<<"failed!"<<std::endl;
  }
  for(auto &e: result)
  {
    std::string out_string;
    out_string = e.title;
    out_string+=SEP;
    out_string+=e.content;
    out_string+=SEP;
    out_string+=e.url;
    out_string+="\n";

    out.write(out_string.c_str(),out_string.size());
  }
  out.close();

  return true;
}








