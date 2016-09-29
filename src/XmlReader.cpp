// (C) 2015-2016 Tim Gurto

#include <set>
#include <sstream>

#include "XmlReader.h"

XmlReader::XmlReader(const char *filename):
_root(nullptr){
    newFile(filename);
}

XmlReader::XmlReader(const std::string &filename):
_root(nullptr){
    newFile(filename);
}

XmlReader::~XmlReader(){
    _doc.Clear();
}

void XmlReader::newFile(const char *filename){
    _doc.Clear();
    _root = nullptr;
    bool ret = _doc.LoadFile(filename);
    if (!ret)
        return;
    _root = _doc.FirstChildElement();
}

void XmlReader::newFile(const std::string &filename){
    newFile(filename.c_str());
}

std::set<TiXmlElement *> XmlReader::getChildren(const std::string &val, TiXmlElement *elem) {
    std::set<TiXmlElement *> children;
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            children.insert(child);
    return children;
}

TiXmlElement *XmlReader::findChild(const std::string &val, TiXmlElement *elem){
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            return child;
    return nullptr;
}

bool XmlReader::findAttr(TiXmlElement *elem, const char *attr, std::string &val){
    const char *const cStrVal = elem->Attribute(attr);
    if (cStrVal != nullptr) {
        val = cStrVal;
        return true;
    }
    return false;
}
