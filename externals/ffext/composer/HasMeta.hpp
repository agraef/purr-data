#ifndef COMPOSER_HASMETA_H_INCLUDED
#define COMPOSER_HASMETA_H_INCLUDED

#include <map>
#include <string>

using std::map;
using std::string;

class HasMeta
{
private:
	map<string,string> meta;
public:
	bool hasMeta(const string &key);
    const string getMeta(const string &key);
    void setMeta(const string &key, const string &value);

    typedef map<string,string>::const_iterator meta_iterator;
    inline meta_iterator meta_begin() const {return meta.begin();}
    inline meta_iterator meta_end() const {return meta.end();}
};

#endif // COMPOSER_HASMETA_H_INCLUDED
