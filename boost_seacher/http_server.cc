#include"searcher.hpp"
#include"cpp-httplib/httplib.h"

const std::string root_path ="./wwwroot";
const std::string input ="data/raw_html/raw.txt";

int main()
{
  ns_searcher::Searcher search;
  search.InitSearcher(input);
  
  httplib::Server svr;
  svr.set_base_dir(root_path.c_str());
  //svr.Get("/hi", [](const httplib::Request &req, httplib::Response &rsp){
     // rsp.set_content("你好,世界!", "text/plain; charset=utf-8");
     // });
     // 
      svr.Get("/s",[&search](const httplib::Request &req,httplib::Response & rsp){
      if(!req.has_param("word"))//如果没有参数，回应应该输入参数
      {
        rsp.set_content("必须要有搜索关键字！","text/plain; charset=utf-8");
        return ;
      }
      
      std::string word =req.get_param_value("word");
      //std::cout<<"用户正在搜索： "<<word<<std::endl;
      LOG(NORMAL,"用户正在搜索："+ word);
      std::string json_string;
      search.Search(word,&json_string);//服务器搜索，搜索到的结果放到json_string 中
      //rsp.set_content(json_string,"application/json");//根据content_type对照表得知
      rsp.set_content(json_string, "application/json");
      });
  LOG(NORMAL,"服务器启动成功。。。");
  svr.listen("0.0.0.0",8082);//任意ip都可以访问8082端口
  return 0;
}
