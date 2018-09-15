#include <iostream>
#include "pugixml.hpp"
#include "pugiconfig.hpp"

using namespace std;

struct xml_string_writer : pugi::xml_writer
{
    std::string result;

    virtual void write(const void *data, size_t size)
    {
        result += std::string(static_cast<const char *>(data), size);
    }
};

string WtiteXml()
{
    pugi::xml_document doc;
    pugi::xml_node pre = doc.prepend_child(pugi::node_declaration);
    pre.append_attribute("version") = "1.0";
    pre.append_attribute("encoding") = "UTF-8";

    pugi::xml_node node = doc.append_child("config");
    pugi::xml_node child_node = node.append_child("glog");
    pugi::xml_node child_level = child_node.append_child("level");
    std::string str = std::to_string(3);
    child_level.append_buffer(str.c_str(), str.length());

    pugi::xml_node glint = node.append_child("glint");
    pugi::xml_node temp = glint.append_child("urls");
    temp.append_attribute("gpu").set_value(0);
    temp = glint.append_child("urls");
    temp.append_attribute("gpu").set_value(1);

    std::string strXmlData;
    xml_string_writer writer;
    doc.save(writer);
    strXmlData = writer.result;

    return strXmlData;
}

void ReadXml(string data)
{
    pugi::xml_document doc;
    //std::string configPath = "confg_file.xml";
    //if( doc.load_file(configPath.c_str()))
    if (doc.load_buffer(data.c_str(), data.size()))
    {
        pugi::xml_node glog = doc.child("config").child("glog");
        cout << glog.child_value("level") << "\n";

        pugi::xml_node form = doc.child("config");
        for (pugi::xml_node url = form.child("glint").first_child(); url; url = url.next_sibling())
        {
            std::string str = url.attribute("gpu").value();
            cout << str << "\n";
        }
    }
}

int main()
{
    string data = WtiteXml();
    cout << data << "\n";
    ReadXml(data);
    return 0;
}
