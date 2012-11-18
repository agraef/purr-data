
framesync

framesync is a library for syncing sounds to video using frame numbers as the
unit of time.  It was developed as part of the sound design for the Cartier
Foundation's Terre Natale project.  There are a couple of concepts in this
library necessary to understand in order to use it:

- the FPS (Frames-Per-Second) is a global value set everywhere in Pd
- the frameclock is global, and each framesync object receives it automatically
- the frameclock starts at 0 and flows linearly

