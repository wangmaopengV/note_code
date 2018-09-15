#include <iostream>
#include <fstream>
#include <cassert>
#include <string>

#include "json/json.h"
#include "glog/logging.h"

#define ILOGW(info) LOG(INFO) << info

int main()
{
    std::string file = "glint.json";

    std::ifstream infile;
    infile.open(file.data());   //将文件流对象与文件连接起来
    assert(infile.is_open());   //若失败,则输出错误消息,并终止程序运行

    std::string data;

    infile.seekg(0, infile.end);
    int length = infile.tellg();
    infile.seekg(0, infile.beg);

    std::cout << "file length : " << length;

    data.resize(length);
    infile.read(&(*data.begin()), length);

    Json::Reader reader;
    Json::Value root;

    /*check json*/
    if (!reader.parse(data.data(), data.data() + data.size(), root) || !root.isObject())
    {
        ILOGW("root parse failed [" << data << "]\n");
        return -1;
    }

    if (!root.isMember("config") || !root["config"].isArray() || 0 == root["config"].size())
    {
        return 0;
    }

    int mode = 0;
    Json::Value &type = root["config"];
    for (int att = 0; att < type.size(); ++att)
    {
        if (type[att].isMember("type") && type[att]["type"].isString() && type[att].isMember("score") &&
            type[att]["score"].isInt())
        {
            std::string temp_type = type[att]["type"].asCString();
            float type_score = type[att]["score"].asInt();

            if (strcmp(temp_type.c_str(), "age") == 0)
            {
                ILOGW("age limit [" << type_score << "]\n");
            }
        }
    }

    return 0;
}
