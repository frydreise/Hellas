// (C) 2015 Tim Gurto

#include <set>
#include <sstream>

#include "XmlDoc.h"

XmlDoc::XmlDoc(const char *filename, Log *debug):
_debug(debug),
_root(0){
    newFile(filename);
}

XmlDoc::~XmlDoc(){
    _doc.Clear();
}

void XmlDoc::newFile(const char *filename){
    _doc.Clear();
    bool ret = _doc.LoadFile(filename);
    if (!ret) {
        if (_debug)
            *_debug << Color::RED << "Failed to load XML file " << filename << ": "
                    << _doc.ErrorDesc() << Log::endl;
        return;
    }
    _root = _doc.FirstChildElement();
    if (!_root && _debug)
        *_debug << Color::RED
               << "XML file " << filename << "has no root node; aborting." << Log::endl;
}

std::set<TiXmlElement *> XmlDoc::getChildren(const std::string &val, TiXmlElement *elem) {
    std::set<TiXmlElement *> children;
    for (TiXmlElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
        if (val == child->Value())
            children.insert(child);
    return children;
}

bool XmlDoc::findStrAttr(TiXmlElement *elem, const char *attr, std::string &strVal){
    const char *const val = elem->Attribute(attr);
    if (val) {
        strVal = val;
        return true;
    }
    return false;
}

bool XmlDoc::findIntAttr(TiXmlElement *elem, const char *attr, int &intVal){
    const char *const val = elem->Attribute(attr);
    if (val) {
        std::string strVal(val);
        std::istringstream iss(strVal);
        iss >> intVal;
        return true;
    }
    return false;
}
