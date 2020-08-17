
# bendinfix

This external provides a helper object to maintain compatibility between the bendin value ranges of different Pd flavors (specifically, vanilla Pd, pd-l2ork version 1, and purr-data a.k.a. pd-l2ork version 2+).

Pd has a long-standing [bug](https://sourceforge.net/p/pure-data/bugs/1262/) in that its bendin object produces an unsigned value range of 0 thru 16383, while the bendout object expects a signed range of -8192 thru +8191. Which means that you have to translate the values when routing pitch bends from MIDI input to MIDI output. It also makes it harder to translate pitch bend input to frequency offsets. This bug has been there for such a long time that it now can't be fixed any more, to maintain backwards compatibility.

However, other Pd flavors have in fact fixed this bug, specifically pd-l2ork and its successor purr-data. This actually made matters worse, though, since now Pd programmers have to cope with a variety of bendin implementations, which makes it hard to maintain interoperability between the different flavors if you need to process MIDI pitch bend events.

The bendinfix external provides a solution that (1) provides a quick way to check which bendin implementation you have, (2) takes the output of the bendin object and translates it to the correct (signed) range, and (3) is binary-compatible with all modern Pd flavors.

## Synopsis

`[bendin]` takes signed or unsigned pitch bend values and translates them to signed values.

- inlet #1: signed or unsigned pitch bend values, depending on your Pd flavor

- outlet #1: output values are always signed, in the -8192 ... 8191 range

- arguments: none

## Usage

(1) Passing 0 to bendinfix yields the value -8192 for a vanilla-compatible bendin, and 0 otherwise. Thus comparing the result against 0 gives you a flag determining whether you have a signed implementation:

  `[0( --- [bendinfix] --- [== 0]` yields 1 if signed, 0 if unsigned bendin implementation

You can then use the computed flag in a patch to set it up for the pitch bend implementation at hand.

(2) Simply routing the output of bendin into bendinfix yields the correct signed pitch bend values:

  `[bendin] --- [bendinfix]` yields signed bendin values in the -8192 ... 8191 range
  
These can then be routed to bendout without further translation, or you can divide, e.g., by 4096, add a MIDI note number, and route the result through mtof to get a pitch bend range of +/- 2 semitones.

Or, if you prefer to work with unsigned values, just add 8192 to the result of bendinfix. This will always give you unsigned values, even if your Pd flavor has a signed bendin implementation.

## See Also

- the infamous [bug #1262](https://sourceforge.net/p/pure-data/bugs/1262/)

- doc/5.reference, midi (and bendin) help patches
