#ifndef COMPOSER_SONG_H_INCLUDED
#define COMPOSER_SONG_H_INCLUDED

#include "HasMeta.hpp"

#include <map>
#include <string>

#include <m_pd.h>

#include "Track.hpp"

using std::map;
using std::string;

class Song : public HasMeta
{
private:
	static map<string,Song *> byname;
public:
	static Song *byName(string songName);
private:
	string name;
	map<string,Track *> tracks;
protected:
	Song(string songName);
public:
	void print();
	Track *getTrackByName(string trackName);
	inline const string &getName() {return name;}
};

#endif // COMPOSER_SONG_H_INCLUDED
