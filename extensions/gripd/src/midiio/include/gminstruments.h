// 
// Copyright 1997-2000 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu> (from 18Dec1997) 
// Creation Date: 26 December 1997 
// Last Modified: Tue Apr 18 11:38:28 PDT 2000 (put CH_X defines here)
// Filename:      ...sig/include/sigInfo/gminstruments.h 
// Web Address:   http://sig.sapp.org/include/sigInfo/gminstruments.h
// Syntax:        C 
// 
// Description:   Defines names for instruments as arranged in General MIDI. 
// 

#ifndef _GMINSTRUMENTS_H_INCLUDED
#define _GMINSTRUMENTS_H_INCLUDED

#define  CH_1                             0
#define  CH_2                             1
#define  CH_3                             2
#define  CH_4                             3
#define  CH_5                             4
#define  CH_6                             5
#define  CH_7                             6
#define  CH_8                             7
#define  CH_9                             8
#define  CH_10                            9
#define  CH_11                            10
#define  CH_12                            11
#define  CH_13                            12
#define  CH_14                            13
#define  CH_15                            14
#define  CH_16                            15

#define  GM_PIANO(X)                      (0+(X))
#define  GM_ACOUSTIC_GRAND_PIANO          (0)
#define  GM_BRIGHT_ACOUSTIC_PIANO         (1)
#define  GM_ELECTRIC_GRAND_PIANO          (1)
#define  GM_HONKYTONK_PIANO               (2)
#define  GM_HONKY_TONK_PIANO              (3)
#define  GM_ELECTRIC_PIANO_1              (4)
#define  GM_ELECTRIC_PIANO_2              (5)
#define  GM_HARPSICHORD                   (6)
#define  GM_CLAVI                         (7)

#define  GM_CHROMATIC(X)                  (8+(X))
#define  GM_CELESTA                       (8)
#define  GM_GLOCKENSPIEL                  (9)
#define  GM_MUSIC_BOX                     (10)
#define  GM_VIBRAPHONE                    (11)
#define  GM_MARIMBA                       (12)
#define  GM_XYLOPHONE                     (13)
#define  GM_TUBULAR_BELLS                 (14)
#define  GM_DULCIMER                      (15)
                                 
#define  GM_ORGAN(X)                      (16+(X))
#define  GM_DRAWBAR_ORGAN                 (16)
#define  GM_PERCUSSIVE_ORGAN              (17)
#define  GM_ROCK_ORGAN                    (18)
#define  GM_CHURCH_ORGAN                  (19)
#define  GM_REED_ORGAN                    (20)
#define  GM_ACCORDION                     (21)
#define  GM_HARMONICA                     (22)
#define  GM_TANGO_ACCORDION               (23)

#define  GM_GUITAR(X)                     (24+(X))
#define  GM_ACOUSTIC_GUITAR_NYLON         (24)
#define  GM_ACOUSTIC_GUITAR_STEEL         (25)
#define  GM_ELECTRIC_GUITAR_JAZZ          (26)
#define  GM_ELECTRIC_GUITAR_CLEAN         (27)
#define  GM_ELECTRIC_GUITAR_MUTED         (28)
#define  GM_OVERDRIVEN_GUITAR             (29)
#define  GM_DISTORTION_GUITAR             (30)
#define  GM_GUITAR_HARMONICS              (31)
                       
#define  GM_BASS(X)                       (32+(X))
#define  GM_ACOUSTIC_BASS                 (32)
#define  GM_ELECTRIC_BASS_FINGER          (33)
#define  GM_ELECTRIC_BASS_PICK            (34)
#define  GM_FRETLESS_BASS                 (35)
#define  GM_SLAP_BASS_1                   (36)
#define  GM_SLAP_BASS_2                   (37)
#define  GM_SYNTH_BASS_1                  (38)
#define  GM_SYNTH_BASS_2                  (39)
                        
