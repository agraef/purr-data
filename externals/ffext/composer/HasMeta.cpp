#include "HasMeta.hpp"

bool HasMeta::hasMeta(const string &key)
{
    return meta.find(key) != meta.end();
}

const string HasMeta::getMeta(const string &key)
{
    return hasMeta(key) ? meta[key] : "";
}

void HasMeta::setMeta(const string &key, const string &value)
{
    meta[key] = value;
}
