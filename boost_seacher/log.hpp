#pragma  once
#include<iostream>
#include<ctime>
#include<string>
#define  NORMAL 1
#define  WARNING 2
#define  DEBUG 3
#define  FATAL 4

#define LOG(LEVEL,MESSAGE) log(#LEVEL,MESSAGE,__FILE__,__LINE__);//传来前两个，后两个自动获取到了
void log(std::string level,std::string message,std::string file,int lineth)
{
  std::cout<<"["<<level<<"]"<<"["<<time(nullptr)<<"]"<<"["<<message<<"]"<<"["<<file<<"]"<<"["<<lineth<<"]"<<std::endl; 
}