#define  GM_STRINGS(X)                    (40+(X))
#define  GM_VIOLIN                        (40)
#define  GM_VIOLA                         (41)
#define  GM_CELLO                         (42)
#define  GM_CONTRABASS                    (43)
#define  GM_TREMOLO_STRINGS               (44)
#define  GM_PIZZACATO_STRINGS             (45)
#define  GM_ORCHESTRAL_HARP               (46)
#define  GM_TIMPANI                       (47)
                         
#define  GM_ENSEMBLE(X)                   (48+(X))
#define  GM_STRING_ENSEMBLE_1             (48)
#define  GM_STRING_ENSEMBLE_2             (49)
#define  GM_SYNTHSTRINGS_1                (50)
#define  GM_SYNTHSTRINGS_2                (51)
#define  GM_CHOIR_AAHS                    (52)
#define  GM_VOICE_OOHS                    (53)
#define  GM_SYNTH_VOICE                   (54)
#define  GM_ORCHESTRA_HIT                 (55)
                          
#define  GM_BRASS(X)                      (56+(X))
#define  GM_TRUMPET                       (56)
#define  GM_TROMBONE                      (57)
#define  GM_TUBA                          (58)
#define  GM_MUTED_TRUMPED                 (59)
#define  GM_FRENCH_HORN                   (60)
#define  GM_BRASS_SECTION                 (61)
#define  GM_SYNTHBRASS_1                  (62)
#define  GM_SYNTHBRASS_2                  (63)
                           
#define  GM_REED(X)                       (64+(X))
#define  GM_SOPRANO_SAX                   (64)
#define  GM_ALTO_SAX                      (65)
#define  GM_TENOR_SAX                     (66)
#define  GM_BARITONE_SAX                  (67)
#define  GM_OBOE                          (68)
#define  GM_ENGLISH_HORN                  (69)
#define  GM_BASSOON                       (70)
#define  GM_CLARINET                      (71)
                            
#define  GM_PIPE(X)                       (72+(X))
#define  GM_PICCOLO                       (72)
#define  GM_FLUTE                         (73)
#define  GM_RECORDER                      (74)
#define  GM_PAN_FLUTE                     (75)
#define  GM_BLOWN_BOTTLE                  (76)
#define  GM_SHAKUHACHI                    (77)
#define  GM_WHISTLE                       (78)
#define  GM_OCARINA                       (79)
                             
#define  GM_LEAD(X)                       (80+(X))
#define  GM_LEAD_SQUARE                   (80)
#define  GM_LEAD_SAWTOOTH                 (81)
#define  GM_LEAD_CALLIOPE                 (82)
#define  GM_LEAD_CHIFF                    (83)
#define  GM_LEAD_CHARANG                  (84)
#define  GM_LEAD_VOICE                    (85)
#define  GM_LEAD_FIFTHS                   (86)
#define  GM_LEAD_BASS                     (87)
                              
#define  GM_PAD(X)                        (88+(X))
#define  GM_PAD_NEW_AGE                   (88)
#define  GM_PAD_WARM                      (89)
#define  GM_PAD_POLYSYNTH                 (90)
#define  GM_PAD_CHOIR                     (91)
#define  GM_PAD_BOWED                     (92)
#define  GM_PAD_METALLIC                  (93)
#define  GM_PAD_HALO                      (94)
#define  GM_PAD_SWEEP                     (95)
                               
#define  GM_FX(X)                         (96+(X))
#define  GM_FX_TRAIN                      (96)
#define  GM_FX_SOUNDTRACK                 (97)
#define  GM_FX_CRYSTAL                    (98)
#define  GM_FX_ATMOSPHERE                 (99)
#define  GM_FX_BRIGHTNESS                 (100)
#define  GM_FX_GOBLINS                    (101)
#define  GM_FX_ECHOES                     (102)
#define  GM_FX_SCI_FI                     (103)
                                
#define  GM_ETHNIC(X)                     (104+(X))
#define  GM_SITAR                         (104)
#define  GM_BANJO                         (105)
#define  GM_SHAMISEN                      (106)
#define  GM_KOTO                          (107)
#define  GM_KALIMBA                       (108)
#define  GM_BAGPIPE                       (109)
#define  GM_FIDDLE                        (110)
#define  GM_SHANAI                        (111)
                                 
