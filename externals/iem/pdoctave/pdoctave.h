// pdoctave functions

// functions return -1 when they fail (= octave object is not instanciated in pd)

int getPDOctaveInstances ();
int writeToOctaveStdIN (const char *cmd);
