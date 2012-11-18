#include "Song.hpp"

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

map<string,Song *> Song::byname;

Song::Song(string songName)
: name(songName)
{
}

Song *Song::byName(string songName)
{
	if(byname.find(songName) == byname.end())
		byname[songName] = new Song(songName);

	return byname[songName];
}

void Song::print()
{
	cerr << "---- Song: " << name << " ----" << endl;

	for(map<string,Track *>::iterator i = tracks.begin(); i != tracks.end(); i++)
	{
		cerr << "  Track[" << i->first << "]: " << i->first << endl;
	}

	cerr << "---- End song (" << name << ") ----" << endl;
}

Track *Song::getTrackByName(string trackName)
{
	if(tracks.find(trackName) == tracks.end())
		return 0;
	else
		return tracks[trackName];
}
