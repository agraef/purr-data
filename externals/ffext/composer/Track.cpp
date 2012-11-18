#include "Song.hpp"
#include "Track.hpp"
#include "Pattern.hpp"

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

Track::Track(Song *_song, string trackName)
: name(trackName), song(_song)
{
}

Track *Track::byName(string songName, string trackName)
{
	Song *song = Song::byName(songName);

	Track *track = song->getTrackByName(trackName);
	if(!track) track = new Track(song, trackName);

	return track;
}

void Track::print()
{
	cerr << "---- Track: " << name << " ----" << endl;

    for(map<string,Pattern *>::iterator i = patterns.begin(); i != patterns.end(); i++)
    {
		cerr << "  Pattern[" << i->first << "]: " << i->second->getName() << endl;
    }

	cerr << "---- End track (" << name << ") ----" << endl;
}

void Track::addPattern(int rows, int cols, string name)
{
    Pattern *pattern = new Pattern(rows, cols, name);
    patterns[name] = pattern;
}

Pattern *Track::getPattern(const string &p)
{
    if(patterns.find(p) != patterns.end())
        return patterns[p];
    else
        return 0;
}

void Track::renamePattern(const string &oldName, const string &newName)
{
    Pattern *pattern = getPattern(oldName);
    if(!pattern) return;
    patterns[newName] = patterns[oldName];
    patterns[newName]->setName(newName);
    patterns.erase(oldName);
}

void Track::copyPattern(const string &src, const string &dst)
{
    Pattern *pattern = getPattern(src);
    if(!pattern) return;
    patterns[dst] = new Pattern(*patterns[src]);
    patterns[dst]->setName(dst);
}

void Track::removePattern(const string &p)
{
    Pattern *pattern = getPattern(p);
    if(!pattern) return;
    patterns.erase(p);
}

