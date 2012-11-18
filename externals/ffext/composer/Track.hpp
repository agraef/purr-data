#ifndef COMPOSER_TRACK_H_INCLUDED
#define COMPOSER_TRACK_H_INCLUDED

#include "HasMeta.hpp"

#include <map>
#include <string>

#include <m_pd.h>

using std::string;
using std::map;

class Song;
class Pattern;

class Track : public HasMeta
{
public:
	static Track *byName(string songName, string trackName);
private:
	string name;
    map<string,Pattern *> patterns;
	Song *song;
protected:
	Track(Song *_song, string trackName);
public:
	void print();
	void addPattern(int rows, int cols, string name);
    Pattern *getPattern(const string &p);
    void renamePattern(const string &oldName, const string &newName);
    void copyPattern(const string &src, const string &dst);
    void removePattern(const string &p);
	inline unsigned int getPatternCount() {return patterns.size();}
	inline Song *getSong() {return song;}
	inline const string &getName() {return name;}

    typedef map<string,Pattern *>::const_iterator pattern_iterator;
    inline pattern_iterator pattern_begin() const {return patterns.begin();}
    inline pattern_iterator pattern_end() const {return patterns.end();}
};

#endif // COMPOSER_TRACK_H_INCLUDED