#define  GM_PERCUSSION(X)                 (112+(X))
#define  GM_TINKLE_BELL                   (112)
#define  GM_AGOGO                         (113)
#define  GM_STEEL_DRUMS                   (114)
#define  GM_WOODBLOCKS                    (115)
#define  GM_TAIKO_DRUM                    (116)
#define  GM_MELODIC_DRUM                  (117)
#define  GM_SYNTH_DRUM                    (118)
#define  GM_REVERSE_CYMBAL                (119)
                                  
#define  GM_SOUNDEFFECT(X)                (120+(X))
#define  GM_GUITAR_FRET_NOISE             (120)
#define  GM_BREATH_NOISE                  (121)
#define  GM_SEASHORE                      (122)
#define  GM_BIRD_TWEET                    (123)
#define  GM_TELEPHONE_RING                (124)
#define  GM_HELICOPTER                    (125)
#define  GM_APPLAUSE                      (126)
#define  GM_GUNSHOT                       (127)

//
// Percussion instruments on channel 10
//

#define  GM_ACOUSTIC_BASS_DRUM            (35)
#define  GM_BASS_DRUM_1                   (36)
#define  GM_SIDE_STICK                    (37)
#define  GM_ACOUSTIC_SNARE                (38)
#define  GM_HAND_CLAP                     (39)
#define  GM_ELECTRIC_SNARE                (40)
#define  GM_LOW_FLOOR_TOM                 (41)
#define  GM_CLOSED_HI_HAT                 (42)
#define  GM_HIGH_FLOOR_TOM                (43)
#define  GM_PEDAL_HI_HAT                  (44)
#define  GM_LOW_TOM                       (45)
#define  GM_OPEN_HI_HAT                   (46)
#define  GM_LOW_MID_TOM                   (47)
#define  GM_HIGH_MID_TOM                  (48)
#define  GM_CRASH_CYMBAL_1                (49)
#define  GM_HIGH_TOM                      (50)
#define  GM_RIDE_CYMBAL_1                 (51)
#define  GM_CHINESE_CYMBAL                (52)
#define  GM_RIDE_BELL                     (53)
#define  GM_TAMBOURINE                    (54)
#define  GM_SPLASH_CYMBAL                 (55)
#define  GM_COWBELL                       (56)
#define  GM_CRASH_CYMBAL_2                (57)
#define  GM_VIBRASLAP                     (58)
#define  GM_RIDE_CYMBAL_2                 (59)
#define  GM_HI_BONGO                      (60)
#define  GM_LOW_BONGO                     (61)
#define  GM_MUTE_HI_CONGA                 (62)
#define  GM_OPEN_HI_CONGA                 (63)
#define  GM_LOW_CONGA                     (64)
#define  GM_HIGH_TIMBALE                  (65)
#define  GM_LOW_TIMBALE                   (66)
#define  GM_HIGH_AGOGO                    (67)
#define  GM_LOW_AGOGO                     (68)
#define  GM_CABASA                        (69)
#define  GM_MARACAS                       (70)
#define  GM_SHORT_WHISTLE                 (71)
#define  GM_LONG_WHISTLE                  (72)
#define  GM_SHORT_GUIRO                   (73)
#define  GM_LONG_GUIRO                    (74)
#define  GM_CLAVES                        (75)
#define  GM_HI_WOOD_BLOCK                 (76)
#define  GM_LOW_WOOD_BLOCK                (77)
#define  GM_MUTE_CUICA                    (78)
#define  GM_OPEN_CUICA                    (79)
#define  GM_MUTE_TRIANGLE                 (80)
#define  GM_OPEN_TRIANGLE                 (81)


#endif  /* _GMINSTRUMENTS_H_INCLUDED */



// md5sum:	6299d04892a6899533b9164aa9e1a874  - gminstruments.h =css= 20030102
