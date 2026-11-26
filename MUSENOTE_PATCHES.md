# MuseNote patch series

This branch is Verovio `version-6.2.0` (43f806031) plus the patches listed
below. They are carried for the MuseNote and MuseSync apps. Every other
file is unmodified upstream Verovio, under the same LGPL-3.0 licence.

## Patches

| # | Date | Commit | Files | Output change | Upstream status |
| --- | --- | --- | --- | --- | --- |
| 1 | 2026-09-25 | Fix two crashes in the PAE importer on malformed input (cherry-picked from 82a6aa3e5) | `src/iopae.cpp`, +13 / -5 | None for multi-line (`@clef:`) Plaine & Easie. A one-line (`%`) input with a key or time signature after the clef now reads them as the opening signatures instead of a change after an empty opening; ids of such input change, pitches do not | Offered as rism-digital/verovio#4450 and withdrawn; not upstream |
| 2 | 2026-09-25 | Keep an explicit xmlIdSeed on a thread that has not built an object yet (95ad8e834) | `include/vrv/object.h`, `src/object.cpp`, +10 / -2 | None for a load on a thread that already built a Verovio object. A seeded load on a fresh thread now keeps the seed (it got random ids) | Not offered upstream |
| 3 | 2026-09-25 | Return instead of assert when glyph not found (97d9daaf3, cherry-picked from upstream beeeb2150, in 6.3.0) | `src/doc.cpp`, +3 / -3 | Scores that crashed now render: a rest whose glyph the font lacks made `Doc::GetGlyphWidth` dereference a null glyph (SIGSEGV while laying out). No other output changes | Upstream (6.3.0) |
| 4 | 2026-09-25 | Decode GABC lyrics as UTF-8 (ba278bbe6) | `src/iogabc.cpp`, +1 / -1 | GABC lyrics with non-ASCII letters come out as written ("lé") instead of invalid UTF-8 (signed char) or "Ã©" (unsigned char) | Not offered upstream |
| 5 | 2026-09-25 | Skip the GABC header before reading the music (0baa9874a) | `src/iogabc.cpp`, +21 / -1 | A GABC file's header (`name: …;` lines and `%%`) is no longer read as lyric syllables; input without a header is unchanged | Not offered upstream |
| 6 | 2026-09-30 | Record the MIDI pitch each note is played at (eedc5ea3a) | `include/vrv/doc.h`, `include/vrv/midifunctor.h`, `include/vrv/toolkit.h`, `src/doc.cpp`, `src/midifunctor.cpp`, `src/toolkit.cpp`, +54 / -3 | None in the SVG, timemap, MIDI or MEI output. `GetMIDIValuesForElement` reports a note's pitch as the MIDI output plays it (staff transposition, octave lines, custom tuning), where it reported the notated pitch; new `Toolkit::GetMIDIPitches` (C++) | Not offered upstream |
| 7 | 2026-10-11 | Do not crash when the MIDI document cannot be built or holds a half octave line (c231613f0) | `src/midifunctor.cpp`, `src/toolkit.cpp`, +13 / -1 | Scores that crashed now load, play and export: an `<octave>` without a start or an end is left out of the MIDI as it is left out of the drawing, and a document its own MEI reader refuses is played as loaded, unexpanded, with a warning. None for a score that did not crash | Not offered upstream; `develop` of 2026-10-08 has both |
| 8 | 2026-10-12 | Do not leave an empty group behind in the MusicXML importer, nor pop the root group (1ec3b2c5c) | `src/iomusxml.cpp`, +9 / -2 | A MusicXML score with a `<part-group>` that has no part in it no longer carries an empty `<staffGrp>`: its MEI export loads again, and its MIDI and timemap are written from the expanded document (repeats played out) where tb.5 played it as loaded. A score with the end of a part-group that was never started now loads (it ended the process). No other id changes: the group is built, then removed. None for a score with neither | Not offered upstream; `develop` of 2026-10-08 has both |
| 9 | 2026-10-12 | Do not crash on a pedal without a start or an octave over a staff with no layer (db2307d47) | `src/midifunctor.cpp`, +5 / -1 | Scores that ended the process while their MIDI or timemap was written now play: a `<pedal>` whose `@startid` names nothing, and an `<octave>` over a staff with no layer in its measure, are left out of the MIDI. None for a score with neither | Not offered upstream; `develop` of 2026-10-08 has both |
| 10 | 2026-10-12 | Repair the MEI to Humdrum converter (Tool_mei2hum) (5e2fb6bb9) | `src/hum/humlib.cpp`, +88 / -15 | `Toolkit::ConvertMEIToHumdrum` only: the Humdrum it writes now holds the score (it was damaged for every score tried, and a chord ended the process). No SVG, timemap, MIDI, MEI or id changes: nothing else reaches the converter | Not offered upstream; `develop` of 2026-10-08 has all eleven faults |
| 11 | 2026-10-12 | Read a MusicXML octave line on its own part, and do not leave one waiting or open (1f1c88522); amended the same day by: Keep a MusicXML octave line on the staff it is written for, and end one that is never stopped on a note (1357f44c5), and by: Octave lines: no write past m_octDis for a staff the score does not have, and a stop is for its own staff's line first (8ed4a67e1) | `src/iomusxml.cpp`, +40 / -15, then +32 / -10, then +21 / -1 | A MusicXML `octave-shift` with no `<staff>` in a part that is not the first is on its own part (it was drawn over the first part, and its notes sounded an octave off); one with no note under it is not built (it took the next part's first note); one never stopped runs to the last note of its staff (it was not drawn, and its notes sounded an octave off). With the amendment: on a part of several staves a line starts on the first note of the staff its direction names (it took the next note read), and a line that waits for its first note is not taken away by another staff's stop. None for a score with none of these | Not offered upstream; `develop` of 2026-10-08 has all three |
| 12 | 2026-10-12 | Do not end the process on what the Humdrum reader or the MEI to Humdrum converter throws (15cd1c51a); amended the same day by: Do not read before the start of an empty syllable in the Humdrum reader (6d6edcbe5) | `src/iohumdrum.cpp`, `src/toolkit.cpp`, +19 / -1, then +7 / -5 | A Humdrum file the reader cannot parse is refused, and an MEI document the converter cannot read a number from gives no Humdrum, where either ended the process. None for a file that did not throw | Not offered upstream; `develop` of 2026-10-08 has both |
| 13 | 2026-10-12 | Humdrum export: a repeat's signs, the score's key and its changes, and five ways a conversion was lost (f762a7300); amended the same day by: Humdrum export: say a key of no sharps or flats, leave out a key that is no number, and two reads out of bounds (ba546c924) | `src/hum/humlib.cpp`, `include/hum/humlib.h`, +91 / -4, then +22 / -5 | `Toolkit::ConvertMEIToHumdrum` only: the Humdrum it writes has a repeat's start, one barline for a multi-measure rest, the key of a score that says it on its `<scoreDef>` or as `@keysig`, and a change of key or of meter after the start; a hairpin it cannot place, an octave out of all range and a rest with no meter no longer lose the conversion, read out of bounds or end the process. No SVG, timemap, MIDI, MEI or id changes | Not offered upstream; `develop` of 2026-10-08 has every one |
| 14 | 2026-10-12 | Do not put a section at index -1 when a repeat end follows a repeat end (d952f330d); amended the same day by: Read a repeat end with no repeat start as MusicXmlInput does: back to the last repeat start, or the beginning (31bb70748) | `src/expansionmap.cpp`, +19 / -6, then +24 / -15 | A score with no `<expansion>` of its own (ABC, Humdrum, MEI without one) in which a repeat end follows a repeat end with no repeat start between: it ended the process or lost a measure; every measure is now played, the second repeat from the last repeat start or the beginning, as `MusicXmlInput` has it (tb.8 alone: from where the first ended). None for a score whose repeats have their starts, nor for MusicXML, whose importer writes the expansion itself | Not offered upstream; `develop` of 2026-10-08 has it |
| 15 | 2026-10-13 | ABC: a double repeat however it is spelled, a repeat end with no start as ABC says, and no read before an empty field value (42b7c120f) | `src/ioabc.cpp`, `include/vrv/ioabc.h`, +63 / -5 | An ABC file with `::` or `:|:` (the sign was read as a repeat end alone, or not as a bar line at all), or with a repeat end that has no start after a repeat end or a double bar (it went back to the beginning, since tb.9; it goes back to that end or double bar, as ABC 2.1 says, and the start that is meant is written on its measure). The MEI of such a file changes, with the ids after a `::`. None for any other ABC file, and none for another format | Not offered upstream; `develop` of 2026-10-08 has all three |
| 16 | 2026-10-13 | Do not follow an order of play that costs more than any score's (in 26069d6fa and de66ee4e0) | `src/doc.cpp`, `src/iomusxml.cpp`, +29 / -1 | A score whose order of play would cost more than 1,000,000 by the count below (200 repeat ends in a row with no start) is played as written, once through, where its timemap and MIDI took minutes and a gigabyte; a MusicXML repeat is read as at most 100 times (a measure marked 2,000 times, which never came back, is played 100 times). None for a score within that cost: the dearest of the corpus comes to 30,000 | Not offered upstream; `develop` of 2026-10-08 has both |
| 17 | 2026-10-13 | A MusicXML clef for a staff the part does not have is passed over (in de66ee4e0) | `src/iomusxml.cpp`, +6 / -1 | A MusicXML part with a `<clef number>` for a staff it does not have, or does not have yet (its `<staves>` comes in a later measure), now loads; it ended the process. The clef is not built. None for a score without one | Not offered upstream; `develop` of 2026-10-08 has it |
| 18 | 2026-10-13 | Give a MusicXML note with no accidental written the sound its file says (b7b3512c9) | `src/iomusxml.cpp`, +25 / -1 | A MusicXML note with no `<accidental>` gets the sound its `<alter>` says, where the key and the accidentals the importer carries would play another key: its `<accid>` is given that `accid.ges` (a natural too, as `accid.ges="n"`), and a tie whose two notes are then of one pitch gets its end. Of the 2785 scores of the corpus 407 change in MIDI, pitch map and MEI (2968 `<accid>` lines and 507 `<tie>` lines; no line added or removed), 222 of them in drawing (244 of 3376 pages, each by its ties), and their Humdrum export; the timemap changes in none and no id of a document moves. None for a note that has an `<accidental>`, and none for MEI, ABC or Humdrum input | Not offered upstream. `develop` of 2026-10-08 reads `<alter>` too (PR #4383), but builds other objects and writes nothing for a natural |
| 19 | 2026-10-13 | Humdrum export: write the order a score is played in, as a label where each section and ending starts and one expansion list (09542e944); amended 2026-10-14 by: Humdrum export: an order of play past the limit at its least is not written (4c6f77118) | `src/hum/humlib.cpp`, +171 / -1, then +18 / -0 | `Toolkit::ConvertMEIToHumdrum` only. A score whose one `<section>` holds an `<expansion>` that names its `<section>` and `<ending>` children (every score that came as MusicXML: 2785 of the 2785 of the corpus) gains one line `*>[A,A1,A,A2,B]`, one line `*>A` for each of them that holds a note or a rest, and a line `!LO:TX:t=` right above the label of an ending whose bracket says something else than its number. Read back, it is played in that order, and its endings are endings. A measure the converter finds nothing in is no longer written; before, it took the barline of the measure before it. A carriage return in the words of a `<dir>` is a space. None for a score with no `<expansion>`, or with one the labels cannot say. The notes and the syllables of every export are as they were (2785 of 2785); three exports gain a barline that was lost. **Not to be shipped without patch 21.** No SVG, timemap, MIDI, MEI or id changes. With the amendment, an order past the limit of patch 27 at its least is not written | Not offered upstream; `develop` of 2026-10-08 writes neither |
| 20 | 2026-10-13 | Humdrum export: write the chord symbols, as text in a spine beside their staff (c92e8fa32) | `src/hum/humlib.cpp`, +115 / -67 | `Toolkit::ConvertMEIToHumdrum` only. A score with `<harm>` elements (2565 of the 2785) gains a `**cdata` spine after each staff that has one, a second line that says `*above` in it, and a line with nothing in the staves for each chord symbol that stands between two notes. Two symbols at one place are one token; the same text twice at one place is written once. A score with none is written as before. `HumGrid::setHarmonyCount` no longer writes one past the end of its counts for the hundred-and-first staff. **Declared limit: the chord symbols of a re-opened export do not follow a transposition.** No SVG, timemap, MIDI, MEI or id changes | Not offered upstream; `develop` of 2026-10-08 has the stub and the bound |
| 21 | 2026-10-13 | Humdrum reader: do not fold a whole-measure rest into the rest before it across a section label, nor one that stands under a chord symbol (c3a3eedb4) | `src/iohumdrum.cpp`, +30 / -2 | **Changes what a seeded load builds, for two kinds of Humdrum file.** A file in which the reader made one `<multiRest>` of a run of whole-measure rests that has (1) a non-null token of a `**cdata` spine on the line of a rest after the run's first, or (2) a section label (`*>A`) in its first spine between two of its rests, now keeps a measure of its own from each such rest on: it has more `<measure>` elements, every measure after that place has another ordinal, and the seeded ids of what is built after it move. Its chord symbols and its sections are no longer dropped. The patch makes no rest of several measures that was not made before. None for a file with neither: all 189 Humdrum files of the parity gate and all 2785 exports of tb.11 give the same MEI, byte for byte | Not offered upstream; `develop` of 2026-10-08 has both |
| 22 | 2026-10-13 | Humdrum export: a double barline, a dynamic at its beat and beside its hairpin, words where no note starts, the segno and coda signs, a mark given by the note it starts on, and a staff, a beat or a text the file says wrongly (22ebddf9e) | `src/hum/humlib.cpp`, +299 / -68 | `Toolkit::ConvertMEIToHumdrum` only. `right="dbl"` is written `=\|\|` (850 exports of the corpus hold one); a `<dynam>` is on the note of its beat where it was on the first note of its measure, and stays when a hairpin starts or ends on its note (17 exports of the corpus gain dynamics or hairpin signs that tb.11 lost, 38 dynamics and 19 signs; none loses one that tb.11 wrote); the words of a `<dir>` at a time no note starts (every "D.C. al Fine" and "To Coda") are written on the note before, where they were left out; `<repeatMark func="segno">` and `"coda"` are written `[segno]` and `[coda]`; a `<dir>` or a `<repeatMark>` with no `@tstamp` is written on the note, rest or chord its `@startid` names (MuseNote's own navigation marks). A `<dir>`, `<repeatMark>`, `<dynam>`, `<hairpin>` or `<staff>` for a staff the score does not have, and one whose beat is no number, is left out with a warning where it read past the end of a vector or lost the whole export; a measure that holds nothing but a change of clef keeps the barline before it. No SVG, timemap, MIDI, MEI or id changes | Not offered upstream; `develop` of 2026-10-08 has none of it |
| 23 | 2026-10-14 | What the engine says of a file is text, not the format of a message (57188f37b) | `include/vrv/vrv.h`, `src/toolkit.cpp`, `src/iohumdrum.cpp`, `src/iopae.cpp`, `src/convertfunctor.cpp`, +60 / -45 | None for any score. Forty-one log calls pass their text as the argument of `"%s"` where they passed it as the format: a percent sign in a score's words no longer ends the process when the score is exported to Humdrum, nor one in a Humdrum header line when the file is opened. The four log functions are marked as printf-like | Not offered upstream; `develop` not looked at for it |
| 24 | 2026-10-14 | ABC: no input ends the process, a tune's key is its own, and a text is not read past its end (31b351462) | `src/ioabc.cpp`, `include/vrv/ioabc.h`, `src/vrv.cpp`, +122 / -53 | An ABC file that ended the process, or never came back, now loads, or is refused where it holds no tune: a field, a decoration, a text or a tuplet that its line does not close, a `]` with no chord, lyrics under a line that ends in grace notes, a key of more than seven flats, a tempo that is no number, an `X` with no number or with no key after it, a key that is the file's last line or says nothing, a length too long for an int, a length of 0 divided by, and what an empty line left behind in the reader's lists. Two changes that are no crash: a tune with no sharps or flats in its key no longer takes over those of the tune read before it, in the same file or by the load before; and a text of any format that ends inside a character of several bytes ends before that character (bytes past the end of the text were read for it). None of the 194 ABC files of the parity set changes, and no output of the corpus | Not offered upstream |
| 25 | 2026-10-14 | ABC repeats: the implied start on a measure that begins with a bar line, an end or a double bar at the left of a measure, `[|`, a line that begins with `::`, and what stands against an ending sign (d322ed7c0) | `src/ioabc.cpp`, +42 / -14 | An ABC tune with a repeat end that has no start, where the measure to go back to begins with a bar line (lines that begin with `|`), or follows a double bar met at the left of a measure or a `[|`; a tune with a repeat end written after a bar line (`C | :| E`); a line of music that begins with `::` (it was read as an information field and lost); a note, a chord or a chord symbol written directly after `[1` (it was passed over with the sign). The MEI of such a tune changes: a `left="rptstart"` or `left="rptboth"` where there was `left="single"`, `"dbl"`, `"end"` or `"rptend"`, a `right="dbl"` for `[|`, the measures of a line that begins with `::`, the note after `[1`. None of the 194 ABC files of the parity set is written in one of these ways: none changes | Not offered upstream |
| 26 | 2026-10-14 | ABC: `Z` with no number is one measure of rest (3376852a7) | `src/ioabc.cpp`, +14 / -3 | `Z`, `Z0` and a `Z` with a number too long for an int are an `<mRest>` (they were a `<multiRest>` of 0, 0 and -1 measures: no time at all, or the notes after it not played after it). 14 of the 194 ABC files of the parity set change, 42 rests in them, each line `<multiRest xml:id="…" num="0" />` now `<mRest xml:id="…" />` with the same id and nothing else. `Z1`, `Z2` … are read as before | Not offered upstream |
| 27 | 2026-10-14 | The cost of an order of play is counted while the order is written (23ad9693e) | `src/doc.cpp`, `src/expansionmap.cpp`, `include/vrv/expansionmap.h`, `src/iomusxml.cpp`, +74 / -21 | None for an order that is followed: timemap, MIDI, pitch map, MEI and pages of all 2785 scores of the corpus are byte-identical. A score whose order is not followed (more than 1,000,000 by the count of patch 16) loads and plays as before, as written, in a fraction of a second where it took minutes or never came back; the `<expansion>` MusicXML's importer leaves in such a document now holds the beginning of the order only (2,253 references for twenty bars each marked ×100, where it held 20,810) | Not offered upstream |
| 28 | 2026-10-14 | MusicXML: files that ended the process: a `<chord/>` with no note before it, numbers that are none, and a part whose first measure does not begin with its `<attributes>` (2a99a405f) | `src/iomusxml.cpp`, +60 / -25 | A MusicXML file with a `<chord/>` on the first note of a layer, a rest whose `<display-octave>` is no number an int holds, a metronome mark whose `<per-minute>` is too large or too small for a double, a chord symbol whose `<degree-value>` is no number, a `<staves>` that is no number of staves, or a part whose first measure begins with a note now loads; each ended the process, the last for a second or later part and for a part of two staves, and read past the end of `m_octDis` in any part. One kind of file that loaded before changes: a score of one part of one staff whose first measure does not begin with its `<attributes>` has that part's id, name and instrument on its `<staffDef>` (it was a bare one, made at the end of the import), and an octave line in it is read (it was passed over). No score of the corpus is such: no output of the corpus changes | Not offered upstream |
| 29 | 2026-10-14 | The MIDI writer comes back from a meter whose unit is below one (35a5f9173) | `src/midi/MidiMessage.cpp`, +6 / -1 | A score whose meter has a unit below zero (a MusicXML `<beat-type>` of -1, or one too long for an int, which comes out below zero) loaded, was drawn and had a timemap, and its MIDI never came back: in the apps it opened and could not be played. It is written, with 0 for the power of two of that unit. None for a unit of 0 or more: the same power of two for each | Not offered upstream |
| 30 | 2026-10-20 | An MEI file's own order of play: no expansion within itself, nothing inside a measure, a copy of a copy (ca2190993) | `src/expansionmap.cpp`, `include/vrv/expansionmap.h`, +25 / -2 | An MEI file whose own `<expansion>` names itself, or two that name each other, or names a layer, staff or note (then a section, then it again), or a section and the section that holds it, now loads and plays; each ended the process (932 of 7,254 hand-made orders) or never came back (334). An expansion within itself, and an element that is no section, ending or measure nor an editorial element at their level (a reading, a choice and what it chooses between...), or that stands inside a measure, is passed over with a warning, as an element that is not there is. None for an order that names only sections, endings, editorial elements, measures and expansions, none inside a measure and no expansion within itself: no output of the corpus changes. Of the hand-made orders that came back on tb.13, 61 that name a staff, layer, note or a reading inside a layer, and 16 that name a section-level scoreDef, staffGrp, staffDef, sb or dir, play otherwise | Not offered upstream |
| 31 | 2026-10-20 | The cost of an order of play: one walk of the document, long ids weighed, nested expansions counted, nothing wraps (5abee1928) | `src/doc.cpp`, +71 / -13 | None for an order whose ids are under 64 bytes, that names no expansion, and whose references all carry their `#` or none does: the cost of every score of the corpus is the same (the dearest 29,977). An expansion the order names is counted as what it plays (sixteen expansions each naming the next twice played a section 65,536 times at a cost of 4, and the load did not come back, on tb.13 too): past the limit, the order is played as written. A section named with and without its `#` is one section, as `Expand` reads it. The count walks the document once (5,000 one-bar sections named once: 27.9 s, now 15.8 s for the whole load and exports); an id weighs 1 more for every 64 bytes, so an order of very long ids past the limit by it is played as written; a reference named more than 1,000,000 times is past the limit even if it names nothing; nothing wraps on 32 or 64 bits | Not offered upstream |
| 32 | 2026-10-20 | MEI: a staff the score has no staffDef for is refused (af3544ffe) | `src/iomei.cpp`, +13 / -1 | An MEI file with a shown `<staff>` whose `@n` the score's `<scoreDef>` has no `<staffDef>` for is refused with an error in the log, as one with a `<staffGrp>` without a `<staffDef>` is; it ended the process at load. None for a file that loaded | Not offered upstream |
| 33 | 2026-10-20 | Humdrum reader: a note outside ten octaves is not looked up in the tables of its accidentals and its linked ties (068def0c6) | `src/hum/humlib.cpp`, +30 / -13 | None for `**kern` notes and ornament notes in octaves 0 to 9. A note of octave 10 or more keeps its pitch and its accidental is played, not shown (`accid.ges`), as upstream already does for a note below the table; an ornament reaching outside the table gets no accidental; a linked tie on such a note is not linked (the two notes still tie). Each read or wrote outside a table of 70 or 400 entries; a file with such a linked tie, refused on tb.13, now loads | Not offered upstream |
| 34 | 2026-10-20 | Humdrum export: the terminal barline comes back from a grid whose last measure holds no slice (65919b6a2) | `src/hum/humlib.cpp`, +5 / -3 | None: `HumGrid::addLastMeasure` checks that its grid and its last measure hold something before it reads them (it called `back()` on an empty list). No converter reaches it through the functions the apps call on tb.13 | Not offered upstream |
| 35 | 2026-10-20 | Humdrum reader: an order of play is read up to what the engine follows (5e327725b) | `src/iohumdrum.cpp`, +9 / -2 | None for an order that is followed. A Humdrum file's own order of play is gathered as the engine's other writers gather one (`OrderOfPlay`) and stops past the limit: 100,000 labels never came back (stored in quadratic time, every later pass quadratic too) and load in 0.9 s; the order is played as written, as before, and its MEI holds the beginning of the list | Not offered upstream |
| 36 | 2026-10-20 | Humdrum reader: a **vvdata token of more than 64 words is one verse (632dd0376) | `src/iohumdrum.cpp`, +26 / -1 | None for a token of at most 64 words (a word is what stands between spaces), split as tb.10 splits it. A longer token is one `<verse>` holding its whole text: 3,000 words never came back (a pass over the document for every verse number) | Not offered upstream |
| 37 | 2026-10-20 | Humdrum reader: a line is read in time linear in its length (9033a960e) | `src/hum/humlib.cpp`, +4 / -1 | None: `HumdrumLine::getChar` reads the line in place, where it copied the whole line for every character (a line of 2 MB never came back) | Not offered upstream |
| 38 | 2026-10-20 | ABC: a tuplet with a number too long for an int, or of no notes, is none (25b554594) | `src/ioabc.cpp`, +28 / -9 | An ABC tuplet whose first number is 0, or whose first or second number does not fit an int, is no tuplet (with a warning), its notes read as written: `atoi` made `(99999999999` a tuplet of 1,215,752,191 notes, and the MIDI and the Humdrum export failed. None for a number that fits; none of the 194 ABC files of the parity set changes | Not offered upstream |
| 39 | 2026-10-20 | A staff's count of lines past what any staff has is five, from any format (6becedd2b) | `libmei/dist/atts_shared.h`, +8 / -1 | A staff of fewer than 0 or more than 32 lines, from any format, has five (a staff of two billion never came back); a count of 0 to 32 is kept, an unset one passes. Generated code: a regeneration of libmei or a base move carries it again. No output of the corpus or the parity set changes | Not offered upstream |
| 40 | 2026-10-20 | MusicXML: a part that begins with a note has the transposition its first attributes give (e0dd4d0a6) | `src/iomusxml.cpp`, +27 / -16 | A MusicXML part that begins with a note and has a `<transpose>` in its first `<attributes>` is transposed (it played as written). Nothing is built, no id moves. **Declared:** the `trans.diat` of a part with an `<octave-change>` is its `<diatonic>` and the octave (upstream took the `<chromatic>`'s semitones: a bass clarinet said a tenth for a ninth; MuseNote spells sounding notes from it), by that attribute alone: none of the corpus, whose seven parts with one transpose by whole octaves, where the two readings agree. A transposition given later in a part is still not read. No score of the corpus begins with a note | Not offered upstream |
| 41 | 2026-10-20 | MusicXML: an ending with a blank number, and one ending in two brackets, are played (495eb5f16) | `src/iomusxml.cpp`, +29 / -7 | **Changes the order of play of 19 scores of the corpus, by design.** An ending with a blank number (MusicXML 4.0: there is an ending, the writer could not tell which) is the pass after the last, and one ending written as two brackets is played whole; both were never played (15 scores, 99 notes). Only the `<expansion plist>` of their MEI changes, and their timemap and MIDI; no object is built that was not, no id moves, no pitch changes, the page is the same. Three of the 19 lost no note: a second bracket of a first ending is heard on the first pass too (2), a "1st time only" bracket on pass 1 as well (1) | Not offered upstream; `develop` of 2026-10-20 has the same code |
| 42 | 2026-10-20 | MusicXML: an ending whose pass a repeat has already left is the next pass (01d1beae7) | `src/iomusxml.cpp`, +3 / -3 | **Changes the order of play of 7 more scores, by design**: an ending numbered like one before it after that one's repeat (a file that contradicts itself; in three of the six the bracket's text says the right number) is the next pass, where it was never played (46 notes). Only the order of play, timemap and MIDI | Not offered upstream |
| 43 | 2026-10-20 | MusicXML: a D.S. looks for its segno, and a To Coda for its coda (2abe20548) | `include/vrv/iomusxml.h`, `src/iomusxml.cpp`, +12 / -9 | **Changes the order of play of 2 more scores, by design**: a `<sound dalsegno>` naming a coda's label is passed over with the warning it always had, where it jumped forward to the coda (194 notes never played); a segno and a coda in one measure both count. Only the order of play, timemap and MIDI | Not offered upstream; `develop` of 2026-10-20 has the same code |
| 44 | 2026-10-20 | The engine's fractions are exact while a result fits an int, and the nearest one toward zero that fits when it does not (010630a20) | `include/vrv/fraction.h`, `src/fraction.cpp`, `src/durationinterface.cpp`, +78 / -42 | **Changes the timemap of one score of the corpus, by design**: Wave, whose 488 entries are in time order (four were out of it; its MEI, MIDI and the notes it starts do not change). Each term of `vrv::Fraction` is taken in 64 bits and reduced before it is stored; a result that does not fit an int is the fraction nearest to it toward zero that does. A tuplet or a time whose terms passed an int no longer wraps (a bar's first tuplet of 32768 notes put a time before one it follows; 1000 bars of partial tuplets of 251 gave times below zero). An MEI note with dots below 0 lasts as one with none, and dots that add less than an int's fraction holds add nothing; an element of `@num="0"` in a tuplet (no valid MEI) lasts as its tuplet says, where the 0 took the tuplet's ratio away too. A time past what the terms hold is the largest they hold (2,147,483,647 quarter notes in the timemap). The mensural durations still multiply `@num` in an int | Not offered upstream |
| 45 | 2026-10-20 | Humdrum export: a score of staves with no measure has one to be written into, and neume notation is refused (3681b2a75) | `src/hum/humlib.cpp`, +15 | `Toolkit::ConvertMEIToHumdrum` and `getHumdrum` only. A chant read from GABC (all three of the parity set's) and a mensural MEI with its clef in the layer ended the process; the mensural score is written with its notes, and a score of neume notation is refused with an error in the log (its export held a clef and no note, when it came back at all). None for any other score that converted | Not offered upstream |
| 46 | 2026-10-20 | MEI: a staffDef of the score with no @lines is a staff of five lines (426a49816) | `src/iomei.cpp`, +8 | An MEI `<staffDef>` of the score's `<scoreDef>` with no `@lines` (MEI allows it) kept the unset value: its staff was drawn with no line, and layout and drawing overflowed in six files. It has five, and the MEI the engine writes says `lines="5"` on it. None for a file whose staffDefs say their count | Not offered upstream |
| 47 | 2026-10-20 | The timemap passes over a hidden staff, as the alignment does: a hidden ossia staff has no staffDef (a83138c05) | `src/midifunctor.cpp`, +4 | An MEI file with a hidden ossia staff (an `<oStaff>` with no layer the engine reads, or any ossia under `ossiaHidden`) loaded and drew, and its timemap, MIDI and pitch map ended the process. None for a file whose timemap came back | Not offered upstream |
| 48 | 2026-10-25 | A score definition's insert flag starts false (1aadb2e1b) | `src/scoredef.cpp`, +1 | `ScoreDef::Reset` left `m_insertScoreDef` unset, and `ScoreDefSetCurrentFunctor` read it for a first measure no score definition with a clef, key, meter or mensuration came before (a GABC chant, a mensural score; UBSan: a load of a value that is no bool). None where the flag was set before it was read | Not offered upstream |
| 49 | 2026-10-25 | MusicXML: a tuplet's numbers are read as the int they are (22792787b) | `src/iomusxml.cpp`, +2 / -2 | A MusicXML tuplet whose `<actual-notes>`, `<normal-notes>` or `<tuplet-number>` passed 32,767 wrapped in a `short int` and was timed as one the file does not write (3 in the time of 40000 as 3 in the time of 1). None for numbers up to 32,767: none of the corpus is larger | Not offered upstream |
| 50 | 2026-11-11 | MusicXML: write the sign of a note whose page would not show its sound (ca114f5) | `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, +379 | A MusicXML note with no `<accidental>` whose page would read another sound than it plays gets the sign of that sound, on the `<accid>` the importer built: 1,336 signs in 160 of the 2,765 MusicXML scores of the corpus, whose MEI export and drawing change (640 outputs: each one's page, MEI and two locator exports). The MEI is byte for byte what MuseNote's Dart pass wrote (V3_2_0_09). No id of a document moves, and no MIDI, pitch map or timemap changes. None for a score with no such note, and none for MEI, ABC or Humdrum input | Not offered upstream |
| 51 | 2026-11-12 | MusicXML: a tuplet holds only the notes its bracket covers (00483f1); amended the same day by: MusicXML: patches 51 and 52 as the corpus gate found them (a2aed60) | `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, +84 / -1, then (with patch 52) +28 / -16 | A beam and a tuplet that start on one note nest by where each really ends in the note's own voice; a tuplet's stop closes what was opened inside it and goes on past it (a beam then ends with the tuplet, with a warning), after the note's own beam end, and the chord the note begins stays open. 7 scores of the corpus change: each tuplet holds only its bracket's notes, and the notes after it play their own time. The objects are built in the same order: no id moves | Not offered upstream |
| 52 | 2026-11-12 | MusicXML: a gap, a hidden rest and an unbracketed tuplet note play their time (6650892); amended the same day by a2aed60 (above) | `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, +199, then (with patch 51) +28 / -16 | A `<forward>` or hidden rest whose length is no sum of written values where it stands, and a note, chord or rest timed by a `<time-modification>` that no `<tuplet>` shows, is set in a tuplet that shows neither bracket nor number, holding written values that add up exactly; a gap that is a sum of written values is written as those values, counted in whole ticks. A walk after the import does it: the import's spaces keep their ids, the surplus ones go, the new tuplets and spaces take ids after every other. 9 scores: 50 spaces rewritten, 39 spaces gone, 31 unseen tuplets | Not offered upstream |
| 53 | 2026-11-12 | MusicXML: a rest or note is written as its ticks play it (cb58fcb) | `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, +91 / -1 | A rest that fills its bar but is typed as another length is the bar's rest (`<mRest>`, the same object and id), as the importer's own comment says; a note, chord or rest whose ticks make a written value (a power of two, up to two dots) and whose type says another is written that value; a grace rest is left out, with a warning. Not for a note in a tuplet, with a time-modification or of a tremolo. 16 scores: 33 rests become bar rests, 9 values are written as they play, 3 grace rests go | Not offered upstream |
| 54 | 2026-11-12 | Tuplets: nested ratios multiply, every MusicXML tuplet level is read, ABC tuplets hold the notes ABC gives them (11e1428) | `src/layerelement.cpp`, `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, `src/ioabc.cpp`, `include/vrv/ioabc.h`, +348 / -129 | A tuplet inside a tuplet plays the product of the ratios (MEI's rule; the innermost alone was read): an MEI file, or a MusicXML file that numbers each level, with nested tuplets plays otherwise, and no score of the corpus has one. A nested MusicXML tuplet holds its own level's ratio (3 and 3 where the inner one said 9); two tuplets that start or stop on one note are both read. An ABC tuplet holds the notes ABC gives it: 13 of the 194 ABC files of the parity set change (180 tuplets built where 130 were, and the notes that went with the lost ones are back; their generated ids move). Every MusicXML, MEI, Humdrum and PAE output of the corpus is byte-identical | Not offered upstream |
| 55 | 2026-11-12 | What a reader asks of the engine: no staff labels, the channels of staves; a text keeps its own spaces through MEI (d6caaaf); amended the same day by: The channels given to staves reach the document the MIDI is written from (bc4c9fe, tb.19) | `include/vrv/options.h`, `src/options.cpp`, `src/view_page.cpp`, `src/doc.cpp`, `src/iomei.cpp`, +43 / -4 | Two options, both off by default: `staffLabelsHidden` (labels are not drawn and leave no room) and `midiStaffChannels` (a listed staff plays on the channel given). With both off no page, MEI, timemap or pitch map of the corpus changes. The MEI reader keeps a space at the start or end of an element's text unless the run holds a line break: 92 MIDI files of the corpus change, in the spaces of 3,673 lyric texts and one track name (the MIDI is written from the document read back from its own MEI) | Not offered upstream |
| 56 | 2026-11-13 | MusicXML: an ending is played in the passes its number names, also where plain bars stand between the endings of a repeat; after a jump back the last ending of the repeat is the one played (6bca52a) | `src/iomusxml.cpp`, +58 / -5 | **Changes the order of play of 272 scores of the corpus, by design.** `MusicXmlInput::CreateExpansion` played the bars of a repeat again whole, an ending among them too: with plain bars between a first ending and the bar that holds its repeat sign, the ending numbered 1 was played on pass 2 as well (and after a D.C. or D.S. although it is not the last). Their MIDI, timemap, pitch map and the `<expansion>` of their MEI change; no page and nothing else of any MEI does. One score (Song For Guy) no longer plays a first ending's bar at all, which only its jump-back pass reached | Not offered upstream |
| 57 | 2026-11-13 | One rule for the order of play, for every format: the general rule (MEI, ABC, Humdrum and any score with no order of its own) reads endings and jumps as the MusicXML reader's does (aefcf4e) | `src/expansionmap.cpp`, `include/vrv/expansionmap.h`, `src/iomusxml.cpp`, `include/vrv/iomusxml.h` | **Changes the order of play of scores that are not MusicXML and have endings, jumps, or measures inside sections, by design.** `ExpansionMap::GenerateExpansionFor` knew repeat signs only, and only measures standing directly in the one section: an ending was played in every pass, no jump was taken, and a score with its measures in endings or sub-sections played no repeat. No output of the corpus changes (21,743 outputs, 0 differ): MusicXML goes through the same code as before, and no bundled score of another format has an ending or a jump | Not offered upstream |
| 58 | 2026-11-14 | ABC: the reader rebuilt on the standard, in two halves: the text read into tunes, each tune written into the score (882ff86) | `src/ioabc.cpp`, `include/vrv/ioabc.h`, `src/expansionmap.cpp`, `include/vrv/expansionmap.h` | **Changes how ABC is read, by design** (MuseNote root cause RC6). Replaces patches 7, 13, 22, 31, 41 and 49's work in the reader. No output of the corpus moves but the ABC inputs' MEI (194 of 21,743). MuseNote's ABC export of 2,765 songs read back: 2,359 play the same in every aspect (56 before); 1,037 hand-written tunes against abc2midi: 984 the same (603). A stored ABC score's notes keep their locator where the old reader read them rightly: 99.6% (hand-written tunes), 93% (MuseNote's exports) | Not offered upstream |
| 59 | 2026-11-14 | ABC: a file is told by its tunes, and its file header sets the defaults of every tune (8db7dda) | `src/toolkit.cpp`, `src/ioabc.cpp` | **Reads ABC files the engine refused or misread.** `IdentifyInputFrom` looked at an ABC file's first character only; a file that began with an empty line, a comment or a file header (ABC 2.1, 2.2) was taken for MEI, PAE or GABC. Of 1,055 hand-written files and tunes read by detection alone, 345 were read before, 1,055 now. No output of the corpus changes (21,743 outputs, 0 differ) | Not offered upstream |
| 60 | 2026-11-14 | Expansion: every tie, slur and line of the score as played is judged by the order of play (b1b4318) | `src/expansionmap.cpp`, `include/vrv/expansionmap.h`, `src/doc.cpp`, +221 | **Changes what 192 scores of the corpus play, by design** (MuseNote root cause RC18, finding W53). A copy of a spanning element got its end when it was made, or as a count of played bars: the second time through or after a jump a tie ended on another pass's note, or on one already played (a note held for the wrong length, its end note attacked again, or a note held past the end). 227 MIDI outputs of 21,743 change (192 songs and 35 corrected files of the same songs); no page, MEI, timemap, pitch map or id | Not offered upstream |
| 61 | 2026-11-14 | Order of play: a repeat sign on the page is played (58ac878) | `src/expansionmap.cpp`, `src/iomusxml.cpp`, +30 | **Changes the order of play of 26 scores of the corpus, by design** (MuseNote root causes RC19, RC20; findings W54, W51). A repeat sign at the end of the last ending of its group was never taken (the passes were those its endings name only), and at a barline that holds an ending's stop the MusicXML reader dropped the repeat's count. Their MEI `<expansion>`, MIDI, timemap and pitch map change (186 outputs of 21,743, with the ended originals and corrected copies of the same songs); no page and no note id | Not offered upstream |
| 62 | 2026-11-14 | MusicXML: a tie joins a note to the next note of its voice (62fff07) | `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, +125 / -32 | **Changes what 31 corpus files play, by design** (MuseNote root cause RC21, finding W55): 41 ties in 27 songs joined a note to a later note of its pitch, one to 79 notes on (the notes between not heard as written); a one-note tie silenced its note. Their MEI, page and MIDI change. The MEI of 287 more scores loses 3,022 ties that had a start and no end (empty groups on the page, never heard); nothing drawn or heard changes there. 1,303 outputs of 21,743 in all; no note id | Not offered upstream |
| 63 | 2026-11-14 | Order of play: an ending is played in every pass its label names and its signs reach (7795a50, 3788b5f, f582922) | `src/expansionmap.cpp`, `include/vrv/expansionmap.h`, +61 / -15 | **Changes the order of play of 7 scores of the corpus, by design** (MuseNote root causes RC26, RC27, RC29; finding W56). An ending's label was read as a list only where commas stood between its numbers ("1.2." the first pass alone, "1-5" passes 1 and -5); every pass a label named was played, as if a sign sent the play back for each; and a sign further on brought round an ending that has its own, passing its second time over. Their MEI `<expansion>`, MIDI, timemap and pitch map change (41 outputs of 21,743 with the locator copies); no page and no note id | Not offered upstream |
| 64 | 2026-11-14 | Order of play: a jump is what its sound says, taken the times it names, as many as its barline holds (5533c67); amended the same day by: MusicXML: ask a sign's sound only where it has one (0e46341), tb.29 | `src/expansionmap.cpp`, `include/vrv/expansionmap.h`, `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, +165 / -59 | **Changes 13 scores of the corpus, by design** (MuseNote root causes RC34, RC35, RC36, RC38, RC39): a jump drawn as a sign alone is that jump in the score (Arriverderci Roma, Ein bißchen Frieden and its v2, Het Dorp, Mexican Hat Dance, Oblivion: their MEI, SVG and ids, no timing); the marks of the bars a multi-bar rest spans are read (Superstition jumps; 10,000 Reasons, I'm So Lonesome I Could Cry, Music to Watch Girls By and Sweet Hour of Prayer play their tempo; Afro Blue and My Tribute show their words). A jump's named times, two jumps at a barline and a repeat taken after the jump in endings change no corpus score: the corrected song files of MuseNote plan V3_2_0_44 use them | Not offered |
| 65 | 2026-11-14 | The score keeps what its file says of its order of play (9dcdca2) | `src/expansionmap.cpp`, `include/vrv/expansionmap.h`, `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, +175 / -31 | **Changes the MEI of 36 corpus scores, by design** (MuseNote root cause RC41): a mark keeps the label of its coda or segno (`@label`) and a jump its named times (`@type` token `times-2.3`), a repeat end its count and after-jump (`repeat-N`, `after-jump`), and a jump given by a sound alone gets a mark that draws nothing (6 scores; their ids move, the annotation locator re-anchors every mark: 0 failures). No order, timing or pitch changes; the page only by the labels' tooltips. The rule for a score with no order of its own reads them, so an MEI file plays as its MusicXML | Not offered |
| 66 | 2026-11-14 | A jump written as its words, or a target as its sign, is played (a371841) | `src/iomusxml.cpp`, `include/vrv/iomusxml.h`, `src/expansionmap.cpp`, +148 / -8 | **Changes the order of play of 49 live songs and 7 superseded versions, by design** (MuseNote root cause RC42): the MusicXML reader reads a jump written as the words of MuseScore's vocabulary and a target written as its sign, where no `<sound>` gives its kind; a kind the score's sounds give is played by them, its words and signs print (an untyped `<dir>`, above the staff where unplaced); a coda sign in a To Coda's measure is that To Coda's; the order of play takes a jump naming its kind's default label to the first target of its kind. The MEI and page of 85 more change (words typed, signs print); ids move where a sign becomes print (annotation locator: 0 failures) | Not offered |
| 67 | 2026-11-15 | Order of play: a repeat played after the jump plays as the first time, and its Fine or To Coda is taken (6616db0) | `src/expansionmap.cpp`, +42 / -3 | **Mends a crash and an order for files that mark a repeat after-jump** (MuseNote root cause RC47): where a jump lands, the repeat it lands in starts at the last repeat start at or before it, or the score's beginning (Maple Leaf Rag so marked ended the process); after a jump back an ending's Fine or To Coda is taken the last time that ending is played. Of the corpus as published, one score plays otherwise by design (The Fool On The Hill); no id moves | Not offered |

### 1. PAE importer crashes

- `PAEInput::CheckHierarchy` started each pass with an empty container
  stack, so content before the first measure token read `stack.back()`
  of an empty list (SIGSEGV). Each pass now starts with the layer token
  on the stack, and an unmatched closing token never pops it.
- `PAEInput::SingleLineToJson` built the data string from `end() + 1`
  when a one-line input had no space after the scoreDef (abort), and
  dereferenced past the end on a trailing space. Both are guarded; a
  line that is only a scoreDef has empty data. The `||` tautology in the
  scoreDef-end test is now `&&`, as the code comment intends.

Regression inputs: `@clef:G-2` + `@data:{=6}B` (was SIGSEGV) and `%G-2`
(was SIGABRT) now load and render.

### 2. Explicit id seed on a fresh thread

Id counters are `thread_local`, and the first object built on a thread
reseeded them randomly. A `setOptions` seed on a thread that had not built an
object yet was therefore replaced by the first object of the next `loadData`,
and that load got random ids. MuseNote's engine worker is a Dart isolate,
which can run a message on another OS thread than the one that built its
toolkit; its annotations are anchored to seeded ids. `SeedID` now marks the
thread seeded, and the first-object reseed runs only on a thread never seeded.

Regression test: MuseNote's `corelib/verovio/tool/parity/seedthread.cpp`
(toolkit built on one thread, seeded import on a fresh thread, ids compared)
fails on tb.1 and passes on tb.2.

## Verification, 2026-09-25

tb.3 against tb.2 (same toolchain, MuseNote's `tool/parity` with the
annotation-locator corpus): 2079 outputs compared, 14 differ, all declared —
the 10 Humdrum inputs (of 189) that crashed tb.2 now export, their status
file, and the 3 GABC chants (header skipped, lyrics valid UTF-8). Structural
note positions identical in every format (0 locator failures over 57,000+
notes); seeded ids deterministic on a fresh thread for 209 of 209 scores.

### 3. Missing glyph (upstream fix)

Upstream's beeeb2150, released in 6.3.0, applied as is. `Doc::GetGlyphWidth`,
`GetGlyphHeight` and `GetGlyphAdvX` asserted that the glyph exists; in a
release build the null glyph was dereferenced. 10 of the 189 Humdrum files in
MuseNote's parity corpus (Verovio's own Humdrum export of bundled scores)
crashed 6.1.1 and 6.2.0 while laying out a rest. Dropped at the 6.3.0 base
move, which contains it.

### 4. GABC lyrics as UTF-8

`GABCInput::ProcessWord` converted the lyric std::string to std::u32string
byte by byte. Regression input: MuseNote's `tool/parity/gabc/kyrie.gabc`
("eléison").

### 5. GABC header

`Toolkit::IdentifyInputFrom` recognises GABC by the `%%` line that ends the
header, but `GABCInput::Import` read the header as music. Regression inputs:
MuseNote's `tool/parity/gabc/*.gabc`; a body-only input is unchanged.

### 6. The pitch the MIDI plays

`GenerateMIDIFunctor` plays a note at its notated pitch moved by the staff's
`@trans.semi` (which the MusicXML importer sets from `<transpose>`) and by
`<octave>` lines, through a custom tuning when one is set, but keeps that
pitch private; `GetMIDIValuesForElement` reported `Note::GetMIDIPitch()`
without them. MuseNote grades a take against a note's pitch and finds the
note's MIDI events by it, so its bundled guitar score (`<octave-change>-1`)
was graded an octave above what its guide plays, and every note under an
MEI 8va/8vb line was off by an octave. The functor now records the pitch of
each note it visits, a secondary tied note included, `Doc::ExportMIDI`
passes the record through, and `Toolkit::GetMIDIPitches` returns it for
every note (a note the MIDI output does not reach keeps its notated pitch);
`GetMIDIValuesForElement` reports it. Carried because the pitch exists only
inside the MIDI functor: outside the engine it would have to be re-derived
from `Doc::ExportMIDI` and the functor's rules, and could drift from the
MIDI.

Regression test: MuseNote's `corelib/verovio/test/native/test_pitch_map_engine.cpp`
(guitar and B-flat transposition, MEI 8va and 8vb, a tie, a chord, two
parts, and the bundled "Dream (When You're Feeling Blue)"), each case also
checked against the MIDI note-ons: 8 of 12 cases fail on tb.3, all pass on
tb.4.

tb.4 against tb.3 (same toolchain, MuseNote's `tool/parity`, which now also
dumps each score's pitch map): 2288 outputs compared, 1 differs, as
declared — the pitch map of "Dream (When You're Feeling Blue)", all 71
notes 12 lower. Every SVG, timemap, MIDI, MEI and locator output is
byte-identical; seeded ids deterministic on a fresh thread for 209 of 209.

### 7. Two crashes on the way to MIDI

Two MusicXML lead sheets of a user's collection ended the process while
their MIDI or timemap was written. In the apps the engine runs inside the
app's own process, and MuseNote opens any file a user imports.

- An `<octave>` with only one of its ends. A MusicXML `octave-shift`
  started and stopped after the last note of the piece is given that note
  as its end and no start. It is not drawn (`View::HasValidTimeSpanningOrder`
  wants both), but `InitMIDIFunctor::VisitOctave` registered it, and
  `GenerateMIDIFunctor::HandleOctave` read the alignment of both ends: a
  null start was dereferenced at the first note of that staff and layer.
  The octave is now left out of the MIDI as it is left out of the drawing,
  so what sounds is still what the page shows.
- A document its own MEI reader refuses. `Toolkit::SetMidiDoc` builds the
  document the MIDI and the timemap are written from by exporting the
  loaded document to MEI and importing that, and ignored the import's
  result. A MusicXML `<part-group>` with no part in it becomes an empty
  `<staffGrp>`, which `MEIInput` refuses ("Each <staffGrp> must contain at
  least one <staffDef>"); `PrepareData()` on the empty document then took
  the first of no visible scores (an assert in a debug build; a release
  build ended with SIGSEGV). The loaded document is now played
  instead, unexpanded, with a warning. (Patch 8 has since stopped the
  MusicXML importer from leaving such a group behind. The fallback stays
  for any other document the reader refuses.)

Carried because both faults are inside the engine: neither file can be
told from a sound one before it is loaded, short of parsing it as the
engine does.

Regression test: MuseNote's `corelib/verovio/test/native/test_midi_document_engine.cpp`
(hand-made copies of what the two files had in them, each checked against
the MIDI note-ons and the pitch map): both cases fail on tb.4, all 14
cases of the target pass on tb.5.

tb.5 against tb.4 (same toolchain, MuseNote's `tool/parity`, whose score
corpus is now 2785 scores): 19930 outputs compared, 0 differ; seeded ids
deterministic on a fresh thread for 2785 of 2785.

### 8. An empty group, and a group end without a start

Patch 7 kept the process alive for a MusicXML `<part-group>` with no part
in it, but the loaded document still held the empty `<staffGrp>`: its MIDI
was played as loaded, with no repeat played out, and its own MEI export
could not be loaded again ("Each <staffGrp> must contain at least one
<staffDef>"). MuseNote's reader loads that export a second time, without
the part names, and keeps the export as the score; both were refused for
"Walking On Sunshine", one of the two lead sheets of patch 7.

- `MusicXmlInput::ReadMusicXml` now removes a group that is closed with no
  `<staffDef>` in it. The group is still built first, so the objects are
  constructed in the order they were, and every other id is as before:
  the MEI of the lead sheet differs from tb.5's by that one line, and all
  1505 other ids are the same.
- The end of a part-group that was never started popped the root group
  off `m_staffGrpStack`; the next `<score-part>` was then added to
  `back()` of an empty vector (SIGSEGV). Such an end is now passed over.

Carried because the group is built inside the importer: outside the
engine it could only be removed by rewriting the file before it is
loaded, or the MEI after.

### 9. A pedal without a start, an octave over no layer

Found by reading the MIDI functors for the fault of patch 7 a second
time. Both ended the process.

- `GenerateMIDIFunctor::VisitPedal` read the alignment of the pedal's
  start. A `<pedal>` whose `@startid` names nothing has none.
- `InitMIDIFunctor::VisitOctave` asserted that the staff the octave
  starts on has a layer, and read its number. A staff with no `<layer>`
  in that measure has none: an assert in a debug build, a null pointer
  read in a release build.

Each is now left out of the MIDI.

Regression test: MuseNote's `corelib/verovio/test/native/test_midi_document_engine.cpp`,
now five cases. On tb.5 the empty group's case fails, and the group end,
the pedal and the octave each end the process (SIGSEGV); all 17 cases of
the target pass on tb.6.

tb.6 against tb.5 (same toolchain, MuseNote's `tool/parity`, 2785
scores): 19930 outputs compared, 0 differ; seeded ids deterministic on a
fresh thread for 2785 of 2785. No score of the corpus has an empty group.

### 10. The MEI to Humdrum converter

MuseNote exports a score as Humdrum with `Toolkit::ConvertMEIToHumdrum`,
which hands the MEI to humlib's `Tool_mei2hum`. Over the 2785 scores of
MuseNote's parity corpus every export was damaged, and a score with a
chord ended the process. Eleven faults:

- **The time base.** The converter counts whole notes, the grid
  (`HumGrid`) quarter notes. `QUARTER_CONVERT`, the conversion, was
  defined empty: every note lasted four times as long as it does and
  overwrote the notes after it ("ZZZ" tokens, lost bars). It is `* 4`
  again.
- **`m_xmlidQ = 1; // for testing`.** An `**xmlid` spine was written for
  every staff, and a chord has no data slice to hang its ids on (a null
  pointer read). The option is read as given.
- **`<ending>`** was passed over by `parseSection`: every measure under a
  first or second ending was lost. It is read as a section is.
- **Clef, key signature and meter as child elements** of `staffDef` or
  `scoreDef`, which is how Verovio's own MEI export writes them, were
  not read (only the attributes were): no clef, key or meter in the
  output, and whole-measure rests with no length.
- **A measure's length** was worked out in whole numbers: 3/8 gave one
  quarter note.
- **A measure number** was read with `stoi()`, which throws on "X1" (a
  pickup bar from MusicXML).
- **`HumGrid::addMeasureLines`** read the first spined slice of the next
  measure without looking; a measure with nothing in it has none.
- **A multi-measure rest** was left empty and its time lost. It is
  written out as that many measures of rest, at most 999: the number is
  the file's own, and a few bytes can say any.
- **A tempo mark** was filed at its measure's start in the converter's
  time, not the grid's: anywhere but at the start of the score it stood
  three quarters of the way back, and overwrote what was there.
- **A syllable** was written as it came: a line break or a tab in it cut
  its line in two. It goes through `cleanVerseText()`, which now also
  takes a carriage return.
- **A note under an octave line** was written at the octave it is drawn
  in. `**kern` holds what sounds: `@oct.ges` is read first.

Carried because the converter is inside the engine: what it writes could
only be mended outside by parsing and rewriting Humdrum, which is the
converter's own work.

Still not carried by the converter, and not patched: chord symbols
(`parseHarm` is not called), the brackets of endings, `<mSpace>`, and
ties given as attributes. Some scores with two voices on one staff still
come out with a "ZZZ" token (5 of the corpus).

Regression test: MuseNote's `corelib/verovio/test/native/test_mei_to_humdrum_engine.cpp`
(15 cases; the exported text is read by the test itself, and read back
by the engine).

### 11. MusicXML octave lines

Three faults of `octave-shift` in `MusicXmlInput`, found with hand-made
files while the octave line of patch 7 was looked at again.

- A direction that names no `<staff>` was put on staff 1 of the score,
  not on the first staff of its part: the staff offset was added only
  when the direction had a `<staff>`. In any part but the first the line
  was drawn over the first part, and the notes under it, written an
  octave away, sounded there.
- An octave line waits on `m_octaveStack` for its first note. One that
  is stopped before a note comes, or still waits when its part ends,
  stayed there and took the first note of whatever came next, another
  part's too. It is removed.
- One that is started and never stopped had no end: not drawn, not
  played, with its notes written an octave away. It runs to the last
  note of its part.

Carried because the line is built inside the importer. Six scores of the
parity corpus have octave lines; none has any of the three faults, and
none of their outputs changed.

Regression test: `test_midi_document_engine.cpp`, three new cases (ten
subcases), each checked against the MIDI note-ons, the pitch map and
whether the line is drawn.

### 12. What the Humdrum reader and the converter throw

humlib reads numbers with `stoi()` and the like, indexes its lines by
spine, and catches nothing. An exception that leaves the engine ends the
process: the apps run it in their own.

- `HumdrumInput::Import` caught a `char *` only. The reader goes on with
  a file it could not parse (a line with the wrong number of fields) and
  then throws `std::out_of_range`. The import now fails.
- `Toolkit::ConvertMEIToHumdrum` caught nothing. A verse numbered
  "chorus" threw `std::invalid_argument`. It now gives no Humdrum, and
  says so in the log.

Not guarded, because the apps cannot reach them (MuseNote pins the input
format, and binds `convertMEIToHumdrum` only): the converters in
`Toolkit::LoadData` for `musicxml-hum`, MuseData and EsAC input, and the
one in `Toolkit::GetHumdrumBuffer`.

Regression test: `test_mei_to_humdrum_engine.cpp`, two of its cases.

### 11, amended. Octave lines on a part of several staves

The review of patch 11 found two faults in it and one older than it.

- A line that waits for its first note was taken away by the stop of
  another staff's line: the stop matched it by its number, and neither
  has one. A piano part with the left hand's 8vb written before the
  right hand's stop kept the left hand's notes an octave off with no
  line over them (tb.6 drew a line in the wrong place). A waiting line
  now keeps the staff its direction names (`@staff`, set when it is
  built), and only the stop on that staff takes it away.
- A waiting line took the next note read for its start, whatever its
  staff, while the notes written an octave off are those of the staff
  the direction names (`m_octDis`). It starts on the first note of that
  staff.
- A line that is never stopped was ended on `m_ID`, what was read last:
  not always a note, and not always in the score (a rest that is not
  shown). It ends on the last note of its staff.

Regression test: `test_midi_document_engine.cpp`, two new cases (three
inputs), and the "ends in its own part" case now looks at the note the
line ends on.

### 13. The Humdrum export, after its review and its device test

Patch 10 made the export hold a score's notes. Its review, and MuseNote
reading its own exports back on a device, found what was still wrong
around them.

- **A repeat's start.** Only `measure/@right` was read, and of it only
  `end`, `rptend` and `invis`. Verovio's MusicXML importer writes the
  start of a repeat as `left="rptstart"` on the measure it starts with
  (1322 of the 2785 scores of the corpus have one): it was lost, and
  read back the repeat went to the beginning. What was left, a repeat
  end after a repeat end, is what patch 14 is about: of the 2785 scores
  exported by tb.7 and read back, 321 ended the process. A repeat sign
  at the left of a measure is now the barline that closes the measure
  before it; `rptstart` and `rptboth` are read at the right too.
- **A multi-measure rest's barline.** Patch 10 writes such a rest out
  as that many measures, and each got the measure's barline: three
  repeat ends for one. Only the last has it.
- **The key on `<scoreDef><keySig sig>`** was read (patch 10), then
  written over with an empty key by `processKeySig`, which builds a key
  from `<keyAccid>` children and found none. MuseNote's own scores say
  their key there: D major came out in C. And `@keysig`, MEI 4's and
  5's name for `@key.sig`, which Verovio writes for a score that came
  as ABC, was not read.
- **A change of key or of meter after the start**, given for the whole
  score (a `<scoreDef>` with no `<staffDef>`: 293 scores of the corpus
  change one or the other), was not written. It is written for every
  staff in the measure it starts, after the barline of the measure
  before. Before that barline a reader takes a new meter for the
  measure that has just ended: Verovio's own then drops a whole-measure
  rest of the new meter (seen on seven scores while this was built).
  Two members of the converter keep the change until that measure is
  read (`include/hum/humlib.h`).
- **Hairpins.** One that ends in a later measure read the meter unit
  past the end of the vector that holds it (the index went on with the
  measures; the vector is by staff). One that ends after the last
  measure dereferenced the end of the measure list. One with no
  `@tstamp` threw from `stod()` and the whole conversion was lost.
- **`Convert::base40ToKern` called `exit(1)`** on an octave out of all
  range, which ends an application. It throws, and patch 12 catches.
- **A whole-measure rest with no meter known** was written `1%0r`, of
  no length, and its barline was lost. It is a whole note.

Still not carried: the brackets of first and second endings and jumps
(D.C., D.S., coda), so a score with them is read back with every
measure once or with the repeat alone; chord symbols; double barlines;
ties given as attributes; a dynamic, a tempo mark and a hairpin's ends
are placed at the start of a measure; a change of key or meter given
for one staff alone is still written before the barline; a key spelled
`sig="mixed"` or a meter count that is no number (`3+2`) loses the
conversion or part of the measure.

Regression test: `test_mei_to_humdrum_engine.cpp`, seven new cases.

### 14. A repeat end after a repeat end

`ExpansionMap::GenerateExpansionFor` makes the order of play from the
repeat barlines of a score that has no `<expansion>`: at each repeat
end it moves the measures since the last repeat start into a new
section. After a repeat end it kept that start. A second repeat end
with no repeat start before it then asked for a section in front of a
measure that had already been moved: `Object::InsertBefore` asserts,
and with assertions off inserts at index -1, eight bytes before the
array of children (a heap overflow each time, by AddressSanitizer).

Three bars, `C :| E :| G`: an ABC file ended the process, an MEI or a
Humdrum file was loaded and played without its second bar. MusicXML is
not affected: its importer writes the expansion itself. ABC tunes often
leave a repeat's start out; MuseNote's composer can write the pattern;
and tb.7's Humdrum export wrote it for one song in nine.

After a repeat end the next range starts with the next measure, and
`CreateSection` makes no section of measures that are in one already
(one repeat end written on both of the measures it stands between).

Where such a repeat goes back to: tb.8 had it repeat from where the
last repeat ended (`C :| E :| G` as C C E E G; one of the two readings
the ABC standard allows a player). `MusicXmlInput::CreateExpansion`
reads the same signs as back to the last repeat start, or the
beginning, with what was repeated on the way played once more
(C C E C E G), so a score played one way from MusicXML and another from
its own Humdrum or ABC: 12 of 145 scores of the corpus with plain
repeats differed after a round trip. Since tb.9 the generated expansion
follows the importer's rule: the sections made since the last repeat
start are kept, a repeat end plays its own section, those again, and
its own once more, and a repeat start puts them aside.

Carried because the expansion is made inside the engine as a score is
loaded: outside it, the pattern could only be found by parsing every
format the engine reads.

Regression test: `test_midi_document_engine.cpp`, two new cases (MEI,
ABC, Humdrum).

tb.8 against tb.6 (same toolchain, MuseNote's `tool/parity`, 2785
scores): 19930 outputs compared, 0 differ; seeded ids deterministic on a
fresh thread for 2785 of 2785. (As tb.7 against tb.6: the gate's scores
are MusicXML, whose expansion the importer writes itself, and none has
an octave line of the kinds patch 11 mends.)

Measured apart, over the same 2785 scores, each exported to MEI and that
converted:

- The text read as it is written (the kern reader of tb.7's check, and
  the engine's own reader with `expandNever`): as on tb.7. All convert,
  none ends the process, 2716 hold every note at its place and pitch, 5
  hold a "ZZZ" token, 1 cannot be read; read back by the engine 2621
  play the same notes.
- The text read back the way an application opens a file, repeats
  played out: none ends the process (321 on tb.7, SIGSEGV); 1677 play
  exactly what the original plays (1266 on tb.7); 946 play another
  number of notes (endings and jumps are not carried), 156 differ
  otherwise, 1 is refused. 1029 exports hold a repeat's start (none on
  tb.7).

### tb.9: the amendments of patches 11 to 14

From the review of tb.8, which found nothing wrong in what tb.8 changed
and seven things around it.

- **Patch 14**: the rule for a repeat end with no repeat start (above).
- **Patch 11**: `m_octDis` holds, for each staff of the score, the
  octave its notes are moved by; an octave-shift that names a staff the
  score does not have (staff 2 in a part of one staff) wrote past its
  end, a heap overflow older than these patches. Such a direction is
  passed over. And a stop with no number matched every open line of the
  part with no number, so that on a piano part it ended the other
  hand's line too, on a note of the wrong staff. A stop is first of all
  for its own staff's lines; only when that staff has none open is it
  taken for another staff's line with the same number, as before.
- **Patch 13**: a key of no sharps and no flats is written (`*k[]`):
  left unsaid after another key, that key stayed in force for a reader.
  A key that is no number (`@key.sig="mixed"`, and since patch 13
  `@keysig` too) threw from `stoi()` and lost the conversion; it is left
  out. A second `<scoreDef>` at the same time as the first (Verovio
  writes one in the section, before the first measure) cleared what was
  known of every staff, and each note after it looked its staff's
  transposition up in an empty vector: the answer was right only while
  the memory was still there. What is said for the whole score is
  cleared; the staves stay. A hairpin's count of measures is checked
  before it is cast to `int`.
- **Patch 12**: `HumdrumInput` takes a syllable's connectors off its
  ends and then read and popped the last character of what was left;
  for a syllable that is a hyphen and nothing else that is an empty
  string.

Known, not changed: a never-stopped octave line that starts on a rest
with no note after it on its staff gets an end before its start, and
with two voices it ends on the last voice's note; a repeat sign at the
left of a measure replaces a final barline on the measure before; in a
generated expansion the measures under `<ending>` elements outside a
repeated range are not played (upstream); `MusicXmlInput` gives the
parts after the first no change of key after the start (upstream).

tb.9 against tb.6 (same toolchain, MuseNote's `tool/parity`, 2785
scores): 19930 outputs compared, 0 differ; seeded ids deterministic on a
fresh thread for 2785 of 2785.

Measured apart as for tb.8, over the same 2785 scores. The text as it
is written: as tb.8, score for score (2716 exact). Read back the way an
application opens a file: none ends the process; 1700 play exactly what
the original plays (1677 on tb.8): 24 more, and one fewer, a score with
first and second endings, which the export does not carry, that had
matched under tb.8's rule by chance; 922 play another number of notes,
157 differ otherwise, 1 is refused.

Native tests (`verovio_engine_tests`, 52 cases): on tb.8 47 pass, 4
fail and 1 ends the process (the octave line on a staff the score does
not have); all 52 pass on tb.9. 7 deliberate breaks of the amendments'
lines, each caught by a case. Not tried, because what they bring back is
undefined behaviour that a case cannot hold without a memory checker
(the review saw each with AddressSanitizer): the staff out of range,
the empty syllable, the hairpin's count.

Native tests (`verovio_engine_tests`, 46 cases): on tb.7 36 pass, 6
fail and 4 end the process; all 46 pass on tb.8. 25 deliberate breaks
of the lines of patches 13 and 14 and of the amendment: 24 caught by a
case each time. The 25th (CreateSection without its guard) is undefined
behaviour: it ended the process in two runs and passed in one. Not
tried, for the same reason: the hairpin's index read past the end of
its vector, which the review saw with AddressSanitizer.

tb.7 against tb.6 (same toolchain, MuseNote's `tool/parity`, 2785
scores): 19930 outputs compared, 0 differ; seeded ids deterministic on a
fresh thread for 2785 of 2785.

The gate does not look at the Humdrum export, which patch 10 changes for
every score. Over the same 2785 scores, each exported to MEI and that
converted: all convert and none ends the process; 5 still hold a "ZZZ"
token. The text, read by a kern reader written for this check (not the
engine's), holds every note as written, at its place and pitch, for
2716 (the order the measures are played in was not looked at: see
patch 13); 68 differ
(about half of them by grace notes and arpeggios, which that reader
does not time as the engine plays them; the others by two voices on one
staff, by tuplets, most of them nested wrongly in the source, and a few
more) and 1 cannot be read (its spines change in number with no split
written). Read back by
the engine's own Humdrum reader, 2620 play the same notes at the same
places, 164 do not, and 1 is refused. On tb.6 every one of the 2785 was
damaged.

Native tests (`verovio_engine_tests`, 35 cases): on tb.6 17 pass, 15
fail (two of them by an exception, which in an app ends the process)
and 3 end the process; all 35 pass on tb.7. 24 deliberate breaks of the
patches' lines, each caught by a case.

tb.2 against tb.1 (same toolchain): 972 outputs compared, 0 differ; the
fresh-thread seeded import matches on 3 of 3 scores (0 of 54 notes matched on
tb.1).

tb.1 against `version-6.2.0`:

- 209 MusicXML and MEI scores: SVG, timemap and MIDI byte-identical to
  `version-6.2.0`, so no note id and no pitch changed.
- 86 PAE inputs (the 78 in `doc/tests/pae` plus 8 regression inputs):
  none crash. Every output the unpatched engine produced is identical
  apart from the MEI export timestamp, except the one-line form noted
  above.

### tb.10: patches 15 to 17, and the amendments of patches 10 to 14

From two independent reviews of tb.9, each finding run again by a second
reviewer, and from the work on the Humdrum export's order of play.

**15. ABC repeats.** tb.9 made one rule for a repeat end with no start
(back to the last repeat start, or the beginning: `C :| E :| G` as
C C E C E G) and gave it to every format the engine works the order out
for. ABC has a rule of its own (ABC 2.1, 4.8): such an end goes back to
the beginning of the tune, or to the latest double bar or end of a
repeated section. Tunes are written "A :| B :|" and mean A A B B, which
tb.9 played A A B A B. `ABCInput` now writes the start that is meant on
the measure it stands before (`left="rptstart"`), so the tune plays the
same from its MEI, where the generated order has the one rule. And the
same section says `::`, `:|:` and `:||:` are one sign: `:|:` lost its
start (`SetBarLine` returned a repeat end as soon as the character
before the `|` was a colon) and `::`, with no `|` in it, was no bar
line at all, so that the measures on either side of it were one
measure. `::` is what MuseNote's own ABC export writes. Third, a field
value of white space only (`w: `) was read one byte before its start.

Of the 194 ABC files of MuseNote's parity set, 8 change (all of them
MuseNote's exports of MusicXML songs): three hold `::`, and five hold a
repeat end with no start after a repeat end or a double bar. For four
of those five the tune is now shorter than its MusicXML original, which
means the beginning: the export, not the reader, is to say that start,
and MuseNote's `AbcSerializer` does from the same day.

**16. The cost of an order of play.** Every time a section is played
again `ExpansionMap::Expand` copies its elements, and
`AddExpandedIDToExpansionMap` lists each copy with all the others: a
section played k times costs k times k. tb.9 brought the rule above to
three more formats, and with it what MusicXML always had: N repeat ends
in a row with no start are N(N+3)/2 sections. A Humdrum file of 200
such bars (1.6 KB) took 29 seconds and 970 MB for its timemap; one
MusicXML measure with `times="2500"` took 1.3 GB, and
`times="100000000"` never came back from `CreateExpansion`.
`Doc::ExpandExpansions` now counts, for each section the order names,
the times it is named, squared, by the number of its elements, and
does not follow an order that comes to more than 1,000,000: the score
is played as written, once through, with a warning. `MusicXmlInput`
reads a repeat's `times` as at most 100. The dearest of the 2,785
scores of the corpus comes to 29,977 (Piano Man; first written here as
23,009, which no run gives), and no section of any is played more than
6 times. Not counted: an `<expansion>` named
inside another.

**17. A clef for a staff the part does not have.** `ReadMusicXmlAttributes`
looked the staff up and asserted it; without assertions the null staff
was kept for the clef's place, and the load ended the process. A file
that is valid MusicXML does it too: a part whose `<staves>2</staves>`
and second clef come in a later measure. The clef is passed over with a
warning (the part's second staff is not there either way).

**Amendments.**

- **Patch 10**: `HumGrid::cleanTempos` copies a tempo mark to every
  voice of its slice. A voice the measure before had and this slice has
  no place for (a staff with an empty `<layer>` before the one with the
  notes, as the Humdrum reader writes for a voice that rests through a
  measure) is a null pointer in the slice, and was read: a tempo mark in
  the next measure ended the process (084f2f47f). Checked in both loops;
  only the second is reached by a tempo mark today (the first finds the
  mark in the first voice it looks at).
- **Patch 11**, octave lines (de66ee4e0): a line ended on the note read
  last, whatever its staff, so that with the other hand's notes read in
  between it ran on over a note that is not under it, and that note was
  played an octave off; it ends on the last note of its own staff (the
  importer now keeps that for each staff). A stop taken for another
  staff's line ended the line and set only the stop's staff back: the
  line's own staff stayed written an octave off to the end of the part.
  A stop whose number one open line has, and only one, is for that line
  whatever staff it is written on. `type="continue"` was read as the
  start of a second line. No object is built that was not built before;
  in a part of one staff a line ends where it did.
- **Patch 12** (c8fb8b5cd): a `**vvdata` token is split at its spaces,
  each word a verse of its own; the words after the first had no token
  (`vtoks[j]` past the end), and a token of one space left no text while
  `vtext[0]` was read. Both ended the process.
- **Patch 13** (084f2f47f): the key of a transposing staff is written as
  it sounds, as its notes are. The written key's count and the
  transposition's fifths were added without their signs: a B-flat
  instrument written in two flats came out with none (said aloud as
  `*k[]` since tb.9), one written with no flats with no key at all. Of
  the 31 scores of the corpus with a transposing staff, 17 change: 12
  gain a key line, 5 have another key token.
- **Patch 14** (26069d6fa): a score with no `<section>` (nothing after
  its definition, or an `<ending>` that stands alone) ended the process
  when its timemap or MIDI was asked for (`assert(section)`, then the
  null section read). And a repeat end at the left of the first measure
  made a section of that measure alone, played it twice, and lost the
  repeat after it.

Known, not changed: from MusicXML a repeat sign written on the other
measure (a backward repeat at the left, a forward one at the right) is
drawn and not played, or read as the opposite sign; with a generated
order an MEI of more than one `<section>` gets no repeats at all; a key
or meter change after the start is written in the export as it is
written, not as it sounds, for a transposing staff; an octave line's
stop that has a number is still taken by a line of its own staff with
another number when that is the only open one there. The unpaired-ends
ceiling is a count, not a measure of memory: an order within it can
still be large for a score of many thousands of elements.

tb.10 against tb.9 (same toolchain, MuseNote's `tool/parity`, 2785
scores): 19930 outputs compared, 8 differ, the 8 ABC files above (their
MEI in the locator set); every score from MusicXML, MEI, Humdrum, PAE
and GABC is byte-identical; seeded ids deterministic on a fresh thread
for 2785 of 2785.

Measured apart as for tb.9, over the same 2785 scores. Read back the
way an application opens a file: as tb.9 score for score (1700 play
exactly what the original plays, 922 another number of notes, 157
differ otherwise, 1 is refused; none ends the process; 5 give the
comparison no verdict on either tag, the converter printing text of its
own into their output). The text as it is written: as tb.9 for 2768.
Of the 17 that differ, all with a transposing staff, 11 hold one line
more (a key that had none), 5 have another key, and one loses its key
line (Superstition: six flats written for a B-flat instrument are eight
at concert pitch, which the converter has no key for).

Native tests (`verovio_engine_tests`, 66 cases; 52 on tb.9): all pass,
and all pass under AddressSanitizer with no report. 36 deliberate
breaks of the lines of this tag, 35 caught by a case (five of them by
ending the process); the 36th is the first of the two null checks in
`cleanTempos`, which no tempo mark reaches today.

### 18. The sound of a MusicXML note that has no accidental written (tb.11)

`MusicXmlInput::ReadMusicXmlNote` never read a pitched note's `<alter>`. A
note with no `<accidental>` was given the sound the importer believed it
had: the key and the accidentals of the bar, of which it holds one set for
the whole score. Four things followed.

- **A part after the first** was read under the key it began in: a change
  of key after the start is read from the first part only. Every note of a
  later part on a step the change moves was played a semitone off. No score
  of the corpus has this; a file a user imports can.
- **A tie over a barline lost its accidental.** The carried accidentals are
  set back at each barline, so an F sharp tied into the next bar, written
  there without a sign, was read as F. The tie then found no note of its
  pitch: it was left without an end and not drawn, and the held note was
  struck again a semitone off (630 notes in 236 scores of the corpus).
- **An accidental was carried to every octave, staff and voice** of the
  part: the importer keeps one per step (115 notes in 45 scores).
- **A sound the file gives by `<alter>` alone** was never seen: the note
  was played as the key has it (2211 notes in 187 scores; 811 of the 2656
  songs write no `<accidental>` at all).

Measured note by note, each note's engine pitch (the pitch map) against its
file's `<step>`, `<alter>`, `<octave>` and the part's transposition: 2971
of the pitched notes in 407 of the 2765 MusicXML scores of the corpus, and
2511 in 377 of the 2656 songs users have. (Of the 429211 pitched notes of
the corpus, 1095 are not in the pitch map on either engine and were not
compared: 611 in three scores whose later parts the engine does not hold,
484 in 35 others.) By route: the four above, 5 notes in 2 scores after a
change of key the importer misses in the first part itself, and 10 notes
at a sign that the importer does not know or that says another sound than
its `<alter>` (My Prayer and Nobody's Perfect, below).

**What the patch does.** Where a note has no `<accidental>`, the importer
still builds one `<accid>` for each accidental it carries for the note's
step, as before. Each is now made from the file's sound instead of the
carried one when the two would not play the same key: the `<alter>` is
read (none is a natural), and where its number of semitones differs from
the carried accidental's, the `<accid>` gets the `accid.ges` and the glyph
name a carried accidental of that sound would have given. Where they play
the same key the note is written exactly as before (so a quarter-tone sign
carried through the bar keeps its own glyph). What the bar carries is not
changed by it: the next note with no sign is compared with the same
carried accidental.

**Only an `<alter>` the engine plays exactly is taken**: a whole number of
semitones the importer has a sign for (-2 to 2). For a quarter tone, for
three flats or sharps, and for a text that is no number, the accidental
the importer makes is played as a natural, which is further from the file
than what is carried: such a note is left as tb.10 read it. (The form
first built took these too; its review found the 29 combinations where
that moved a note away from its file. No file of the corpus, the songs,
the scales or MuseSync's starter songs has such an `<alter>`: the values
there are 1, -1, 2 and -2, counted over every file.)

**A natural is written out** (`accid.ges="n"`, glyph name
`accidentalNatural`), although the engine plays a note with no accidental
as a natural anyway. MuseNote reads the engine's MEI export again through
its own `engineInputOf` (an editable copy of a score, the MEI and MusicXML
exports), and that reading gives a note with no gestural accidental the
bar's or the key's. A first form of this patch wrote nothing for a
natural: 99 scores and 915 notes of the corpus were then played
differently in a copy or an export than in the reader (measured by the
review of that form on 2026-10-12; a deliberate break that brings the form
back fails three cases). With the natural written out it is 1 score and 2
notes, the same two as on tb.10 (below). This is the form tb.9 already
writes for a natural it carries.

**No object is built and none removed**, so no id moves. The patch adds no
`new`, no `AddChild` and no removal; the loop runs once for each entry of
the same list as before (over a copy of each entry, where it ran over a
reference); what it calls builds no `Object`. Measured: the ordered list
of every `xml:id` of the MEI export is the same on tb.10 and tb.11 for all
2785 scores (2304293 ids) and for the 2656 songs (2280803 ids), and the
pitch map and the timemap name the same notes.

Carried because the sound of a note is fixed while the engine reads the
file: after it, nothing outside the engine can tell a carried accidental
from a written one, and a tie that found no end is already lost.

**Declared change**, tb.10 against tb.11, the 2785 scores of the corpus,
each loaded with `{"xmlIdSeed":1}` in a process of its own, and its MIDI,
timemap, pitch map, MEI export (without its timestamp) and the SVG of
every page compared byte for byte, by the builder and again by its
reviewer with scripts of their own:

- 2378 scores are identical in all five.
- 407 differ in MIDI, pitch map and MEI, all three in each of them; 222 of
  those also in the SVG. The timemap differs in none. No score loads on
  one engine and not on the other, and none has another number of pages.
- MEI: 2968 `<accid>` lines and 507 `<tie>` lines differ; no line is added
  or removed and no other line differs. None of the changed `<accid>`
  lines has a written accidental (`accid=`). By `accid.ges` before and
  after: none to flat 951, none to sharp 738, flat to natural 678, sharp
  to natural 464, natural to flat 61, flat to sharp 38, natural to sharp
  18, none to double sharp 12, sharp to flat 7, flat to double flat 1. Of
  the ties, 490 gain an end, 10 lose theirs and 7 end on another note.
  Over the whole corpus, of 34529 ties, those with no end go from 3517 in
  475 scores to 3037 in 286.
- Drawing: 244 of the 3376 pages differ, in 222 scores (page 1 in 208 of
  them). Each of the 244 holds a tie that the MEI changes. On 229 only the
  tie curves differ; on 13 the tie curves and the places of what stands
  around them (a tie takes room); 2 are the pages named next. No
  accidental is drawn that was not, and none is taken away.
- Ids on the pages: the same, in order, on every page but 2 (God Put a
  Smile Upon Your Face page 2, Money, Money, Money page 1). On each of the
  two, one id differs, and it is no id of the document: the text of a
  volta bracket's number, a `Text` the engine builds each time it draws
  the bracket (`View::DrawEnding`). It is in neither the MEI, the timemap
  nor the pitch map, and it is another id every time the page is drawn, on
  tb.10 as on tb.11. It takes its number from the counter all ids come
  from, and that counter stands elsewhere because of a tie: before a page
  is drawn the engine lays out every page that a tie of its first and last
  bars reaches, and laying out builds objects.
- The Humdrum export of the 407 scores differs in 403 (4 cannot be
  exported and read back on either engine). A kern note says its sound, so
  the export now says the file's; read back, 655 more natural signs are
  drawn than from tb.10's export (the Humdrum reader draws a natural where
  a note goes against the key), and a sharp or flat given by `<alter>`
  alone is written hidden. The pitches read back agree with the direct
  load exactly as often as on tb.10 (399 of 403).
- Among the 407 are 30 of the 103 scales MuseNote generates itself (460
  notes): every harmonic and melodic minor, the chromatic scales and the
  four G sharp minor files. Its generator writes `<alter>` and never
  `<accidental>`.

The wrong notes, tb.10 to tb.11: corpus 2971 in 407 scores to 3 in 2; the
songs 2511 in 377 to 3 in 2. The three that stay are notes that have an
`<accidental>` (see "Known and left").

MuseNote's reading of the engine's own export: every score's MEI export,
through `engineInputOf`, loaded again, its pitch map against the direct
load's. tb.10: 1 score, 2 notes. tb.11: 1 score, 2 notes, the same (My
Prayer, bars 3 and 7). The export loaded again as it is, which is what the
reader does: 0 scores on both.

MuseSync takes its game notes from the pitch map. 3 of its 24 starter
songs change: dont_speak 28 notes, unforgettable 1 note and 1 tie,
waltz_of_the_flowers 1 note. The other 21 are byte-identical in MIDI,
timemap and pitch map.

**Where the change is no improvement.** Four bars of the collection, each
a place where the file itself says two things:

- Three bars tie two notes of different sounds (ties written for slurs, or
  a wrong `<alter>`): Skyfall bar 38, You Know I'm No Good bar 9,
  Unforgettable bar 26. tb.10 read the second note as the first and held
  it; now it is struck as the file gives it, and a tie is ended elsewhere
  or left open. `MatchTies` is not changed by this patch.
- Nobody's Perfect bar 73: a C written with a sharp whose `<alter>` is -1,
  and seven C after it with the same `<alter>` and no sign. tb.10 played
  all eight as C sharp, the sign carried; now the first is C sharp, by its
  sign, and the seven are C flat, by their `<alter>`. The chord around
  them (A, G, A) makes the sharp the likelier truth: the file's `<alter>`
  is wrong there.

The other 14 of the 17 ties that lose their end or end on another note are
improvements: a tie that tb.10 had ended on a wrong note many bars on now
ends on the next bar's note (Money, Money, Money bars 23 to 28), or a tie
its file never ends is let go where tb.10 had ended it bars later on a
note it read wrong.

**Known and left.**

- *Heard, not shown.* The page is drawn from the `<accidental>` elements,
  and this patch draws nothing more. 2211 notes in 188 scores of the
  corpus (1753 in 158 of the songs) have an `<alter>` that the page as the
  file writes it does not show: 2183 against the key alone, 25 against an
  accidental written earlier in the bar, 3 at a tie. (The reviewer's own
  model of the page counts 2215 in 189 and 1755 in 159.) tb.10 played all
  but 2 of them as that page reads, against the file; tb.11 plays them as
  the file says, and the page is as it was. Showing them is the next step,
  outside the engine. Of the 1755 in the songs, 1261 are in 82 songs that
  write no `<accidental>` at all, and 494 in 77 songs that do write some:
  there either the page or the `<alter>` is wrong, and only Nobody's
  Perfect has been judged by ear.
- *A file that leaves `<alter>` out and counts on the key* is now played
  in naturals. That is MusicXML's own rule, and no song of the collection
  was found to count on the key, but for such a file it is a change. Held
  by a test.
- *A note with an `<accidental>` is read by its sign, as before, not by
  its `<alter>`.* Three notes of the corpus stay wrong by their file for
  it: Nobody's Perfect bar 73 (above), and My Prayer bars 3 and 7, a B
  with `<accidental>double-flat</accidental>` and `<alter>-2`.
  `double-flat` is no name of MusicXML's (it is `flat-flat`) and the
  importer does not know it: the note gets an `<accid>` that says nothing
  and is played as B natural, on tb.10 and on tb.11. Taking the `<alter>`
  for a note whose sign gives no value was weighed and left out: 2 notes
  in 1 score, and a wrong `<alter>` beside a sign exists (Nobody's
  Perfect). Held by a test, so that a change of it is seen.
- *A double sharp given by `<alter>` alone* is written `accid.ges="ss"`
  with the glyph name `accidentalSharpSharp`, where a written double sharp
  is `accid="x"`. Nothing is drawn from it today; whoever draws it must
  write `x`, or two sharps are drawn.
- *The key signature drawn for a later part.* A part in another key than
  the first, or a change of key in one part alone, is still drawn with the
  first part's signature after a change. The sound is right since this
  patch; the drawing needs new objects.
- *`ConvertAlterToAccid`* has `2` twice in its table and no `3`: a triple
  sharp given by `<alter>` alone is not known and the note keeps what is
  carried (upstream).
- *Found beside it, in the same function, older, and not mended here:* a
  `<chord/>` on the first note of a layer ends the process (an empty stack
  is read), a rest whose `<display-octave>` is no number ends it too (an
  exception nothing catches), and a note before a part's first
  `<attributes>` reads one past the end of the octave-line table. For the
  next tag.
- *The move to 6.3.0.* This patch does not simply drop there. Upstream
  reads `<alter>` but writes no `<accid>` for a natural, and MuseNote's
  reading of an MEI would give such a note the key's accidental back (the
  99 scores and 915 notes above). Before that move either that reading
  stops filling in the key on an engine export of MusicXML, or the written
  natural is carried on. (Upstream as read on 2026-10-12.)

Regression test: `test_midi_document_engine.cpp`, 24 new cases.

Native tests (`verovio_engine_tests`, 90 cases; 66 on tb.10): all pass, in
one process and each in a process of its own, and under AddressSanitizer
with no report. 26 deliberate breaks of the lines of this patch, each caught by a case
(the 19 of its builder and the 17 of its reviewer are among them, in the
form the final lines take). Under AddressSanitizer 150 scores of the corpus (75
the patch touches, 75 it does not) were loaded, drawn, played, exported
and read back, one process each: no report. 1670 hostile files of the
reviewer's: nothing new in the changed lines.

Parity gate, tb.10 against tb.11: 19930 outputs, 2257 differ. They are
the MIDI, the pitch map and the MEI of the 407 scores, their two locator
exports, and the page of 222 of them: the same 407 and the same 222 as
measured above, and nothing else. No timemap differs. Seeded ids on a
fresh thread: the same for 2785 of 2785.

Measured apart as for tb.10, over the same 2785 scores. Read back the
way an application opens a file: as tb.10 score for score, verdict and
number of notes (1700 play exactly what the original plays). The text
as it is written: as tb.10 but for 5 of the 407 scores, whose exports
already differed from their score and now differ in other notes.

### 19. The order a score is played in

Up to tb.11 the export held each measure once, in the order the measures
stand in, with its repeat signs. The order they are played in was not
written: first and second endings, D.C., D.S., coda and fine. Read back,
such a score was played once through, or with its repeat signs alone. Of
the 2785 scores of the corpus, each exported and read back with its
repeats played out, 1700 played what the original plays; 924 played
another number of notes, 160 differed otherwise and 1 was refused.

Verovio's MusicXML importer keeps the measures in `<section>` and
`<ending>` children of the score's one `<section>` and says the order
with an `<expansion plist>` of their ids. The converter passed the
`<expansion>` over ("Don't know how to process section/expansion").

In Humdrum the order is a label where each part starts and one list of
the labels. The converter now writes both, at the start of the first
measure it makes of each child:

- A section is named with letters (A to Z, then AA, AB). An ending is
  named with the letters of the section before it and its place among
  that section's endings (A1, A2): Verovio's Humdrum reader takes a label
  that ends in a number for an ending only when the label with no number
  before it is its start, and passes over any other such label.
- What an ending's bracket says is written as the text of its label
  (`!LO:TX:t=1, 2`, a line right above the label) when it is not that
  number.
- An ending with no section before it is named as a section: played in
  its turn, with no bracket.
- A child gets a label when it holds a note or a rest. One the
  converter makes no measure of gets none, nor does one whose measures
  hold nothing (no staff, or a tempo mark and nothing to play), and the
  list leaves it out. A reader drops a section with nothing in it: with
  a label for it, the read-back was exported again with no label at
  all, because the list named a section that was not there. A list that
  names a label that is not there is not followed by a reader.
- With no list to write, no label is written. Labels with no list keep
  Verovio's reader from playing the repeats at all ("An expansion cannot
  be generated with more than one section").
- The list and the labels are written in the first spine as a tempo
  mark is, which the converter's grid copies into every other spine; the
  text of a bracket is a layout line in the first spine alone.

**The text of a bracket and two voices.** The grid writes a label in
one voice for each staff, whatever the notes around it are in: where
the measure before a label ends with a staff in two voices, it joins the
voices before the label (`*v *v`) and parts them again after it. A
layout line it made as wide as the notes around, so the text of a
bracket stood before the join; a reader gives it to the join, and shows
the ending's count in its place (in Olsen-Banden a bracket that says 2
came back saying 1). `HumGrid::addNullTokensForLayoutComments` now
leaves a layout line that stands before a label or a tempo mark in one
voice, so the join comes first and the text stands right above its
label. Words on a note in the middle of two voices keep the two voices
around them, as before.

The same joining is why the barline before a label can have fewer
fields than on tb.11: in 8 exports of the corpus a barline has another
number of fields in its staves than on tb.11 (26 barlines: An_die_Musik,
Caruso, Cry_Me_A_River, El_dia_que_me_quieras, Ma_Cherie_(simple),
Nyan_Cat, Olsen-Banden, Spain). The notes, their order and what is
played are the same.

**A measure with nothing in it.** A `<measure>` in which the converter
finds no staff (or none that the score has, patch 22) makes a measure
of the grid with no line in it, and the grid cannot write one: it left
out the barline of the measure before it, so that the two measures
around it came out as one, and for a score that ends with such a measure
it looked for the last line in a list with nothing in it. Such a
measure is now taken out of the grid before it is written, and the score
then ends with the barline of the last measure that holds something.
Three exports of the corpus show it (a measure of an `<mSpace>`, which
the converter does not know). All_Through_the_Night ends with one: its
export ended with a line of fewer fields than its spines and was refused
when read back, on tb.11 and with these patches as first built; it is
read back now and plays the same. In Fatback and Green_Onions a section
starts with one, after a repeat (in Green_Onions it holds a change of
clef, which the grid moved out of it): the label stands in that measure,
so it is no longer one with no line, and the barline before it with its
repeat sign, which tb.11 loses, is written.

Nothing is written, and the export is as before, for: a score with no
`<expansion>` (MEI, ABC and Humdrum imports with no labels, and the
scores made in MuseNote's own editor, which puts `<ending>` beside loose
measures); two `<expansion>` elements; a measure, an `<app>` or a
`<staff>` beside the sections; a list that names what is no child of the
section; an `<expansion>` in a section that is not the score's one; and a
score in which the converter finds no staff.

One more change, for what the text of a bracket goes through: a carriage
return in the words of a `<dir>` (it gets past the XML reader as a
reference) ended the line of its layout parameter for whoever reads the
file. It is a space now, as a line feed is.

The label is written as a tempo mark is, and tb.11 already carries the
crash fix that needs (`HumGrid::cleanTempos`, two null checks): a staff
with a layer that holds nothing before the layer with the notes, which
the reader writes for a voice that rests through a measure.

**This patch is not to be shipped without patch 21.** With the converter
change alone, a section that starts between two whole-measure rests is
folded away by the reader, the list then names a section that is not
there, and nothing of it is followed. Measured (the exports of patches
19, 20 and 22 read back by tb.11's reader, 2785 scores): 2613 play what
the original plays where 2619 do with patch 21; one score that matches
on tb.11 (Early_Mornin_Rain) no longer does; 9 scores lose an ending.

Still not carried: the order of a score with no `<expansion>` (the
engine's own generated order has faults of its own with endings, patch
14's "known"); an `<expansion>` under an inner section; a bracket that
says nothing (the reader shows the ending's count: 8 scores of the
corpus); the shape of a bracket's end.

Known, and left: the export writes a list of whatever length the file
gives, and Verovio's Humdrum reader stores a long list in quadratic time
(`PlistInterface::AddRefAllowDuplicate` copies the whole list for every
reference: a hand-made MEI of 120 KB with 40000 references exports to a
file that takes 3.6 s to load, measured by the review). The list is not
followed at that length (patch 16), so the time buys nothing. Hostile
input only; the cause is in `src/plistinterface.cpp`, which is being
mended apart from these patches.

Regression test: `test_mei_to_humdrum_engine.cpp`, ten cases of the
first build and "the text of an ending's bracket stands right above its
label where the staff is in two voices", "a measure that holds nothing
is left out, and the barlines around it stand", "a section whose
measures hold nothing gets no label, and the export of the read-back
export has its labels".

### 20. Chord symbols

`Tool_mei2hum::parseHarm` read a chord symbol's text and returned: the
code that places it was commented out, and no export held one (117051
`<harm>` elements in 2565 of the 2785 scores of the corpus).

Each `<harm>` is now a token of a spine of free text beside its staff:

- **`**cdata`, not `**mxhm`.** The converter's grid names the spine
  `**mxhm`, which is MusicXML's root and kind: a reader of that takes
  "G-" for G flat, drops the "/" of "C/E", and garbles every "♭". The
  spine is renamed after the grid is written out, and a second line says
  `*above` in it (a reader puts `**cdata` below the staff unless told).
- **Where.** On the line of the notes that start at its beat. Where none
  does, on a line of its own after the last note that started before it,
  with nothing in the staves: a reader spaces such lines evenly between
  the notes around them, so the place is exact for a symbol half way and
  near it otherwise. At the end of its measure or after it, on the last
  note. The beat is counted in the meter of the staff the symbol is for.
- **Two at one place** are one token, one after the other with a space
  between: a reader gives them back as one symbol with both texts.
  **The same text a second time at one place is not written again.** A
  file can draw one symbol twice, one on top of the other (633 places in
  86 scores of the corpus hold the same text two or more times and
  nothing else, and 40 more hold a text twice among others); joined to
  itself it came back as "A A".
- **A rest of several measures** is written out as that many measures
  (patch 10). Its chord symbols are written with the first of them only:
  under every one, a reader could not make one rest of them again.
- **Left out, with a warning:** a symbol for a staff the score does not
  have; one with no `@tstamp`, or before the first beat, or in a measure
  with nothing in it; one on a staff past the hundredth (the grid keeps
  count of chord spines for a hundred staves, and the symbol would stand
  in a column the file does not have). Left out in silence: a text that
  is empty, ".", or starts with "!", "*" or "=", which a reader would
  take for no token, a comment, an interpretation or a barline.
- A tab, a line break or a carriage return in the text is a space.

`parseHarm` writes into the last measure of the grid. That is always the
measure `parseMeasure` has just made: `parseMeasure` adds its measure to
the grid before it does anything else and takes none away, and
`parseHarm` is called right after it has returned (from `parseSection`,
`parseLem` and `parseRdg`), with nothing between. The only other place
that adds a measure is `parseStaffDef`, which is reached from a
`<scoreDef>` outside a measure. (The measures that hold nothing are
taken out of the grid when the whole score has been read, not before.)

`HumGrid::setHarmonyCount` tested its index with `>` where `>=` is
meant: for the hundred-and-first staff it wrote one past the end of its
vector (seen with AddressSanitizer). Nothing called it for the converter
before; `Tool_musicxml2hum` does, for a score of more than a hundred
parts.

**Declared limit, not mended.** The chord symbols of a re-opened export
do not follow a transposition. The notes of a Humdrum file move with the
transposition the reader is asked for; its chord symbols are free text,
which the reader wraps in a `<rend>`, and the engine's transposer looks
for a chord's root one level down only. A score opened from MusicXML and
transposed a tone up shows D, Bm7, A/C♯; its export, opened and
transposed, shows C, Am7, G/B over the same notes. A case holds this as
it is ("the chord symbols of a re-opened export stay as they are written
when the score is transposed"). On tb.11 an export has no chord symbols,
so nothing wrong is shown there.

**What comes back, counted for each `<harm>` element** (the 2619 scores
of the corpus that play the same, 2515 of them with chord symbols):
114148 go out. 109480 come back as themselves, with their text on their
staff, measure and beat. 3338 stand with another text at their place
(1581 places) and come back as one symbol with the texts joined. 694
are a text that is already written at their place, and are written
once (at 587 places every text is the same). 610 come back in their
measure on another beat, and 26 are lost (in 12 scores); 12 symbols are
read back at a place the original has none at. In all, 111683 `<harm>`
elements are read back. On the patches as first built, of 114081: 108826
as themselves, 3252 joined, 1359 doubled into a symbol with one text in
it twice (623 places), 606 on another beat, 38 lost. (The first entry said "110990
of 111629 come back": that counted places after joining the texts of a
place on both sides, which hid the joined and the doubled ones.)

Still not carried: the exact beat of a symbol between two notes unless
it is half way; two different symbols at one place as two elements (they
come back as one, with a space); a symbol before the first beat or with
no `@tstamp`; figured bass and the other kinds of `<harm>` are written
as their text; a symbol after the first beat of a rest of several
measures (it is written on a line of its own in the first of them, which
then comes back as a measure of rest of its own, and the others as one
rest). In a measure the converter has no length for (a staff whose only
notes are in a layer after one that holds nothing), a symbol between two
notes is written on the note before it.

Regression test: `test_mei_to_humdrum_engine.cpp`, eleven cases of the
first build and "the same chord symbol twice at one place is written
once", "chord symbols a little after a note have lines of their own, in
the order of their beats", "a beat is counted in the meter of the staff
it is said for".

### 21. The reader: rests of several measures, labels and chord symbols

`HumdrumInput::analyzeMultiRest` makes one `<multiRest>` of consecutive
measures that hold nothing but a whole-measure rest of the same length,
parted by plain barlines. A measure whose rest stands under a `**mxhm`,
`**dynam` or `**text` token, or under words, is not folded into the rest
before it. Two things were not looked at:

- **A `**cdata` token.** Of several measures of rest with a chord symbol
  each, all but the first were folded into one rest and their chord
  symbols dropped (five measures under G, C and D came back as one rest
  under G).
- **A section label.** Two whole-measure rests with a label between them
  (the last measure of one section and the first of the next) were made
  one rest: the second section went with its first measure, an expansion
  list that names it then names what is not there, and nothing of the
  list is followed.

Both are what patches 19 and 20 write. The patch adds one question,
asked in the two places where a rest is folded into the rest before it:
does this rest keep its measure? It does when a label stands between it
and the rest before (before or after the barline that parts them), and
when a non-null token of a spine whose type starts with `**cdata` stands
on its line. Nothing else of the function is changed, so **the patch
only ever takes folds away**: it makes no rest of several measures that
tb.11 did not make. A rest under a chord symbol still takes the plain
rests after it into a rest of its own, by the rule for plain rests (the
same length, a plain barline between).

**What this changes for a file that loads today.** A Humdrum file is
built differently by a seeded load when, and only when, tb.11's reader
made one rest of several measures out of a run of whole-measure rests
that has

1. a non-null token of a spine whose type starts with `**cdata` on the
   line of a rest of the run that is not its first, or
2. a line whose first field is a section label (`*>` followed by a name,
   with no `[` in it: not an expansion list) between two rests of the
   run. A label that stands in another spine only does not count: such
   a run is one rest, as it was (a case holds that).

Such a run is cut before each such rest. The file then has more
`<measure>` elements (one more for each cut); every measure after the
first cut has another ordinal; the ids the reader makes from line and
field numbers stay, and the seeded ids of what is built after the cut
(rests of several measures, the text of chord symbols) move; the chord
symbols and the section or ending that were dropped are built; and
where a section was dropped the order of play is another. Two examples,
each as tb.11 built it and as it is built now:

- one staff, `c | r | r under G | r | d`: one rest of three measures
  and no chord symbol; a measure of rest, then a rest of two measures
  under G;
- two staves, list `[A,A,B]`, a label between two measures of rest:
  played C r r, C r r, D; played C r, C r, r D.

A mark anchored by measure ordinal on such an imported file would move.

No file MuseNote wrote before tb.12 is of either kind (its exports held
neither a label nor a `**cdata` spine: all 2785 exports of tb.11 give
the same MEI on both readers), none of the 189 Humdrum files of the
parity gate is, and MuseNote has no Humdrum file among its assets and
test fixtures. As patch 15 does for the ABC files it is about, this
patch changes what an importer builds for an input that loads today.

A plain run of rests is one rest as before, also with a blank line, a
change of clef or an expansion list between two of them.

**Patch 19 is not to be shipped without this one** (the numbers are in
its section); and without it the chord symbols of patch 20 that stand on
runs of rest measures are lost on the way back (487 `<harm>` elements
fewer are read back, in 144 scores of the corpus).

Known, and left as it is: a rest under a `**mxhm`, `**dynam` or `**text`
token, or under words, takes the plain rests after it across any
barline, a repeat sign or a double barline too, and the sign then stands
after the whole rest: `c | r under p :| r | d` is played with the repeat
ending a measure late. That is upstream's and on tb.9; the patch leaves
those files as they load today, and does not give the fault to a rest
under a `**cdata` token (a case holds that).

Regression test: `test_mei_to_humdrum_engine.cpp`, five cases of the
first build and "a Humdrum file with a label before the barline between
two measures of rest keeps both, and one with a label in another spine
than the first does not".

### 22. A double barline, dynamics and hairpins, words where no note starts, signs and marks, and what a file says wrongly

What the export left out or misplaced, and what it did with a staff or a
beat that a file gives wrongly, each a few lines of `Tool_mei2hum`.

**What was left out or misplaced.**

- **`right="dbl"`** was not read: a double barline came out as a plain
  one (850 exports of the corpus hold one now, 1778 in all).
- **A dynamic** went to the first note of its measure, whatever its
  beat: the test for "past it" was true of every note up to it. It is on
  the note that starts at its beat. Where none does, it is moved to the
  next note, or to the last of its measure. Two dynamics are not joined
  in one token: a reader takes the letters of "f ff" for one dynamic,
  "fff". So one of two at one note is left out, and that is said: a
  dynamic that is moved to a note that has one already is left out ("a
  dynamic with no note of its own is left out"), and a dynamic at its
  own beat takes the place of whatever its note holds, one that was
  moved there or another at the same beat ("a dynamic takes the place of
  another at its note"). (The first entry said the one on its own beat
  is kept "with a warning" whichever is written first; when the moved
  one was written first it went without a word.)
- **A hairpin and the dynamic on its line.** The signs of a hairpin are
  written when every measure has been read, each on the note of its
  beat, and they took the place of what stood there. With dynamics on
  their own beat, "p, a crescendo from that note, f where it ends" came
  out with neither dynamic, and a crescendo that ends where a diminuendo
  starts lost one of the two. That is the shape MuseNote's own editor
  writes (`<dynam>` and `<hairpin>` with `@tstamp`). A sign now joins
  what its line holds: `p <` at the start, `[ f` at the end, `[ f >`
  where one hairpin ends, a dynamic stands and the next hairpin starts.
  Verovio's reader takes the letters of such a token for the dynamic
  and the rest for hairpins, and takes an opening sign with a closing
  one straight after it (`< [`) for a hairpin as long as its note; so
  the sign that closes a hairpin stands first and the sign that opens
  one last, unless it closes the hairpin that opens on that very line.
  No dynamic is lost to a hairpin and no hairpin to a dynamic.
- **The words of a `<dir>`** at a time no note starts were left out
  ("dir elements not occuring at note/rest times are not yet
  supported"): every "D.C. al Fine", "To Coda" and "Fine", which stand
  at the end of their measure. They are written on the last note or rest
  of the staff's first voice before them. Never on a line where that
  voice has nothing (the line of another staff's note, or of its grace
  note): Verovio's reader puts a space into the voice for words on no
  token, and what follows is played late. (Words at the very time a note
  of another voice starts are on that line, as before.) The beat is
  counted in the meter of the staff the words are for.
- **`<repeatMark>`** was not known. `func="segno"` and `func="coda"` are
  written as the words of a direction, `[segno]` and `[coda]`, which
  Verovio's reader draws as the signs (514 signs in 287 exports of the
  corpus); it gives them back as `<dir><symbol glyph.name>`, which is
  written the same way again. A repeat mark with words is written as its
  words. One with neither keeps a warning. No other `<symbol>` is
  written: the reader's names for them are not the engine's.
- **A mark given by the note it starts on.** MuseNote's own editor
  writes a navigation mark as `<repeatMark func=".." startid="#note"
  place="above">`, with no `@tstamp` and no `@staff`; the converter read
  only a beat and left every such mark out ("no timestamp on dir
  element"). A `<repeatMark>` or a `<dir>` with no beat (none, or one
  that is no number) is now written on the line and the staff of the
  note, rest, chord or whole-measure rest its `@startid` names: the id
  of what a token was made of is kept with the token. So a segno, a
  coda, a "Fine" and a "D.C. al Fine" made in MuseNote are exported and
  come back on their beat, the signs as signs and the others as their
  words. (Not one on the first note of a measure in which its staff goes
  from one voice to two, nor in a first measure of two voices: it is
  written before the voices part, where a reader does not find it, as
  words with a beat are. Found by the second review.) A mark that names nothing of its own measure (no id, a note of
  another measure, a note inside a chord, a grace note at the very start
  of a measure) is left out with a warning that says its measure. A
  `<dynam>` with `@startid` and no beat is left out as before (its own
  function, not this path); the editor writes dynamics and hairpins
  with `@tstamp`.

**What a file says wrongly.** The staff of a `<dir>`, a `<repeatMark>`,
a `<dynam>`, a `<hairpin>` and a `<staff>` is a number the file gives,
and the converter looks up by it what it keeps for each staff.

- **A staff the score does not have.** The meter of the staff was read
  with the number unchecked in `parseDir` and `parseDynam`: past the end
  of where the meters are kept for a number above a thousand or below
  one (a heap-buffer-overflow under AddressSanitizer, on tb.11 too;
  `<repeatMark>` was a new way to it with the first build of this
  patch), and with the first build the whole export was given up for a
  `<dynam>` with a negative staff. A
  hairpin's staff was read with `stoi()`, which throws on text that is
  no number, and looked up unchecked when its signs were written. A
  `<staff>` with such a number had each of its notes refused where they
  were to be written, but a whole-measure rest or a syllable on it was
  looked up first, and the whole export was given up. Each of these is
  now refused before anything is looked up, with a warning that says the
  element and its measure and nothing of the file's own text ("dynam for
  a staff the score does not have in measure 7"), and the rest is
  written: in `parseDir`, `parseDynam`, `parseHairpin`, and for a
  `<staff>` in `parseMeasure` and `parseSection`. (`parseHarm` refused
  such a staff from the start.) A staff with no number of its own is the
  one after the staff read last, as before.
- **More staves than are kept count of.** What is kept for a staff (its
  meter, the length of its measure, its count of verses) is kept for a
  thousand staves, and the number of staves was taken from the first
  measure, however many it has. A score of more is written as far as a
  thousand, with a warning.
- **A beat that is no number.** `@tstamp` and `@tstamp2` were read with
  `stof()` and `stod()`, which throw on text that starts with no number
  ("x", "soon") or with one that is none to count with ("nan",
  "1e999"), and nothing in the converter catches: one such attribute and
  there was no export. Such a beat is no beat: the element is left out
  as one with no `@tstamp` is (or, for a mark, written by its
  `@startid`), and the rest is written. A number with something after it
  ("3rd") is read as it always was, up to where the number ends.
- **Words, or a tempo mark, that stand in their measure before the
  staff they are for.** The line made for them had as many staves as had
  been read so far, not as the score has: the words had no place in it,
  and the line had too few fields, so that the file was refused when it
  was read back. (The tempo mark's line was found by the second review.)
- **The text of a dynamic** was written as it stood: a tab or a line
  break in it made a line of a field too many, or two lines, and a text
  that begins as an interpretation, a comment or a barline does was
  taken for one. The file was refused when it was read back; with the
  first build a hairpin on the same note took the text's place, and the
  file was read by that accident alone. The text is one token, as a
  chord symbol's is, and one that a reader takes for no token is left
  out. (Found by the second review.)
- **A measure that holds nothing but a change of clef.** The grid moves
  a clef that a measure starts with to the end of the measure before it,
  and left such a measure with no line at all, which it cannot write:
  the measure before lost its barline (a repeat sign with it), and when
  it was the last measure of a score the grid looked for the last line
  in a list with nothing in it (the process ends there under the
  standard library's own checks). `HumGrid::adjustClefChanges` now
  leaves a clef that is the only line of its measure where it is.
- **A syllable on a note of a chord** (`<note syl="la">` inside
  `<chord>`), or on a grace note at the very start of a measure: such a
  note has no line of its own, the syllable was written into the line
  that is not there, and the process ended. It is left out, as a
  `<verse>` of such a note is.

**What is said.** The warnings these patches add say what is left out,
with its measure where it stands in one, and nothing of the file's own
text. Older warnings of the converter do say text of the file, as on
tb.11: the words of a `<dir>` or a `<repeatMark>` that has a beat and
names no staff ("staff number required on dir element in measure 3
(ignoring text: ...)"), the text of a `<dynam>` given by `@startid`, an
octave that is no number, the name of an element the converter does not
know. What the converter says
reaches the log as text since the integrator's patch of the log calls
(both places in `toolkit.cpp`), so a percent sign in such words is no
longer the format of a message. (The first entry said that all the
warnings of this patch are free of the file's text; that was true of
the warnings added, not of the one for words with no staff, which the
patch gave a second element to reach.)

Still not carried: `<repeatMark>` as an element (it comes back as a
`<dir>` with the sign or the words) and the `@type` of jump words; the
sign in MuseNote's "To Coda", which its editor writes as a letter of the
music font inside the words (U+E048 in `<rend fontfam="smufl">`): it is
written as that letter, which a reader draws in the font of the words (a
case holds that); words with a sign in them at the second export (the
reader nests the sign in a `<rend>` the converter does not look into:
"To [coda]" is written once and not again from the read-back); a double
barline before a repeat start; a final barline before a repeat start; a
tempo mark that is not on the first beat; words on the first note of a
measure in which a staff splits into two voices (they are written before
the split, where the reader does not find them: as on tb.11); a hairpin
with a dynamic between its two ends (the reader then shows the word
"cresc." in the hairpin's place).

Known, and left: **a staff that transposes by more than an octave** is
exported by whole octaves only (`Convert::transToBase40` knows no
interval past an octave); mending it changes how every file exported so
far reads back, which needs the owner's word. **A negative meter unit**
(`meter.unit="-4"`) is exported as `*M4/-4`, and the read-back does not
come back (`smf::MidiMessage::makeTimeSignature`; on tb.11 too). **An
MEI whose later measure has a staff with no `<staffDef>`** ends the
process when the engine loads it (`setscoredeffunctor.cpp`, on tb.11
too); the converter leaves such a staff out. **A `<staff>`, a
`<staffDef>` or a meter unit whose number is no number** gives the whole
export up (`stoi`); the engine does not load such a file either. **A
chord symbol's text of any length** is written, and the reader is slow
on a very long one (200,000 letters: a minute). Three hairpins that open
on one note come back as one, and word dynamics ("cresc.", "poco f") are
written and not read back as dynamics, on tb.11 too.

Regression test: `test_mei_to_humdrum_engine.cpp`, four cases of the
first build and "a mark, a dynamic or a hairpin for a staff the score
does not have is left out, and the rest is written", "a beat that is no
number is no beat: its mark, dynamic or hairpin is left out, and the
rest is written", "a staff the score does not have is left out with what
stands on it, and the rest is written", "a score of more staves than the
converter keeps count for is written as far as it does", "a hairpin
stands with the dynamic on its line, and with the hairpin that ends or
starts there", "a navigation mark made in MuseNote is written on the
note it starts on", "a measure that holds a change of clef and nothing
to play keeps its clef, and the barlines around it stand", "a syllable
written on a note of a chord is left out, and the engine goes on",
"words that stand in their measure before the staff they are for are
written in a line as wide as the others", "a dynamic that takes the
place of another is said to", "words where no note starts are not put on
the line of another staff's grace note", "a sign with words before it is
written with them, upright as a sign alone is"; and, after the second
review, "a dynamic's text is one token: a tab or a line break in it is a
space, and what a reader takes for no token is left out", "a tempo mark
that stands in its measure before one of the staves is written in a
line as wide as the others", "a hairpin in a measure that holds nothing
is left out with its measure, and the engine goes on", "a hairpin that
ends in a later measure stands before the dynamic on its line, and two
that start on one note are both written", "a chord symbol whose text is
part of another's at its place is a symbol of its own".

### tb.12: patches 19 to 23

By the owner's decision of 2026-10-12 ("Build it all"): the whole of
what the investigation of the export proposed, as separate patches with
their own tests, under an exception to this branch's rules on size and
on features, and with one declared limit, which is pinned by a case and
not mended: the chord symbols of a re-opened export do not follow a
transposition (patch 20). The four patches were built, then reviewed by
an engineer who had not written them, and made again with what the
review found mended: this is the second build, and every number below
is measured on it. A second engineer then reviewed the second build and
found it safe to ship; what that review found is at the end of this
section. Patch 23 is another matter and has its own section.

Measured apart, as for tb.9 and tb.10: the parity gate sees neither the
export nor the order a Humdrum file is played in. tb.11 against tb.11
with the four patches and the integrator's patch of the log calls, the
same toolchain, one process per score, over the 2785 scores of the
corpus. "As first built" is the same with the four patches before
their review.

**The round trip.** Each score exported, the export read back with
default options, and what it plays compared with what the original
plays:

| | the same | another number of notes | differ otherwise | refused | end the process |
| --- | --- | --- | --- | --- | --- |
| tb.11 | 1700 | 924 | 160 | 1 | 0 |
| patches 19 to 22 as first built | 2618 | 2 | 164 | 1 | 0 |
| patches 19 to 22 | 2619 | 2 | 164 | 0 | 0 |
| patches 19, 20 and 22 without patch 21 (tb.11's reader) | 2613 | 2 | 170 | 0 | 0 |

No score that matches on tb.11 stops matching, and none that matches
on the first build. 919 match that did not on tb.11: 900 of the 924
that played another number of notes (22 of them now differ otherwise, 2
are as they were), 18 of the 160 that differed otherwise (142 still
do), and the one that was refused, All_Through_the_Night (patch 19, "a
measure with nothing in it"). The two with another number of notes are
Le_mille_bolle_blu and the MEI fixture BrookeWestSample. Without patch
21, Early_Mornin_Rain matches on tb.11 and no longer does, six scores
match only with it, and 9 scores lose an ending.

**Read back the way the apps read a file** (drawn, then loaded again
with `expandAlways` for the timemap and the MIDI): the same verdict for
every score, 2619 / 2 / 164.

**Second generation** (the read-back exported again, and that read
back): 2610 play what the original plays, 5 another number of notes,
170 differ otherwise; none ends the process, at either generation. Ten
play the same at the first generation and not at the second
(Caresse_sur_l'océan, Electric_Feel, Heartbeats, Here_Comes_the_Sun,
Home, Les_Feuilles_Mortes, Oh,_Pretty_Woman, Song_For_Ireland,
The_Logical_Song, Woman_In_Love), the same ten as on the first build.
Ten second exports hold "ZZZ" tokens, five first exports do (as on
tb.11).

**Endings.** All 2364 endings of the 1063 scores that have one come
back as endings; what the brackets say is the same in 1055 of the 1063
(1054 on the first build: Olsen-Banden is right now). In the other 8 a
bracket says nothing, and the reader shows the ending's count. 400
bracket texts are written, in 336 exports, each right above its label.

**Chord symbols**, counted for each `<harm>` element in the 2619 scores
that play the same (2515 of them have chord symbols): 114148 go out;
109480 come back as themselves; 3338 come back joined to another text
at their place (1581 places); 694 are a text already written at their
place, written once (587 places hold nothing but one text several
times); 610 come back on another beat of their measure; 26 are lost, in
12 scores (a symbol before the first beat, which is left out; one in a
measure the converter finds nothing in; symbols at the end of a short
measure, which are joined to the one before; and those after a measure
the converter writes nothing for, which are counted a measure off); 12
are read back at a place the original has none at. With tb.11's reader
in the place of patch 21, 487 fewer `<harm>` elements are read back, in
144 scores.

**The text of every export against tb.11's**, column by column, through
every split and merge of the spines: the same notes in the same order in
all 2785, and the same syllables. What differs:

- every export has a list and its labels (8382 labels: 6018 of sections
  and 2364 of endings, the endings in 1063 exports), and 2565 have a
  chord spine (114473 tokens);
- 850 exports hold double barlines (1778);
- three exports have one barline more: Fatback and Green_Onions (the
  end of a repeat before a section whose first measure the converter
  finds nothing to play in) and All_Through_the_Night (its last, where
  tb.11 writes a broken line);
- in 8 exports a barline has another number of fields in its staves (26
  barlines), because the voices of a staff are joined before a label
  and parted after it (patch 19);
- 17 exports gain dynamics or hairpin signs, 38 dynamics and 19 signs
  (356 where tb.11 writes 299); none loses one;
- 1095 exports hold the words of directions and signs (4135 tokens)
  where 911 did (2984), and 287 of them hold segno and coda signs (514).
  In 6 exports words stand right before a tempo mark of the same beat,
  where a reader does not find them (as on tb.11), and in 4 a sign
  stands among words.

**Against the first build of these patches**, the same way: 2656 of the
2785 exports are the same file, byte for byte. Of the other 129: in 113
a chord token no longer holds one text twice (the tokens are as many);
15 gain dynamics or hairpin signs that the sign of a hairpin had taken
the place of (15 dynamics and 19 signs; none loses one); Olsen-Banden has the
text of its bracket above its label; All_Through_the_Night has its last
barline and a well-formed last line. The notes and the syllables are
the same in all 2785.

**What the reader patch changes for files that load today**, each file
loaded with `{"xmlIdSeed":1}` on tb.11 and with the patches, the MEI
compared byte for byte but for the time it was written:

- the 189 Humdrum files of the parity gate: the same for all 189 (4 are
  refused, on both);
- the 2785 exports of tb.11, which are the Humdrum files MuseNote wrote
  up to now: the same for all 2785 (1 refused, on both);
- MuseNote has no `.krn` file among its assets and its test fixtures;
  the two Humdrum files its device test holds as text have labels and no
  measure of rest.

Of the 2785 exports of the patched converter, 146 are built differently
by the two readers (more measures with patch 21, never fewer): that is
what patch 21 is for. The reader is the first build's, unchanged by the
second.

**Memory.** A Debug build of the final sources with AddressSanitizer
and the standard library's own checks switched on
(`_LIBCPP_HARDENING_MODE_DEBUG`): the 141 native cases, together and each
in a process of its own, with no report. And hostile inputs, one process
each, every one taken each way an app takes a file: the raw MEI
converted; the MEI loaded, taken back from the engine and converted;
each export read back with default options and the apps' way, drawn and
played, exported again, and that read back (about 2000 conversions and
4000 read-backs in all).

- The review's 321 hand-made and random inputs: 315 end with 0 and no
  report (one of them, a measure of several thousand long notes, only
  when run alone: beside the other runs it did not come back in ten
  minutes). The other 6 end the process when the engine itself loads the
  hostile MEI, after the converter and the reader have taken the raw
  file and its export without a report: an assertion of the Debug build
  in `setscoredeffunctor.cpp` (a staff with no `<staffDef>` in a later
  measure, 1 input) and in `preparedatafunctor.cpp` (an `<ending>` or a
  `<section>` that holds nothing, 5 inputs). Neither is in the converter
  or the Humdrum reader, and both are on tb.11.
- The review's two slow inputs (a negative meter unit; a list of 120000
  references) do not come back in two minutes: the two "known" of
  patches 19 and 22.
- The first build's 112 hand-made inputs and 182 scores of the corpus:
  all end with 0, no report.
- 67 inputs made for this build (a staff, a beat and a number of staves
  that a file says wrongly; measures with nothing in them or with a clef
  alone; scores of 102 and of 1100 staves): 61 end with 0 and no
  report; 4 end the process when the engine loads them, by the same
  assertion about a staff with no `<staffDef>`; and the two scores of
  1100 staves come back only when run alone.

The three inputs of the review that ended the process on the first
build and not on tb.10 (a `<repeatMark>` with percent signs in its
words, twice, and one for staff 5000) end with 0 and no report.

**Native tests** (`verovio_engine_tests`, 141 cases; 90 on tb.11): all
pass, together and each in a process of its own, on the Release build
and on the sanitizer build. On a fresh copy of tb.11 the four patches
apply in order with no offset, each stage builds and passes tb.11's own
90 cases, and after the fourth 139 of the 141 pass: the two that do not
are the integrator's cases about the log text, which need its patch of
the log calls. Of the 51 new cases, 49 fail or end the process on
tb.11's engine (46 fail, 3 end the process: the two about the log and
the syllable on a note of a chord). The two that pass there hold what
must not change: a score in which the converter finds no staff is
written with no label and the engine goes on, and a rest under a chord
symbol is not made one with the rest after a repeat sign or a double
barline. With the patches as first built, 14 of the 19 cases of the
second build fail or end the process (13 fail, the syllable on a note of
a chord ends it), and the 5 that pass hold what the first build did
right and no case held (the review's surviving breaks).

**Deliberate breaks.** 209 breaks of the lines and conditions of the
four patches, each compiled into a second copy of the tree behind a
number and switched on one at a time, in a build with AddressSanitizer
and the standard library's own checks, against the cases of the Humdrum
file: the 77 of this build, the review's 19, and 113 of the first
build's 115 (the text two of them stand on is gone; a break of this
build stands for each). 207 are caught, 192 by a failing case and 15
by ending the process. All 19 of the review's are caught, the seven no
case caught before among them: the two about a chord symbol a little
after a note and the one about the first staff's meter, the one about
words on a grace note's line, the one about a sign with words, and the
two about a label before the barline of the second rest and in a spine
that is not the first. Two breaks are caught by no case, and each is
the same program as the sources:

- "the place of a mark that starts on a note is worked out from its
  beat too": that place is read only for a mark with no note to start
  on, so working it out for the others changes nothing.
- (of the first build) "labels are written where there is no staff":
  with patch 22 a score in which the converter finds no staff has every
  staff refused, so it holds no note, no child gets a label, and the
  test that was broken never decides. It stays for patch 19 on its own,
  where a case needs it.

**The second review**, by an engineer who had written neither build:
safe to ship. Its 302 hostile inputs of its own, each taken every way an
application takes a file, on the sanitizer build with the standard
library's checks: none ends the process in the converter or in reading
an export, and no report; 28 trip the engine's own assertion when it
loads the hostile MEI (a staff with no `<staffDef>`), as on tb.11, one
another of its assertions (a hairpin whose end is before its start),
and one is the long chord text above. Every input the first review
named is mended. On every eighth score of the corpus (349) none that
matches stops matching and no dynamic is lost. It found the two faults
that section 22 marks as its own (a dynamic's text, a tempo mark's
line), both mended, and nine deliberate breaks that no case noticed:
five cases were added, which catch the four that matter (a hairpin in a
measure that holds nothing, which is memory given back and written to;
the sign that closes a hairpin in a later measure; a second hairpin on
one line; a chord text that is part of another), and the others are
recorded here: a mark with a beat, a staff and a note it starts on goes
by its beat; a mark that starts on a note stands after a grace note
before it; a layout line before a clef; two measures in a row that hold
nothing; a label that is not the very next line.

After it, on the final sources: 146 native cases (90 on tb.11), all
passing together and each in a process of its own, and on the sanitizer
build. Nine more deliberate breaks, of the two fixes and of four of the
review's: eight caught; the ninth (the tempo set in the staves read so
far only) is the same program, as the grid copies a tempo mark to every
staff of its line. The round trip, read back the way the applications
read a file: 2619 the same, 2 another number of notes, 159 differ
otherwise, 5 with no verdict (as on tb.11), none refused; no score that
matches on tb.11 stops matching, and the two fixes change the export of
no corpus score. Parity gate, tb.11 against tb.12: 19930 outputs, 0
differ; seeded ids on a fresh thread the same for 2785 of 2785.

Known, not changed (beyond what each section says): the 164 scores whose
export does not play what the original plays for other reasons and the
two that play another number of notes; the ten scores of the second
generation above; a chord symbol after the first beat of a rest of
several measures (the first of its measures comes back as a measure of
rest of its own under it, the others as one rest); under a
transposition, the chord symbols of a re-opened export (the declared
limit); the reader's time for a long expansion list, a staff that
transposes by more than an octave, a negative meter unit and a staff
with no `<staffDef>` in a later measure (patches 19 and 22, "known").

**Found on the way, not in these patches** (each with a file that shows
it, in the work's scratch directory; all of them on tb.11 too):

- `Tool_mei2hum::processHairpin` counts the beat of a hairpin wrongly
  in a meter whose unit is not a quarter note (it multiplies a time in
  quarter notes by four and divides by the unit, where the beat is that
  time by the unit over four), and with the meter of the thousandth
  staff, which only a meter in the `<scoreDef>` sets: in 6/8 a hairpin from beat 1 to
  beat 4 is written from the first eighth note to the sixth, in 2/2 one
  from beat 1 to beat 2 ends on the second quarter note. In 4/4 it is
  right. Mending it moves the hairpins of every export in such a meter.
- Words and a tempo mark at one beat, the words written first: the
  layout line stands before `*MM`, and a reader does not find the words
  (6 exports of the corpus).
- Words with a sign in them ("To" and a coda sign) come back from the
  reader with the sign inside a `<rend>`, which the converter does not
  look into: the second export has neither the words nor the sign (4
  such tokens in the corpus).
- A tempo mark that stands in its measure before one of the staves is
  written in a line as wide as the staves read so far: with two staves,
  a line of one field, and the file is refused when read back.
  (`parseTempo` sizes its line by `m_maxStaffInFile`, as `parseDir`
  did.)
- The grid counts verses for a hundred staves: a score whose
  hundred-and-first staff is sung gives no export
  (`HumGrid::setVerseCount` lets the index one past the end through),
  and one whose hundred-and-second is sung gets a line with one field
  too many.
- `analyzeMultiRest`: a rest under a `**mxhm`, `**dynam` or `**text`
  token, or under words, takes the plain rests after it across a repeat
  sign or a double barline (patch 21's "known").
- `Tool_mei2hum::parseStaff` takes a staff's length from its first
  layer: a staff whose first layer holds nothing and whose notes are in
  a later one makes a measure of no length, the next measure is written
  into it, and its barline is lost.
- In the Dart wrapper, `stripHumdrumSecondaryExpansions` left a
  secondary list as it was when it stood in the first spine with null
  interpretations beside it (`*>norep[A,B]<TAB>*`): every column had to
  match. The engine then followed no list, and the file was played
  without its repeat. Mended in the wrapper (MuseNote's side) with the
  vendoring of this tag.
- Loading an MEI with an `<ending>` or a `<section>` that holds nothing
  trips an assertion of a Debug build (`preparedatafunctor.cpp`, line
  1587); found by the review, in five of its random inputs.

### 23. What the engine says of a file is text, not the format of a message

`LogWarning`, `LogError`, `LogInfo` and `LogDebug` take a format, as
`printf` does. Forty-one calls handed them, as that format, text that
holds words of the file:

- `Toolkit::LogRedirectStop` and `LogRedirectStart` (`toolkit.cpp`), with
  everything a converter wrote to `cerr` while it worked. The MEI to
  Humdrum converter says what it leaves out with the words themselves
  ("staff number required on dir element ... (ignoring text: ...)"). A
  score whose words hold a percent sign ended the process when it was
  exported: "%s" eight times read memory that is no text, "%n" wrote to
  it.
- 33 warnings of the Humdrum reader (`iohumdrum.cpp`), several of which
  name a line or a token of the file ("Decoration is invalid: ...",
  "Problem parsing mensuration: ...", "dirtok for ... is empty"). A
  Humdrum file with a percent sign in such a line ended the process when
  it was opened; and a rhythm written with a percent sign (`4%3`), which
  is ordinary kern, was read as a format wherever a warning named its
  token.
- 5 messages of the PAE reader (`iopae.cpp`) and one debug line
  (`convertfunctor.cpp`).

The applications run the engine with its log switched on and at the level
of warnings, so each of these was reached.

**What the patch does.** Each of the forty-one calls passes its text as
the argument of `"%s"`. And the four functions are marked as printf-like
(`VRV_LOG_FORMAT` in `vrv.h`, an attribute for GCC and Clang, nothing
elsewhere), which is what lets a compiler see such a call at all: with
the mark, `-Wformat-security` names every call whose format is not a
literal. The applications build the vendored engine with every warning
off, so no build of theirs says anything; the package has an audit for
it (`tool/log_format_audit.py`, MuseNote's side, run at each vendoring):
it compiles every engine source again, syntax only, with that one check
as an error, and it refuses to run when the mark is not there. It found
the forty-first call.

No output of any score changes: a message that held no percent sign is
the same text. No object is built or removed.

Carried because the calls are the engine's.

Regression test: `test_mei_to_humdrum_engine.cpp`, two cases (words
with percent signs in a score that is exported; a Humdrum header line
with percent signs in a file that is opened). On tb.11 both end the
process. Six deliberate breaks, each caught: the two calls the cases
reach by the cases, two of the other calls by the audit, and the mark
taken away or emptied by the audit, which then says it cannot see.

Known and left: the same check with `-Wformat` names 12 calls whose
arguments are of another type than their format says (a `size_t` for
`%d` in `doc.cpp`, `iomusxml.cpp`, `preparedatafunctor.cpp`,
`resources.cpp` and `editortoolkit_shared.cpp`; one argument too many in
`ioabc.cpp`; and in `editortoolkit_neume.cpp`, which the applications
do not use, a number given for `%s`). Upstream's, not changed here.

### tb.13: patches 24 to 29, and the amendment of patch 19

From the two reviews of tb.10 and their second readers, the attack on
tb.11, and a reading of the whole ABC reader. tb.10 and tb.11 added none
of these faults: each is the same on tb.9. The stage for every
comparison below is tb.11: the five patches were built and measured on
it before tb.12 was tagged, and "with the patches" is tb.11 with them.
tb.13 is tb.12 with them, with what the review of tb.13 found mended
(patch 29 among it) and with the amendment of patch 19; its gates were
run again on it. Both are at the end of this section.

**24. ABC: no input ends the process.** `ABCInput` read on for the
character that closes a thing without asking where its line ends, kept
objects it had deleted, and took a number of the file as it came.
Thrown out of the library, and so the end of the process:

- an inline field with no `]` on its line (`[K:D …`), and `[|:`, a bar
  line, which was read as a field named `|` up to the next `]`;
- a `!` that ends a line (the line-break sign of ABC before 2.1, in old
  tune books), and a `!` that no other closes;
- a `"` with nothing after it, and a text that is not closed;
- a tuplet that is the last thing on its line (its place came back as
  -2, and the line was read from before its start);
- lyrics under a line of music that ends in grace notes (the note was
  read before its index was looked at);
- a key of more than seven flats (`K:Fb`, `K:Db min`: the steps to
  alter were read from before the first of the seven);
- a tempo with no number after its `=` (`Q:1/4=fast`), or one too large
  (`std::stof`), and a tempo text with no closing quote (cut at "no
  position").

An object that was not there, or was gone:

- a `]` with no chord open;
- a file with an `X` and no key after it: the tune was never begun, and
  the end of the reader read it. A key that is the file's last line,
  with no line end, was not read at all (a line was read only while
  another followed it), and a key with nothing in it (`K:`) was no
  field: the lines of music after it were taken for the header, to the
  end of the file or into the next tune;
- an `X` with no number (`X:`, `X: `, `X`) began no tune: the header's
  end read the tune that was not there, or, for a second tune, put its
  score into the tune before, whose export the engine could not read
  back;
- line breaks set to `<none>` in the header and set back after it: the
  first line of music ended in a section with nothing in it, whose last
  child was asked what it is;
- a chord symbol, a slur or a tie open in a last measure that has no bar
  line, at the empty line that ends the tune: deleted with the measure
  that was never closed, and left in the list its next note looks in.
  The note after the empty line read it, or wrote its id into it. This
  is a tune as it is written every day (a tie or a slur left open at
  the end, an empty line, the next tune); only the slur ends a process
  that has no sanitizer, and not every time;
- two changes of clef in a row with no note between them (`[K:…][K:…]`)
  while a tuplet waits for the rest of its notes: the tuplet was deleted
  with the notes it had, and the line's lyrics read a note that was
  gone. (Without a tuplet the two clefs were put into a beam and then
  into the layer, an object with two parents: an assertion where
  assertions are on.)

Never back:

- a length too long for an int (`C4294967295`, and so for a rest and a
  space): it comes out below zero, and the loop that counts its dots
  never ended;
- a meter whose unit is too long for an int (`M:4/99999999999999999999`):
  below zero, and the MIDI writer never came back from working out which
  power of two it is.

And three reads that end nothing on the machines these were run on:

- `UTF8to32` (`src/vrv.cpp`), the converter every reader gives its texts
  to, took the second, third and fourth byte of a character without
  asking where the text ends. A lyric, a title or a chord symbol that
  ends inside a character of three or four bytes was read one or two
  bytes past its end. Found by the hostile ABC set under
  AddressSanitizer (2 of its 5,107 files); it is no fault of ABC's: 7 of
  56 MusicXML files with such a text in `<words>`, a lyric, a part name
  and a title report it on tb.11 too. A character the text ends inside
  is now left out;
- an `I:decoration` with nothing after it read the sign from past the
  end of the instruction's text (the tune's `!fermata!` was then four
  notes);
- a rest or a space of length 0 (`z0`) was divided by it, which ends the
  process on a processor that traps the division (x86-64, which the
  Android build has; not the arm64 these were run on, where the division
  gives 0, the number the patch gives).

What the patch does, in the same order: each of those loops is bounded
by its line, and what is open at the end of a line ends there; an inline
field is `[`, a letter, `:` (ABC 2.1, 3.3); a `!` that no other closes
on its line is passed over; a `]` with no chord open is passed over; the
lyrics' index is looked at first; a key of more than seven signs alters
all seven steps; a tempo with no number is given none, and its text runs
to the end where it is not closed; a tune begins at its `X` with a
number or without, its header ends at its key, be that the last line or
an empty `K:`, and a file with no tune in it is refused
(`ABCInput::Import` returns false, as `Toolkit::LoadData` expects of an
importer; each of the nine such files of the sets below ended the
process on tb.11 or was refused there too); the last child of a section
is asked for only where there is one; what `FlushControlElements`
deletes is taken out of the three lists first; a stack of elements with
no note among them goes to the layer as it is, and a tuplet that waits
is closed with the notes it has; a length below zero has no dots
counted; a meter unit below zero is none; a character is converted only
where the text holds all of it; the decoration sign is taken only where
the instruction has one; a length of 0 is not divided by.

**Also in patch 24, and no crash: a tune's key is its own.** The steps a
key alters are kept in two variables outside the reader, and a key with
no sharps or flats did not empty them. A tune in C or A minor read after
a tune in D was played with F sharp and C sharp: the next tune of the
same file, and the next file loaded by the same process, which in the
apps is every file after the first. The reader empties them where a tune
begins, at the `X` line that opens it. (As first built, any X field
emptied them, and one written inside a tune, `[X:2]`, took the key from
the notes after it: found by the review of tb.13.) It is here because
the case for a key of eight flats cannot stand beside the other ABC
cases without it (every tune in C after it was read in seven flats), and
because it is a fault a user meets. The parity gate could not see it: it
reads each ABC file in a process of its own.

Declared change of patch 24, for a file that loaded before: a tune with
no sharps or flats in its key, read after one that has some, is read in
its own key. `[`, a character that is no letter, `:` is no inline field
(`[|:` is a repeat's start; `[::` opens a chord, as `[:` did). Two
changes of clef in a row are no longer put into a beam that is then
thrown away: the ids of the objects built after them are those of one
object less, and a tuplet that waits for notes at that point is kept
with the notes it has, where it was deleted with them. A tempo of `inf`
or `nan` is none (it was kept, and no note of the tune was played). A
second tune that begins with an `X` with no number is a tune of its own.
A text, in any format, whose last bytes are the beginning of a character
of several bytes ends before them: it ended in a character made of
those bytes and of what lay after the text (for the first byte of a
two-byte character that was the text's own closing zero, so `caf` and
the byte C3 was read as "cafÀ" and is now "caf"). None of these is in
the 194 ABC files of the parity set, and no output of the 2785 scores
of the corpus changes.

**25. ABC repeats, the rest of what ABC 2.1 says.** tb.10 wrote the
start that a repeat end with no start means on the measure it stands
before, where that measure had no bar line on its left, and looked for
the place to go back to at the right of measures only.

- A tune whose lines begin with `|` is the ordinary way to write one,
  and gives the first measure of each line a left bar line: the start
  was not written, and the end went back to the beginning
  (`|: C | E :|`, `| G | A :|`, `| B |]` was C E C E G A C E G A B).
  The start is now written over whatever bar line the measure begins
  with; an end that stands there stays, as `rptboth`.
- A double bar met while the measure is still empty stands on the left
  of that measure (`C | E |`, then `|| G | A :|` on the next line), and
  was not a place to go back to. It is one, and so is the thick-thin
  `[|`, which was read as a bar line of one stroke.
- An end met while the measure is still empty (`C | :| E | G :|`) is
  the end of the repeat before it: it has the start that is meant
  written for it, uses up a start that was written, and the measure it
  stands on is the place the next end goes back to. Small and plainly
  right: the engine's own order already repeats at an end on the left.
- A line of music that begins with `::` was taken for an information
  field (its second character is a colon) and its measures were lost: a
  field begins with a letter.
- An ending sign took the character after its first digit with it.
  `[1C` lost its note; `[1"D"d` lost its quote, so that the letters of
  the chord symbol were read as notes, the notes after it as a chord
  symbol, and the last quote of the line opened a text that nothing
  closed (one of the exceptions of patch 24: `|[1"D"d4:|[2"G"g4|]`, a
  line as tune books have it); `[1[EG]` lost its bracket. ABC 2.1 (4.9)
  asks for no space after the sign. The sign is now `[`, its number, and
  further numbers with `-` between them (`[1-3`), and no more. This is
  about what stands after the sign only: first and second endings are
  still not read as such (known, below).

Of the two reviewers' 102 tunes with an answer in ABC 2.1, 76 play as it
says on tb.11 and 94 with the patches (first reviewer 62 and 70 of 75,
second 14 and 24 of 27); none that was right is wrong. The eight that
are left are the known ones below: five with endings, two with `|::`,
one whose last measure has no bar line.

Declared change of patch 25: the tunes described in its row. Of the 194
ABC files of the parity set 13 have a line that begins with a bar line;
in each the line begins `|:`, a written start, and none has a double bar
or an end at the left of a measure, a `[|`, or a line that begins with
`::`. 41 have an ending sign, each followed by a space, after a bar line
or first on its line. None changes, to the byte.

**26. A bare `Z`.** `Z` with no number is one measure of rest (ABC 2.1,
4.5). The digits after it were read with `atoi`, which gives 0 for none,
and given to a `<multiRest>`: a measure with no time, so that every note
after it started a measure early, and the tune came back a measure short
for each. MuseNote's own ABC export writes `Z` for an empty bar. It is
now an `<mRest>`. A `<multiRest num="1">` plays the same in every meter
tried (4/4, 3/4, 6/8, none) but is drawn with a "1" over the bar, as a
multi-measure rest is; an `<mRest>` is drawn as an empty bar is, is what
MuseNote's own notation rule says an empty bar is, and is what the
export was written from. `Z0`, and a number too long for an int (which
comes out as -1, and after which the notes of the tune were not played
where they stand), are read the same way. `Z1` stays a `<multiRest>` of
one, as it is written; `Z2` and up are as before.

Declared change of patch 26: 14 of the 194 ABC files of the parity set,
42 rests: `rests_ties_slurs.abc` (1) and thirteen of MuseNote's exports:
All You Need Is Love (5), Baby Blue (2), Beauty and the Beast (5),
Cavatina (6), Circle Of Life (4), Daniel (4), Fly away (2), Matchmaker
(1), My Sweet Lady (2), Princess Leia's Theme (3), The Song Is You (3),
Wonderful Land (2), Yellow Submarine (2). In each MEI the lines
`<multiRest xml:id="…" num="0" />` are `<mRest xml:id="…" />`, with the
ids they had, and no other line differs. The timemap of each changes
(and the MIDI of eleven), the number of notes of none. One before and
after: Wonderful Land, whose MusicXML original is 66 measures and 264
quarter notes long. Its ABC export read back on tb.11: 64 measures,
255.5 quarters. With the patches: 66 measures, 263.5 quarters.

**27. The cost of an order of play, counted while it is written.**
Patch 16 counts what following an order costs and does not follow one
over 1,000,000. It counted when the whole order stood in the document,
and both writers (`ExpansionMap::GenerateExpansionFor` for a score with
no order of its own, `MusicXmlInput::CreateExpansion` for MusicXML) had
added each reference with `PlistInterface::AddRefAllowDuplicate`, which
copies the whole list twice for every one. The ceiling bounded memory
and not time: a MusicXML file of 15 KB, sixty bars each marked ×100,
took 140 seconds in `loadData`, and the time grew with the fourth power
of the number of bars; 400 plain backward repeats took half a minute.

Each writer now gathers the references in an `OrderOfPlay` and gives the
list to the `<expansion>` once. `OrderOfPlay::Add` keeps the least that
following the order can cost: each section, by the times it is named,
squared (a section holds one element at the least, so this is never
more than the count of patch 16), and adds nothing once that is past the
limit. The limit is one constant, `OrderOfPlay::s_maxCost`, which
`Doc::ExpandExpansions` uses too. So no order that was followed is cut
short: where the least cost passes the limit, the cost does. The writer
stops when the least cost is past the limit and not when it has reached
it, so that what it leaves always costs more than is followed: a
hundred bars each a repeat of its own marked ×100 come to 1,000,000 to
the unit, and the bar after them is written too (a case holds it).

*What becomes of the order that is cut short.* It stays in the
document, the beginning of the order as far as it was written: an
`<expansion>` with 2,253 references for twenty bars each marked ×100
(20,810 by the signs), 10,365 for two hundred repeat ends in a row with
no start (20,300). `Doc::ExpandExpansions` counts it as it counts any
order, finds it over the limit (it was written until it was), and does
not follow it: the score is played as written, once through, with the
warning it had. An exported MEI holds the same short list; read back, it
is counted again and not followed again. It must not be taken for the
score's order by anything else, and a tag that raises the limit must
have such exports made again: a list cut at 1,000,000 could be within a
higher limit. (The engine writes the order of a score with none of its
own into the copy it plays from, not into the document it exports; in an
export the short list is there for MusicXML only, and with
`expandAlways`.)

The comment above the count in `src/doc.cpp` said 23,000 for the dearest
score and that a measure marked 2,000 times "is played as it is
written". It now says 30,000, that a marked repeat is read as at most
100 and then followed or not by its cost (a measure marked 2,000 times
is played 100 times), and that past the limit all repeats of the score
are dropped, not only the dear one.

Declared change of patch 27: none for an order that is followed. For
one that is not: the short `<expansion>` above, in the document
MusicXML's importer builds and in its MEI.

**28. MusicXML files that ended the process.** Older than tb.10, in
`MusicXmlInput`:

- a `<chord/>` on the first note of its layer: there is no note before
  it, and the last of an empty list was read. The note is now left out
  with the error the importer has for a chord whose start is not found;
- `std::stoi` and `std::stod` on a text of the file, which throw out of
  the library for a text that is no number or one too large: a rest's
  `<display-octave>` (`x`, 99999999999999999999; a rest with a type and
  one that is the whole bar), a metronome mark's `<per-minute>` (1e999,
  1e-999, four hundred digits), and a chord symbol's `<degree-value>`
  (`x`, nothing, too large). Such a number is now passed over: the rest
  gets no `oloc`, the mark no `mm`, the symbol no "add";
- a `<staves>` that is no number of staves (`x`, nothing, 0, -1, 32768,
  99999): the part was left with no staff at all, and its first note was
  put on the staff that was not there. A part has one staff at the
  least. (Found by the memory review of tb.10; reproduced before it was
  mended.)
- a part whose first measure does not begin with its `<attributes>` (a
  note, a rest, a `<harmony>` or a `<backup>` stands before them). A
  part's `<staffDef>`s, and the places of its staves in the table of
  octave lines (`m_octDis`), were made from the `<attributes>` the first
  measure begins with, and such a part got neither. Its pitched notes
  read the table one past its end (the read the attack on tb.11 found: a
  stale number there moved the note by octaves), and an octave line
  written in it was passed over. With no `<staffDef>` for a staff that
  holds notes, the engine's score-definition pass read the one that was
  not there: the process ended for a second or later part, and for a
  part of two staves. Only a score of one such part of one staff loaded,
  by the `<staffDef>` the importer gives a score that has none at all,
  which carries neither the part's name nor its instrument. Every staff
  of a part now has its `<staffDef>`, of five lines where no
  `<attributes>` said how many, and its place in the table, made by the
  lines that made them before (moved into one place and called from
  two). This is the cause, so the read of the table needs no check of
  its own: a staff's number is never past it.

Declared change of patch 28: such a file loads. For a file that loaded
before: a score of one part of one staff whose first measure does not
begin with its `<attributes>` has the part's id, name and instrument on
its `<staffDef>` (`<staffDef xml:id="P1" n="1" lines="5">` with its
`<label>`, where it was a `<staffDef>` with an id of its own and no
name), an octave line in it is read, and the seeded ids of what the
importer builds after the `<staffDef>` are other ids, since it is now
built first. No score of the corpus is such: no output of the corpus
changes.

**29. A meter whose unit is below one, in the MIDI writer (35a5f9173).**
`MidiMessage::makeTimeSignature`, in the midifile library inside the
engine, works out which power of two a meter's unit is by shifting the
unit to the right until it is 0: `while (bottom >>= 1) base2++;`. A
number below zero stays below zero when shifted, and the loop never
ended. Patch 24 keeps ABC's own unit from coming out below zero; the
review of tb.13 reached the same loop from MusicXML, by a `<beat-type>`
of -1 or one too long for an int, and the builder had declared it from
MEI. Such a score loads, is drawn and has a timemap, and its MIDI never
came back: in the apps it opens and cannot be played. The loop now
shifts while the unit is above 1, which gives the same power for every
unit of 0 or more, and 0 for one below. No output of the corpus changes,
nor of any score whose meter's unit is 0 or more. Regression test:
`test_midi_document_engine.cpp`, one case: the units `-1` and
`-99999999999999999999` from MusicXML, and the power of two the MIDI
gives the units 1, 3, 4 and 8, which no case read before (a loop that
counted one power too many passed every case). Three deliberate breaks
of the loop, each caught: the old loop (the case does not come back),
one power too many, and the unit less one. Carried because the loop is
in the engine's own copy of that library, as upstream Verovio has it.

**Amendment of patch 19 (4c6f77118).** Patch 27 makes the two writers of
an order stop once its least cost (each child by the times it is named,
squared) is past the limit: what they leave is the beginning of the
order, which the engine does not follow. Patch 19's export wrote that
beginning as the score's order, the list and a label for each child. It
now counts the same least cost and writes no order at all once it is
past the limit (`orderOfPlay` in `src/hum/humlib.cpp`, +18 / -0; the
number is written there again, as humlib is built without the engine's
classes). An order within that count is the whole order the file says,
and is written, whether or not the engine follows it: the engine also
weighs each child by its elements. No order of the corpus is near the
limit: the export of no score changes. One case (a section named 1,000
times is written with its list and its labels; one name more, and no
order is written); four deliberate breaks of the new lines, each caught.

**Known, not changed.**

- *ABC, first and second endings* (`[1`, `[2`, `|1`, `:|2`) are passed
  over: both are played each time. 40 of MuseNote's 183 exports in the
  parity set hold `[1`. Five of the reviewers' tunes.
- *ABC, `|::` and `::|`* mean three times in ABC 2.1 and are read as
  twice. Two of the reviewers' tunes; a case says so.
- *ABC, the last measure of a tune with no bar line after it* is not
  read: `C D E F | G A B c` plays four notes. One of the reviewers'
  tunes (`C :: E`).
- *ABC, a line that begins with a letter and a colon* is an information
  field, as ABC 2.1 has it: `c:| …` loses its measures.
- *ABC, a tie* is written with a start that names no note (the `#` is
  missing): the tied note is struck again.
- *ABC, `K:C bass`*, a key followed by a clef named without `clef=`, is
  read as C flat major: the `b` of "bass" is taken for a flat. `K:G bass`
  is G flat, `K:bass` seven flats. `K:C clef=bass` is read right.
- *ABC, a second tune's default note length* is worked out from the
  first tune's meter.
- *ABC, `[::`* opens a chord (a `[` before the dots of a bar line is not
  known as part of the bar line).
- *ABC, a rest of more measures than the engine's clock holds*
  (`Z2147483647`): the notes after it are not played after it.
- *ABC, a file whose lines end in a carriage return alone* has no line
  the reader knows, and is refused (it ended the process).
- *The cost of an order*: past the limit all repeats of a score are
  dropped, not only the dear one, and a repeat marked more than 100
  times is read as 100. An order within the limit can still be slow:
  3,000 double repeats in a row (33 KB of ABC, cost under the limit) take
  28 seconds for the timemap, on tb.11 and with the patches. The
  Humdrum reader adds a file's own expansion list as the two writers
  did (20,000 labels in one list: 1.7 seconds).
- *An order of play that an MEI file writes itself* is counted as it
  was: patch 27 is about the two writers of the engine. What the memory
  review of tb.10 found there stands, and is the same on tb.9 (its
  runs; five of its files run again here end the process with the patches as
  on tb.11): an
  `<expansion>` that names itself, or two that name each other, end the
  process at the timemap (the stack runs out); a chain of them, each
  naming the next twice, is counted as cheap and takes gigabytes; an
  order that names a section and then the section that holds it, or
  something that is no section, ends the process; the count walks the
  document once for every different reference (5,000 sections each
  named once: 5 seconds more than tb.9); it is kept in a `size_t`,
  which is 32 bits on armeabi-v7a, where 4,096 times a section of 256
  elements counts as nothing; and ids of thousands of characters are
  counted as short ones (14 KB: 1.2 GB).
- *A staff of very many lines, a part of very many staves*: the
  engine's time grows with the square of them (40,000 lines: 10
  seconds; 4,000 staves: 200 seconds), from any format; in ABC one
  header line is enough (`K:stafflines=99999999999`, found by the review
  of tb.13). A meter unit below zero, which stood here, never came back
  from the MIDI writer from any format: patch 29.

**The gates.**

Parity gate (`tool/parity/run.sh`, then `compare.sh` against the dump
of tb.11): 19930 outputs compared, 14 differ, the 14 ABC locator exports
of patch 26 named above, each by its `<mRest>` lines and nothing else;
seeded ids on a fresh thread deterministic for 2785 of 2785. So the
SVG, timemap, pitch map, MIDI and MEI of every score of the corpus are
as on tb.11 to the byte, and with them the order every score is played
in. The gate does not play its locator inputs: the timemap and MIDI of
its 194 ABC, 189 Humdrum, 8 PAE and 3 GABC inputs were set beside
tb.11's one by one: 380 the same, the 14 not.

Native tests (`verovio_engine_tests`, 115 cases and 3214 assertions; 90
cases on tb.11): all pass, in one process and each in a process of its
own, Release and under AddressSanitizer with no report; and so on a
fresh copy of tb.11 with the five patches applied in order (each patch
builds on the ones before it). Of the 25 new cases 22 fail, end the
process or do not come back on tb.11 (the stage sources built with the
new test files, in a third directory): 21 without a sanitizer (13 fail,
7 end the process, 1 does not come back), and one more under
AddressSanitizer (what an empty line leaves in the reader's lists: a use
after free). The other three hold lines of tb.10 that no case held: the
white-space field with a value long enough for the sanitizer to see,
the limit to the note, and a stop whose number no open line has.

Deliberate breaks (`mut12.py`: every line and condition of the five
patches, and the ten breaks of tb.10's lines that its reviews found
passing every case; in a second copy of the tree with build directories
of its own, one of them with AddressSanitizer for the breaks that bring
back a read out of bounds): 112 tried, 106 caught, 6 not. Of the ten of
the reviews, nine are caught: seven by the ABC cases with two bars
between ends, lines that begin with a bar line, `::` last and first on
a line and a second tune, the limit by the cases at 1,000,000 and
1,000,004, the white-space field under the sanitizer, the number of
another staff's line by a case of its own. The tenth is the same
program (the octave stop's line, under Corrections). The other five
that are not caught:

- a rest and a space of length 0 divided by it (two): on arm64 the
  division gives 0, the number the patch gives, so the two are one
  program on the machine these ran on. The lines are for x86-64, where
  the division traps; no x86-64 build was run;
- the ending sign's numbers looked for from its second character: the
  same program, since the sign is taken only where its first character
  is a digit;
- two stops that save time and nothing else: the engine's own writer
  going on through what was repeated on the way once the order is full,
  and MusicXML's importer going on through its loops. The output is the
  same to the byte. The time is not: 20,000 repeat ends in a row, 3.0
  seconds for the timemap against 5.5; 3,000 bars each marked ×100, 1.4
  seconds to load against 9.9; both grow with the square of the bars. A
  case that held them would be a case with a clock.

ABC inputs, one process each, with the first page, the timemap, the
pitch map, the MIDI and the MEI asked for and the MEI read back. The
count is of the inputs that end the process, are reported by the
sanitizer or an assertion, or do not come back:

| ABC inputs | files | tb.11 | tb.11, sanitizer | patches | patches, sanitizer |
|---|---|---|---|---|---|
| The second reviewer's spellings of colon, bar, the two brackets, space and a note in three places of a tune (`fuzzabc.py`) | 4662 | 1529 | 1529 | 0 | 0 |
| First hostile set: one fault each by hand, one hostile token in five places of a tune, header fields with hostile values, 2,500 lines of tokens drawn at random | 5107 | 2711 | 2724 | 0 | 0 |
| Second hostile set: ending signs with all that can stand against them, texts that end inside a character | 1983 | 888 | 936 | 0 | 0 |
| The memory review's own set | 6059 | 622 | not run | 0 | 0 |

With the patches nine of these files are refused by the importer (no tune in
them: an `X` and no key, nothing at all, a file with no line end the
reader knows); six of the nine ended the process on tb.11 and three
were refused there. Every other file loads, and the MEI of every one
that loads is read back by the engine (on tb.11 the export of 135 was
not).

MusicXML inputs for patch 28, the same way:

| MusicXML inputs | files | tb.11 | tb.11, sanitizer | patches | patches, sanitizer |
|---|---|---|---|---|---|
| The files named by the attack on tb.11 and by the memory review | 8 | 7 | 8 | 0 | 0 |
| First set of variants: `<chord/>` with no note before it, numbers that are none, notes before `<attributes>`, `<staves>` | 90 | 36 | 38 | 0 | 0 |
| Second set: parts that do not begin with their `<attributes>` in every place and with every thing first, octave lines in them, texts that end inside a character | 108 | 32 | 51 | 0 | 0 |

The slow files of the reviews, and orders near the limit on both sides
(seconds for `loadData` and for the timemap, peak memory; the machine
was shared, and under load while these ran):

| File | Size | tb.11: load / timemap / MB | patches: load / timemap / MB | Played, patches |
|---|---|---|---|---|
| MusicXML, 60 bars each a backward repeat marked ×100 | 15 KB | 115 / 17 / 270 | 0.04 / 0.01 / 12 | 60 notes, as written |
| the same, 100 bars | 25 KB | not back in 200 s | 0.04 / 0.02 / 14 | 100, as written |
| the same, 3,000 bars | 785 KB | not back in 200 s | 0.97 / 0.26 / 137 | 3,000, as written |
| MusicXML, 400 plain backward repeats | 93 KB | 31.7 / 5.1 / 153 | 0.18 / 0.10 / 27 | 400, as written |
| the same, 2,000 | 497 KB | not back in 200 s | 0.68 / 0.25 / 99 | 2,000, as written |
| Humdrum, 400 repeat ends with no start | 4.3 KB | 0.13 / 28.8 / 157 | 0.12 / 0.10 / 27 | 400, as written |
| MEI, 20,000 repeat ends with no start | 2.4 MB | not back in 200 s | 6.2 / 2.1 / 728 | 20,000, as written |
| MEI, 82 ends, cost 1,000,000 | 10 KB | 0.12 / 5.1 / 101 | 0.03 / 0.87 / 103 | 3,878, followed |
| the same with one note more, cost 1,000,004 | 10 KB | 0.04 / 0.15 / 13 | 0.03 / 0.02 / 12 | 92, as written |
| MusicXML, one bar of 48 notes ×100, cost 1,000,000 | 6 KB | 0.01 / 1.05 / 99 | 0.02 / 0.34 / 98 | 4,800, followed |
| the same with 49 notes, cost 1,020,000 | 6 KB | 0.01 / 0.06 / 10 | 0.01 / 0.01 / 9 | 49, as written |
| 17 parts, a 32-bar chorus ×8 (the first reviewer's) | 665 KB | 0.69 / 3.8 / 337 | 0.80 / 4.3 / 353 | 35,904, followed |

103 files in all (the two reviewers' 85, 18 of mine). For the 90 that
came back on tb.11 the timemap and the MIDI are the same with the patches to
the byte, followed or not; the 13 that did not come back in 200 seconds
are each played as written with the patches, a note for a bar.

**Corrections to earlier entries.**

- **tb.10, patch 11 (octave lines)**: "in a part of one staff a line ends
  where it did" is not true in three cases. With two numbered lines open
  at once on one staff, each now ends at its own stop (tb.9 ended both
  at the first), and the staff's notes are set back at the first stop,
  so the note between the stops lies under a line without being moved;
  its sound was wrong on tb.9 too. A `continue` with no line open no
  longer starts a line (tb.9 drew one from there, always "above"). And a
  stop with a number takes a line of that number that still waits for
  its first note on another staff: tb.9 kept that line. No code is
  changed for these: the second is tb.9's guess taken away, the third is
  the rule doing what it says for the file whose stop is that line's
  (left alone, the line would run to the end of its part, the fault
  patch 11 was written against), and two octave lines at once on one
  staff is no notation.
- **tb.10, patch 15**: "ABC's repeats read as ABC says" was true of an
  end with no start in a tune of one line with no bar line before its
  measures, and of the double repeat inside a line. With patch 25 it is
  true of the tunes above as well, and still not of first and second
  endings, of `|::` as three times, or of a last measure with no bar
  line.
- **tb.10, patch 16**: a marked repeat is read as at most 100 and then
  followed or not by its cost; "a measure marked 2,000 times … is played
  as written, once through" is true only where 100 times is over the
  limit. Past the limit every repeat of the score is dropped. The
  ceiling bounded memory and not time until patch 27, and for an order
  an MEI file writes itself it bounds neither (above).
- **tb.10, patch 13 (Humdrum export)**, known and not changed here: a
  staff that transposes by more than an octave (tenor and baritone
  saxophone, bass and contrabass clarinet) is exported by whole octaves
  only, its notes and its key (`Convert::transToBase40` has no entry
  past an octave; the engine's own reader makes the same omission, so
  the file reads back as written, and any other Humdrum program gets
  notes a tone or more off). A key held by the `<scoreDef>` of a score
  whose `<staffDef>` transposes is not moved. A concert key past seven
  signs is not written.
- **tb.10, the comment above the count in `src/doc.cpp`**: corrected by
  patch 27 (23,000 was 30,000; "played as it is written" was 100 times).
- **tb.10, one line of patch 11 that no case holds, and none can**: the
  octave stop's `if (!named) m_octDis[staffNum] = 0;`. Taken out, every
  case passes, and 3,063 MusicXML files with octave lines (3,000 drawn
  at random for this, with numbers, staves, stops with no line and two
  lines at once; the reviewers' 63) give the same MEI and the same
  pitches. It is the same program without it: a staff's notes are moved
  only while a line is open on it, a stop that no number names closes
  every line open on its own staff, and the loop that closes a line sets
  its staff back. The line is left as it is.

**The review of tb.13.** A second engineer, who had not written the
patches, reviewed tb.13 as it was first put together: its code, then
18,233 ABC files made for it (every token alone, in pairs and in threes;
rich lines cut at every place; 28 fields by 41 hostile values, in the
header, the body and inline; 1,440 ways one tune follows another; 324
shapes of lyrics), 390 MusicXML files, 85 orders of play at and around
the limit, and eleven deliberate breaks of its own. Of the ABC files,
2,165 end the process on tb.12 and load on tb.13, 81 do not come back on
tb.12 and load on tb.13, and 400 end it on tb.12 and are refused on
tb.13 (they hold no tune); of the MusicXML files, 55 end the process on
tb.12 (32 by a segmentation fault, 23 by an exception out of the
library) and load on tb.13. Every order of play is followed, or played
as written, at the same point on both, with the same timemap and MIDI to
the byte; the slow ones come back at once (60 bars each marked ×100: 67
seconds to load and 11 for the timemap on tb.12, 0.04 and 0.02 on
tb.13). What it found:

- *An X field inside a tune emptied the tune's key* (patch 24 as first
  put together): `[X:2]` or `[X:]` in a tune in D, and the notes after
  it were played in C (327 of its files). ABC has no X there, and the
  reader takes it as any other field. Mended (patch 24, above), and held
  by a case.
- *The MIDI writer's loop*: mended for every format (patch 29), where
  patch 24 had kept ABC from reaching it.
- *Four deliberate breaks that no case caught.* One takes the
  instrument away from every MusicXML part of one staff (it is put on
  the staff where patch 28 makes the staff's definition): a case holds
  it now. The other three are the same program for every file that is
  not malformed in one way each: a header line that is a `K` alone, or a
  `K` and one character other than `:` (the break takes it for the key,
  the patch reads on to the key); `[`, a character that is neither a
  letter nor `|`, and `:` (the break takes `[::` for a field, the patch
  opens a chord: both wrong, and declared above); and a text whose last
  character is cut (the break reads the bytes the cut character leaves
  as a character of their own: none, or U+0000, which no output of the
  case shows; only where one of them is no byte of a character's
  middle is it a letter the patch leaves out).
- *The comment of the amendment of patch 19* said more than the code
  does (that no order the engine does not follow is written); it says
  what the code does now.
- Two changes it measured, declared here: a `<staves>` of 65,538 or
  -65,534, which tb.12 read as two staves (the number cut to 16 bits),
  is no number of staves now, and the part has one; a note written for
  its staff 2 stands on staff 1, with `staff="2"`, at the same pitch. A
  second `X` with no number inside a header begins a tune of its own
  (the rule of patch 24), with no title: the title read before it is
  the first, empty tune's.

Known, from the review, and not changed (each the same on tb.12; for a
later tag):

- an ABC tuplet whose numbers are too long for an int
  (`(3:99999999999`): its notes are hundreds of millions of quarter
  notes long, and a load of the tune's MEI or of its Humdrum export
  does not come back;
- the Humdrum reader's table of the accidentals of a bar holds ten
  octaves (`analyzeKernAccidentals`, `src/hum/humlib.cpp`), and a note
  above them is read past the table's end: a `**kern` note of the tenth
  octave or above (`4cccccccccccccc`), and so the Humdrum export of a
  MusicXML note of `<octave>10</octave>`, read back. The hardened build
  stops there; a sanitizer build reports a read of the heap;
- a MusicXML part whose first measure begins with a note loses the
  `<transpose>` of its later `<attributes>`: it plays as written (C and
  E, where the same part that begins with its attributes sounds B flat
  and D);
- an ABC order of play just within the limit can still be large to
  follow: 2,000 bars of 120 quarter notes, each repeated, are 480,000
  notes, 130 seconds and 3.8 GB, on tb.12 as on tb.13.

**On tb.13.** The same toolchain; Release, and AddressSanitizer with the
standard library's checks on (`_LIBCPP_HARDENING_MODE_DEBUG`). Native
tests (`verovio_engine_tests`, 174 cases and 5544 assertions; 146 on
tb.12): all pass, in one process and each in a process of its own, and
under the sanitizer with no report. Deliberate breaks of what was added
in integration and after the review: ten, each caught (two of the key,
three of the MIDI writer's loop, one that takes the instrument away,
four of the export's amendment). Parity gate, tb.12 against tb.13: 19930
outputs compared, 14 differ, the ABC locator exports of patch 26, by
their `<mRest>` lines and nothing else; seeded ids on a fresh thread
deterministic for 2785 of 2785. Measured apart over the 2785 scores, as
for tb.12: the export read back the way an application opens a file, and
the text as it is written, are as on tb.12 score for score. The audit of
log formats (`tool/log_format_audit.py`): 299 engine sources, 0 calls.

### tb.14: patches 30 to 49

Two sources. Patches 30 to 40 and 44 to 49 are faults a file a user can
open meets: the list the review of tb.13 left (above, "Known, from the
review"), the faults three builders found while they mended it, and what
the integration of their work found. Patches 41 to 43 are MuseNote's
plan V3_2_0_13 ("Every bar of a song is played"): the order of play the
MusicXML importer builds left out bars of 35 of the 2,759 scores of the
corpus, 484 notes drawn and never heard, and three of its causes are the
importer's. Each patch was built and measured on tb.13 with the patches
before it; each commit's message holds what it found, what it does and
what it changes, and the cases named below hold it.

**30 to 32. MEI.** An `<expansion>` that names itself, or two that name
each other, or names a layer, staff or note and then a section, ended the
process (932 of 7,254 orders made for it) or never came back (334); a
copy of a copy was written while it was read (30; an order may name a
section, an ending, a measure, another expansion, or an editorial
element at their level, and nothing inside a measure). The cost of an
order of play walked the document once for each section it named (5,000
sections: 27.9 s), counted an id of 14,000 bytes as 1 (280 MB more for
the timemap), counted an expansion it names as one element whatever it
plays (sixteen nested ones played a section 65,536 times at a cost of 4,
and never came back, on tb.13 too), and its products could wrap (31). A `<staff>` whose `@n`
has no `<staffDef>` ended the process at load (32). Cases:
`test_tb14_M_engine.cpp`.

**33 to 37. Humdrum.** The reader's tables of accidentals and linked ties
were read and written past their ends for a note of octave 10 or more,
or an ornament outside them (33); `HumGrid::addLastMeasure` read an
empty list (34); an order of play of 100,000 labels was stored in
quadratic time and never came back (35); a `**vvdata` token of 3,000
words made a pass over the document for each word (36: a token of more
than 64 words, a word being what stands between spaces, is one verse);
and each line
was copied once for every character read from it (37: a line of 2 MB
never came back). Cases: `test_tb14_H_engine.cpp`.

**38 to 40. ABC, any format, MusicXML.** An ABC tuplet whose number does
not fit an int, or of no notes, is none (38). A staff of fewer than 0 or
more than 32 lines has five, from any format (39: two billion lines
never came back; generated code, carried again at a regeneration of
libmei). A MusicXML part that begins with a note has the transposition
its first `<attributes>` give (40: it played as written), and the steps
of a transposition with an `<octave-change>` are its `<diatonic>`'s and
the octave, where upstream took the `<chromatic>`'s semitones (a bass
clarinet said a tenth for a ninth; MuseNote spells sounding notes from
`trans.diat`): declared, by that attribute alone, and for none of the
corpus, whose seven parts with an octave change transpose by whole
octaves, where the two readings agree. Cases:
`test_tb14_X_engine.cpp`.

**41 to 43. The order of play of a MusicXML score.** All three are in
`MusicXmlInput::CreateExpansion`, and each changes the order of play of
scores of the corpus by design; only their `<expansion plist>`, timemap
and MIDI change, no id moves, no pitch changes, and the page is the
same.

- 41: an ending with a blank number (MusicXML 4.0: there is an ending,
  the writer could not tell which) is the pass after the last, and one
  ending drawn as two brackets with one number is played whole. Both
  were never played: 15 scores, 99 notes. 19 scores change, three of
  them with no note lost before (a first ending's second bracket is
  heard on the first pass too, in two; a "1st time only" bracket on
  pass 1 as well, in one).
- 42: an ending numbered like one before it, after that one's repeat
  (the file contradicts itself; in three of the six the bracket's text
  says the right number), is the next pass, where it was never played:
  7 more scores, 46 notes.
- 43: a `<sound dalsegno>` looks only among segnos and a `tocoda` only
  among codas, as MusicXML pairs them, and a segno and a coda in one
  measure both count: 2 more scores (Ma Que Nada, Oblivion), whose
  D.S. named a coda's label and jumped forward over 194 notes.

With the three, 145 notes in 12 scores are still not played, each the
file's: a coda reached with no To Coda (4 scores), and bars no reading
of the file reaches or that follow its Fine (8). Jumps written only as
words ("D.S. al Coda", 67 spellings in 113 scores) are not followed:
words are presentation in MusicXML, `<sound>` is playback. A coda's
repeat after a jump back is not taken, as MusicXML's `after-jump`
default and upstream say (measured, not built: 4 scores would play 15
more notes, and 6 that lose none would play a coda's repeat twice).
Cases: `test_tb14_order_engine.cpp` (eleven orders, each pinned).

**44. The engine's fractions.** `vrv::Fraction` multiplied its int terms
before it reduced, and a time or a duration whose terms passed an int
wrapped: a bar's first tuplet of 32,768 notes put a time before one it
follows, 1,000 bars of partial tuplets of 251 gave times below zero and
a MIDI of 268,946,750 ticks. Each term is taken in 64 bits and reduced
before it is stored; a result that still does not fit is the fraction
nearest to it toward zero that does, so nothing wraps and the order of
exact results is kept. A note's dots and its own ratio are taken in
fractions too. Declared: the timemap of Wave, whose 488 entries are in
time order now (four were out of it); an element of `@num="0"` in a
tuplet (no valid MEI) lasts as its tuplet says, where the 0 took the
tuplet's ratio away too; a time past what the terms hold is the largest
they hold, 2,147,483,647 quarter notes in the timemap, where a note that
starts and ends past it has no length. Cases: `test_tb14_F_engine.cpp`.

**45 to 47. Found while the export and the readers were measured.** The
Humdrum export of a chant read from GABC (all three of the parity set's)
and of a mensural MEI with its clef in the layer ended the process; a
score of neume notation, which the converter cannot read, is refused
(45). An MEI `<staffDef>` with no `@lines` kept the unset value, and
layout and drawing overflowed with it in six files (46). The timemap of
an MEI file with a hidden ossia staff read a null staff definition (47).
Cases: `test_tb14_N_engine.cpp`.

**48 and 49. Found in the integration of tb.14.** `ScoreDef::Reset` left
the flag `m_insertScoreDef` unset, and the first measure of a chant or a
mensural score read it (48; the sanitizer run of the chant cases of
`test_tb14_N_engine.cpp` holds it). The MusicXML importer kept a tuplet's
numbers in a `short int`: 3 in the time of 40,000 was timed as 3 in the
time of 1 (49; a case in `test_tb14_X_engine.cpp`).

Known, and not changed (each the same on tb.13; for a later tag):

- humlib's own fraction, `HumNum` (`src/hum/humlib.cpp`, about 70
  operators), multiplies its int terms before it reduces, as `Fraction`
  did before patch 44: the Humdrum export of an ABC `(3:999999999:3abc`
  or of an MEI tuplet of `numbase="999999999"`, read back, does not come
  back (the engine's own times of both are exact);
- the MIDI writer makes a tick an int from a double
  (`src/midifunctor.cpp`, the note on and off of `GenerateMIDIFunctor`):
  a time past 17,895,697 quarter notes does not fit (UBSan:
  float-cast-overflow);
- the mensural duration functions multiply ints before they make a
  fraction of them, and the mensural-to-CMN conversion takes a remainder
  in an int: mending them means carrying a `Fraction` through those
  functions, a patch of its own;
- a MusicXML transposition given after the first `<attributes>` of a part
  is read nowhere (patch 40 reads the first);
- the rules of patches 41 and 42 play a pass that no number of a file
  names where the file contradicts itself: Home Among The Gum Trees
  (endings 1, 2, 3 and then a second 1, with no repeat after the 2 or the
  3) plays bars 2 to 16 a fourth time to reach the last bracket, and The
  Corner Grocery Store (endings "1, 2, 4" and "3", then a blank "Fine"
  bracket that repeats) a fifth verse; tb.13 played neither bracket. Two
  shapes no score of the corpus has go the same way: an ending numbered
  like one before it with other endings between, and a group whose first
  ending opens a repeat of its own right after another group (tb.13 left
  out that group, tb.14 plays the group before it again);
- older faults of the same importer that tb.14 leaves as they were: an
  ending `1-8` is read as 1 and -8 (Chan Chan plays two passes, not
  eight), and a first ending whose repeat sign stands outside its
  bracket is played on the second pass too (Chicken Dance, I Can't Make
  You Love Me);
- what the native cases can see: patch 48 only under UBSan (a chant or
  mensural case run with it), patch 37 only as a case that does not come
  back, and patch 31's walk only as a time bound (1.5 s) that a loaded
  machine can miss with the patch in.

**The review of tb.14.** A second engineer, who had not written the
patches, reviewed the twenty as first put together: their code, the
claims of each commit against runs on tb.13 and tb.14 in Release and
under the sanitizers, the 28 scores whose order changes bar by bar, and
inputs of its own. It found no crash, hang or sanitizer report that
tb.14 brought, and the 28 lose no bar tb.13 played (their pages are the
same to the byte). What it found, and what became of it:

- *Nested expansions past the cost* (tb.13 too): sixteen expansions each
  naming the next twice, a file of 1.5 KB, never came back. Mended in
  patch 31, and held by a case.
- *An order that names a `<choice>`* was passed over by patch 30, where
  tb.13 played it: an edition's choice and what it chooses between stand
  where a reading does. Mended in patch 30, and held by a case.
- *Words counted as spaces* by patch 36: forty words two spaces apart
  were one verse, where tb.13 split them into forty. Mended, and held.
- *The steps of a transposition with an octave change* (upstream, older):
  patch 40 moved the line that writes them, and now wrote them for parts
  that begin with a note too. Mended in patch 40, and declared above.
- *Three claims of the commits* that runs contradicted, now said as the
  code does: an order that names a section with and without its `#` (31),
  an element of `@num="0"` in a tuplet and the largest time (44).
- *What the rules of patches 41 and 42 make of two files that contradict
  themselves*, declared above, and left as they are.

Each mend was broken on purpose and its case caught it; the gates below
were run again on the series with the mends.

**On tb.14.** The same toolchain; Release, and AddressSanitizer with
UBSan and the standard library's checks on
(`_LIBCPP_HARDENING_MODE_DEBUG`). Native tests (`verovio_engine_tests`,
220 cases and 6,685 assertions; 174 on tb.13): all pass, in one process
and each in a process of its own, and under the sanitizers with no
AddressSanitizer report. UBSan reports two lines, both in
`include/vrv/boundingbox.h` (102 and 103: `Stem::CompareToElementPosition`
adds an unset bounding box, in layout), from three cases of tb.13 of a
staff in two voices: the same on tb.13, in code no patch touches; left.
Deliberate breaks: each builder's, patch 49's in integration and the
four of the review's mends, caught,
except seven of the 27 of patch 44 that no case can hold, because no
timing path reaches them: a quotient's numerator and a remainder in an
int (only layout and the mensural conversion divide by a fraction or
take a remainder), an equality in an int (both sides wrap alike), a
negative denominator kept (only a negative `@num` divides by one), the
early exit at the bound (equivalent by design), the sign of a negative
result that does not fit (none is made), and the constructor without
`Fit` (its inputs are small, and every result after it is fitted).
Parity gate, tb.13 against tb.14: 20,320 outputs compared, 169 differ:
the 28 scores of patches 41 to 43 (their MEI by its `<expansion plist>`
and nothing else, their timemap, MIDI and pitch map, and their two
locator exports; the pitch maps gain the 1,338 notes now played and no
pitch changes), and the timemap of Wave (patch 44, in time order);
seeded ids on a fresh thread deterministic for 2785 of 2785. Measured
apart, one process for each of 3,171 inputs (the 2,785 scores and the
parity set's ABC, Humdrum and GABC): the Humdrum export, read back with
the default options, plays exactly as the score for 2,834 on both; the
export differs for the 28 scores alone (each reads back exactly on
both) and for the three chants, whose export ended the process on tb.13
and is refused on tb.14. MuseSync's 24 bundled songs: the same on both.
The audit of log formats (`tool/log_format_audit.py`): 299 engine
sources, 0 calls.

### 50. The signs of the sounds a MusicXML page would not show (tb.15)

Patch 18 gives a MusicXML note with no `<accidental>` the sound its
`<alter>` says, and draws nothing new: from tb.11 on, 1,336 notes in 160
of the 2,765 MusicXML scores of the corpus (and 482 in 30 of MuseNote's
scales, whose files have written their signs since) sounded otherwise
than their page reads. MuseNote mended the page in its own code: a pass
over the engine's MEI wrote the signs, and its reader loaded the score a
second time from that text (MuseNote's plan V3_2_0_09; on a phone 250 to
360 ms more per open, where the signs were the only reason for it).
MuseSync, which uses this engine too, had no such pass. The importer is
where a reader decides what a MusicXML file shows: it writes the signs
now, and MuseNote's pass is gone (MuseNote's V3_2_0_22).

**The rule** is MuseNote's (V3_2_0_09, R1 to R12, with its owner's
decisions of 2026-10-15 and 2026-10-20), ported from its Dart reading of
the page and its pass, which were its specification:

- Bar by bar; in a bar staff by staff; on a staff voice by voice, each
  from left to right; the notes of a chord in their order; a grace note
  where it stands.
- A line (a staff, a voice, a pitch name, an octave) reads as its staff's
  own key at the start of a bar (a key change in the score's definition
  is every staff's; one in a staff's definition, or in a layer, is that
  staff's), then as the last sign written on it.
- A note sounds as its sounding accidental says, else as its written
  sign, else natural. One whose line reads otherwise gets the sign of its
  sound, which then counts as written: the first note gets it, later ones
  are covered, one that goes back gets its own.
- A note that sounds as its own voice reads it gets its sign when another
  voice of its staff shows a sign of another sound on its line in the bar
  (signs a musician may not strictly need, but no note read wrong in any
  order of the voices).
- A note a drawn tie holds (from a note of its pitch name, octave and
  sound, not itself) reads as that note, unless a sign before it on its
  line in its bar says another sound; it gets no sign for another voice.
- Two sounds on one line of a chord (of its printed notes): each note
  without a sign gets one, unless a note of the chord on the line shows
  that sound.
- Left alone, and counted in a warning: a note that is not printed, that
  has not exactly one accidental object, or that sounds as no sign says
  (a quarter tone).

**What it writes**, on the `<accid>` the importer built for the note: the
written sign (`n`, `s`, `f`, `x` for a double sharp, `ff`), the sound left
as it is (a natural, which has none, gets `accid.ges="n"`), and the
`glyph.auth` and `glyph.name` the importer carried taken off (kept, the
glyph would be drawn in place of the sign, the wrong one once the score is
transposed). No object is built or removed. The walk runs at the end of
`MusicXmlInput::ReadMusicXml`, before the repeats are written out (a copy
of a bar carries its signs) and before the document is paged; a
transposition at load moves each sign with its note.

**Measured** (tool/parity, tb.14 against tb.15, and MuseNote's Dart pass
of tb.14's exports as the reference):

- The MEI export of every MusicXML score of the corpus, in all three
  forms (the editor's, the canonical, the canonical with an id on every
  note: 3 × 2,765), is byte for byte the text MuseNote's pass made of
  tb.14's (the export's date aside).
- 640 outputs differ, exactly those of the 160 scores: the page, the MEI
  and the two locator exports of each. Every other output of the parity
  set is the same, the MIDI, pitch map and timemap of all 2,785 scores
  among them; seeded ids are the same on a fresh thread (2,785 of 2,785).
- Each of the 1,336 signs is drawn, with its glyph, and none of them was
  before; the number of accidentals drawn grows by exactly the signs; the
  notes, accidentals, rests, chords and measures drawn keep their ids and
  their order.
- Against MuseNote's second load of the passed text the pages differ: a
  load from MEI draws otherwise than a load from MusicXML anyway (38 of 40
  scores with no sign), and the objects the engine makes while laying out
  (a system, a clef, a key or time signature at the start of a line, an
  expansion's end) take other generated ids. Nothing stored by the apps
  is keyed to those (V3_2_0_09, step 3).
- The import of the corpus's ten largest files takes 1.04 to 1.08 s
  against 1.01 to 1.07 s.
- `tool/log_format_audit.py`: 0 calls.

**Cases**: `test/native/test_tb15_signs_engine.cpp` in MuseNote's
`corelib/verovio` (15, a small MusicXML score each); 11 deliberate breaks
of the walk, each caught. Two cases of patch 18 that pinned "nothing more
is drawn than the file writes" now say the sign is drawn. MuseNote's own
cut bars of the songs stay in its monorepo; the corpus check holds them.

**Upstream status**: not offered.

### 51 to 53. A MusicXML score plays each note, rest and gap for the ticks its file gives (tb.16)

The engine times a note or a rest by its written value (`@dur`, `@dots`,
or their `.ges`), the ratio of its innermost tuplet and its own
`@num`/`@numbase`; it never reads `@dur.ppq`, the ticks of the file.
Wherever the MusicXML importer wrote a value other than the ticks, the
score played otherwise than its file: on tb.15, 177 elements in 31 of
the 2,765 MusicXML scores of the corpus, so notes came early or late and
bars stretched. MuseNote grades a take by the same timing (its plan
V3_2_0_25, findings W17 to W20). Four causes, all in the importer:

- **A tuplet kept notes after its end (51).** `ReadMusicXmlBeamsAndTuplets`
  nested a beam and a tuplet that start on one note by the first beam-1
  "end" of any voice, where the loop closes a beam only on an "end" with
  no "continue"; files that mark beam 1 "end" while beam 2 goes on put
  the beam inside the tuplet. Closing the tuplet then left that beam
  open, and every later note went into it, at the tuplet's ratio (Will
  You Still Love Me Tomorrow bar 71: four 32nds after a triplet of 16ths
  at 2/3, bar 72 a sixth of a beat early).
- **A gap of no binary length was written short (52).** `FillSpace` split
  a `<forward>` into spaces of power-of-two lengths in floating point and
  truncated each to whole ticks: a gap a third of a quarter long became a
  chain of spaces down to 1 and 0 ticks, and the voice ended short.
- **Notes timed by a ratio no bracket shows played their plain value
  (52).** A `<time-modification>` of 3 in 2 with no `<tuplet>` was
  written `dur="8"` and played a full eighth.
- **A rest or note whose type disagrees with its ticks played its type
  (53).** Whole rests that fill a bar of another length (each played four
  beats: Since I Don't Have You, in 6/8, gained a beat of silence at
  each), rests typed without their dot, and grace rests, which take no
  time in the file and played as eighths.

**The rule.** The import writes each note, rest and gap so that the
engine plays it for exactly the ticks the file gives, and so that the
page shows what plays; no object built today changes its place in the
order of construction, so no id moves.

- **51.** A beam and a tuplet that start on one note nest by where each
  really ends in the note's own voice (the beam's end by the loop's own
  rule, the tuplet's stop): both are built as before and attached in
  that order. A tuplet's stop comes after the note's own beam end, and
  closes what was opened inside it and goes on past it; a beam that does
  ends with the tuplet, and the next notes are beamed afresh, with a
  warning. The chord the note begins stays open: its other notes follow
  the stop, and its own last note closes it.
- **52.** A gap whose length times the ratio of its open tuplets is a sum
  of written values is written as those values, counted in whole ticks.
  A gap that is not, and a note, chord or rest timed by a
  `<time-modification>` that no open tuplet gives, go into a tuplet that
  shows neither bracket nor number (`bracket.visible="false"`,
  `num.visible="false"`): for a gap, r in the time of the largest power
  of two below r, r the odd part of its length's denominator, holding
  written values that add up exactly; for a note, its own ratio, and
  neighbours of one ratio share one. Inside an open tuplet the unseen
  one holds the whole ratio of the gap's time: the engine times a note
  by its innermost tuplet alone, as the importer's own nested tuplets
  take a note's whole time-modification. A walk after the import does
  it, before the signs of patch 50: the import's spaces are kept (their
  ids) and given their values, the surplus deleted, and what it adds
  takes ids after every object the import built.
- **53.** A rest that fills its whole bar (no slash, no tuplet, no
  time-modification, not in a chord, at the bar's start) becomes the
  bar's rest, `<mRest>`, where the importer's comment always said so; it
  is one object either way, and keeps its id. Otherwise a note, chord or
  rest whose ticks make a written value (long to 2048th, up to two dots)
  is written that value. A grace rest is built and then dropped (MEI has
  no rest that takes no time), with a warning. Notes in a tuplet, with a
  time-modification, or of a fingered tremolo are left to their ratio.

**Measured** (tool/parity, tb.15 against tb.16):

- 185 of 19,930 outputs differ, all of the 31 scores: the MEI in its
  three forms, the page, the timemap, and the MIDI of 30 of them (Let It
  Be's last rest is now three beats long, as its file says, and nothing
  sounds after it). Every other output is the same, the pitch maps of all
  2,785 scores among them; seeded ids are the same on a fresh thread
  (2,785 of 2,785).
- Every element's time against its ticks (each note, rest, chord and
  space of the 2,765 MusicXML exports, by the engine's own rule): 177
  differ in 31 scores on tb.15, none on tb.16.
- Layers longer than their bar: 181 bars on tb.15, 146 on tb.16, none
  new. Every element of those 146 bars carries its file's ticks and plays
  exactly them: the files overfill them.
- Ids: every note of tb.15's export keeps its id, in order, and every
  other id is kept, but 39 surplus spaces of inexact gaps and 3 grace
  rests; 33 rests become bar rests with their id. Nested tuplets: 11 in 4
  scores on tb.15 (the fault of patch 51), none on tb.16.
- By patch: 51 changes 7 scores (each tuplet holds only its bracket's
  notes), 52 changes 9 (50 spaces given their values, 39 gone, 31 unseen
  tuplets), 53 changes 16 (33 bar rests, 9 values written as they play,
  3 grace rests gone); one score is changed by 52 and 53.
- Load and MEI export of the corpus's ten largest files, best of three:
  0.100 to 0.199 s against 0.102 to 0.198 s.
- `tool/log_format_audit.py`: 0 calls.

**Found by the gate**, and mended before the tag (a2aed60): a tuplet
whose stop is on the first note of a chord closed that chord with it, and
its other notes were lost (Como la Flor 5, Va Pensiero 5, El dia que me
quieras 1, The Blue Side of Lonesome 1) or began a chord of their own (in
We Are the World every later id moved); a beam that ends with its tuplet
was closed by the tuplet, and the beam end then ended a beam span opened
elsewhere on the staff (All By Myself, bars 39 to 40); an unseen tuplet
inside an open one took only its own ratio (no score of the corpus has
one).

**Cases**: `test/native/test_tb16_timing_engine.cpp` in MuseNote's
`corelib/verovio` (16, a small MusicXML score each, timed by the
timemap's fractions; 10 give another result on tb.15, 6 must not change
and do not); 13 deliberate breaks of the patches, each caught.

**Upstream status**: not offered.

### 54. Every tuplet is timed and read as its file says, nested ones by MEI's rule (tb.17)

MEI's rule for a tuplet inside a tuplet is that the ratios multiply. The
engine timed a note by its innermost tuplet alone
(`LayerElement::GetAlignmentDuration`): an MEI file written to MEI's
rule, and a MusicXML file that writes each level's own numbers
(`<tuplet-actual>`), played a triplet inside a triplet 3 in 2 where it is
9 in 4 (bar 2 of the probe at beat 13/3). The MusicXML importer made up
for it where a file gave only `<time-modification>`: the inner tuplet
took the whole ratio, so the page showed "9" and the MEI meant another
length to a reader that follows MEI. MuseNote's plan V3_2_0_26 (W21,
the owner's choice of 2026-11-12: MEI's rule everywhere), with what the
survey of the tuplet paths found beside it (W22, W25).

- **Timing.** The ratios of every tuplet around an element multiply. One
  tuplet gives its ratio as before.
- **MusicXML levels.** A tuplet takes its own level's ratio: the numbers
  its notation writes, else the time-modification of the note it times
  over the levels open around it. Every `<tuplet type="start">` of a note
  opens a level, outer first (the one that stops later; an outer level
  with no numbers is timed by the first note it holds directly, and is 1
  in 1 with no number shown when it holds none); every stop closes its
  level by `number`, inner first, and a chord's later notes do not give a
  stop again. One start and one stop a note were read: two stops on a
  note left the outer level open over the notes after it (a half note at
  2/3, bar 2 at beat 10/3), and of two starts the outer bracket was lost.
  Patch 51's nesting of a beam and a tuplet takes the innermost level's
  own stop; patch 52's gaps and unbracketed notes follow the rule inside
  open levels (a time-modified note whose ratio is not the product of the
  open levels gets an unseen tuplet of what is left over).
- **ABC.** A tuplet takes the next notes ABC gives it, r of `(p:q:r` (p
  was used: `(3:2:2` took a third note), wherever their beams begin and
  end. Only a beamed group of exactly its notes stayed in a tuplet: a
  triplet of quarters was no tuplet at all, a tuplet whose notes were in
  two groups was deleted, with notes it already held, and one that ended
  inside a group took the whole group. A rest and a space count; what
  takes no time before its first note (a change of clef) stands before
  it; a bar line or the next tuplet closes one that still waits, with a
  warning. `(5`, `(7` and `(9` are in 3 in a compound meter (6/8, 9/8,
  12/8) and in 2 otherwise, as ABC 2.1 has it (9 was always in 3, 5 and 7
  always in 2).

Not changed: the Humdrum export (`Tool_mei2hum::parseTuplet` already
multiplies), the Humdrum and PAE readers (they never nest), the MIDI
tremolo's stroke count (no timing ratio).

**Measured** (tool/parity, tb.16 against tb.17):

- 13 of 19,930 outputs differ, all MEI exports of ABC files of the
  parity set: `tuplets.abc` and 12 of MuseNote's own ABC exports. tb.17
  builds each of the 180 tuplets those files write; tb.16 built 130, and
  dropped notes with some of the others (Star Wars: 93 notes, now 101).
  Their generated ids move, as ABC ids are made in the order of building.
- Every other output is byte-identical: all MusicXML, MEI, Humdrum, PAE
  and GABC scores, in every form. No score of the corpus has a nested
  tuplet or a note with two tuplet notations.
- Seeded ids are the same on a fresh thread (2,785 of 2,785). MuseSync's
  24 starter songs: 120 outputs, none differs.

**Cases**: `test/native/test_tb17_tuplets_engine.cpp` in MuseNote's
`corelib/verovio` (19: nested MEI, nested MusicXML with and without level
numbers, two starts and two stops on a note, a chord that repeats a stop,
a gap and an unbracketed level inside nested ones, six ABC cases; 14 give
another result on tb.16, counting one subcase of tb.10's clef case that
pinned "the tuplet is dropped" and now says it holds its note; 5 must not
change and do not). 13 deliberate breaks, each caught.

**Upstream status**: not offered.

### 55. What a reader asks of the engine, in place of editing its file (tb.18)

MuseNote's reader loaded a score, took the engine's MEI, cut the part
names out of it (it draws them itself, over the page) and wrote a MIDI
channel for each staff into it, and loaded that text a second time:
1,528 of the corpus's 2,765 MusicXML scores were loaded twice for the
names alone, 250 to 360 ms more per open on a phone. MuseNote's plan
V3_2_0_27 (W11, W12): the reader asks the engine for both.

- **`staffLabelsHidden`** (off by default): `View::DrawLabels` draws
  nothing and keeps no width, for a staff, a staff group or a layer. The
  page is the one a score with no names gives; the MEI keeps the names.
- **`midiStaffChannels`** (empty by default): a list of "staff:channel".
  `Doc::ExportMIDI` plays a listed staff on that channel in place of its
  instrDef's (0 to 15; an entry that is no staff and channel is passed
  over). Set after a load, it is what the next MIDI is written with.
- **A text keeps its own spaces through MEI.** `MEIInput::LeftTrim` and
  `RightTrim` took all white space off the start of an element's first
  text and the end of its last. That is the file's layout only when the
  run holds a line break; a space on one line is text. A `<rend>` that
  began with a space after another run lost it when a score went out to
  MEI and came back ("Ø CODA" drawn "ØCODA"; 469 scores of the corpus
  changed text on a second load).

**Measured** (tool/parity, tb.17 against tb.18, both options off):

- 92 of 19,930 outputs differ, all MIDI files: the spaces of 3,673 lyric
  texts and one track name, and nothing else in them. The MIDI is written
  from the document read back from its own MEI, which now keeps the
  spaces the file has. Every page, MEI export, timemap and pitch map is
  byte-identical; seeded ids are the same on a fresh thread (2,785 of
  2,785).

**Cases**: `test/native/test_tb18_one_load_engine.cpp` in MuseNote's
`corelib/verovio` (3 cases, 9 subcases); 6 deliberate breaks, each caught.

**tb.19, the same day** (bc4c9fe): MuseNote's check of its reader on the
corpus found that `midiStaffChannels` set after a timemap or a MIDI had
been written did nothing: the MIDI is written from a document made at the
first of them, with the options of that moment. `Toolkit::RenderToMIDI`
and `RenderToMIDIFile` pass the channels on to it each time. One more
subcase (it fails on tb.18); no output of the parity set changes.

**Upstream status**: not offered.

### 155. A score laid out again draws as it drew when loaded: no barline keeps a box from a layout before (tb.67)

MuseNote plan V3_2_0_101 (root cause RC224, finding W181). Ours: upstream's develop has the same code (5109b20dd,
2026-11-25).

- **The cause.** `Measure::SetDrawingBarLines` makes a repeat's end and the next bar's repeat start one barline within
  a line (`rptboth` on the first, none on the second's left) and two at a line break (the end closes the line, the
  start opens the next). `View::DrawMeasure` draws a barline only where it has a rendition, and drawing resets only the
  box of what it draws (`BBoxDeviceContext::StartGraphic`). A re-layout (`Toolkit::RedoLayout`: MuseNote's reader
  switches between paged and continuous by `setOptions` and `redoLayout`) first lays the whole score out on one line
  for the line breaker: there the repeat start the layout before had drawn at a line start is not drawn, keeps that
  box, and `AdjustXPosFunctor` makes room for it (383 units at Africa's bar 37). The line breaker and the spacing
  estimate read the widened bars: the pages broke and spaced otherwise than at the load. A load's first layout is not
  affected (nothing was drawn before it); every later one was.
- **The fix.** `ResetHorizontalAlignmentFunctor::VisitMeasure` resets the box of a measure's left barline at the start
  of each horizontal layout: a barline the layout draws gets its box as before, one it does not has none. A right
  barline always has a rendition (`SetDrawingBarLines` never leaves it none), so drawing resets its box every layout.
- **What it changes.** The SVG of a score laid out again after a layout that drew a repeat start at a line start which
  the one-line layout joins to the repeat end before it: it is drawn as a load. A load's output: none. No MEI, MIDI,
  timemap, pitch map or id.
- **Left as it was.** A layout makes its systems, and the clefs, keys and meters its lines start with, anew, with new
  ids: a re-layout's SVG carries other ids for those than the load's, as before.

**Cases**: MuseNote's `test/native/test_tb67_relayout_engine.cpp`: a score of bars each repeated, every line opening on
a repeat start split from the repeat end before it (at each of the reader's four page widths); laid out again with
nothing changed; switched from paged to continuous, against a continuous load; and back to paged, its MEI the load's
too: every page drawn as the load's, ids aside. On tb.66 the 12 re-layout checks fail (the last line 3 to 6 units wider
at the three widest pages; at the narrowest, one page becomes two); undone, the patch is tb.66.

**Gate of tb.67** (patch 155, MuseNote plan V3_2_0_101, on tb.66). Native: the suite, 579 cases (80,487 assertions),
and under ASan, integer-divide-by-zero and signed-integer-overflow 578, no report; `tool/log_format_audit.py`: 301
engine sources, 0 calls. A Debug engine (signed overflow halting it) over every input: 3,612, all through, and through
the frozen readers the 3,392 MusicXML and ABC inputs, all through. Parity against tb.66: 22,961 outputs, none differs;
seeded ids 3,218 of 3,218; every input reads back from its MEI (3,612). MuseNote's reader switch over its 3,198 songs at
its four page widths: after paged, continuous and paged again, tb.66 drew 128 song-widths in 76 songs otherwise than
the load, and the continuous view after the switch 129 otherwise than a continuous load; tb.67 none and none; its
loads, paged and continuous, the same as tb.66's (12,792 and 12,792). The locator gate: 0 failures. MuseSync's 24
starters: the same. `bar_times.py`: as tb.66. MuseNote's export gate in every format, run on tb.66 and tb.67 with the
same export code: no song-aspect worse, its six refused exports tb.66's (neume notation), and every file it writes the
same as tb.66's, its ids numbered in order (the gate loads unseeded). The frozen readers' whole
MEI: the same as tb.66's for the 3,089 songs, the 80 of 3.0.1 and the 194 ABC files. MuseNote's lyric gate, every song
with words (2,676) at its 16 layouts and render paths: each load judged as on tb.66; the continuous view reached by a
re-layout judged as a continuous load in every song (tb.66: 6 songs otherwise, up to 3 lines more). MuseNote's measure
of every line of its corpus (the reader's biggest notes and the editor's biggest zoom): every trace as tb.66's.
V3_2_0_92's taps (213 songs, 12 layouts, 3 renders each, the third after the switch): none right on tb.66 wrong on
tb.67, no bar misframed. Render time, 100 songs in two layouts, the two engines alternated, three passes each on a
loaded machine: within the spread (tb.67's median 4.5 % below tb.66's, its sum 1.5 % below; one pass after the other
had read +10.9 %, the load's drift).

**Upstream status**: not offered.

### 154. A line too full for its words is laid out again: its notes tighter, then its words smaller (tb.66)

MuseNote plan V3_2_0_98 (root cause RC211, finding W187; what remains: MuseNote W192). Ours: upstream's develop squeezes
the same way (2026-11-24).

- **The cause.** `JustifyXFunctor` fits a system laid out wider than its page by one ratio below 1 over its justifiable
  width. The words are spaced to touch (`AdjustSylSpacingFunctor`, `lyricWordSpace`): squeezed, they are drawn over
  each other. In MuseNote's 2,676 songs with words, on tb.65: 4,442 places (on 1,553 lines of 368 songs) in its editor
  at its biggest zoom (900 wide), 305 (75 lines, 39 songs) in its reader at its biggest notes (1000); with
  justification off, none. Three quarters stand on a bar wider than its line alone; the rest on lines the breaker
  overfilled (151 to 153).
- **The fit** (`Page::FitHorizontally`, between `LayOutHorizontally` and `JustifyHorizontally`, where justification
  applies: breaks, no `noJustification`, no `adjustPageWidth`). A system whose justification ratio
  (`System::GetJustificationRatio`, as `JustifyXFunctor` computes it) is below 1 and below the lowest that keeps each
  word clear of the next and each hyphen its dash (`SafeSqueezeRatio`, by the distance between their notes, which the
  squeeze moves) is laid out again:
  - first with its notes tighter: `System::m_drawingSpacingFactor`, times the ratio, while that narrows the system by
    a hundredth at least (`EstimateJustificationRatio` returns its estimate times the factor, past its bound);
  - then with its words smaller: `System::m_drawingLyricScale`, times the ratio, while the squeeze would still draw a
    word over the next, down to two fifths (MuseNote's floor): smaller words open the gaps between them whether or not
    the system narrows. The syllables' text only (`Syl::GetDrawingLyricScale`, in `View::DrawSyl`; an elision's mark
    with it); word gaps, hyphens and their lines keep the option's size;
  - at most 8 passes over the page; a system with no words, or with its words at two fifths, is squeezed as before.
    `Page::LayOut` sets both back to 1 first.
- **What it changes.** The SVG of a line squeezed so that its words were drawn over each other: its notes, and on some
  its words smaller. A squeeze that keeps the words clear, and a line without words, are as before. No MEI, MIDI,
  timemap, pitch map or id.
- **Left as it was.** A line whose words cannot fit even at two fifths is squeezed, its words at two fifths
  (MuseNote W192).

**Cases**: `test/native/test_tb66_line_fit_engine.cpp`: a line too full for its words is laid out with its notes
tighter, the words whole; a squeeze that keeps the words apart is left as before (it holds on tb.65 too, as it must);
words that cannot fit their line whole are set smaller on that line only, to two fifths at most. The other two fail on
tb.65. Undone, each part fails its own: `FitHorizontally`, the notes stage, the words stage, the safe squeeze, the
floor, the smaller words drawn. One part has no case of its own: the squeeze measured by the notes' distance, not the
syllables' (a syllable pulled into the room before it, in a narrow window no synthetic input reached); MuseNote's
lyric gate guards it (All in Love is Fair).

**Gate of tb.66** (patches 151 to 154, MuseNote plan V3_2_0_98, on tb.65; the gate's fourth run, on 152's third
form). Native: the suite, 575 cases (80,321 assertions), and under ASan, integer-divide-by-zero and
signed-integer-overflow 574, no report; `tool/log_format_audit.py`: 301 engine sources, 0 calls. A Debug engine
(signed overflow halting it) over every input: 3,612, all through, and through the frozen readers the 3,392 MusicXML
and ABC inputs, all through. Parity against tb.65: 22,961 outputs, 3,226 differ, all SVG, as declared (lines break and
space otherwise); no MEI, MIDI, timemap, pitch map or id; seeded ids 3,218 of 3,218; every input reads back from its MEI
(3,612). The locator gate: 0 failures. MuseSync's 24 starters: the same. `bar_times.py`: as tb.65. MuseNote's export
gate in every format: no song-aspect worse, its six refused exports tb.65's (neume notation). The frozen readers' whole
MEI: the same as tb.65's for the 3,089 songs, the 80 of 3.0.1 and the 194 ABC files. MuseNote's lyric gate, every song
with words (2,676) at its 16 layouts and render paths, each syllable judged against its own size on tb.65: words over
each other on one bar alone, Cinnamon Girl's, its words at two fifths (MuseNote W192): 2 places at the reader's biggest
notes (tb.65: 305, in 39 songs), 3 in the editor's biggest zoom (tb.65: 4,442, in 368); none on a line whose words
are not at two fifths, and no word below two fifths; lines with smaller words: 175 at the reader's biggest notes, 1,545
in the editor's biggest zoom (75 at two fifths), 3 at its 1,400 and 1 in A4 print. Lines, each counted as it is drawn:
more than on tb.65, the more the narrower the page (the reader's biggest notes 54,925 to 62,901, +14.5 %; the editor's
biggest zoom +15.0 %; A4 print +5.5 %; the editor's 3,600 tile +1.7 %). MuseNote's measure of every line of its corpus
(3,198 songs at the reader's biggest notes and the editor's biggest zoom, 160,029 lines): every line's start counted
exactly as it is drawn but 170, those declared under 152 (a barline split at the break, 168 lines; an arpeggio on the
first chord, 2). V3_2_0_92's taps (213 songs, 12 layouts, 3 renders each): none right on tb.65 wrong on tb.66, no bar
misframed. Render time, 100 songs in two layouts: +9.2 % to +9.7 % (four runs). The gate's third run, on 152's second
form, had passed every gate, with 568 lines' starts a unit off; 11 SVGs differ between the two.

**One commit** (c8fe8fa): 151 to 154 are one commit, not one each as the patch queue's rule says (MuseNote's
`corelib/verovio/AGENTS.md`): each builds on the one before in the same functions, and they were never built apart.
Kept so by the owner's decision of 2026-11-25; the tag is not moved.

**Upstream status**: not offered.

### 153. The spacing estimate counts what the line breaker counted beyond the measures (tb.66)

MuseNote plan V3_2_0_98 (root cause RC215, finding W187). Ours: upstream's develop is the same (2026-11-24).

- **The cause.** `System::EstimateJustificationRatio` stretches a line's notes before it is laid out, by the room its
  measures' cast-off widths leave on the page. Those widths are stored before the line has a drawing scoreDef: its
  staff labels counted nothing, and a later line's clefs, key and meter nothing. A line that fitted at the break grew
  when it was laid out, and was squeezed (with 152's cause, a fifth of the overlaps of MuseNote's editor at its
  biggest zoom, in its trace on tb.64).
- **The fix.** `System::m_castOffStartWidth`: what the line breaker counted for the line beyond its measures (the first
  line's labels; a later line's start, 152; the cautionary change a line ends with, 152; a held line's overhang, 151).
  The estimate counts it with the margins.
- **What it changes.** The note spacing of a line the estimate stretched into room it did not have: any score's lines
  may space otherwise. The SVG only.

**Cases**: `test/native/test_tb66_line_fit_engine.cpp` in MuseNote's `corelib/verovio` (8 cases for patches 151 to
154): with the staff labels shown, a line whose bars fit is laid out no wider than its width. It fails on tb.65, and
with the stored width undone (as RC214's and RC216's do, which count through it).

**Upstream status**: not offered.

### 152. The line breaker counts the start a new line draws, and the change a line ends with (tb.66)

MuseNote plan V3_2_0_98 (root cause RC216, finding W187). Ours: upstream's develop is the same (2026-11-24), its
comment saying what was missing ("we would need two parameters").

- **The cause.** `CastOffSystemsFunctor` counted a line's start as the last scoreDef's own drawing width
  (`ScoreDef::GetDrawingWidth`, from `View::SetScoreDefDrawingWidth`: a G clef and 0.4 of a sharp for any key, no
  meter), and as 0 until it met one; after a change mid-score, that change's own width. A key of many accidentals, a
  meter at a line's start and a change's real state were not counted. Nor was what a line ending before a change of
  clef, key or meter ends with: that change, cautionary (`SetCautionaryScoreDefFunctor`).
- **The fix.** The scoreDef a later line starts with (`m_upcomingScoreDef`) is kept apart from the current line's
  start: the page's drawing scoreDef, each change applied as it is met (its draw flags, as the engine sets them, are
  the change the next measure draws), and each clef or key inside a measure as `ScoreDefSetCurrentFunctor` applies
  them, the flags cleared after each measure (`ApplyMeasure`). What each staff draws is taken as the engine draws it
  and `BBoxDeviceContext` bounds it (`CalcClefExtent`, `CalcKeyExtent`, `CalcMeterExtent`, `AddGlyphsExtent`): a clef
  by its glyph (a tablature's at its notation size); a key's accidentals stepped by their advance (as a device context
  measures it, rounded up: `CalcGlyphAdvance`) and a step, and its naturals, where `View::DrawKeySig` draws them, by
  their width and a step of their own; a meter's lines of figures, its summands joined by their sign, each centred on
  the advance of the line with the more glyphs, and its enclosing glyphs; each with its margins, and an element drawn
  empty (a key of none, a hidden clef) with none. And it is placed as `AdjustXPosFunctor::VisitMeasure` places a
  measure's scoreDef alignments (`CalcScoreDefEnd`): staff by staff, each element moving its alignment, and every one
  after it, as far as it must to clear what its staff drew before. So a key goes clear of the widest clef of the
  staves that draw a key, and a staff with a key of none draws its clef alone; a staff placed after another moves on
  alignments the first placed already, and every one after them: a meter changing on the lower staff alone moves the
  line's first bar on by the upper staff's key as well (about 550 units in the case below). What each staff draws is
  what the engine's draw flags say, kept as the engine keeps them: a key changing on any staff is drawn again on every
  staff (`ReplaceDrawingValuesInStaffDefFunctor`), empty where it is a key of none.
  - A new line's start (`CalcLineStartWidth`) is where its first measure's left barline goes after its clefs and keys
    and a meter changing there, less where it went within the line: a change right before that measure was laid out
    there, its naturals with it. The room a line's first measure keeps clear of a system start line, on a system of
    several staves or with a group symbol (`AdjustXPosFunctor`, a barline's width). And the abbreviated labels.
  - A measure fits its line if it fits with the changes between it and the next measure, which a line ending there
    ends with, cautionary (`CalcCautionaryWidth`, `CalcChangeWidth`: placed so from the measure's right barline; an
    ending's end or another milestone may stand among them).
  - A line starting with a held measure (151) takes its start and the line before it its end from that measure.
  - The first line's start stays in its first measure, whose width holds it.
- **For the counting** (found by MuseNote's measure of every line of its corpus, below):
  - `Clef::GetClefGlyph` and `MeterSig::GetSymbolGlyph` take the resources to look the glyph up in (`clef.h/.cpp`,
    `metersig.h/.cpp`; the calls before hand theirs on, unchanged). The clef and meter counted are a staffDef's
    current ones in the breaker's copy of a scoreDef, in no document: `Object::GetDocResources` finds none for them
    (and warns), the clef's glyph came back 0, and the meter's lookup by glyph name or number would have used the
    missing resources.
  - `View::DrawLabels` stores the abbreviated labels' width, which the breaker counts for every line but the first,
    before it draws the full label or finds it empty (`view_page.cpp`): a part whose name is hidden, its abbreviation
    drawn on the next lines, kept none. Only the breaker reads that width.
- **What it changes.** Where lines break: in a key of any accidentals, after a change, before one, in a system of
  several staves, with a hidden name's abbreviation. Any score's lines may break otherwise; in a key of several
  accidentals they hold fewer bars, which tb.65 set past their width and squeezed. The SVG and the page count only.
- **Left as it was.**
  - An encoded system break taken in `smart` mode starts a line whose start is not counted, as before (neither MuseNote
    nor MuseSync breaks in that mode).
  - A barline the engine draws otherwise at a break than within a line (`Measure::SetDrawingBarLines`: a repeat end
    and the repeat start after it, or a double barline on either side, are one within a line and split at a break) is
    counted as the one-line layout has it: the line starting there is 104 to 135 units short (168 lines of MuseNote's
    corpus: 74 at its reader's biggest notes, 94 in its editor at the biggest zoom).
  - An arpeggio on a line's first chord is spaced against the line's start (`AdjustArpegFunctor`, which moves the
    first measure's left barline with it), in the one-line layout against the bar before: the line is 29 units short
    (Ashoken Farewell's bar 18, at both).
  - A held line's overhang is the one-line layout's; the final layout may widen the line by more.
  - Not counted, as on tb.65: a clef's enclosure (`@enclose`), a mensur, a meter of several (`meterSigGrp`), a key's
    own accidentals (`keyAccid`: counted as its type's), and a clef or meter in a font of its own (`@fontname`).

**Cases**: in the same file: a key of seven flats, a change of key and of meter mid-score: no line is laid out wider
than its width; in bars of quarters, at every page width from 700 to 1,600 in tens, a line of two bars or more starting
right after a key change holds the bars the next line holds, which starts alike, and one ending before a key change
that cancels the old key (MusicXML `<cancel>`) is laid out no wider than its width. Each fails on tb.65. And the guard
the plan named (`test/native/test_tb66_line_counts_engine.cpp`): through a subclass of the toolkit, on every line of
nine scores at page widths 700 to 1,600 (the two-staff ones in tens), what the breaker counted
(`System::m_castOffStartWidth`) against what the line draws beyond its measures (its labels, its first measure's left
barline moved, its last measure's end grown past its right barline, from the score laid out on one line): seven flats;
changes of key and meter, a key cancelled and one of none; a hidden name's abbreviation; and two staves, each score a
placement a wrong rule misses: the wider clef on one and the wider key on the other; a bass in C beside a clarinet in
seven sharps; a meter changing on the lower staff alone, at a line's start; and at a line's end, the key changing on
the upper staff and the clef on the lower, the meter on the upper and the key on the lower, and the meter (16/16) on
the upper and the clef on the lower with no key changing, those of a change checking that a line starts at its bar.
Two of the line's ends are MEI: MusicXML puts a clef change inside its bar, and a staff's meter changing with its key.
It reads a member 153 adds and does not build on tb.65; it failed on tb.66 as first built (the gate's second run), and
on 152's second form, which took each alignment as its widest staff's (the bass in C: every later line 34 units
over). Undone, each part fails its own: the line's start, the cautionary change, the change at a line's start taken
off, the naturals, the start line's room, the clef's resources, the abbreviation's width, a glyph's right by its box
(not its advance), the advances rounded up; and the placement: each alignment by its widest staff (the bass in C, 34
units a line), each staff's own start (the two of keys, 36), alignment by alignment with no shift carried (the meter
on the lower staff, 550), and the right barline's end before a cautionary change (the meter and the clef, 13). A held
line's own start has no case of its own: MuseNote's measure of every line of its corpus guards it (the gate of tb.66,
under 154).

**Upstream status**: not offered.

### 151. A measure held for what overhangs it takes a line alone when it does not fit with the next (tb.66)

MuseNote plan V3_2_0_98 (root cause RC214, finding W187). Ours: upstream's develop is the same (2026-11-24).

- **The cause.** `CastOffSystemsFunctor::VisitMeasure` holds a measure whose content overhangs its barline by more than
  5 units (a direction, a dynamic, a tempo), so that no break comes between it and the next. At the break the held
  measures and the next went to the new line together, unchecked: two measures that do not fit one line were set on
  one, and squeezed (a quarter of the overlaps of MuseNote's reader at its biggest notes, in its trace on tb.64).
- **The fix.** At a break the held measures and the next must fit the new line together, with what that line ends
  with (152). If they do not, the held measures take that line alone, and the next begins the line after. A line that
  ends with a held measure counts what overhangs it, which the layout makes room for (`AdjustXOverflowFunctor`), and a
  change right after it, cautionary (152), so the estimate leaves room for them (153).
- **What it changes.** Where lines break after a held measure. The SVG and the page count only.

**Cases**: in the same file: a measure held for a direction's words that overhang its barline, which does not fit with
the next, takes a line alone, and no line is laid out wider than its width. It fails on tb.65, with the held measures
alone undone, and with the held line's overhang undone.

**Upstream status**: not offered.

### 150. Each part's key and meter change is its own (tb.65)

MuseNote plan V3_2_0_97 (root cause RC127, finding W120). Ours (upstream #3877: "no plans").

- **The cause.** `MusicXmlInput::ReadMusicXmlAttributes` read a key or meter change after the first bar from the first
  part only, into a score-wide `scoreDef` every staff took: another part's own change was dropped (only its meter's
  unit kept), and one numbered for one staff of a part (`<key number="2">`) was every staff's. A part's staff showed
  the first part's key, as a B-flat instrument after a key change, and its notes were spelt by it (a D sharp in four
  sharps written with a sharp, the key drawn being two). Their sound was right (patch 18 reads each note's `<alter>`).
  The first bar was right already: each part's key is its own `staffDef`'s.
- **Each part's change is recorded** for the staves it is for, as it is read (`RecordStaffChanges`: a key or meter
  with a number its staff's alone; the first of a bar, as before), and **written once every part is read**
  (`ResolveStaffChanges`, before the clef changes are placed). A change every staff has, alike, stays score-wide, as the
  first part's was read: no object is made, so every id is as before. Any other goes to a `staffDef` of each staff it
  is for, in the bar's `scoreDef` (the engine draws a staff's own: `ScoreDef::ReplaceDrawingValues(const StaffDef *)`;
  the reader's pass for the notes' signs reads it there): the first part's object is moved to the first of its staves it
  was read for (a copy to the others), so a staff that does not change keeps its key or meter; a later part's is made
  then (`ConvertKey`, `ReadMusicXMLMeterSig`, the reader's state they set restored).
- **What it changes.** A score whose parts change key or meter differently, or alone, or for one staff: its MEI, SVG and
  the notes' written signs; new objects, so new ids, in such a score only. No song of the corpus has one (345 of 3,198
  change key or meter after the first bar; the one with several parts changes alike), so none changes.
- **Left as it was.** A part's first-bar `<staff-details print-object="no">` (MuseNote W116). MuseNote's notation model
  holds one meter for every staff, so a part's own meter is taken there for every part (MuseNote W189, RC213).

**Cases**: `test/native/test_tb65_key_meter_per_part_engine.cpp` in MuseNote's `corelib/verovio`, 7 cases: a later
part's key change alone is its staff's (the MEI, the key drawn, its D sharp unwritten); the first part's alone; two
parts changing differently (a B-flat instrument); parts changing alike stay score-wide; a key numbered for one staff of a
two-staff part; a later part's meter alone; a single part's key and meter score-wide. On tb.64 the five of a staff's own
fail; the alike and single-part cases hold, as they must. Each part undone fails its own: nothing recorded, or nothing
written (the five of a staff's own fail); every change taken as not alike (the alike and single-part cases); a key's
number not read (the numbered one); the first part's object left score-wide (the three where its change is not every
staff's).

**Gate of tb.65** (patches 148 to 150, MuseNote plans V3_2_0_95 to V3_2_0_97, one gate, on tb.64). Native: the suite,
566 cases (19,144 assertions), and under ASan, integer-divide-by-zero, signed-integer-overflow and libc++'s checks 565,
no report; `tool/log_format_audit.py`: 301 engine sources, 0 calls. A Debug engine (asserts on, signed overflow halting
it) over every input: 3,612, all through, and through the frozen readers the 3,392 MusicXML and ABC inputs, all through,
no report. Parity against tb.64, drawn with tb.65's engine data: 22,961 outputs, 1 differs, as declared: Honky-Tonk
Woman's SVG (its 8 triangle heads, their stems and boxes, two beams with them); seeded ids 3,218 of 3,218; every input
reads back from its MEI (3,612). The locator gate: 0 failures. MuseSync's 24 starters: the same. `bar_times.py`: as
tb.64. MuseNote's export gate in every format: no song-aspect worse. The frozen readers' whole MEI: the same as tb.64's
for the 3,089 songs, the 80 of 3.0.1 and the 194 ABC files. The gate's first run drew with the apps' data, which has no
triangle until tb.65 is vendored, and showed no difference; its overflow check found `View::DrawText`'s, fixed before
the tag (149); the steps that read no drawing stand from it.

**Upstream status**: not offered.

### 149. No arithmetic on unset values, and nested tuplets' ratio as a fraction (tb.65)

MuseNote plan V3_2_0_96 (root causes RC209 and RC210, findings W154 and W188). Ours.

- **Unset values (RC209).** The engine did arithmetic on an unset place or box edge before testing for unset, a signed
  overflow the language leaves undefined (the release build wrapped it, and the wrapped value happened to change
  nothing drawn). Each is tested first now:
  - `Rest::GetOptimalLayerLocation` added the rest's offset to the other layers' place, unset when none of their
    elements beside it gives one (their rest, a space): that place is no candidate, as the current layer's unset place
    already was none;
  - `BoundingBox::HorizontalLeftOverlap` and its three siblings read the edges of an element with no box of its own
    (a whole-bar rest of a pickup, when the layers are adjusted: `Stem::CompareToElementPosition`): no box, no overlap,
    as `VerticalSelfOverlap` has it;
  - `Slur::AddSpannedElements` read a chord's edges, which has no box of its own (its notes, stem and dots are spanned
    with theirs): such an element lies under no part of the slur; a tuplet bracket and a cross staff still read;
  - `View::DrawText` read the right of a text that drew nothing (an empty syllable: the frozen 6.2.0 reader's, of an
    empty `<text>` after an elision): the drawn right stays where the last text drawn put it. Found by the gate's new
    check.
- **The ratio of nested tuplets (RC210).** `LayerElement::GetAlignmentDuration` multiplied the levels' ratios (our patch
  of the levels) in an int: a MusicXML tuplet of 70000 in 40000 over a time-modification of 3 in 40000, the reader
  nesting a hidden 3 in 70000 inside it, passed it, and its notes were timed backwards. The product is a `Fraction`,
  reduced as it is taken and fitted to an int (`Fraction::Fit`); the duration takes only the ratio, so every ratio that
  fitted times the same.
- **The gate's class guard.** The native suite's sanitizer build and the Debug engine over every input check signed
  overflow too, failing on any report.
- **What it changes.** Nothing drawn or played that fitted before: the overflows' wrapped values were never chosen. The
  tuplet's timing in a file of numbers past an int.

**Cases**: `test/native/test_tb65_unset_values_engine.cpp`, 5 cases: two layers resting together; a stem beside a
pickup's whole-bar rest; a slur over a chord; an empty syllable's text (A Big Hunk O' Love, read through the frozen
reader as the app imports a score); a note in tuplets whose ratios multiply past an int (timed by the product; a product past an int even reduced keeps its notes in
order). On tb.64 each halts at its overflow (built with
the check), and the tuplet's timing fails in any build. Each test undone halts its own case under the check: the
rest's, the overlap readers', the slur's, the text's (the song), the tuplets' product.

**Upstream status**: not offered.

### 148. Triangle noteheads are drawn as triangles (tb.65)

MuseNote plan V3_2_0_95 (root cause RC157, finding W141; with it MuseNote's own: the app's staged data, RC207/W185,
and each app's data, RC208/W186). Ours: upstream keeps the triangles unsupported (develop, 2026-11-22), Leipzig having
none.

- **The cause.** `Note::GetNoteheadGlyph` had its triangle cases commented out, and the fonts' data held no triangle
  (`fonts/supported.xml` left them out): a note with `head.shape="isotriangle"` (MusicXML `<notehead>triangle`) was a
  plain head.
- **The cases.** An up triangle by its duration and fill, as the diamond and the rectangle are: breve and longer E0BA,
  whole E0BB, half E0BC (solid E0BE), quarter and shorter E0BE (void E0BD); a right triangle (`rtriangle`) E0C8 white,
  E0C9 black, by the diamond's rule. Drawn where the font has the glyph (`Resources::GetGlyph`); a font without it
  gives the plain head.
- **The glyphs, optional.** `fonts/supported.xml` takes the up triangles (E0BA–E0BE) and the right ones (E0C8, E0C9),
  marked `optional="yes"`: the engine requires its default fonts (Bravura, Leipzig) to hold every glyph it supports, and
  Leipzig has no triangle ("Expected 657 default SMuFL glyphs but could load only 650", then "Leipzig font could not be
  loaded", at every start). `fonts/generate.py` counts `SMUFL_COUNT` over the required glyphs (650) and writes
  `IsOptionalSmufl`; `Resources::LoadFont`'s check of a default font, by that count, is unchanged. The data: Bravura and Petaluma
  all seven, Gootville the up ones, Leland E0BD and E0BE, with their anchors (the stems at the base's corners).
- **What it changes.** A triangle note's head and its stem: Honky-Tonk Woman's 8 in the corpus: its SVG alone, in parity against tb.64 (the gate of tb.65, under
  150).

**Cases**: `test/native/test_tb65_triangle_noteheads_engine.cpp`, 6 cases: each duration and fill of both triangles
in Bravura; a MusicXML triangle read and drawn; a triangle's stem at its base, where the glyph's anchors put it; with
Leipzig, the plain head; a plain note and a diamond as before; the default fonts load with no error. On tb.64 the three
of the triangles fail; the others hold, as they must. Each part undone fails its own case: the up triangles (their
three), the right ones, the plain head where the font has no triangle (Leipzig), the glyphs' anchors (the stem), and
`SMUFL_COUNT` over every supported glyph, 657 (Leipzig incomplete: the default fonts' case).

**Upstream status**: not offered.

### 147. The lyrics of two voices on one line, and lyrics placed above (tb.64)

MuseNote plan V3_2_0_94 (root causes RC205 and RC206, findings W183 and W184). Ours, on the idea of upstream's
e5f1b2456 (#4493, 6.4.0), whose code stands on 6.3.0's lyric elements (#4405, not taken: 22 hunks on 6.2.0), with its
reader line.

- **Two voices, one line (RC205).** The engine draws every voice's verse of one number on one line (`View::GetSylYRel`
  goes by the verse number), but `Page::AdjustSylSpacingByVerse` spaced each voice's words alone (patch 85's
  `RecordSylSpacingFunctor`, by staff, layer and verse), so one voice's word ran into the other voice's next. A line of
  verses is now a staff's side and verse number, every voice of the staff together: `RecordSylSpacingFunctor` records
  and replays it, and `AdjustSylSpacingFunctor` spaces a measure's verses at its end, in the order of their alignments
  (`AdjustVerse`, the body `VisitVerse` had). Two verses of different voices at one alignment are not spaced against
  each other: the one reaching furthest right is the previous, and the others move with it (spaced, they asked the
  measure for room from their alignment to itself, which widened it and left them as they were). Each voice keeps its
  own label abbreviation, shown on its first verse of a system.
- **Placed above (RC206).** The MusicXML reader left a lyric's `placement` unread (6.2.0's line was commented out); it
  is read into `verse@place` (upstream's line), which the engine already draws above the staff. The frozen 6.2.0
  reader is not touched.
- Left out of #4493: one line shared by a staff's lyrics below and the next staff's above (`ShareLyricLines`). No song
  of MuseNote's has lyrics on two staves of a part, and such lyrics keep their own room, as before.
- **What it changes.** A staff with one verse number in two voices, and a verse marked above: in MuseNote's songs the
  SVG of 16 (15 with two voices' lyrics, and Paperback Writer's verse 5 above the staff) and Paperback Writer's MEI
  (`verse@place`). No MIDI, timemap, pitch map or id. A staff with one voice's lyrics is spaced as before, in the same
  order.

**Cases**: `test/native/test_tb64_lyric_lines_engine.cpp` in MuseNote's `corelib/verovio`, 5 cases: two voices' words
word after word drawn apart, and in the order sung whichever voice the page draws first; two voices' words at one time
keep their note's place and the measure its width; a lyric placed above is read so and drawn above the staff, one with
no placement below, as before; each voice's label abbreviation on its first verse of a system. On tb.63 3 of the 5
fail; the cases of one time and of the labels hold, as they must. Each part undone fails its own case: the line keyed by
voice again, no order at the measure's end, no rule for one alignment, no reader line, the labels shared.

**Gate of tb.64** (patch 147, MuseNote plan V3_2_0_94, on tb.63). Native: the suite, 548 cases (18,832 assertions), and
under ASan, integer-divide-by-zero and libc++'s checks 547, no finding; `tool/log_format_audit.py`: 301 engine sources,
0 calls. A Debug engine (asserts on) over every input: 3,612, all through, and through the frozen readers the 3,392
MusicXML and ABC inputs, all through. Parity against tb.63: 22,961 outputs, 19 differ, as declared: the 16 songs' SVG
and Paperback Writer's MEI (read, seeded, read back); seeded ids 3,218 of 3,218; every input reads back from its MEI
(3,612). The locator gate: 0 failures. MuseSync's 24 starters: the same. `bar_times.py`: no song breaks a rule.
MuseNote's export gate in every format: MusicXML and ABC as before; the Humdrum export cannot hold the new
`verse@place` (Paperback Writer), Humdrum keeping no side for a lyric (MuseNote RC212, accepted by its owner
2026-11-23). The
frozen readers' whole MEI: the same as tb.63's for the 3,089 songs, the 80 of 3.0.1 and the 194 ABC files. On
MuseNote's 12 layouts and 3 ways of drawing a page, over its 2,676 songs with words: no syllable of one voice over
another's but in the editor's narrowest tile, where 4 places of 13 remain, an older fault of the narrowest sizes
(MuseNote W187, RC211). MuseNote's taps (its V3_2_0_92 probe, 116 songs): none right on tb.63 is wrong on tb.64.

**Upstream status**: not offered.

### 146. Every copy the order of play makes has its own id (tb.63)

MuseNote plan V3_2_0_93 (root cause RC204, finding W85). Ours. Where an MEI order names a part again from inside an
order being played, `ExpansionMap::Expand` makes a new container for that pass, a section, ending, `lem` or `rdg` as
the part's own is. It named it by `GeneratePredictableIDs`, `<id>-rend<N>` with N one past the copies of `<id>` the
expansion map records, and never recorded it there, as it records each copy of a part
(`AddExpandedIDToExpansionMap`). Every new container of a part so took the same name: an order four levels deep made
30 sections `R-rend2`, ten levels deep 2,046; and none led back to the container as written
(`GetNotatedIdForElement`, `GetExpansionIdsForElement`).

- **Each new container is recorded** as a copy of the container it repeats, as each copy of a part is: the next one is
  named after it (`R-rend2`, `R-rend3` … in the order they are made), and the map leads it back to the one written.
- **What it changes.** Only a document whose order makes such a container: its containers' ids, and what the map
  answers for them. What is played is the same. No input of the corpus makes one (3,609 loaded with their order
  written out), so no output of it changes.

**Cases**: `test/native/test_tb63_container_ids_engine.cpp` in MuseNote's `corelib/verovio`, 4 cases: orders 1, 4 and
10 levels deep hold each id once, their containers named `R-rend2` onwards as made, each leading back to `R`, and play
as before (2 to the power of the levels, a whole note each); an ending named again three times, `E`, `E-rend2`,
`E-rend3`; a plain repeat named as before; the document for playback (without `expandAlways`) the same. On tb.62 (the
recording undone) 3 of the 4 fail; the plain repeat passes, as it must.

**Gate of tb.63** (patch 146, MuseNote plan V3_2_0_93, on tb.62). Native: the suite, 543 cases (18,788 assertions),
and under ASan, integer-divide-by-zero and libc++'s checks 542, no finding, but one timing case over its bound by the
Mac's load (MuseNote W90, RC100: the same work on tb.62, 0.9 to 1.5 s alone); `tool/log_format_audit.py`: 301 engine
sources, 0 calls. A Debug engine (asserts on) over every input: 3,612, all through, and through the frozen readers the
3,392 MusicXML and ABC inputs, all through. Every input loaded with its order written out (3,609): no container made,
no id held twice, the 4,111 copies of sections and endings in 2,052 inputs named the same as on tb.62. Parity against
tb.62: 22,961 outputs, none differs; seeded ids 3,218 of 3,218; every input reads back from its MEI (3,612). The
locator gate: 0 failures. MuseSync's 24 starters: the same. `bar_times.py`: no song breaks a rule. MuseNote's export
gate in every format: exit 0, no song-aspect worse. The frozen readers' whole MEI: the same as tb.62's for the 3,089
songs, the 80 of 3.0.1 and the 194 ABC files.

**Upstream status**: not offered. Upstream's develop (fb66a3829, 2026-11-20) is the same, and has no issue for it.

### 145. A tempo change plays from where it stands in its bar (tb.62)

MuseNote plan V3_2_0_91 (root cause RC189, finding W167). Ours. The engine gave a bar one tempo, the last one met in it,
from the bar's start. `InitMaxMeasureDurationFunctor::VisitMeasureEnd` timed the whole bar by it, and
`InitOnsetOffsetFunctor` and `InitTimemapAdjustNotesFunctor` timed every note, rest and grace of the bar by it.
`GenerateMIDIFunctor::VisitMeasure` wrote it at the bar's first tick, the timemap stamped it on every note of the bar,
and `Measure::EnclosesTime` measured the bar by it. A change at beat 3 so sounded from beat 1, and one at the barline
retimed the whole bar it ends.

- **A bar keeps its tempos** (`Measure`): the one in effect at its start and each change in it, by its score time in
  the bar, adjusted (`midiTempoAdjustment`). Of two at one place the one given last is kept. It answers the tempo at a
  time and the seconds to it, piece by piece.
- **The duration** functor starts a bar's list with the tempo in effect, and adds each `<tempo>` at its start's
  alignment (the layout is done before the timemap; one with no place, at the bar's start). It times the bar piece by
  piece: a multi-bar rest's factor times the whole. The tempo carried on is still the last one met.
- **The notes, rests and graces** are timed by the bar's real time at their score times; an unaccented grace's length
  is set by the tempo where it stands.
- **The MIDI** writes each of the bar's tempos at its own tick. The document no longer writes the score's tempo at tick
  0 itself: written first, it kept the first bar's own tempo there out of the MIDI, which then played the score's
  while the timemap played the bar's. The first bar writes tick 0's.
- **The timemap** stamps a note with the tempo at its onset, and each change in a bar gets its own entry where no note
  may start; a mark that gives the tempo already playing gets none.
- **What it changes.** The MIDI and timemap of 6 songs, from the bar of the change on: All By Myself, Defying Gravity
  (bar 71), The Meeting of the Waters (a 72 the bar's last tempo hid is heard), September Song (60 from quarter 58,
  not 57) and I Remember It Well ×2 (120 for the first 1.5 quarters, then 80). No MEI changes, so no id moves.

**Cases**: `test/native/test_tb62_tempo_within_the_bar_engine.cpp` in MuseNote's `corelib/verovio`, 11 cases: a
change at beat 3 plays from beat 3, in the timemap, the MIDI's tempo and the timemap's tempo; a change at the barline
leaves its bar at the tempo before it; two changes in one bar; of two at one place, the last; a note sounding across a
change; a grace note before a change; the tempo adjustment on every piece; the first bar's own tempo at tick 0; the
element at a time found in the bar that plays then; a mark that gives the tempo already playing adds nothing; and, as
before, a change at a bar's start, a bar with none, a multi-bar rest. With all of the patch undone, 9 of the 11 fail
(the two that pass: what plays as before, and the mark that changes nothing, which tb.61 gives no entry either); each
of its 11 parts undone fails its own case. tb.58's case of a bare
`<sound tempo>` placed by its offset now expects its tempo at its place (quarter 6), not at its bar's start.

**Gate of tb.62** (patch 145, MuseNote plan V3_2_0_91, on tb.61). Native: the suite, 539 cases (18,725 assertions),
and under ASan, integer-divide-by-zero and libc++'s checks 538, no finding; `tool/log_format_audit.py`: 301 engine
sources, 0 calls. A Debug engine (asserts on) over every input: 3,612, all through, and through the frozen readers the
3,392 MusicXML and ABC inputs, all through. Parity against tb.61: 22,961 outputs, 12 differ, the MIDI and timemap of the
6 songs declared; seeded ids 3,218 of 3,218; every input reads back from its MEI (3,612). The locator gate: 0 failures.
MuseSync's 24 starters: the same. `bar_times.py`: no song breaks a rule. MuseNote's export gate in every format: exit 0;
7 song-aspects worse in those 6 songs, each the export's own fault that the tempo now played where it stands shows:
the Humdrum export writes a bar's tempos at its start (MuseNote W176, RC198), and the MusicXML export moves a mark
between its divisions (W177, RC199); the ABC export plays 4 of them better. The frozen readers' whole MEI: the same as
tb.61's for the 3,089 songs, the 80 of 3.0.1 and the 194 ABC files.

**Upstream status**: not offered. Upstream's develop is the same, and has no issue for it.

### 144. The SVG's boxes of text, as upstream writes them now (tb.61)

MuseNote plan V3_2_0_90 (root cause RC197, finding W174). Upstream, cherry-picked: 90fc4d036 (#4565, develop, not yet
released), on ours: the syllable ink bounds of a09acec80 (#4405, not taken), which it reads. With `svgBoundingBoxes` on,
`SvgDeviceContext::DrawSvgBoundingBox` wrote a text's box group inside its `<tspan>`, where SVG allows no `<g>`; gave a
lyric syllable an empty box (`View::DrawSyl` turns its vertical extent off, lyric lines being spaced by the staff
alignment); copied an element's `<title>` (`@label`) into its box group; and looked for a floating element's content
box on the element, not on its current positioner.

- **The boxes** (90fc4d036): `StartBoundingBoxGraphic` writes a text's box group after its `<text>`, in the element that
  holds it, and no `<title>` in a box group; `AddChild` passes over those groups, so what is drawn after a text keeps its
  order; a syllable's box and its text's take their height from the ink of the text; a floating element's content box
  (`svgContentBoundingBoxes`) is its current positioner's.
- **The ink bounds** (ours, from a09acec80): `Syl` keeps `m_drawingTextInkTop`, `m_drawingTextInkBottom` and
  `m_hasDrawingTextInkBounds`, cleared by `Syl::Reset`; `View::DrawSyl` stores them from the text's extent before it
  draws it.
- **What it changes.** The SVG of 3,217 of parity's inputs (3,197 of MuseNote's 3,198 bundled scores, 20 others) and
  nothing else: no MEI, MIDI, timemap, pitch map or id. After MuseNote's
  patcher the page drawn is the same but for what is not drawn (box groups holding a `<title>`, which the patcher left
  in 189 songs' pages until now, and emptied elements written self-closing). Every lyric syllable and its text get a box
  (409,597 each, in 2,661 songs). MuseNote frames a bar by the outline of all the boxes in it, so its taps on bars do
  change where that outline put a bar on the wrong line already (MuseNote W175, RC200, fixed in MuseNote plan
  V3_2_0_92; this entry first said they did not). The SVG grows 7.4 %. What it opens
  for MuseNote's later releases (a lyric followed while it plays, a word tapped) is in MuseNote plan V3_2_0_90.

**Cases**: `test/native/test_tb61_svg_text_boxes_engine.cpp` in MuseNote's `corelib/verovio` (4 cases): a text's box
group is written after its `<text>`, none inside a `<tspan>`, and a note's box is as before; a lyric syllable's box holds
the ink of its text, its baseline within; an element's `<title>` is in its own group, once, and in no box group; with
content boxes a direction has its content box, none without the option. The 4 fail on tb.60. Each part undone fails its
own: the group's place, the title, the syllable's box, the ink bounds stored, the content box's positioner. And V3_2_0_84's
cases read a syllable's drawn rects only (`connectorsOf`: its whole group, its box groups left out).

**Upstream status**: upstream's own (90fc4d036, develop). The ink bounds are a part of upstream's a09acec80; nothing is
offered.

**Gate of tb.61** (patch 144, MuseNote plan V3_2_0_90). Native: the suite, 528 cases (18,558 assertions), and under ASan,
integer-divide-by-zero and libc++'s checks 527 (the RC106 arena case is left out of that build), no finding;
`tool/log_format_audit.py`: 301 engine sources, 0 calls. A Debug engine (asserts on) over every input: 3,612, all
through, and through the frozen readers the 3,392 MusicXML and ABC inputs, all through. Parity against tb.60: 22,961
outputs, 3,217 differ, all SVG, as declared; seeded ids 3,218 of 3,218; every input reads back from its MEI (3,612).
MuseNote's patcher and box parser over the 3,217: the page drawn the same (3,015) or without only what is not drawn
(202), the taps' boxes the same but for 5 bars (W175; boxes compared, no tap run). The locator gate: 0 failures
(every generated id of 501,777 MusicXML notes kept). MuseSync's 24 starters: the same. `bar_times.py`: no song breaks
a rule, as tb.60. MuseNote's export gate
in every format: exit 0, no song-aspect worse. The frozen readers' whole MEI: the same as tb.60's for the 3,089 songs,
the 80 of 3.0.1 and the 194 ABC files.

### 143. A rest marked measure="yes" is a whole-bar rest only where its ticks fill its bar (tb.60)

MuseNote plan V3_2_0_89 (root cause RC196, finding W123). Ours. Patch 70 made a rest with no `<type>` a whole-bar rest
only where its ticks fill its bar, "as `measure="yes"`": a rest marked `measure="yes"` stayed one in any bar. In a bar
its notes do not fill (`metcon="false"`) the engine plays a whole-bar rest for no time
(`LayerElement::GetAlignmentDuration`), so a pickup bar of only such a rest was not played, and the music after it
started early (the probe's C at 0, not 1). Now `measure="yes"` makes a whole-bar rest where `TicksFillTheBar` holds;
elsewhere the rest is one of its ticks' value, as one with no type (`WriteItsTicks`, `ValueOfItsTicks`): the page
shows what plays. A full bar's `measure="yes"` rest is read as before.

**Cases**: `test/native/test_tb60_bar_and_tempo_engine.cpp` in MuseNote's `corelib/verovio`, 2 cases: a pickup of one
beat holding a `measure="yes"` rest (with no type, and with a whole's) plays from the second beat and reads as a
quarter rest; a full bar's stays an mRest. The first fails on tb.59; the rest judged by its ticks undone fails it.

**Upstream status**: not offered. Upstream knows the playback (#3146, open); #3207 (open) proposes an mRest timed by
`@dur.ges`; #4126, in this branch's base, gives a whole-bar element no time in a `metcon="false"` bar.

**Gate of tb.60** (patches 140 to 143, MuseNote plan V3_2_0_89). Native: the suite, 524 cases (18,530 assertions),
and under ASan with libc++'s checks and `-fsanitize=integer-divide-by-zero` 523 (the RC106 arena case is left out of
that build), no finding; `tool/log_format_audit.py`: 301 engine sources, 0 calls. A Debug engine (asserts on) over
every input: 3,612, all through, and through the frozen readers the 3,392 MusicXML and ABC inputs, all through.
Parity against tb.59: 22,961 outputs, none differs; seeded ids 3,218 of 3,218; every input reads back from its MEI
(3,612). The locator gate: 0 failures (every generated id of 501,777 MusicXML notes kept). MuseSync's 24 starters: the
same. `bar_times.py`: 2,650 live songs, none breaks a rule. MuseNote's export gate in every format: exit 0, no
song-aspect worse than tb.59's, the same 6 chant exports refused. The frozen readers' whole MEI: the same as tb.59's
for the 3,089 songs, the 80 of 3.0.1 and the 194 ABC files. Each part undone fails its own case (14 breaks), the frozen
reader's guards under the sanitizer.

### 142. The MusicXML reader measures a full bar by the meter's counts summed (tb.60)

MuseNote plan V3_2_0_89 (root cause RC193, findings W171 and W169). Ours, with the idea of upstream's a9ac27d5c (#4577,
develop, unreleased). `MusicXmlInput`'s full-bar test (`metcon`) worked out the bar's length itself. It multiplied an
additive meter's counts, so 3+2/8 made a bar of six eighths: a full one was marked not full, and its whole-bar rest
played for no time. And it divided by the meter's unit, which a `<time>` with `<beats>` and no `<beat-type>` leaves at
0: undefined behaviour, which ends the process on x86 and gives 0 on ARM, so every bar was marked not full. The clef
placed at the end of the bar before a clef change divided by the same unit (on ARM it stood at the bar's start).

- **The full-bar test** asks `TicksFillTheBar`, the exact rule two other tests of the reader use: the counts summed, a
  `Fraction`, a bar of no known meter never marked.
- **The clef** divides by the unit only where it is a number (`std::max(1, m_meterUnit)`): any value past the bar's
  end does there, as its comment says.
- **The frozen 6.2.0 reader** (`iomusxml620.cpp`, by patch 129's rule): its two divisions give 0 where the unit is 0,
  which is what 6.2.0's gave on ARM, so for a file 6.2.0 read it builds what it built, and it ends the process no
  more. Its multiplied counts are 6.2.0's reading, and stay. Its header names the guards marked "MuseNote patch 142".
- Of #4577 only the sum is ported. Its pick conflicts with this branch's bar time (RC116, patch 97) in two hunks, and
  its removal of the temporary `MeterSig` moves seeded ids, so it is not taken.

**Cases**: 4: a 3+2/8 bar holding a whole-bar rest is full (the next note at 2.5 quarters), and one of five eighths is
not marked; a `<time>` with no `<beat-type>` marks no bar and keeps the clef at the bar's end; the frozen reader reads
that file as 6.2.0 did, with no division by 0 under `-fsanitize=integer-divide-by-zero`; and a scan: `iomusxml.cpp`
divides by `m_meterUnit` nowhere. All but the frozen reader's fail on tb.59; that one fails under the sanitizer with
its guards undone ("division by zero", `iomusxml620.cpp`).

**Upstream status**: the sum is upstream's (a9ac27d5c, #4577, unreleased); the division by 0 is not offered.

### 141. A mark placed past its bar's end makes no time (tb.60)

MuseNote plan V3_2_0_89 (root cause RC5, finding W38). Ours. The aligner gave a mark with a time stamp (words, a
dynamic, a tempo, a chord symbol) past its bar's end an alignment past the right barline, and moved the bar's end out to
it (`MeasureAligner::GetAlignmentAtTime` → `SetMaxTime`). The MIDI and the timemap take the bar's length from there
(`InitMaxMeasureDurationFunctor::VisitMeasureEnd`), so the bar played its notes and then waited. Patch 69 (RC55) keeps
the MusicXML reader's marks inside their bars; files of MEI, Humdrum and ABC, and MuseNote's editor's, still met it.
A time stamp at or past the bar's end now takes the right barline's alignment, as one exactly at the end did: the bar
keeps its length, and the mark is drawn at the barline. Time stamps are aligned after every layer of the measure
(`AlignHorizontallyFunctor::VisitMeasureEnd`), so its end is known.

**Cases**: 2: words six quarters into a 4/4 bar (the next bar at 4 quarters, 2,000 ms, in the timemap and the MIDI);
a dynamic past what a bar not full holds (the bar keeps its two quarters). Both fail on tb.59, and both with the
barline undone.

**Upstream status**: not offered. Upstream's comment there calls it "a tstamp event after the last note".

### 140. One rule for the tempo an element gives (tb.60)

MuseNote plan V3_2_0_89 (root causes RC184 and RC191, findings W162 and W121). Ours. Five places worked out the tempo
a score definition or a `<tempo>` gives, each its own way (`Doc::CalculateTimemap`, `Doc::ExportMIDI`,
`InitMaxMeasureDurationFunctor::VisitScoreDef` and `VisitTempo`, and the MusicXML reader's tempo places of RC182), and
none read `@midi.mspb`. And `Tempo::CalcTempo` took a dot off the unit's denominator, so ♩. = 60 played 80 quarters a
minute, not 90 (upstream's 325a8f220, #3215, 2023, called ×4/3 a dot correct).

- `Tempo::PlaysAt`: `@midi.bpm`, else `@midi.mspb` (60,000,000 ÷ it), else `@mm` in its unit and dots; one of 0 or
  less gives none. The five places call it.
- `CalcTempo`: a dot makes the unit half as long again, and each more dot half the one before it (× 2 − 2^−dots).

**Cases**: 7: ♩. = 60 plays 90 and ♩.. = 60 105, ♩ = 60 and a half at 60 as before, a score definition's too, in the
timemap and the MIDI; `@midi.mspb` plays, the score's and a `<tempo>`'s; `@midi.bpm` before `@midi.mspb` before
`@mm`; a negative `@midi.bpm` and 0 microseconds give none (a `@midi.bpm` of 0 is none by its attribute class
already); the MusicXML reader judges a bare `<sound tempo>` by a dotted mark's tempo; and a scan: no source but
`tempo.cpp` calls `CalcTempo` or reads `@midi.bpm`, `@midi.mspb` or `@mm`. Each fails on tb.59 but the one of 0 or
less, which was written after; each part undone fails its own (the dots, `@midi.mspb`, each guard, each of the six
calls reading for itself).

**Upstream status**: not offered.

### 139. A MusicXML elision is drawn with the symbol its file gives (tb.59)

MuseNote plan V3_2_0_88 (root cause RC190, finding W168). Upstream, cherry-picked: d751fb2ed and 9473d055a (#4446,
6.4.0), and ours on them. MusicXML joins two syllables sung on one note with an `<elision>`, whose content names the
symbol drawn between them: an undertie (‿), a no-break space, a hyphen, an underscore; left empty, the undertie, or the
glyph its `@smufl` names. `MusicXmlInput` gave every elision `@con="b"`, the undertie, whatever the file said: a
no-break space was drawn as an undertie.

- **The elision's symbol** (d751fb2ed, 9473d055a): `ConvertElisionToCon` reads the `<elision>`'s text into `syl@con`:
  a no-break space `s`, `-` and `‐` `d`, `_` `u`, `~` and `˜` `t`, `^` and `ˆ` `c`, `ˇ` `v`, `⁀` and `⁔` `i`, `‿` `b`;
  empty, the `@smufl` glyph's (`lyricsHyphenBaseline` and its non-breaking form `d`, the `lyricsElision` glyphs `b`),
  else `b`; any other text `b`, with a warning. The second commit takes back the first's whitespace parsing, so an
  ASCII space reads as empty (`b`). Each conflicts once, in `ReadMusicXmlHarmony`, on a line the second takes back
  (this branch tests `!rootStep` where upstream tests the harmony's text): kept ours. The net change is upstream's
  final `ConvertElisionToCon`, its declaration and its call.
- **The Humdrum export** (ours, `Tool_mei2hum::parseVerse`): it joined the syllables of one note's verse with a space,
  which the Humdrum reader makes an elision drawn with an undertie. A syllable joined to the next by a space
  (`@con="s"`) is written with `&#160;`, which the reader keeps as one syllable holding a no-break space, drawn the
  same; any other join is written as before.
- **What it changes.** 46 elisions in 11 of MuseNote's songs hold a no-break space: they are drawn with a space, the
  undertie gone, and the syllables spaced so (Carinhoso fits 8 systems for 9, and the ids of what its layout draws,
  none in the MEI, move with the system it loses). 16 more, in 7 songs and Paperback Writer's third, follow a verse
  number ("1."), which the reader makes the verse's label: nothing to join, as before. No object is added: no
  generated id moves, and the MIDI, timemap and pitch map do not change. MuseSync's songs have no elision. The
  Humdrum export of the 11 reads each such join back as one syllable. The frozen 6.2.0 reader (`iomusxml620.cpp`,
  patch 129) is not touched.

**Cases**: `test/native/test_tb59_elisions_engine.cpp` in MuseNote's `corelib/verovio` (8 cases): a no-break-space
elision is `s`, with no undertie drawn; a `@smufl` glyph `lyricsHyphenBaseline` gives `d`; the elisions of two verses
on one note keep their own symbols; a syllable joined by a space comes back from the Humdrum export as one syllable,
with no undertie; and, as before, an empty elision and U+203F are `b`, the undertie drawn; an ASCII space is `b`;
MuseNote's export of a verse number (`<text>2.</text><elision/>`) is a label and one syllable; an undertie elision's
Humdrum round trip is `b`, the undertie drawn. The 4 of RC190 fail on tb.58. Each part undone fails its own: the
symbol not read, the 4 of RC190; the `@smufl` glyph not read, its case; the Humdrum join by a space, its case.

**Upstream status**: upstream's own (d751fb2ed and 9473d055a, in 6.4.0). The Humdrum export's join is this branch's,
not offered.

**Gate of tb.59** (patch 139, MuseNote plan V3_2_0_88). Native: the suite, 510 cases (17,002 assertions), and under
ASan with libc++'s checks 509 (the RC106 arena case is left out of that build), no finding; `tool/log_format_audit.py`:
301 engine sources, 0 calls. A Debug engine (asserts on) over every input: 3,612, all through, and through the frozen
readers the 3,392 MusicXML and ABC inputs, all through. Parity against tb.58: 22,961 outputs, 44 differ, as declared:
the 11 songs' MEI (the score's and the locator's two, each change one `@con` made `s` from `b`, 46 in all) and their
drawings; seeded ids 3,218 of 3,218; every input reads back from its MEI (3,612). The locator gate: 0 failures (every
generated id of 501,777 MusicXML notes kept). MuseSync's 24 starters: the same. MuseNote's export gate in every
format: exit 0, each format's figures the same as tb.58's; one census differs, as declared: Humdrum's syllables, in 10
songs more, each `s` join read back as one syllable. The frozen readers' whole MEI: the same as tb.58's for the 3,089
songs, the 80 of 3.0.1 and the 194 ABC files.

### 138. A tempo a MusicXML file changes with a bare `<sound>` plays (tb.58)

MuseNote plan V3_2_0_86 (root cause RC182, finding W160). Upstream, cherry-picked: bd1515b1a (#4443, 6.4.0) and
5c7736156 (#4503, 6.4.0), and ours on them. MusicXML gives a tempo with `<sound tempo>`: in a `<direction>`, with its
words or metronome mark, or straight under the `<measure>`, a bare sound with no mark on the page. `MusicXmlInput`
read a bare one only in part 1, bar 1, as the score's start tempo (`scoreDef@midi.bpm`); `ReadMusicXmlSound` read no
tempo, so a later bare tempo change was lost and the music played on at the tempo before it.

- **The tempo of a bare `<sound>`** (bd1515b1a, 5c7736156): a `<tempo>` with its `@midi.bpm` and `@tstamp` (the bar's
  time and the sound's `<offset>`), as a direction's is made; a `<sound>` inside a `<direction>` is left to
  `ReadMusicXmlDirection`, which makes its `<tempo>`. Applied as text, their hunk lands in `ReadMultiRestMarks`
  (patch 94 stands where upstream's `ReadMusicXmlSound` ends) and keys a control element by its bar's count where this
  branch keeps its `Measure` (`m_controlElements`): it does not build. Ported into `ReadMusicXmlSound`.
- **Only where it changes the tempo** (ours): the tempos the import makes are kept by bar and `@tstamp`
  (`m_tempoPlaces`: the start tempo before the first bar; each direction's, by its `@midi.bpm` or else
  `Tempo::CalcTempo`; each bare one built), and a bare `<sound tempo>` builds its `<tempo>` only where the tempo in
  effect at its place (`TempoAt`, else `MIDI_TEMPO`) is another. 350 of MuseNote's songs and 1 of MuseSync's hold
  bare `<sound tempo>`s that give their start tempo again, in each part (All Blues); upstream's commits as they are
  would build an object in each and move every id after it, for no change of tempo.
- **Read as a direction's are** (ours): the tempo as a double (with `as_float` a start tempo of 79.9998 was another
  number than the scoreDef's, and built a `<tempo>`), and the offset as an `int` (a `short` held none past 32,767
  divisions: four quarters at 10,080 a quarter).
- **What it changes.** No output of MuseNote's songs or MuseSync's: none of their bare `<sound tempo>`s changes the
  tempo. A user's file that changes its tempo with a bare `<sound>` plays it, from the start of its bar (the engine
  plays a bar at the last tempo met in it, as it does a direction's: MuseNote W167/RC189, open), and its ids after each new tempo move once. The
  frozen 6.2.0 reader (`iomusxml620.cpp`, patch 129) is not touched.

**Cases**: `test/native/test_tb58_sound_tempo_engine.cpp` in MuseNote's `corelib/verovio` (6 cases): a bare tempo
change at bar 3 played from bar 3 (its `<tempo>`, with no text; the MIDI's tempo events; the timemap's bar 4 at 8 s); a
bare tempo at its `<offset>`, and at one past 32,767 divisions; a change written in the second part alone; and, as
before, part 1 bar 1's start tempo, also with a fraction (no `<tempo>`); a bare tempo that gives the tempo in effect,
in each part and after a metronome mark (the MEI the same as without it, ids and all); a direction's `<sound tempo>`
standing at its offset (one `<tempo>`, #4503). The 3 of RC182 fail on tb.57. Each part undone fails its own: no tempo
built, the 3 of RC182; the rule, all 6; a direction's tempo not recorded, the given-again case; #4503's check, the
direction case; `as_float`, the start-tempo case; a `short` offset, the offset case.

**Upstream status**: upstream's own (bd1515b1a and 5c7736156, in 6.4.0). The rule, and the tempo and offset read as a
direction's are, are this branch's, not offered.

**Gate of tb.58** (patch 138, MuseNote plan V3_2_0_86). Native: the suite, 502 cases (16,940 assertions), and under
ASan with libc++'s checks 501 (the RC106 arena case is left out of that build), no finding; `tool/log_format_audit.py`:
301 engine sources, 0 calls. A Debug engine (asserts on) over every input and 4 scores of tempo changes: 3,616, all
through, and through the frozen readers the 3,392 MusicXML and ABC inputs, all through. Parity against tb.57: 22,961
outputs, 0 differ; seeded ids 3,218 of 3,218; every input reads back from its MEI (3,612). The locator gate: 0
failures (every generated id of 501,777 MusicXML notes kept). MuseSync's 24 starters: the same. MuseNote's export gate
in every format: exit 0, no song-aspect worse than tb.57; over the 4 scores of tempo changes, the MusicXML and Humdrum
exports play the same in every aspect. The frozen readers' whole MEI: the same as tb.56's for the 3,089 songs, the 80
of 3.0.1 and the 194 ABC files.

### 137. The Humdrum export writes each staff's program as its instrument code (tb.57)

MuseNote plan V3_2_0_85 (root causes RC176 and RC177; finding W79, its instruments). The MEI-to-Humdrum converter
(`Tool_mei2hum`) wrote a staff's name and abbreviation (`*I"`, `*I'`) but no instrument code, and dropped the program
of the staff's `instrDef@midi.instrnum`. The Humdrum reader turns a code into the staff's General MIDI program
(`HumdrumInput::addInstrumentDefinition`, `HumInstrument::getGM`), and humlib had that direction only: a score's
Humdrum export, read back, played none of the programs the score names.

- **A program's code** (`HumInstrument::getHumdrumForGM`, the reverse of `getGM`). Where the table gives a program
  several codes, the one named after the General MIDI instrument itself (a table of 30: `piano` for 0, `vox` for 52,
  `tromt` for 57, …), else the table's first; none for a program the table lacks (60 of the 128).
  `getGM(getHumdrumForGM(p))` is `p` for the 68 that have one.
- **Each staff's program** (`Tool_mei2hum::parseStaffDef`, `m_staffPrograms`): the `instrDef@midi.instrnum` of the
  first staffDef of its number, else of its staffGrp. Cleared at each conversion.
- **Written** (`writeInstrumentCodes`, on the file as written, the way patch 131 writes its tempo): one line after the
  first `*staff` line, `*I` and the code in each `**kern` or `**mens` spine whose staff has a program with a code, `*`
  in the others. A staff on program 0 that the file gives no name (no `*I"` before the first data; a `*I""` names a
  group) gets none: MIDI plays program 0 where none is said, and the reader names a staff that has no name after its
  code (`getLabelFromInstrumentCode`), a name the score does not show. A staff with no name on another program is
  named so: Humdrum gives no program without a code (MuseNote's owner, 2026-11-20).
- **RC177, an entry at index 0** (`HumInstrument::getGM`, `getName`, `getHumdrum`, each by name and of an instrument
  of its own, and `setGM`): `find` gives the table's first entry 0 and −1 for none, and the six took 0 for none. The
  table's first code, `accor` (the accordion, 21), had no program, name or code, and `setGM` added it a second time.
- **What it changes.** A Humdrum export only: one interpretation line more where a staff has a program with a code,
  so the ids the reader gives by line move after it, in a new export. Read back, such a staff plays its program, in
  an `instrDef` with the `@midi.instrname` the reader writes. No reader changes.

**Cases**: `test/native/test_tb57_humdrum_instruments_engine.cpp` in MuseNote's `corelib/verovio` (8 cases): a staff's
program written as its code and read back; two staves, each in its own spine; a staffGrp's program, the accordion's
(RC177); a staff's code in its own spine only, and the program it starts with; a program with no code, and a staff
with none, written with none; program 0 written only on a staff the file names (a name `*I""` makes a group's, a name
given later); every program's code read back as that program, the accordion by each way in; a converter used twice.
On tb.56, 6 of the 7 that build there fail (the seventh asserts that no code is written, which tb.56 does not); the
round trip needs `getHumdrumForGM`. The program 0 case also failed on the fix as first built, which wrote every code.
Each of the patch's 19 parts undone, under the address, division and overflow sanitizers, fails a case.

**Upstream status**: not offered.

**Gate of tb.57** (patch 137, MuseNote plan V3_2_0_85). Native: the suite, 496 cases (16,866 assertions), and under
ASan with libc++'s checks 495 (the RC106 arena case is left out of that build), no finding; `tool/log_format_audit.py`:
301 engine sources, 0 calls. A Debug engine (asserts on) over every input and every Humdrum export: 6,810, all
through. Parity against tb.56: 22,961 outputs, none differ; seeded ids 3,218 of 3,218; every input reads back from
its MEI (3,612). The locator gate: 0 failures. MuseSync's 24 starters: the same. MuseNote's export gate in every
format: MusicXML and ABC as on tb.56; Humdrum 2,964 songs play the same in every aspect, and 2,727 of the songs
listed for instrument play their programs now (172 left: 122 programs with no code, the format's; 49 songs on
channels Humdrum does not keep; one program out of range). Its census against tb.56: `instrDef` and
`@midi.instrnum` as the score's in 2,371 songs more; `@midi.instrname` new in 2,383, and a staff's name in 887
(3 of them clarinets in B♭, whose name draws its flat as a glyph), both accepted by MuseNote's owner.

### 136. A lyric line ends where the file says, and an empty syllable is no syllable of a word (tb.56)

MuseNote plan V3_2_0_84 (root cause RC172, finding W153). Upstream, cherry-picked: #4339 (92b03ade9, 8497ed330;
6.3.0), #4444 (7d781df87; 6.4.0) and 20c1936cc's verse half (6.4.0). MusicXML ends a lyric line (the line under a
held syllable) with `<extend type="stop"/>` on its last note. `MusicXmlInput::ReadMusicXmlNote` dropped a `<lyric>`
with no `<text>`, so the stop was lost and the line ran on to the note before the next syllable of its verse. MEI ends
a line with an empty syllable on the next note, and the lyric preparation took one for a word's next syllable: a
hyphen to it was not drawn (`View::DrawSylConnector` stops where the next syllable has no box), an empty syllable
could start a hyphen or a line, and the MIDI writer gave it a lyric event of no text.

- **A stop ends its line** (92b03ade9, 7d781df87). The reader holds a note's stops per staff and layer
  (`m_pendingExtenderStops`) and gives the layer's next note or chord an empty syllable in the stop's verse where it
  has no syllable of its own there; a chord's notes are one event. `include/vrv/iomusxml.h`'s one conflicting hunk
  keeps this branch's members and upstream's.
- **An empty syllable is no syllable of a word** (92b03ade9, 7d781df87), ported into
  `PrepareLyricsFunctor::FollowSyl`, where patch 85 (a77c758) moved the following of a syllable: applied as text, the
  rule does not build. A hyphen before an empty syllable ends on its note with no next syllable (`m_nextWordSyl`):
  it is drawn up to that note where there is room for a dash. An empty syllable starts no hyphen and no line. A line
  before it ends on the note before it, as before.
- **No MIDI lyric event for an empty syllable** (92b03ade9; `Syl::IsEmpty` in `GenerateMIDIFunctor::VisitSyl`,
  8497ed330).
- **No verse on a tab group** (20c1936cc's verse half, which patch 121 left for this). The reader as it is cannot reach
  it: a stop is taken by a note or a chord only, and a tab note's element is its tab group. Taken so the code reads as
  upstream's.
- **What it changes.** No MEI of MuseNote's songs or MuseSync's (none has a stop): no id moves. A user's file with
  stops reads the ids after each stop once more (each stop's empty syllable is a new object). 16 songs draw one hyphen
  more where its note leaves room for a dash (at the parity set's scale 15: Kiss Me (capo 3) has no room there); in
  those 16 the layout's own ids move, since each connector drawn builds a temporary `Syl`, which takes a generated id.
  The MIDI of 35 songs loses its 74 empty lyric events and nothing else (the writer passes a secondary tied note's
  syllables and every verse but a note's first, so most empty syllables never wrote one); in 2 of them a lyric changes
  places with the key and time signatures of its tick (the writer's `qsort` calls two meta events of one tick equal;
  tempos are in track 0, which keeps its order). The frozen 6.2.0 readers (`*620*`, patch 129) are not touched.

**Cases**: `test/native/test_tb56_extender_stops_engine.cpp` in MuseNote's `corelib/verovio` (7 cases): a stop ends
its line on its note, and the next note gets an empty syllable; a word's first syllable followed by an empty syllable
on its tied note keeps its hyphen; an empty syllable writes no MIDI lyric event; and, as before, a line with no stop,
a stop whose next note has a syllable of its own, hyphens with an elision and an empty syllable between words, a stop
before a note of a single TAB staff. The 3 of RC172 fail on tb.55. Each change undone fails its own: the stop held for
the next note, the stop case; an empty syllable no word's next syllable, the hyphen case; no MIDI event for an empty
syllable, the MIDI case. The tab guard undone fails none, and none can.

**Upstream status**: upstream's own (#4339 in 6.3.0; #4444 and 20c1936cc in 6.4.0). The port into `FollowSyl` is
this branch's, not offered.

**Gate of tb.56** (patch 136, MuseNote plan V3_2_0_84). Native: the suite, 488 cases (16,658 assertions), and under
ASan with libc++'s checks 485 and, on their own, the 2 that run skips (W154's), no finding; `tool/log_format_audit.py`:
301 engine sources, 0 calls. A Debug engine (asserts on) over every input: 3,612, all through, and through the frozen
readers the 3,392 MusicXML and ABC inputs, all through. Parity against tb.55: 22,961 outputs, 51 differ, each as above
(16 pages, 35 MIDI files); no MEI, timemap or pitch map; seeded ids 3,218 of 3,218; every input reads back from its
MEI (3,612). The locator gate: 0 failures (every generated id of 501,777 MusicXML notes kept). MuseSync's 24
starters: the same. MuseNote's export gate in every format: exit 0, no song-aspect worse than tb.55 (the 2 songs
added since, judged on both engines: none worse); its MusicXML export writes the stop on a line's last note, beside
the empty syllable, which it reads back as before. The frozen readers' whole MEI: the same as tb.54's (tb.55 leaves
them alone) for the 3,089 songs, the 80 of 3.0.1 and the 194 ABC files.

### 135. A bar of a whole-bar rest reads its interpretations as a bar of notes does (tb.55)

MuseNote plan V3_2_0_83 (root cause RC171, finding W79's tempo). `fillContentsOfLayer` reads a bar of a whole-bar rest
in a branch of its own (`hasFullMeasureRest`), which took from its tokens their directions and pedal marks only
(upstream: "Deal with time/clef/key changes in measures with whole-measure rests later"). A `*MM` there was lost (All
By Myself, bar 30: the return to 58, three times in its order), and so were a `*tscale`, an ottava's end (no
`<octave>` was made), a simile, a verse label, a join, the state of the staff and of its dynamics, and a color.

- **One function.** What an interpretation sets for the music after it is read by `handleInterpretationState`, which
  the note loop and that branch call (the note loop's code, moved). The branch has no note to pass: an ottava's end
  takes the notes it kept.
- **What stays.** A clef, key or meter change in such a bar was read already (at the bar's start, and the branch's
  clef compensation), once, and still is (a case holds each).

### 134. A section's starting tempo is played again where the section is entered again at another; the starting tempo is the tempo playing (tb.55)

MuseNote plan V3_2_0_83 (root cause RC170, finding W79's tempo). The first `*MM` went into the scoreDef only ("handled
by scoreDef"), which plays it once: a section that holds it, played again by the expansion list, kept the tempo the
play had reached (Meeting of the Waters, `*>[A,A]`: its second pass kept the rallentando's 132 for 22 beats).

- **Played again where it has to be.** A first `*MM` is also a `<tempo>` of its measure where a section label stands
  above it in its spine, the expansion list the score is played by names that section more than once, and the score
  changes its tempo after its first note (a `*MM` or a `*tscale`): `isStartTempoPlayedAgain`, with the list's labels
  kept in `m_labelsPlayed` beside RC137's `m_labelsInTurn`. Three songs of the catalog (Meeting of the Waters, William
  Tell and v2); elsewhere the scoreDef alone holds it, as before (made for every first `*MM` in a section, it gave 280
  songs and the 104 Humdrum samples a `<tempo>` their play does not need, and the export gate's census 2,900
  differences: the owner chose the narrower rule).
- **The tempo playing.** The starting tempo was not the tempo playing (`m_midibpm`, which a `*tscale` and a tempo text
  take): it stayed the reader's 120 whatever the file's first `*MM`, the tempo taken from a movement's name or the
  default 400 was (`*MM100`, then `*tscale:2`, played 240). It is each of those now. Mensural music reads no `*tscale`:
  its default is left as it was.

### 133. The Humdrum reader starts at the last *MM before the first note (tb.55)

MuseNote plan V3_2_0_83 (root cause RC169, finding W79's tempo). Of two `*MM` before the first note `addMidiTempo`
stopped at the first, while the music plays the last: a score that gives two tempos at its start is exported so (Cello
Suite No. 1, 79.9998 then 72, played its export at 79). The scan goes on to the first note, and the last one counts.

### 132. The Humdrum reader reads a tempo's number whole (tb.55)

MuseNote plan V3_2_0_83 (root cause RC168, finding W79's tempo). `addMidiTempo` read the first `*MM` with `stoi`
(79.9998 read as 79), and `handleTempoChange` rounded a later one to a whole number (`int(x + 0.5)`). Both read it with
its fraction (`^\*MM(\d+\.?\d*)`, `getMatchDouble`): Reality, This Masquerade and v2, Something's Got a Hold on Me.

### 131. The Humdrum export writes the 120 a score with no tempo plays at, where the reader would give another (tb.55)

MuseNote plan V3_2_0_83 (root cause RC167, finding W79's tempo). `Tool_mei2hum` wrote a `*MM` only for a tempo the
score gives, and a score that gives none plays at 120, the engine's tempo for an MEI file without one. The Humdrum
reader gives a file that names none a tempo of its own (`HumdrumInput::addDefaultTempo`, "a basic Renaissance
default": 400 a minute when its lines last more than a half note on average, 280 for mensural music): 16 songs of the
catalog played their Humdrum export at 400 (tb.54).

- **One rule.** The reader's rule is humlib's, `Convert::defaultMidiTempo`, which `addDefaultTempo` asks: the same
  tempos as before.
- **Written where it is needed.** `writeTempoReadOtherwise`, as the file goes out: the converter reads it again for its
  rhythm and, where the rule gives another tempo than 120 and the file names none before its first note, writes
  `*MM120` after the clef, key and meter, above the expansion list and the first label (the start's tempo, no
  section's), in each `**kern` or `**mens` spine as the grid writes a tempo, with the spines of the first line that
  has them (a global comment, such as the grid's note of an underfilled measure, has none). Where the rule gives 120
  the file says nothing, as before: 16 songs of the catalog are written so, and every mensural score.

**Cases**: `test/native/test_tb55_humdrum_tempo_engine.cpp` in MuseNote's `corelib/verovio` (16 cases); the pinned
exports of `test_mei_to_humdrum_engine.cpp` (4) and `test_tb14_N_engine.cpp` (2) hold the `*MM120` line where the
rule gives another tempo (the chord symbols' half notes, none). 12 of the 16 fail on tb.54; the 4 that pass hold what
must not change (no tempo where the reader plays 120; a score's own tempo alone; a section's tempo nothing changes, or
of a section played once, in the scoreDef alone; a clef, key or meter change in a whole-rest bar read once). Each part
of the five, undone once under the address, division and overflow sanitizers, is caught: 16 of 16. Two cases of tb.51
are left out of those runs: they stop at signed overflows tb.53 has already (MuseNote finding W154).

**Upstream status**: not offered (MuseNote's export round trip; 131's shared rule is a move of upstream's code).

**Gate of tb.55** (patches 131 to 135 together, MuseNote plan V3_2_0_83). Native: the suite, 481 cases (16,604
assertions), and under ASan with libc++'s checks 480, no finding. A Debug engine (asserts on) over every input and
every song's Humdrum export: 6,806, all through. Parity: 22,961 outputs, none of tb.54's inputs otherwise (16 new, of
the two songs list 16 added); seeded ids 3,218 of 3,218; every input reads back from its MEI (3,612). The locator gate:
0 failures. MuseSync's 24 starters: the same. MuseNote's export gate in every format: exit 0; the Humdrum export's 23
tempo differences gone; no song-aspect worse than tb.54, 6 better (the tempo marks of Meeting of the Waters, William
Tell and v2, 10,000 Reasons, I'm So Lonesome I Could Cry, Sweet Hour of Prayer); MusicXML and ABC as on tb.54.
Found on the way, not this tag's: W154 (signed overflows where an offset is added to an unset place, `rest.cpp` and
`boundingbox.h`, on tb.53 already) and W155 (an `<sb/>` before the first measure: the export's `*-` has one field).

### 130. A note's accidentals and accidental-marks in the file's order, kept together in a chord, the note sounding by the one next to it (tb.54)

MuseNote plan V3_2_0_81 (root cause RC164, finding W147), upstream #4454 (1d437b90c, 225f86568, 0a2716e22, taken
with `cherry-pick -x`; the `#include` list of `src/iomusxml.cpp` settled by keeping ours and adding `<iterator>`).
`MusicXmlInput::ReadMusicXmlNote` added a note's `<accidental>`s, and its `notations/accidental-mark`s, to the note in
the file's order, while the engine draws a note's accidentals outward from the note in MEI order. MusicXML writes
them left to right, so two accidentals or two marks of one note were drawn the other way round: a note written
flat-sharp showed sharp-flat. And `Accid::AdjustX` left an accidental where it found room on its right also beside
the other accidental of its own note, so in a chord a note's two accidentals could be drawn apart, with others
between them. And a note sounds by its first accidental (`Note::GetChromaticAlteration`), which was the one furthest
from it: an F sharp written natural-sharp played F. Probed on tb.53 (12 written inputs, 300 random chords): nothing
drawn on top of anything; every pair without a natural reversed; 79 of 531 pairs split in the chords.

- **Read from the last to the first.** A note's `<accidental>`s are added from the last one in the file to the first,
  its accidental-marks in reverse document order (`xpath_node_set::sort(true)`). The accidentals are still read
  before the marks, so one accidental with one mark keeps its order. The accidental next to the note is now its
  first, and so its sound, as the page shows.
- **No room taken beside the note's own accidental.** `Accid::AdjustX` keeps the shortcut "enough space on the right"
  for accidentals of other notes only.
- **What stays as it was.** A natural is still drawn next to the note, whatever the order in the file: the rule for
  unisons in `AccidSpaceSort`, which upstream kept (0a2716e22 took back the comparator change of 1d437b90c). One
  accidental per note, a chord's accidentals and an accidental with a mark are drawn, and sound, as before. After the
  patch 61 of the 531 pairs are split, one of them further than before (a six-note chord with three pairs), as upstream
  draws them.
- No song of MuseNote's catalog or of MuseSync's has such a note (MusicXML allows one `<accidental>` per note); no id,
  timemap, MIDI or pitch map of the songs moves. The frozen 6.2.0 reader (`iomusxml620.cpp`, patch 129) is not
  touched.

**Cases**: `test/native/test_tb54_upstream_features_one_engine.cpp` in MuseNote's `corelib/verovio` (8 cases): the
order of two accidentals, alone and in a chord; the sound of an F sharp written natural-sharp; two accidental-marks; a
pair kept together in a chord; and, as before, one accidental per note in a chord, an accidental with a mark and its
sound, a natural next to the note. The five of RC164 fail on tb.53. Each change undone fails its own: the accidentals
read in the file's order, 4 (the sound with them); the marks in document order, 1; the room taken beside a note's own
accidental, 1 (a build of the reading change alone).

**Upstream status**: upstream's own (#4454, merged to develop in 7f9e3651d, 2026-11-18; in no release yet).

**Not taken with it**: 02f360168 (#4573, the overlap check of floating elements without a dynamic cast), adapted to
6.2.0's `Is()`: the 20 largest songs of MuseNote's catalog, each loaded and every page drawn, 5 rounds with the
engines alternating, drew byte for byte the same and took 0.9 % less time in all, inside the spread of one song's runs
(5.6 %). MuseNote finding W148, closed: not taken.

**Gate of tb.54** (patch 130, MuseNote plan V3_2_0_81). Native: the suite, 464 cases (16,420 assertions), and under
ASan with libc++'s checks 463 (16,403), no finding; `tool/log_format_audit.py`: 301 engine sources, 0 calls. A Debug
engine (asserts on) over every input: 3,610, all through, and through the frozen readers the 3,390 MusicXML and ABC
inputs, all through. Parity against tb.53: 22,947 outputs, 0 differ (no song has such a note); seeded ids 3,216 of
3,216; every input reads back from its MEI (3,610). The locator gate: 0 failures (every generated id of 501,573
MusicXML notes kept). MuseSync's 24 starters: the same. MuseNote's export gate in every format: exit 0, no
song-aspect worse than tb.53. The frozen readers' whole MEI: the same as tb.53's for the 3,087 songs, the 80 of 3.0.1
and the 194 ABC files.

### 129. MusicXML and ABC: upstream 6.2.0's readers, kept as they were, for the ids MuseNote 2.1 to 3.0.1 stored (tb.53)

MuseNote plan V3_2_0_80 (root causes RC162 and RC163, findings W99 and W146), ours (bad1637, amended by 8d220a4).
MuseNote 2.1 to 3.0.1 shipped upstream 6.2.0 (43f806031) unpatched, and a mark they stored names its note by the id
6.2.0's reader gave it: with `xmlIdSeed` 1 an id counts the objects built before it. The fork's MusicXML and ABC reader
patches build other objects. Loaded with tb.52, 77 of the catalog's 3,087 songs name their notes otherwise (of their
17,182 notes, 7,418 old ids name no note and 851 another, in 38 songs), and so does every ABC file of the parity set (of
13,374 notes, 11,944 name none and 1,375 another). MEI without ids, the key form of PAE and Humdrum name them as 6.2.0
did.

- **6.2.0's readers beside today's.** `src/iomusxml620.cpp`, `src/ioabc620.cpp` and their headers are upstream's files
  at 6.2.0 inside the namespace `vrv::v620`, where their file-local helpers stay (`parseInts`, which
  `expansionmap.cpp` has too). The option `legacyReaders` ("" by default) set to "6.2.0" reads a MusicXML or ABC load
  with them; any other value is refused (`LoadData` returns false, and logs why). No other load changes.
- **Each load begins as 6.2.0's first load in a process began.** 6.2.0's ABC reader keeps the steps a key alters in a
  variable of the process, and a key with no sharps or flats does not empty it: read after a tune in D, a tune in C
  took D's sharps, as gestural accidentals, objects that move every id after them (40 of the parity set's 194 ABC
  files name their notes otherwise when another was read before them; MuseNote root cause RC163). The frozen reader
  empties it where a load begins; within a file it is carried from tune to tune, as 6.2.0 carried it. What a run of
  3.0.1 had read before a mark was stored, no later load can know (MuseNote finding W146).
- **What they take from the later patches.** Run with every MusicXML and ABC load forced through them, the native
  suite (453 cases) had 21 cases end the process (an assertion, libc++'s checks, a fault), 8 throw out of the engine
  and 2 never come back. MuseNote catches nothing the engine throws, so each would end the app; an old shared bundle
  can bring any file. Each fault is guarded as the patch that mended it guards it, marked "MuseNote patch 129 (from
  patch N)" (patches 8, 11, 15, 17, 24, 27 and 28). Where that guard would build other objects for a file 6.2.0 read,
  it does not:
  - an ABC group of elements with no note among them (two changes of clef in a row) goes to the layer alone, and the
    beam 6.2.0 made for it is still made: its id counts in every id after it;
  - an ABC rest or space of length 0 is 0 long, as the division by it gave on arm64; a length too long for an int
    (it comes out below zero) counts no dots, and the smallest int is divided by as before;
  - an ABC `[|:` is still a field named "|" up to its "]"; only a field with no "]" ends at the end of its line;
  - a later ABC tune whose X names no number is read into the tune before it, as 6.2.0 read it (a first one is
    begun: 6.2.0 ended the process);
  - a MusicXML repeat writes the order of play up to past what the engine follows (`OrderOfPlay::s_maxCost`), and such
    an order is refused whole, as the live reader's is (`times="100000000"` never came back); and the order is built
    once, where each reference added copied it whole (a long order took the square of its length), the same order;
  - a MusicXML staff with no `<staffDef>` is given one of five lines (a part that begins with a note had none, but
    the one staff of a score of one part, which 6.2.0 gave one, and the engine read the one that was not there).

  Two change what a file 6.2.0 read holds, and no id: a tuplet that waits for its notes at such a group of elements
  is kept with them (6.2.0 deleted it with its notes, and the line's lyrics then read notes that were gone), and a
  note of a part that begins with a note takes no octave from past the end of the octave-line table. One changes the
  objects of a file 6.2.0 read: an `I:decoration` that names no sign, where 6.2.0 took the sign from past the end of
  its text; the sign stays "!".
- **Measured.** Upstream 6.2.0's native library as MuseNote 3.0.1 shipped it (built from the monorepo at 04dfe261,
  loaded with 3.0.1's options) against tb.53 with the legacy readers, every note by its position: the catalog's 3,087
  songs (496,454 notes), the 80 songs 3.0.1 bundled (11,361) and the 194 ABC files of the parity set (13,374, 6.2.0
  reading each in a process of its own): every id the same.

**Cases**: `test/native/test_tb53_legacy_readers_engine.cpp` in MuseNote's `corelib/verovio` (9 cases): the ids of
two catalog songs and two ABC files pinned from 6.2.0's own load, a format the readers do not read, a release that is
not kept, the state an ABC load begins with, and an input that reaches each guard. 5 of the first 6 fail on tb.52 (the
sixth holds what does not change). Each part broken once, under the address, division and overflow sanitizers, is
caught: the dispatch (5 breaks) and the guards and the state a load begins with (37 of 38); the 38th, the order built
once, removes time only (copied for each reference, the three cases take 160 s under the sanitizers, built once 32 s),
which no case without a clock holds.

**Upstream status**: not offered (MuseNote's own ids).

**Gate of tb.53** (patch 129, MuseNote plan V3_2_0_80). Native: the suite, 457 cases (16,360 assertions), and under
ASan 456 (16,343), no finding; the 9 cases of `test/native/test_tb53_legacy_readers_engine.cpp` under the address,
division and overflow sanitizers too. Every native case once more with every MusicXML and ABC load forced through the
frozen readers (ASan, libc++'s checks, asserts): no crash, hang, exception or report. A Debug engine (asserts on) over
every input: 3,610, all through, and through the legacy readers the 3,390 MusicXML and ABC inputs, all through.
Parity: 22,947 outputs, 0 differ (the canonical load is untouched); seeded ids 3,216 of 3,216; every input reads back
from its MEI (3,610). The locator gate: 0 failures. MuseSync's 24 starters: the same. MuseNote's export gate in every
format: exit 0, no song-aspect worse than tb.52. The legacy-ids gate: every note keeps 6.2.0's id (496,454, 11,361
and 13,374). The frozen readers' whole MEI with the guards against without them: the same for all 3,361 files, but
for its timestamps.

### 128. A rest inside a tab group is timed by its group (tb.52)

MuseNote plan V3_2_0_78 (root cause RC156, finding W140), ours (550b604). `InitOnsetOffsetFunctor` timed a rest inside a
tabGrp twice: the group's end added the group's duration, and the rest, visited as a layer element of its own, added
its own as well, so what followed it was played late and the bar ran long (an MEI guitar-tab file's
`<tabGrp dur="4"><tabDurSym/><rest/></tabGrp>`: the next note at quarter 3, not 2). Upstream has it from 6.2.0 to
develop; its e2f797f22 (patch 121) puts such a rest into each rest of a single TAB staff from MusicXML, so this patch
comes before it. A rest in a tab group now takes its group's time, as the group's notes do, and adds none.

### 127. A `<chord/>` note whose chord is not found is deleted (tb.52)

MuseNote plan V3_2_0_78 (root cause RC155, finding W126), ours (8866238), crediting upstream f1998d2b0 (6.4.0): patch
28 returned from a `<chord/>` note whose chord the reader could not find ("Chord starting point has not been found")
without deleting the note it built. That commit's `delete note;` alone: its other half passes over a chord note after
a rest at a measure's start in another way, which builds other objects and moves ids; patch 28 already reads that case.

### 126. A direction's voice does not move the voice-less notes after it (tb.52)

MuseNote plan V3_2_0_78 (root cause RC154, finding W125); upstream 53584d697 (develop), cherry-picked.
`ReadMusicXmlDirection` let a direction's `<voice>` select the current layer, so the voice-less notes after it went to
that voice's layer (patch 95, RC112, sends back a note that names its own voice; one that names none stayed).

### 125. Nested slurs are adjusted innermost first (tb.52)

MuseNote plan V3_2_0_78 (root cause RC153, finding W139); upstream eb00ec45a (develop), cherry-picked onto patch 91:
both keep the outer slurs in the order of the positioners (patch 91's code and comment are kept in the two conflicting
hunks), and upstream's stable sort, innermost first, is taken. In document order an outer slur was placed before the
slur inside it had risen over its own inner one, and was drawn through it (three nested slurs, the inner over high
notes: 41 units of overlap; now clear by 38).

### 124. A Humdrum trill line that ends at a grace chord names the chord (tb.52)

MuseNote plan V3_2_0_78 (root cause RC152, finding W138); upstream 828f7264d (develop), cherry-picked. `addTrill` ended
the extension line at a grace token by a note id; a grace chord's id is a chord's, so the line named an element that
does not exist and was not drawn.

### 123. A Humdrum ornament follows its note above or below, to a staff the score has (tb.52)

MuseNote plan V3_2_0_78 (root cause RC151, findings W137, W127). Upstream 992d2e644 (6.4.0), cherry-picked:
`getNoteStaff` looked for the above/below mark after lowercase pitch letters only, so the ornaments of a note below
middle C stayed home while the note moved. And ours (d50c0fe): it returned the note's staff plus or minus one with no
bound: on the top staff an ornament marked above got staff 0, and with 992d2e644 one marked below on the bottom staff
got one past the last, while the note stays on its own. The staff is now one the score has, as a hairpin's already is.

### 122. Humdrum unpitched notes are not taken for errors (tb.52)

MuseNote plan V3_2_0_78 (root cause RC150, finding W136); upstream 2fee90407 (6.3.0, its issue 4350), cherry-picked:
an unpitched note (`R`) on a `*clefX` staff was coloured as an error, and a chord's unpitched notes were placed on one
line.

### 121. A TAB staff's content is built as a TAB staff's (tb.52)

MuseNote plan V3_2_0_78 (root cause RC149, finding W135). Upstream, cherry-picked: e2f797f22 (6.3.0) keeps the rests
of a part of a single TAB staff, which were dropped (each a `<rest>` in its tabGrp, timed by its group: patch 128);
d7b74259b (6.4.0) has an arpeggio name every note of a tab chord, not the first; 90a440db7 (6.4.0) gives a stemless
tab note no duration sign; 20c1936cc (6.4.0), its articulation half: a note's `<articulations>` are not given to its
tabGrp, which holds none (its verse half guards the verses upstream adds at lyric extender stops, 92b03ade9, which this
branch does not have). And ours (cac6aa3), the same class: a `<technical>` mark other than a string, fret, fingering or
thumb position (a hammer-on, a bend, a harmonic) went the same way, refused by the tabGrp with an error and lost to
memory; it is left out as an articulation is. Upstream develop still does.

### 120. An MEI grace group may stand in a tuplet (tb.52)

MuseNote plan V3_2_0_78 (root cause RC148, finding W134); upstream 0b4b78a9b (6.4.0), cherry-picked: the MEI reader
refused a `<graceGrp>` inside a `<tuplet>`, and its grace notes were dropped.

### 119. Two clefs at one place keep the file's order (tb.52)

MuseNote plan V3_2_0_78 (root cause RC147, finding W133); upstream e2e5cd302 (6.4.0), cherry-picked: a clef inserted
after an element inside a chord or a beam was recorded at the wrong time, and one inserted before a measure's first
element was recorded at all, so a second clef at the same place went before the first.

### 118. A tuplet whose notations are not printed is drawn with none (tb.52)

MuseNote plan V3_2_0_78 (root cause RC146, finding W132); upstream e2c687382 (6.3.0), cherry-picked: the `print-object`
of the `<notations>` holding a tuplet's start was not read, so a hidden tuplet was drawn with its number and bracket.

### 117. An unpitched note on the percussion channel plays its instrument's key (tb.52)

MuseNote plan V3_2_0_78 (root cause RC145, finding W131); upstream 3d744d933 (6.4.0), cherry-picked: an unpitched note
got its place on the staff only, so the MIDI writer played every percussion note at key 0. For a part on MIDI channel
10, the key of its instrument's `<midi-unpitched>` is its `@pnum`, played untransposed and untuned; patch 6 records it,
so the pitch map has it. The drawing is the same (unpitched notes still compare alike).

### 116. A circle-x notehead is drawn as one (tb.52)

MuseNote plan V3_2_0_78 (root cause RC144, finding W130); upstream 378720075, 0c9330162, 60a4472dc (6.4.0),
cherry-picked (CHANGELOG.md left out): `circle-x` was mapped to a slash; it is noteheadCircleX. They also map
`triangle` to `isotriangle` in place of `rtriangle`: the MEI value only. Neither is drawn: `GetNoteheadGlyph` has the
triangle cases commented out (here and upstream to develop), and the font data has no triangle glyph (MuseNote RC157,
W141, open).

### 115. A no-chord shows what its file shows (tb.52)

MuseNote plan V3_2_0_78 (root cause RC143, finding W129); upstream ede5a0f20 (6.4.0), cherry-picked (CHANGELOG.md left
out): `root-step@text` and, for the kind "none", `kind@text` were not read, so a "C" was drawn where the file shows
"N.C." or nothing.

### 114. A breath mark or caesura stands at the end of its note (tb.52)

MuseNote plan V3_2_0_78 (root cause RC142, finding W128); upstream 44c85a898 and 75df1a822 (6.3.0), cherry-picked: a
MusicXML `<breath-mark>` or `<caesura>` was placed at its note's start plus half a beat, so after a note longer than a
beat it stood in the note's middle. It is placed at the note's end.

**Gate of tb.52** (patches 114 to 128 together, MuseNote plan V3_2_0_78). Native: the 16 cases of
`test/native/test_tb52_upstream_round_two_engine.cpp` fail on tb.51 where they test a fix, but RC155's, whose fix is
shown by `leaks --atExit` on its case (18 leaks on tb.51, none on tb.52); each of the commits, undone alone, is caught
by its own case (992d2e644 undone with patch 123's bound on it, by its subcase); the suite, 448 cases (15,954
assertions), and under ASan 447 (15,937), no finding; a Debug engine (asserts on) over every input: 3,610, all through.
Parity: 107 of 22,947 outputs differ, all of 27 songs (breath marks in 7, caesuras in 11, no-chords in 8, a triangle
notehead's value in Honky-Tonk Woman: their MEI, its locator copies, and page 1 of 26 of them); no generated id moves,
no timemap, MIDI or pitch map changes; seeded ids 3,216 of 3,216; every input reads back from its MEI. The locator
gate: 0 failures. MuseNote's export gate in every format, with its MusicXML export's breath and caesura rule moved with
patch 114 (it found such a mark's note by this branch's old placement): Cambalache's four no-chords, now empty, are not
exported (declared); its four reader guards give tb.51's results; MuseSync's 24 starters play the same.

### 113. Order of play: the part after the part played before is found past what plays nothing (tb.51)

MuseNote plan V3_2_0_77 (root cause RC139, finding W79). `ExpansionMap::Expand` moves a part the order names for the
first time to just after the part played before it, where the element after that part is another one. It took the very
next element only: a system break, a page break or a score definition there left the part where it is written, and the
score played it out of its turn. I Saw Her Standing There (an `<sb/>` after bar 44) played its endings 43, 44, 45 where
its order says 44, 43, 45. Those elements, which play nothing, are looked past. Upstream's line is the same at 6.4.1
(renamed `IsAnyOf`: a textual conflict at the base move, nothing more).

### 112. Humdrum reader: brackets played in turn are no two endings; a change stays in its ending; a disjunct tie read end first (tb.51)

MuseNote plan V3_2_0_77 (root causes RC137, RC140 and RC141, finding W79).

- **RC137.** `inDifferentEndings`, `atEndingBoundaryStart` and `atEndingBoundaryEnd` judged two sections different
  endings by the numbers their labels end in alone. An ending a file draws as two brackets is two sections, labelled by
  their places (B1, B2), and a tie from the first bracket into the second, which the expansion list plays right after
  it, was broken: its note struck again (Crying In The Chapel, Coconuts, Here I Am Lord). One check for the three,
  `endingsApart`: the numbers differ, and the primary expansion list never plays the second right after the first. The
  pairs are read by `prepareSections`, as `storeExpansionLists` takes the list (the one before the first data line, or
  the primary of several); the list's parsing is shared, `readExpansionList`. A tie hanging at such a bracket's edge is
  drawn as any tie with no other end is.
- **RC140.** `addSystemClefKeyTimeChange` added a change's score definition to the enclosing section: inside an ending
  it landed after the ending, whose later measures kept the meter before, and a whole-measure rest there took that
  meter's length (The Fool On The Hill, Bohemian Rhapsody; eight more drew the change after the ending). It is inserted
  right before its measure, in the measure's container, once `setupSystemMeasure` has placed the measure.
- **RC141.** `processHangingTieStart` looked for a tie's end read first, in an earlier layer of the measure, by time
  alone (upstream's note there: "deal with disjunct ties as well"), and a disjunct tie's notes are apart: both were left
  hanging. A disjunct start (`[[`, `__`) takes a disjunct end (`]]`, `__`) of its pitch that comes after it.

### 111. Humdrum export: a tie whose notes are apart is disjunct; a measure as long as the engine plays it (tb.51)

MuseNote plan V3_2_0_77 (root causes RC136 and RC138, finding W79).

- **RC136.** `Tool_mei2hum` wrote every tie `[` and `]`. The Humdrum reader joins those only where the second note
  starts as the first ends (`processTieEnd`): a tie across a rest or a space was broken in two, and its second note
  struck again (Superstar, Birdland, A-Tisket, Chaka Khan, Good Riddance, Long Cool Woman, Stop and Stare). A tie's ends
  are kept as their notes are written (`markTiesApart`, called from `parseNote`: when its start ends, when its end
  starts; a grace note takes no time); once both are known and they differ, both marks are made a disjunct tie's, `[[`
  and `]]` (a middle `__`): the note being written in its own token, the other in the token it was written as, found by
  `keepId`'s id. `keepId` keeps a chord's notes by their places, so a chord needs no id of its own for it.
- **RC138.** The converter timed a measure by its meter, where the engine times it by what it holds. In a measure its
  meter does not fill (`@metcon="false"`) the engine gives a whole-measure rest no time
  (`LayerElement::GetAlignmentDuration`); it was written as long as the meter, and the space after it as well (Viva la
  Vida, Guantanamera, Apologize and five more, all files of a forward past the barline: the measure came back a bar
  longer each pass). Such a rest is written over the plain space after it in its layer, as long as it; with no space
  after it, it is not written, and the filler of its layer or of its staff is the rest, seen as it was
  (`m_untimedRests`). `parseMeasure` makes a measure as long as its longest staff and fills each shorter staff once to
  it: it filled them to the meter (a pickup no staff fills came back as long as the meter; and beside a staff longer
  than the meter a shorter one had two fillers).

All three, measured by MuseNote's export gate over every song, the Humdrum export read back by the engine, against
tb.50: songs whose notes or times differ 22 to 0 (7 live to 0), how long 22 to 0, how loud 11 to 1, channel 12 to 2;
tempo and instrument unchanged; no song differs anew in any aspect. Parity: 2 of 22,947 outputs differ, I Saw Her
Standing There's MIDI and timemap (patch 113: its endings in its order's turn); no Humdrum sample's MEI changes; seeded
ids 3,216 of 3,216; every input reads back from its MEI. None of the six functions changes upstream at 6.3.0, 6.4.0,
6.4.1 or on develop.

**Cases**: `test/native/test_tb51_humdrum_last_kinds_engine.cpp` (10 cases): a tie across a space at the barline and
across a rest, a middle apart on either side, the second note of a chord with no id, a tie whose end is written first
in an earlier layer, a grace tied to its note (must stay plain); a tie from one bracket of an ending into the next, a
tie hanging at such a bracket's edge, the primary of two expansion lists; a whole-measure rest over the space after it
(a measure of the meter and one of half), a pickup's rest on another staff and in another layer, a measure of space
there, a pickup of unequal staves tied into the next measure, a staff longer than the meter; endings written 2, 1, 3
with a system break, a page break, a score definition before the last; a meter change inside an ending and at a
section's start. On tb.50 eight fail (41 checks); the two must-not-change cases pass. Twenty-nine breaks, each part of
the three patches alone: every one caught.

### 110. Humdrum reader: a merged tuplet ends where its second part does; a tremolo of tuplet strokes has their count (tb.50)

MuseNote plan V3_2_0_76 (root causes RC135 and RC132, finding W79). `HumdrumInput::mergeTupletsCuttingBeam` renumbered
the tuplets after a merge from two elements on: a second tuplet of one element (a space of a tuplet's value after a
beam's first notes, which patch 109 now writes) kept its end's old number, the merged tuplet ended under none, and it was
opened around the beam, the beam's notes after it losing their length (Your Song v2: 784 notes otherwise with patch 109
alone, none with this). Its end is renumbered too. And a tremolo the reader makes of strokes of a tuplet's value (12,
24...) is given their count, `@num`, as the MIDI writer reads it: its `@unitdur`, a power of two, made fewer strokes.

### 109. Humdrum export: what a container holds keeps its time; tremolos, tempos and arpeggios written (tb.50)

MuseNote plan V3_2_0_76 (root causes RC131 to RC134, finding W79).

- **RC131.** `Tool_mei2hum` read a layer's content with a list of its own in the layer, the beam and the tuplet, and
  dropped what a list lacked, with its time and a warning only: a space in a tuplet or a beam, a beam in a beam, an
  `mSpace` (Superstar, Boogie Woogie, Give Me One Reason, Fatback). One dispatch now, `parseLayerElement`; a beam in a
  beam stacks its starts and ends, its container's end deferred as a tuplet's is; a space takes no beam mark (a beam
  started on a space a reader does not draw); an `mSpace`, and a rest the score does not draw (`@visible="false"`), is
  a rest not drawn (`ryy`).
- **RC132.** A tremolo had no case, and was dropped (Le mille bolle blu). `parseTremolo` writes it with `Tool_tremolo`'s
  markup, as `GenerateMIDIFunctor::VisitBTrem` plays it: a bowed tremolo `@N@` from its `@num`, an enclosing tuplet's
  num, its `@unitdur`, or its note's value and slashes (`BTrem::CalcIndividualNoteDuration`); a fingered one `@@N@@`, its
  two notes halved, as the engine times them. The file is read again, and `Tool_tremolo` writes the strokes. In
  `Tool_tremolo`: the strokes' beams are counted from the beams open where the note stands (`getOpenBeamCount`), the
  note's own starts and ends kept (in a beam of the score the strokes' beams did not close, and a reader lost the beams
  from there); each tremolo has its own `*tremolo` and `*Xtremolo` (one went from a spine's first tremolo to its last,
  and plain beamed pairs of one pitch between were read as tremolos). `HumdrumFileBase::insertNullInterpretationLineAbove`
  passes over lines with no spines: a global comment at a tremolo's end (a system break) was copied for the new line's
  fields, a line of one field, and the file could not be read.
- **RC133.** `parseTempo` read a tempo's `@bpm` and `@mspb`, which MEI does not have: `@midi.bpm`, then `@midi.mspb`
  (60,000,000 / mspb; it was multiplied). A tempo the MusicXML reader writes was never written (William Tell).
- **RC134.** `buildIdLinkMap` linked a control event by `@startid` only: an `<arpeg>` given by `@plist` alone, as the
  MusicXML reader writes one, is linked to each element it names (Ashokan Farewell, Help!, Money Money Money).

Both, measured by MuseNote's export gate over every song, the Humdrum export read back by the engine, against tb.49:
songs whose notes or times differ 45 to 22 (25 live to 7), tempo 119 to 23, how long 45 to 22, how loud and channel
better by 12 and 14; counted note by note, no song worse, 29 better, 23 now as the song; MusicXML and ABC unchanged.
Parity: 2 of 22,947 outputs differ, the locator dumps of two of the parity set's damaged Humdrum samples (written by the
6.2.0 converter: All You Need Is Love, The Imperial March), their tuplets now inside the beam; the locator gate's 8
notes of All You Need Is Love's measure 6 with them. Seeded ids 3,216 of 3,216; every input reads back from its MEI.

**Cases**: `test/native/test_tb50_humdrum_kept_engine.cpp` (8 cases): a space in a tuplet and in a beam, a beam in a
beam (in the middle, first and last), a beam starting with a space, a bar of space; hidden rests; tremolos (an eighth
in no beam, a quarter's chord, in a beam and at its end, @num 3, fingered), each read back as the MIDI plays it, and
drawn as a tremolo again; a *tremolo for each of two tremolos, one before a system break; tempos by @midi.bpm and
@midi.mspb; an arpeggio by @plist; Your Song's beam; must not change: plain beams and tuplets, a tempo in @mm, a chord
with no arpeggio, six triplets in one beam. On tb.49 seven fail. Sixteen breaks, each caught. Three cases of
`test_mei_to_humdrum_engine.cpp` that pinned the lost bar of an `mSpace` now hold its rest, and play as their scores.

Not handled (drawing only; each played right; MuseNote W122): a tremolo sharing a beam with other notes is not folded
by the reader (`checkForTremolo` takes whole beam groups); an unmeasured tremolo is written as its note; a fingered
tremolo with neither `@unitdur` nor `@beams` as its two notes, halved. The MIDI writer reads no `@midi.mspb` (W121).

### 108. getMIDIValuesForElement builds the timemap of the document it reads (tb.49)

MuseNote plan V3_2_0_75 (root cause RC130, finding W119); upstream ad1570a6d (develop, after 6.4.1), cherry-picked.
`Toolkit::GetMIDIValuesForElement` read the document for playback (`m_midiDoc`) and, where it had no timemap, built
the timemap of the loaded document (`m_doc`): where the document for playback is a copy (a score with no order of its
own), its values were empty until a timemap had been rendered. MuseNote's `noteDetail` asks it first. The other timed
queries (`GetElementsAtTime`, `GetTimeForElement`, `GetTimesForElement`) already build on the document they read.

### 107. A bow tremolo's count of notes, and a tablature note's course, ask for no memory past what is played (tb.49)

MuseNote plan V3_2_0_75 (root cause RC129, finding W118). Upstream 61866183d (6.4.0), cherry-picked: the MIDI writer
expanded a bow tremolo into a list of notes as long as a count it worked out unchecked (negative or huge for a secondary
tied note, a note shortened by graces, a huge `@num`, a mensural unit), and the list asked for memory until the process
ended; secondary tied notes are now passed over, `@num` is held to 256, and a unit below a 2048th is not expanded. And
ours (8a2125c), the same class found by the audit: `VisitNote` resized the tablature hold list to the note's course, a
number the file gives (MusicXML `<string>`, MEI `@tab.course`; upstream develop still does), so a course of
2,000,000,000 ended the process the same way. The held notes are kept by course in a `std::map`, walked in course order
as the vector was; a course with no note held has no entry where the vector had one of pitch 0, which every walk
passed over, so the MIDI written is the same.

### 106. The MIDI writer keeps octave lines by span (tb.49)

MuseNote plan V3_2_0_75 (root cause RC128, finding W117); upstream c8582c622 (develop, after 6.4.1), cherry-picked.
`GenerateMIDIFunctor::HandleOctave` started a line at the first element of its layer at or after its start, its end not
looked at, and ended or started one line at each element: a layer with nothing inside a line had the next note after it
moved, and of two lines ending on one note the second stayed on. Every line of the layer that ends before the element
now ends, and every line that holds it starts; a grace comes before the note it shares its place with. Patch 6 records
the pitch the MIDI plays, so the pitch map has the same pitches.

### 105. Each staff's visibility is its own <staff-details> (tb.49)

MuseNote plan V3_2_0_75 (root cause RC127, finding W116); upstream 8339461f8 (6.4.0), cherry-picked.
`ReadMusicXmlMeasure` set every staff's visibility from the part's first `<staff-details>`: hiding staff 2 hid staff 1
and its notes. Each staff now reads the `<staff-details>` of its number, else one with no number. The same cause, not
mended here (MuseNote's W120, its own plan): mid-score `<key>` and `<time>` are read from the first part only and set
for every staff.

### 104. A grace's slash by its value, and a direction's offset in an int (tb.49)

MuseNote plan V3_2_0_75 (root cause RC126, finding W115). Upstream 8300807c9 (6.3.0), cherry-picked: `grace@slash`
was read by its presence, so `slash="no"` made a slashed grace, played before the beat. Upstream af2adb2a7 (6.4.0),
its direction half by hand: a direction's `<offset>` was read into a `short`, and one past 32,767 divisions wrapped.
(Its other half reads the offset of a `<sound>`'s tempo, which 6.4.0 added and this branch does not have.) The class
audited: every other yes/no attribute is read with `as_bool()`; every other `short` holds a staff, voice, line or
number, no count of divisions.

### 103. The staff a cross-staff note is on, for its notations and its octave (tb.49)

MuseNote plan V3_2_0_75 (root cause RC125, finding W114). Upstream 6d5d4f85a (6.3.0; by hand: the marks stay filed by
the bar they were read in, patch 68), 15c17a299 and 2387e3807 (6.4.0), cherry-picked: the MusicXML reader took a
cross-staff note's staff from its layer for its notations (fingering, breath, caesura, dynamics, fermata, glissando,
mordents, trill, turn), for the octave-shift that sets its written octave (under an 8va on its own staff it was drawn
an octave off), and for whether it gets `@oct.ges` (with only its layer's staff shifted it was played at that shift).
And ours (3e58a6a): upstream reads `m_octDis` at the note's `<staff>` whatever it names, past the vector's end for a
staff the score does not have (tb.9 closed the same read for an octave-shift); such a note is drawn on its layer's
staff, so its notations and octave are now that staff's.

**Gate of tb.49** (patches 103 to 108 together, MuseNote plan V3_2_0_75). Native: the cases of
`test/native/test_tb49_upstream_fixes_engine.cpp` fail on tb.48 where they test a fix (the tremolo of `@num` 100,000
plays 100,000 notes; the course of 2,000,000,000 passed 2 GB under a watchdog and was stopped), and each of the eleven
commits, undone alone, is caught by its own case (our bound of patch 103 also stops the hardened ASan build, in
`ReadMusicXmlNote`); the suite, 414 cases (15,152 assertions), and under ASan 413 (15,135), no finding. Parity: 14 of
22,940 outputs differ, all of three songs: Sing (MEI, page, MIDI, timemap: its 14 graces marked `slash="no"` are `acc`,
patch 104) and both versions of Non, je ne regrette rien (MEI, page: the "rall." where its file's offset of -70,400
divisions puts it, before the song, so at the left barline of bar 1; tb.48 read the offset wrapped, bar 21. The file is
wrong: MuseNote corrects it as its v3). No generated id moves; seeded ids 3,215 of 3,215; every input reads back from
its MEI. The locator gate: 28 failures, one change, Sing's 14 graces in both MusicXML kinds, their position and id kept
and their fingerprint's grace kind `unacc` to `acc`: the declared change, accepted by the owner (2026-11-19); a mark
stored by id finds them, one stored by position and fingerprint (development builds only) does not. MuseNote's export
gate in every format and its four reader guards give tb.48's results; MuseSync's 24 starters play the same. A Debug
engine (asserts on) over every input: 3,609, all through.

### 102. Humdrum export: a grace note keeps its kind, and a grace chord is a grace (tb.48)

MuseNote plan V3_2_0_73 (root cause RC122, finding W79). `Tool_mei2hum::parseNote` wrote every grace `q`, and the
Humdrum reader reads `q` as a grace before the beat (`GRACE_unacc`) and `qq` as one on it (`GRACE_acc`; humlib: "qq
means a grace note on the beat, and q means a grace note before the beat"). An MEI `@grace="acc"`, played on the beat,
came back before it, and the notes around it moved (Radetzky-Marsch, Under the Bridge, Three Little Birds). An `acc`
grace is now written `qq`; any other grace is still `q`. And a chord that is a grace by its own `@grace`, its notes
saying none (as the MusicXML reader makes one), was written as a plain chord, with its duration, in the place of the
note it stands before (Musetta's Waltz: `8e 8f` took an eighth and carried the bar's chord symbol). Its notes now take
the chord's `@grace`, `parseChord` queues it with the grace notes, and the queue writes it as a grace token
(`addGraceToken`), as it writes a grace note. (The mensural twins are left as they are. A grace written on a chord's
notes only, the chord plain, is not handled; the MusicXML reader never writes it, and no song has it.)

Measured by MuseNote's export gate over every song, the Humdrum export read back by the engine, against tb.47: 16
exports change, each a song with an `acc` grace; songs whose notes or times differ 60 to 45, how loud 154 to 139,
channel 165 to 153; no song worse in any aspect; 15 better. Parity: 0 of 22,940 outputs differ (no reader or drawing
changes); seeded ids 3,215 of 3,215; every input reads back from its MEI.

**Cases**: `test/native/test_tb48_humdrum_grace_kinds_engine.cpp`:
- an eighth grace before a note, `acc` and `unacc`: written `qq` and `q`, the Humdrum read back playing as the MEI;
- a chord that is a grace by its own `@grace`, `acc` and `unacc`: one grace token, the note after it in its place;
- must not change: a chord that is no grace, written with its time.
The first two fail on tb.47. Each part broken and caught: every grace `q`; the chord's notes not taking its grace (the
queue is then written again from within itself, and the run ends); the chord not queued; the chord written as a data
token.

### 101. Humdrum export: only a tie is a tie's middle (tb.47)

MuseNote plan V3_2_0_72 (root cause RC121, finding W79). `Tool_mei2hum::parseTieStart` took any link ending at a tie's
start note for a tie arriving there (a tie's middle, written `_` by `parseTieStop`), and `parseTieStop` any link starting
at its end note for one leaving. A slur ending where a tie starts lost the tie's start `[`, one starting where it ends
lost its end `]`, and a reader of the Humdrum struck the held note again (A Teenager in Love). Only a tie is taken so
now.

### 100. Humdrum export: a layer number a staff has no layer of is a rest not seen (tb.47)

MuseNote plan V3_2_0_72 (root cause RC120, finding W79). `Tool_mei2hum::parseStaff` placed a layer at the spine its number
names, and made a spine for every number up to the largest. A staff whose layers are numbered with a gap (its one layer
its 2, a file's second voice, as the app's two-staff scales are written) got a spine of null tokens only, with no time,
and a reader timed the staff by nothing: the music came back a beat late and five times as long (Alle Jahre Wieder:
quarters 0 to 32 read back as 1 to 152). Each layer number the staff has no layer of is now filled with a rest not seen
(`ryy`), the length of the staff, after the short layers are filled to it (patch 96). The converter's own note, "need to
add invisible rests in un-specified layers", was a warning only and is gone. (The mensural twin fills no layer at all
and is left as it is.)

Both, measured by MuseNote's export gate over every song, the Humdrum export read back by the engine, against tb.46:
songs whose notes or times differ 238 to 60, how loud 328 to 154, channel 339 to 165; no song worse in any aspect; 178
better. Parity: 0 of 22,940 outputs differ (no reader or drawing changes); seeded ids 3,215 of 3,215; every input reads
back from its MEI.

**Cases**: `test/native/test_tb47_humdrum_layers_ties_engine.cpp`:
- a staff whose one layer is its 2, the same under a staff of layer 1 (a two-staff scale), and layers 1 and 3: every
  spine timed, the Humdrum read back playing as the MEI;
- a tie from a note a slur ends on, and to one a slur starts on: one start, one end, played as the MEI;
- must not change: a tie's middle (`[`, `_`, `]`), and a staff of layers 1 and 2 (no rest added).
The first two fail on tb.46. Each part broken and caught: no fill, the rest always in the first spine, a tie's start or
end taking any link.

**Upstream status**: not offered.

### 99. A coda or segno sign's name written beside it is its name (tb.46)

Found by MuseNote plan V3_2_0_54's export gate (finding W76); fixed by plan V3_2_0_70 (root cause RC117). The MusicXML
reader reads a jump or a target from its words alone (patch 66). A file that writes a coda sign and its name "coda"
beside it, each in a direction of its own with no `<sound>` (Livin' La Vida Loca bar 68), was read as two targets in
one bar: the sign (`repeatMark`) and the words (`dir type="coda"`). The ABC export wrote `!coda!!coda!` on one note.
And since the order of play keeps one target label per measure, a name with a numeral ("Coda II") labelled the
measure's target only when the words were read before the sign.

Words that name a coda or segno whose sign the measure holds (in a direction of its own, with no words) are now that
sign's name: printed as written, untyped. The sign is the one target, labelled by its name (`codaii`), in the MEI and
in the order of play (`MeasureHoldsSign`, `NameOfSign`).

Parity, against tb.45: 32 of 22,940 outputs differ, all of 8 MuseNote songs (6 live): in each, the name's `dir` loses
its `type`, and its page the class with it. No element is added or removed (no id moves); no timemap, MIDI or pitch map
changes. Seeded ids 3,215 of 3,215; every input reads back from its MEI.

**Cases**: `test/native/test_tb46_sign_names_engine.cpp`: a coda or segno sign and its name, the name written after
or before it, at its beat or offset (one target, the words printed); "Coda II" beside a sign written first (the mark
labelled, and "To Coda II" after "D.C. al Coda II" lands there); five shapes that must not change (a name alone, a sign
alone, a sign and its name in one direction, a To Coda's sign, a segno in a To Coda II's bar, unlabelled). The first
two fail on tb.45. Each part broken and caught: the words still a target, the order of play or the mark unlabelled, a
sign with words counted as a sign, any jump's label taken for the name.

**Upstream status**: not offered.

### 98. A forward is time in its voice wherever it stands (tb.45)

Found by MuseNote's finding W35 and traced by plan V3_2_0_66 (root cause RC3); fixed by plan V3_2_0_68. The MusicXML
reader took a `<forward>` as cursor movement only. It made time when a later note needed the gap, or when the forward
was the bar's very last element (`!node.next_sibling()`), and then its whole length after what its voice held:

- a barline or a direction after it, a second forward, or a backup after it left the bar short (All Through the Night
  bars 62 and 63, two forwards in each voice: 2 beats of 4; Rock Around the Clock bar 42; Consider Yourself bar 17);
- a forward after a backup, over time its voice already had, was added again (Cabaret v2 bar 16: 5 beats of 4);
- a forward that filled its voice did not move the cursor, so its bar was marked not full (`metcon="false"`).

A forward with no note, backup or forward after it in its bar now brings its voice (its `<voice>`, else the current
layer's) to the cursor, with space only where the voice holds nothing yet; any other forward moves the cursor alone.
The furthest a bar's forwards carry the cursor is the bar's time, as MusicXML gives it: a bar they carried past
everything it holds, and back, is filled to there in the forward's voice at its end. A first version that filled at
every forward broke Chaka Khan v2 bar 20, where a forward of no voice takes the cursor to a chord symbol and is backed
up from; it was dropped.

Parity, against tb.44: 503 of 22,919 outputs differ. 38 ended songs of the MuseNote corpus, never served, play their
forwards (the files that cross their bars' ends, corrected since as new songs); the 35 songs MuseNote corrected in plan
V3_2_0_67 are new outputs. Of the live songs, 12: four play as their files give (the bars above); two play the padded
pickup their files write (America the Beautiful v2, Birdland: MuseNote finding W101, the files corrected); three fill a
voice to their bar's end, play identical (El día que me quieras v2, New York State of Mind v2, La Brabançonne's last
bar); four are marked full or not full by their true time (Fantomas, High Noon, Since I Fell for You v2; Superstar bar 1,
overfull by a sixtieth of a quarter). Seeded ids on a fresh thread: 3,212 of 3,212; every input reads back from its MEI.

**Cases**: `test/native/test_tb45_forwards_engine.cpp`: a forward before a barline or a direction, in a voice or of
none; forwards in a row; a forward after a backup; a forward then a backup with nothing after (the bar four beats; of
voice 2, its time in voice 2's layer); and two shapes that must not change (a forward to a chord symbol, backed up
from; forwards before their voice's notes). Four fail on tb.44. Each part broken and caught: the old "anything after it"
test, filling the forward's whole length, no fill at the bar's end, that fill always in the current layer.

**Upstream status**: not offered.

### 97. A bar is full by its longest voice (tb.44)

Found by MuseNote plan V3_2_0_66's export gate (root cause RC116). The MusicXML reader marked a bar not full
(`metcon="false"`) by where its cursor stood at the bar's end, which is where the voice written last ends; the bar's
longest voice was computed a few lines later, for its right barline. A bar whose voice written last ends early (All I
Ask of You's MusicXML export: voice 2, a half rest, written after voice 1's full bar) was marked not full. The bar's
time, its longest voice's, is now taken first and decides it.

Parity, against patch 95 alone: 69 of 22,674 outputs differ, the MEI of 23 songs and their locator MEIs, by
`metcon` only: 23 bars lose `metcon="false"` (full by their longest voice), 14 gain it (a voice runs past the meter
while the cursor ended on it: an overfull bar). No page, MIDI, timemap or pitch map changes, and no note moves.

**Cases**: `test/native/test_tb44_voice_layers_engine.cpp`: a whole note, a backup, and a half rest of voice 2 written
last make a full bar; a bar of one half note in 4/4 is still not full. The first fails on tb.43; the break (the cursor
again) caught.

**Upstream status**: not offered.

### 96. A staff is as long as its longest layer, each layer filled to it, in the Humdrum export (tb.44)

Found by MuseNote plan V3_2_0_66's export gate (root cause RC115). `Tool_mei2hum::parseStaff` (and its mensural twin)
took a staff's length from its first layer: a staff whose first-written layer is the shorter ended early, and the next
measure overwrote its last beats (`.ZZZ` tokens, rests of negative length). The MusicXML reader makes a bar's layers in
the order its voices first appear, so a bar that starts with voice 2 lists layer 2 first: In Your Own Sweet Way's
Humdrum export, garbled from bar 21. A staff is now as long as its longest layer, and each layer shorter than it is
filled to its end with a rest that is not seen (`ryy`): the staff's length alone was not enough, since a layer left
short ran its spine out of step with the others (I Just Can't Wait to Be King: two layers of two beats beside one of
four, then a bar of two; their spines merged before the barline). The measure's filling of a short staff, which
reached only its first spine, had hidden that where the first layer was the short one. (The mensural path writes every
length as 0 in the case tried, so it cannot show the fault there; it takes the staff's length by the same rule.)

No output of the parity set changes: the converter writes none of them. MuseNote's export gate (every song's Humdrum
export, read back by the engine), against tb.43: Goldfinger and its v2, English Country Garden and How to Save a Life
now play as their songs; no song plays worse. (The gate's census of element kinds counts more unseen rests.)

**Cases**: `test/native/test_mei_to_humdrum_engine.cpp`: a staff whose first layer, layer 2 or layer 1, is the
shorter converts with no `ZZZ` and no length below 0, and plays as written when read back; and two short layers
beside a full one, then a bar of two, play as written read back. The first fails on tb.43, the last with the staff's
length alone: both breaks caught.

**Upstream status**: not offered.

### 95. A note goes into its own voice's layer (tb.44)

Found by MuseNote plan V3_2_0_66, reading the bars of its census of bar times (root cause RC112; finding W73). The
MusicXML reader took a note's layer from its `<voice>` and `<staff>` only at a bar's start and after a `<backup>`
(`SelectLayer`, `m_isLayerInitialized`); any other note went into the layer of the note before it. A voice that begins
or resumes where the cursor already is needs no backup in MusicXML: Big Girls Don't Cry writes voice 1's last rest
right after voice 2's notes, both at the third beat's end, and the engine put the rest into voice 2's layer, which then
held five quarters in a bar of four (the bar played five). A node of another voice than the current layer's now goes
to that voice's layer: on its own staff, or, where the voice has none there yet but one on another staff of the bar,
that one (the note drawn cross-staff, as its voice's other notes there are). A node of the current voice, or one that
names none, stays in the current layer, as before: a note of the voice on another staff is drawn there, cross-staff
(placing every voiced node by its staff, as the patch first did, took cross-staff notes off their voice: an MEI's
cross-staff notes exported to MusicXML and read back, 16 of 18 lost).

Parity: 55 of 22,674 outputs differ, all of the 9 songs that write a voice change with no backup (20 bars): each
keeps its notes and pitches, its voices now in their own layers. Six play their bars at the file's length again (Big
Girls Don't Cry 303 quarters, now 300; How to Save a Life 518, now 508; Your Song and its v2 5 21/32 fewer; Paper Planes;
Chaka Khan), three only change layers (In Your Own Sweet Way, All I Ask of You, The Winner Takes It All). Chaka Khan's
MIDI has 3 note-ons fewer: unisons of its two voices now struck at one time on one channel, every note in the
timemap as before. The annotation locator: 64 notes (74 by source ids) moved from another voice's layer to their own,
and 629 generated ids renumber after them; MuseNote's resolver places 2,472 of 2,517 marks of the 3.2.0 kind on their
notes, 32 not at all, 13 on a note of the same pitch and length in the same bar. Seeded ids 3,177 of 3,177; read back
from MEI 3,571 of 3,571.

**Cases**: `test/native/test_tb44_voice_layers_engine.cpp` in MuseNote's `corelib/verovio`, three: two voices written
with a voice change and no backup read the same layers, notes and MIDI as the same voices written one after the other
with a backup, on one staff and on two, each layer two half notes; a note of the current voice on another staff stays
in its layer with its staff; a note of another voice on a staff its voice has no layer on joins the voice's layer,
drawn there, the same as written voice by voice. The first and third fail on tb.43; three breaks caught (tb.43's early
return; every voiced node placed by its staff; no layer of the voice on another staff looked for).

**Upstream status**: not offered.

### 94. A multi-bar rest's own barlines and chord symbols are read with it (tb.43)

Found by MuseNote plan V3_2_0_29's export round trip (root causes RC110 and RC111; findings W43, W95, W96). Two faults
of the MusicXML reader's handling of a multi-bar rest, which is one MEI measure for all its bars:

- On the bar the rest begins in, the reader built the rest at the `<attributes>` that hold its `<multiple-rest>`,
  read what followed them for directions and sounds alone (`ReadMultiRestMarks`) and stopped reading the bar.
  MusicXML lets a bar hold its elements in any order: a barline written after those attributes was lost (a repeat
  start, an ending, a one-bar rest's right barline: MuseNote's own export, writing the attributes first, played Afro
  Blue's last repeat from the top), and a chord symbol (Afro Blue writes Fm7 and F♯7 there, over three rests).
- The rest's last bar's barlines were read after the rest's measure was placed in the order of play (`AddMeasure`):
  what they say of the order, a repeat end or an ending's stop, was left pending and applied to the bar after the
  rest. A score whose rest ends a repeat repeated the bar after it.

`ReadMultiRestMarks` now reads the rest's first and last bars' barlines, wherever the bar writes them, and its first
bar's chord symbols (where they would stand before the rest's attributes), before the rest's measure is placed; the
read after it in `ReadMusicXmlPart` is gone. A chord symbol of a later bar has no place in the rest's one measure: it
is left out with a warning.

Parity: 4 of 22,674 outputs differ, all of Afro Blue, the corpus's only file with a chord symbol after a rest's
attributes: its MEI, its page and its two locator MEIs gain Fm7 at bar 50 and F♯7 at bars 66 and 70, and the generated
ids after them renumber; its MIDI, timemap and pitch map are unchanged. No file of the corpus writes a barline after a
rest's attributes. Afro Blue's rest that ends ending 1 with a repeat (bar 32) is now placed with the rest; it played
right before, because bar 33's own ending took the pending stop over, and its sections are the same. 19 other songs
end a rest's last bar with a barline of style alone, set on the measure at once either way. Seeded ids 3,177 of 3,177;
read back from MEI 3,571 of 3,571.

**Cases**: `test/native/test_tb43_rest_marks_engine.cpp` in MuseNote's `corelib/verovio`: one score written two ways
(the rests' barlines and chord symbol before their attributes, and after) reads the same MEI but for its generated ids
and the same MIDI: the two-bar rest's measure with its repeat start and end and the chord symbol, the one-bar rest's
measure with its double barline, the rest repeated (the last note at quarter 24; on tb.42, 20 written one way, 32 the
other); a chord symbol in a rest's second bar is left out and the log says so. Both fail on tb.42; four breaks caught
(no barline read there, the last bar's read after the measure is placed, no chord symbol read, a later bar's read).

**Upstream status**: not offered.

### 93. A chord's beam is read once (tb.42)

Found by MuseNote plan V3_2_0_28's export round trip (root cause RC109; finding W39). The MusicXML reader read a
beam's begin and end from every note of a chord, where the marks are the chord's. Written on each of its notes
(Notation Composer does; MusicXML allows it), a begin read while the chord was open started a beam inside the chord
(`ReadMusicXmlBeamStart` refuses a second beam only when a beam is on top), the chord's next note found no chord on
top to join ("Chord starting point has not been found") and the bar's later notes were lost with it, and an end with
nothing left open went to `CloseBeamSpan`: Love Is Strange kept one note of 21 in each of four bars, the bar marked
not full, with beam spans pointing at notes it dropped. A break in the second beam was set on every note of the chord
besides the chord. A chord's marks are now read once, at its first note, from the first of its notes that carries any
(`BeamMarksOf`): the begin the beam is built from, the begin a tremolo counts its beams by, the end and its break. A
later note of a chord reads none. Where a beam and a tuplet start on one note, their nesting still compares the marks
where the file writes them, as it does the tuplet's stop.

Parity: 7 of 22,674 outputs differ, all of Love Is Strange, the corpus's only file that writes a chord's marks on more
than its first note (20 chords; 1,953 beamed chords carry them on the first note only): its page, MIDI, timemap, pitch
map, MEI and its two locator MEIs. Its four bars read their 7 chords and 21 notes each, full (342 notes in all, now
422; 338 played, now 418); 8 beam spans fewer (82, now 74), and the generated ids after bar 2 renumbered. The spans
left are the file's: 77 first beams it never ends in their bar, and 31 ends of the first beam while it marks a higher
one "continue", read as a break of the second beam (a fault of the file, which no reading of a correctly marked file
reaches). Seeded ids 3,177 of 3,177; read back from MEI 3,571 of 3,571; locator 0 failures.

**Cases**: `test/native/test_tb42_chord_beams_engine.cpp` in MuseNote's `corelib/verovio`: a bar of beamed chords and
a bar whose second beam breaks, each with the marks on every note, on the first only and on the second only: every
note read, in its beams, the bar full, no beam span, the break on the chord alone. Both fail on tb.41; four breaks
caught (a later note reading its marks again, the marks read from the first note only, the end read from each note,
the begin read from each note).

**Upstream status**: not offered.

### 92. Every movement plays its own order of play (tb.41)

Found by MuseNote plan V3_2_0_62's device check (root cause RC107; finding W94). `Doc::ExpandExpansions` generated an
`<expansion>` for every `<score>`, then followed the first it found in the document: a document of several movements,
an ABC tune book among them, played the others as written, their repeats and jumps left out (two tunes, each repeated,
every movement asked for: 64 notes played of 82). Each score's own order is followed, the first `<expansion>` in it;
their costs are counted together against patch 84's bound (the document as played). An order named (`expand`) is
followed alone, as before.

No output of the parity set changes (22,674 outputs, 0 differ): every score of it holds one movement.

**Cases**: `test/native/test_tb40_mei_reads_back_engine.cpp`: two tunes, each repeated, play 16 notes from the file and
from its MEI, every movement asked for; the break (the first score's order alone) caught.

**Upstream status**: not offered.

### 91. Nested slurs are laid out in the document's order (tb.40)

Found by MuseNote plan V3_2_0_62's round-trip gate (root cause RC106; finding W93). `AdjustSlursFunctor` adjusted
each outer slur to its inner slurs walking a `std::map` keyed by the slurs' positioner pointers: in the order of their
memory addresses. Where slurs nest, the order decides where they go, so a score was laid out otherwise from one load to
another: Imagine (Humdrum), its MEI loaded while the file's toolkit was alive, put line 2 71 units lower than the
file's own page. The outer slurs are taken in their positioners' order, the document's.

No output of the parity set changes by it: a dump's fresh process gives the positioners rising addresses, the order
they are made in.

**Cases**: `test/native/test_tb40_mei_reads_back_engine.cpp` in MuseNote's `corelib/verovio`: Imagine laid out with the
engine's objects given rising, then falling addresses from an arena of the test's own; the break caught.

**Upstream status**: not offered.

### 90. A number is written so that it reads back as itself (tb.40)

Found by MuseNote plan V3_2_0_62 (root cause RC104; the audit's W32). `Att::DblToStr` and `Att::MeasurebeatToStr`
wrote every number rounded to four decimals, through a stream of six significant digits: a beat of 1.21875 was written
"1.2188" and read back as another beat (138 corpus songs carry beats written to four decimals; in 5 a line drawn from
the MEI moved by a unit). Each number is written as the shortest decimal that reads back the same (15 significant
digits, else 16, else 17), in the classic locale and in plain form.

Parity: the MEI of 421 outputs changes, 1,363 lines, numbers only (a beat of 2 and 11/12 is "2.9166666666666665"
where it was "2.9167"). No SVG, timemap, MIDI or pitch map changes by it.

**Upstream status**: not offered.

### 89. The MusicXML reader holds a text as MEI holds it (tb.40)

Found by MuseNote plan V3_2_0_62 (root cause RC103; the audit's W32, "a text read back from MEI is divided into runs
otherwise"). The reader held texts in forms MEI cannot hold, so the same score read back from its MEI was drawn
otherwise: texts side by side (a tempo's "(", " = ", its number and ")"), an empty text (an empty lyric), and white
space with a line break at a text's ends (a lyric "nel" and a newline), which the MEI reader takes as the file's
layout. At the end of the import the texts of every element are joined where they stand side by side, an empty one is
removed, and the white space with a line break at the start of the first or the end of the last is taken off. That
rule, the MEI reader's, is one pair of functions now (`LeftTrimLaidOut`, `RightTrimLaidOut` in `vrv.cpp`), used by
both readers.

Parity: the page of 282 scores (183 by their tempo marks, 98 by an empty lyric, 1 by both), their runs and boxes, no
mark moved; 4 more where a lyric ended in a line break, their bars as wide as their notes need; the MEI of 7 songs
(21 outputs), the line break gone from a lyric. No timemap, MIDI or pitch map changes.

**Upstream status**: not offered.

### 88. A document holds each id once (tb.40)

Found by MuseNote plan V3_2_0_62 (root cause RC102; the audit's W32). Ids came from one counter per thread, which a
seed sets back, and nothing kept a document from being given an id it already held:
- a load under the seed its file's ids were made with (MuseNote's reader loads its own MEI with `xmlIdSeed` 1) gave
  the objects it makes without an id (texts, stems, measure numbers, lines, pages) ids the file gives to other
  elements. Of the 3,157 corpus songs, 3,138 MEI views held ids twice on their pages, a note's among them in 2,964:
  the app gave such a note another element's box, and a tie, a connector or an ending's bracket was drawn into another
  element's group;
- another document's seeded load on the thread (a thumbnail, an import, a print) did the same to a document's later
  layouts, and a thread that had built nothing reseeded at random on its first object.

`MEIInput::Import` reads its file's ids first and moves the counter past those of the generated form
(`Object::ContinuePastIDs`: each read back to its place in the sequence with the hash's inverse, the counter to the
start of the widest stretch none is in). Each document keeps its own place in the sequence (`DocIDScope`, swapped in by
the outermost of a load, a re-layout, the drawing of a page, the document for playback, the export and an edit;
`Object::ContinueIDs` marks the thread seeded). The ids a file gives are kept.

Parity: the ids of 20 MEI inputs' own objects on their pages (nothing else on them), and of the order of play the MEI
of 20 Humdrum scores writes (patch 75's copy, now on the document's own place). Annotation locator: 0 failures (a
million notes). Seeded ids on a fresh thread: 3,177 of 3,177.

**Cases**: `test/native/test_tb40_mei_reads_back_engine.cpp`: a score's MEI loaded again with the seed holds each id
once and keeps its notes' ids; a re-layout after another toolkit's seeded load holds each id once; a re-layout on
another thread gives the loading thread's ids; two seeded loads of one MEI give the same ids. Three breaks caught.
The parity gate gains `roundtrip/status` (MuseNote's `tool/parity/roundtrip.cpp`): every input read back from its MEI
as it was, each id once on every page, after another toolkit's seeded load too: 3,571 of 3,571.

**Upstream status**: not offered.

### 87. An order of play given among other orders is followed (tb.39)

Found by MuseNote plan V3_2_0_61 (root cause RC98; the audit's finding
W31). A score may give more than one order of play: an MEI `<choice>` of
expansions, or Humdrum's `*>[A,A,B]` beside `*>norep[A,B]`, which the
Humdrum reader holds as `<choice><orig>` (the main list, whichever the
file gives first) `<reg>` (the others). `ExpansionMap::Expand` looked for
the parts an order names under the order's own parent, the `<orig>`; the
parts stand beside the `<choice>`, none was found, and the score played
its bars once (MuseNote worked round it by loading every Humdrum score
twice more, from a copy with its other orders blanked: RC99). The order's
scope is now its parent or, where that is an editorial element that holds
none of the parts the order names, the nearest ancestor that does. An
order inside a reading that holds its own parts keeps the reading. The
order played is still the one the engine shows (the first visible: the
`<orig>`).

No output of the parity set changes (22,674 outputs, 0 differ; seeded ids
on a fresh thread 3,177 of 3,177): no corpus file gives two orders. One
plain load plays what a load with `expandAlways` plays, timemap and MIDI
byte for byte, for the 299 Humdrum inputs at hand.

**Cases**: `test/native/test_tb39_order_among_orders_engine.cpp` in
MuseNote's `corelib/verovio` (3 cases); the rule broken on purpose, the two
cases with two orders fail.

**Upstream status**: not offered.

### 86. The document for playback reads the score as loaded (tb.38)

Found by MuseNote plan V3_2_0_59 (root cause RC94), measuring the order of
play. Since patch 75 the engine's MEI holds the order a score is played in,
where no reader wrote one (`WithOrderOfPlay`), and that order is written on
a copy, on ids from the thread's counter, which is set back after. The
document for playback (`Toolkit::SetMidiDoc`) read that MEI straight after,
and gave its own new objects (the accidentals it makes of `@accid.ges`) the
same ids: an order's sections and the order itself shared their ids with
accidentals. Nothing was played wrong, the sections standing first, but a
document holds each id once, and patch 84's lookup of what an order names
walks the document for an id held twice. The document for playback reads
the score as loaded (a private `GetMEI` with the order left out) and works
the order out itself, by the same rule, as it did before patch 75.
`ExpansionMap::FindWritten` says it once for an id an order names that two
elements hold.

**Upstream status**: not offered (patch 75 is ours).

### 85. The verses of a layer in one walk (tb.38)

Found by MuseNote plan V3_2_0_59 (root cause RC92; the audit's finding W7).
`Doc::PrepareData` resolved the lyric connectors with a walk of the whole
document for every staff, layer and verse, and `Page::AdjustSylSpacingByVerse`
spaced the syllables with a walk of the page for each verse: a Humdrum
`**vvdata` token of 3,000 words (a verse for each word) took 17 seconds by
the square of its words, and patch 31 (tb.14) kept a token of more than 64
words as one verse to stay clear of it, otherwise than the format says.

- `PrepareLyricsFunctor` walks each staff and layer once and follows each
  of its verses on its own (the current syllable of each verse number): the
  notes it tracks are the same for every verse of a layer, and a syllable in
  no verse is followed in each, as each walk met it.
- `RecordSylSpacingFunctor` records in one walk of the page what the walk of
  each verse met (the systems and the ends of the measures, the staves of
  its number, its verses), and each verse is spaced in turn from its record,
  as before: the verses still move the measures one after the other.
- Every `**vvdata` token is split into its words (`kMaxVvdataVerses` goes).

3,000 words now take 0.1 s, 10,000 0.4 s (never came back on tb.13).

**Upstream status**: not offered; `develop` of 2026-10-08 walks each verse.

### 84. An order of play is followed as far as the score as played can be held, and a score that cannot be played is not played (tb.38)

Found by MuseNote plan V3_2_0_59 (root causes RC91 and RC93; the audit's
findings W6 and W7).

- **One document for playback, or none (RC91).** Where the engine could not
  follow a score it played the score as written, every repeat and jump
  dropped: the MIDI document could not be read back from the score's own
  MEI (patch 7), or its order was past the bound (patch 16). The warning
  that said so was erased by the caller's next `ResetLogBuffer`. Such a
  score is now drawn and not played at all: `SetMidiDoc` returns false and
  keeps the reason, every caller resets its log first and returns what it
  returns for nothing (`RenderToMIDI` and `RenderToTimemap` an empty string,
  `RenderToTimemapFile` false and no file, the pitch, time and expansion
  lookups empty), and logs the reason ("The score cannot be played: ...").
  `Doc::ExpandExpansions` marks an order past the bound as refused
  (`ExpansionMap::IsRefused`).
- **The copies of an element share one list (RC93).** `ExpansionMap` kept,
  under every copy's id, its own list of all the copies, and every list was
  written to at each copy: k copies cost k times k. One note repeated 2,000
  times peaked at 839 MB, 16 notes at 5 GB. Each id now names its group
  (`m_groupOf`, `m_groups`): the same lists, in the same order, written once;
  the engine reads `FirstOf`, `LastOf` and `CountOf`, and the toolkit's two
  calls copy a list. 2,000 times one note: 45 MB.
- **The parts an order names are found where they stand (RC93).** `Expand`
  looked each one up in the whole document (`FindDescendantByID`), which
  grows with every copy, and searched the parts played so far as a list:
  200 repeat ends in a row took 21 seconds. The objects of the document are
  recorded by their ids before it is expanded (`RecordWrittenIds`, every one
  and those a walk of what is shown meets), with every object `Expand` makes
  (`RecordMade`); a part is found where it stands under the expansion's
  parent, and an id held twice or under something hidden is looked for as
  before. The parts played are a set. The place of a new container is found
  by `prevSect`'s ancestors (`GetDirectChild` searched the whole parent
  first: 2 to the 14th plays took 13 seconds, now 2.5).
- **The bound is the elements the score holds as played (RC93).** The cost
  was each section by the times it is played, squared, against 1,000,000;
  past it the order was dropped. It is each section's elements by the times
  it is played, against `OrderOfPlay::s_maxCost` = 100,000: the dearest of
  MuseNote's 3,571 scores and inputs holds 7,819 (Piano Man); 100,000 take
  some 350 MB and 2.5 seconds on a desktop. The writers of an order stop at
  one element a reference, past the bound (`OrderOfPlay::Add`, and the MEI
  to Humdrum converter's own count). A repeat is played as often as it says:
  the clamps at 100 go (a MusicXML `times`, an ABC repeat's colons, an ABC
  `P:` count, the order rule), and the ABC `P:` field is read until it names
  more than the bound, not cut at 2,000 letters.

**Upstream status**: not offered; `develop` of 2026-10-08 keeps a list for
every copy and plays a document it cannot read back without its order.

**Gates of tb.38** (patches 84 to 86): the native suite, 368 cases with
asserts in Release and under the address sanitizer with libc++ hardening;
the new cases fail on tb.37 (6 of 8; the other two guard what stays as it
was, and their breaks are caught), and 12 deliberate breaks, each caught:
the refusal, the vvdata split, the MusicXML and the two ABC counts, the
read of the score as loaded, a syllable in its own verse, the walk of the
verses (reading and spacing), the copies' lists, the lookup of a part, the
place of a container, and the linear count.
Parity tb.37 to tb.38: 22,674 outputs over 3,177 scores and inputs, 0 differ (no
score of the corpus reaches a limit these patches change); seeded ids on a fresh
thread, 3,177 of 3,177. MuseSync's 24 starters give the same timemap and MIDI.

### 83. The pieces of a jump written apart in its bar are its print (tb.37)

Found by MuseNote plan V3_2_0_56 (root cause RC80), tracing the songs the guards
refused. "D.S. § al Coda ⊕" is often written as several directions in one bar:
the words "D.S.", a segno sign, the words "al Coda", a coda sign. A sign with no
sound of its own there was read as a target (a segno in the D.S.'s own bar; a coda
the To Coda jumps to inside the bar it returns from), and the words "al Coda" alone
as a To Coda (patch 80, which reads "Al Coda" as the To Coda some editions write).
A D.S. never lands in its own bar, nor a To Coda in the bar of the jump back. In a
bar that holds a D.C. or D.S. (by its sound or its words), a segno or coda sign
with no sound of its own and the words "al Coda" alone are now that jump's print
(`MeasureHoldsJumpBack`, `IsTailOfJump`): drawn, and no mark of the order. "Al
Coda" in a bar with no jump back is a To Coda, as in patch 80.

### 82. A Fine that names its times ends the play in that pass (tb.37)

Found by MuseNote plan V3_2_0_56 (root cause RC79). MusicXML gives any sound
particular times through a repeat (`time-only`), and a Fine with them ends the
play in that pass. The reader kept a Fine as a bare flag, and the order ended the
play at a Fine only after a jump back: a Fine meant for the last time through a
repeat, in a song with no D.C. or D.S., could not be said in a file at all. The
reader keeps a Fine's times (a type token of its MEI, as a jump's), and the order
ends the play after the Fine's part in a pass it names. A Fine with none ends it
after a jump back, as before.

### 81. Jump words that say more: "with repeat" plays the repeats again, "2nd time to Coda" is for that pass (tb.37)

Found by MuseNote plan V3_2_0_56 (root cause RC75, its second part). Words a jump
is written in can say what MusicXML says with `after-jump` and `time-only`. "With
repeat", "(w. repeat)", "play repeat" after a D.C. or D.S.: the repeats are played
again after the jump (`JumpInfo::m_playRepeats`, the type token "repeats" in the
MEI, read back by the order rule). "2nd time to Coda", "at 5th verse go to Coda":
a To Coda for the pass its words name (the times token, as a sound's
`time-only`). The reader passes the times and the repeats of words on as a sound's
(`PlayJumpOfWords`). A wording that names what the order cannot hold (a lyric
verse, "Repeat C for solos") is not guessed at.

### 80. Jump words of other spellings and languages, and a jump with an instruction that is no part of the order (tb.37)

Found by MuseNote plan V3_2_0_56 (root cause RC75). `JumpOfWords` (patch 66) read
whole phrases of MuseScore's vocabulary. It now also reads a comma for a dot
("D,S,al.fine"), a phrase in brackets ("(fine)", "(Per la coda)"), "al" written
"a" or with a dot ("D.S. a Coda", "D.C.al.Coda"), a To Coda in other languages
and editions ("Zur Coda", "Per la coda", "Al Coda"), and a D.C. or D.S. followed
by an instruction with no part in the order ("D.S. and fade", "to fade", "vamp
and fade on E", "D.C. (to Verse 2)"): that jump. Each is a whole phrase of a stated
form.

### 79. A D.C. or D.S. in an ending is taken the last time the ending is played (tb.37)

Found by MuseNote plan V3_2_0_56 (root cause RC74). A D.C. or D.S. in an ending
that names no times was dropped: only the jump of the endings' last part was read,
after all of them, and the play went back to the repeat's start as if the endings
were a repeat (Pistol Packin' Mama: a D.S. in its first ending went to bar 1, not
the segno). It is taken the last time the ending is played. Where the ending has
no repeat sign of its own it is the ending's way back; where it has one, the
repeat comes first, then the jump (MuseScore's practice, the owner's choice; a
file that names its times is followed).

Cases (patches 79 to 83): `test_tb37_refused_songs_engine.cpp` (8 cases): a D.C.
or D.S. in an ending with no repeat sign (Pistol Packin' Mama's and Joshua Fit The
Battle Of Jericho's shapes) and in an ending for two passes; a D.C. at a repeat's
end after the repeat; jump words of other spellings and languages; a jump followed
by an instruction, and what is no jump left as words; "with repeat" (and the score
read back from its MEI); a To Coda for the pass its words name (and read back); a
Fine that names its pass (and read back); the pieces of a jump written apart in its
bar (Hero's, Nellie the Elephant's, Every Little Thing She Does Is Magic's and
Ironic's shapes, the marks of the MEI counted). One case of
`test_tb31_jumps_in_words_engine.cpp` turns round ("D.S. and fade" is that D.S.).
Each new case failing on tb.36. Breaks, each caught (13): the jump in an ending
not taken; taken before its last pass; a comma not read as a dot; brackets kept;
the To Coda of other languages not read; the instruction after a jump not read;
"with repeat" not read; a pass to coda not read; the MEI's "repeats" not read back;
a Fine's times not kept; the play not ended at a timed Fine; a sign beside a jump
back read as a target; "al Coda" beside a jump back read as a To Coda. Native suite
360 of 360; with asserts and the address sanitizer the same; the engine with
asserts over every live song and the cases (2,771 of 2,771). Log-format audit: 0
calls. Parity tb.36 to tb.37: of 22,191 outputs, 264 differ, in 48 songs, each a
song whose jump words or jump in an ending these patches read. 25 play otherwise,
each judged by MuseNote's song guards: 16 they refused on tb.36 pass, 4 they now
refuse lacked in their files a target the words ask for (corrected in MuseNote's
song list, or kept by the owner as they play), the others they refused before as
after. The MEI and page of the 23 more change (words typed, signs print). 45 keep
every note id; in 3 (Hero, Ironic, Every Little Thing She Does Is Magic) generated
ids move where a sign becomes print, every note in its place (the annotation
locator: 0 failures, 479,936 of 480,161 generated ids kept). Seeded ids 3,108 of
3,108. MuseSync's songs: every note the same, all 33. MuseNote's guards:
`bar_times.py` passes (finding W73 as before, 44 songs), `endings_whole.py` names
the same 10 files. MuseNote's export gate over list 9, with its export of a jump
"with repeat" (MuseNote RC81): no song worse in any format (MusicXML 2,756; ABC by
abc2midi 2,338, six named apart; ABC by the engine 2,610; Humdrum 218, finding W79
as before). The song guards over list 9 refuse 152 songs, against 164 on tb.36.

**Upstream status**: not offered (patches 79 to 83).

### 78. A document read back from its own MEI keeps the work its music is of (tb.36)

Found by MuseNote plan V3_2_0_55 (root cause RC73), in parity of patch 75. The
MEI writer writes the work the music is of as `<music decls>` (`Doc::m_musicDecls`,
set by the Humdrum reader), and the MEI reader did not read it: a document read
back from its own MEI lost it (patch 75's copy of 20 Humdrum test files; the MIDI
document; MuseNote's transposition, which reloads the score's MEI). The reader
now reads it.

### 77. A To Coda written as a coda of type "toCoda" is a To Coda (tb.36)

Found by MuseNote plan V3_2_0_55 (root cause RC72, finding W80). MEI has no
`repeatMark` function for a To Coda. MuseNote's editor writes one as a coda
refined by its type (`func="coda" type="toCoda"`), and MuseNote's exports read
it so; the order rule (`MarksOf`, every format) read every coda `repeatMark` as a
coda sign, and knew a To Coda only as the MusicXML reader writes it (`<dir
type="tocoda">`). A D.S. al Coda made in MuseNote never went to its coda (the
editor's segno, To Coda, D.S. al Coda and coda played 1 2 3 4 2 3 4 5). A coda
`repeatMark` whose type's first token is "toCoda" (any case) is now a To Coda
jump, with its label and its times; one with no such type is a sign as before.

### 76. The parts of an ending a sign or a repeat divides keep their order through Humdrum (tb.36)

Found by MuseNote plan V3_2_0_55 (root cause RC70). Since tb.35 (patch 74) such
an ending holds its parts as sections, and the order names the parts. The MEI to
Humdrum converter labelled only the parts standing directly in the score's
section, and wrote no order at all when the list named anything else: the
export of such a score was played once through read back. The converter now
labels each part the order names: the first with the ending's label, each other
with it, its place in small letters and the ending's number again (`*>A2b2` after
`*>A2`), so that the label ends in the number and starts with the section's
letters. The Humdrum reader reads such a label, of the ending's number, as a
part of that ending: a section inside it, the ending's bars before it its first
part (the ending's own id is then its label's with "-ending"). Read back, the
ending holds the same bars in the same parts and plays the same order. A reader
that follows only the list (humlib's `thru`) plays the labels as the order says.

### 75. A score with no order of its own is given out with the one it is played in (tb.36)

Found by MuseNote plan V3_2_0_55 (root cause RC68, finding W50). Every export of
MuseNote's starts from the engine's MEI of the score (`getMEI`). For a score with
no `<expansion>` (made in an editor, an MEI file), the one rule (tb.21) worked
the order out only to play it, and the MEI given out held none: the Humdrum
converter, which writes an order only from an `<expansion>`, wrote a D.C. as its
words, and read back the score was played once through. `Toolkit::GetMEI` (a
whole score, score-based, no filter) and `Toolkit::ConvertMEIToHumdrum` now pass
a score with no order through a copy (`Toolkit::WithOrderOfPlay`): read with
`Doc::m_writeOrderOnly`, so that `ExpandExpansions` writes the one rule's
`<expansion>` where a score has none and plays nothing out, and written out
again. The loaded score, its drawing and its ids are untouched, and the thread's
id counter is set back after the copy (`Object::GetIDCounter`,
`SetIDCounter`). A score with an order of its own, or one whose order cannot be
worked out (more than one section at its top, editorial content), is given out
as it is.

Cases (patches 75 to 78): `test_tb36_order_written_engine.cpp`: a score made as
MuseNote's editor makes it with a D.C. al Fine, a D.S. al Coda (patch 77) and
endings beside loose bars, given out with its order and through Humdrum played as
the score; one with an order of its own given out with it (alone, and beside a
second score with none); the loaded score drawn with the same ids after it is
given out; a score whose order cannot be worked out given out as it is; an ending
divided by a coda sign (first and second ending) and by a repeat, through Humdrum
the same bars in the same parts and the same order; `<music decls>` kept, with an
order of its own and without. Three cases of `test_mei_to_humdrum_engine.cpp` turn
round (a score made in MuseNote, with endings or a D.C., is exported with its
order: MuseNote finding W50). Each new case failing on tb.35. Breaks, each caught
(13): getMEI giving no order; the converter given none; the id counter not set
back; an order written over one of its own; the copy played out; a To Coda read
as a coda; the list not naming a part; no part recorded; a part's label not
written; a part's label without the number; the reader keeping no part; the first
part not its own section; `@decls` not read. Native suite 352 of 352; with asserts
and the address sanitizer the same; the engine with asserts over every live song
and the cases (2,771 of 2,771). Log-format audit: 0 calls. Parity tb.35 to tb.36:
of 22,191 outputs, 20 differ, the MEI of the 20 Humdrum test files with repeats,
each given its order (one `<expansion>` and the sections it names), every other
line and every note id the same; the 105 others are the 15 songs published since
(list 10), new. Over every live song with its order taken out (2,765, as a score
made in an editor is), the MEI given out is tb.35's but for the order (1,633 are
given one), no element id moves, and every one plays the same. Seeded ids 3,108 of
3,108. MuseSync's songs: every note the same, all 33. MuseNote's export gate over
list 9: no song worse in any format (MusicXML 2,756; ABC by abc2midi 2,335, six
named apart: abc2midi plays four passes at most, MuseNote RC67; ABC by the engine
2,608; Humdrum, judged now, 218 songs that play otherwise, against 221 on tb.35,
listed as MuseNote finding W79).

**Upstream status**: not offered (patches 75 to 78).

### 74. An ending a sign divides stays whole, and a repeat inside an ending is played (tb.35)

Found by MuseNote plan V3_2_0_54 (root causes RC62 and RC65, finding W63). Where a
sign (segno, coda) or a repeat sign inside an ending starts a new part of the order
of play, the bars on one side were moved out of the ending: `WriteOrderOfPlayFor`
(ABC, MEI) put the first part in a section before the ending, and the MusicXML
reader began a plain section at the sign. The bracket stopped at the sign, and read
from MusicXML a first ending's bars after a sign were played in the second pass
too. The parts now stay in the ending as sections inside it (MEI 5 allows an
ending to hold sections): the reader gives a part begun inside an open ending, at
a sign or after a repeat sign, that ending's passes, and builds one ending element
for an ending's parts; `WriteOrderOfPlayFor` puts a part that does not fill its
ending in a section inside it; the expansion keeps an ending whose parts are
played, and places a copy after the ending the part before it stands in.
Building it found that the order had no repeat inside an ending (RC65): a part
beginning a repeat there took the ending's passes, its closing sign was read as
the ending's own, and the ending did not go on after it. A repeat begun and
closed inside an ending is now the ending's own, played its times in each pass
the ending is played; its sign does not send the play back for the next pass.

### 73. A decoration written before an ABC bar line is the bar line's (tb.35)

Found by MuseNote plan V3_2_0_54 (root cause RC61, finding W63). ABC 2.1, 4.14:
decorations may be applied to bar lines. The ABC reader kept only a jump written
after a bar's last event and before its bar line at the bar's end; any other
decoration (segno, coda, fermata...) waited for the next note, in the next bar.
Every decoration so written is now the bar's last event's, at the end of the bar
the line closes. Four's coda, read back from its ABC export, no longer divides its
second ending.

Cases (both patches): `test_tb35_ending_kept_whole_engine.cpp`: a coda sign inside
a second ending and a segno inside a first ending (the ending holds all its bars;
the first ending's bars after the sign played in its pass only; the score played
out puts a copy after the ending), a repeat inside an ending, closed at its end
and closed with the ending going on (each MusicXML score and its MEI twin alike),
and ABC: `!coda!|`, `!segno!|`, `!fermata!|` in the bar the line closes, one
before a note unchanged, Four's shape, a coda inside an ending. Each failing on
tb.34. Breaks, each caught (9): only jumps at the bar's end; a part begun in an
ending not the ending's; no one ending for its parts; the part after a repeat sign
in an ending not the ending's; ABC and MEI moving a part out of its ending; the
expansion removing an ending as unplayed; a copy put inside an ending; no repeat
inside an ending; an ending not going on after its own repeat. Native suite 346 of
346; with asserts and the address sanitizer 346 of 346; the engine with asserts
over every live song and the cases (2,771 of 2,771). Log-format audit: 0 calls.
Parity tb.34 to tb.35: 19 of 22,086 outputs differ, in 4 scores, each judged:
Everything, Livin' La Vida Loca and Till There Was You (files whose "2." starts on
the repeat sign's bar, now held as written; their files are corrected, MuseNote
RC63), and one ABC export (I Guess That's Why They Call It The Blues: a repeat
opened at its second ending, `::`, is played; its bars played 63 on tb.34 and 70
now, the song's 71). The annotation locator: 0 failures, every note id kept.
Seeded ids 3,093 of 3,093. MuseSync's songs: every note the same, all 33.

**Upstream status**: not offered (patches 73, 74).

### 72. A part played again plays under its own score definitions (tb.34)

Found by MuseNote plan V3_2_0_52 (root cause RC58, finding W61). The expansion
(`ExpansionMap::Expand`, every format) copies a section or an ending after the
bar the play comes from; a score definition written before the part, outside
it, or at the score's start, was not copied with it, so a part played again kept
the time signature (or key, clef, transposition) it arrived with: a whole-bar
rest written in 4/4 before an ending played two beats on its later passes (Those
Were the Days v2), a 3/4 first bar reached again from a 4/4 bar played four
(Caresse sur l'océan). Before expanding, the definitions in effect at the start
and the end of each part as written are recorded (the score's own first
definition included, with every scoreDef, staffDef and clef, key or time
signature in a layer); where the order places a part after another bar than the
one written before it, what of its own differs from what is in effect there is
said again just before it, in a score definition whose ids are the part's with
`-def` and `-def-N`. The played-out score shows the change where it happens.

### 71. A bar of empty space plays its meter (tb.34)

Found by MuseNote plan V3_2_0_52 (root cause RC57, finding W61). The engine
timed the whole-bar elements by the meter and left `mSpace` out, so a bar of
empty space (the MusicXML reader's form of a hidden whole-bar rest) was played
for no time (Green Onions, Fatback). MEI's events that completely fill a
measure are `mRest`, `mSpace` and their kin: an `mSpace` is timed and aligned
as an `mRest` (`layerelement.cpp`, `alignfunctor.cpp`).

### 70. A rest with no type is a whole-bar rest only where its ticks fill its bar (tb.34)

Found by MuseNote plan V3_2_0_52 (root cause RC56, finding W61). The MusicXML
reader read a rest with no `<type>` as a whole-bar rest even where its ticks did
not fill its bar; in a bar shorter than its meter the engine plays a whole-bar
rest for no time (What Makes You Beautiful, bar 38). Now it is a whole-bar rest
only where its ticks are its bar's by the meter (as `measure="yes"`); otherwise
a rest of the value its ticks make, or, where no value makes them, the longest
value under them in a tuplet that shows nothing for the ratio left. A hidden one
that does not fill its bar is a gap of its ticks.

### 69. A mark placed past its bar's end is placed where its time falls (tb.34)

Found by MuseNote plan V3_2_0_52 (root cause RC55, finding W61). The MusicXML
reader gave a direction or chord symbol whose `<offset>` takes it past its bar's
end a time stamp past the bar (MEI's are from 0, the left barline, to the right
one), and the engine moved the bar's end to it (`MeasureAligner::GetAlignmentAtTime`
→ `SetMaxTime`): the bar played its notes and then waited (Wave: eight bars,
about 63 beats of silence; Breathless v2, I Know Where I'm Going, Petit Papa
Noël). The reader records each bar's right barline as a time stamp and the unit
of its meter; at placement a mark past its bar's end goes into the following
bar its time falls in, at its beat, as it already times a mark inside its bar
by its offset; past the last bar, at its right barline. One placed before its
bar's start (a negative offset) goes back to the earlier bar its time is in;
before the first bar, at its left barline. A jump or a sign stays in its own
bar at its barline: its sound takes effect where the file gives it. A mark
that spans keeps its end, counted from the bar it now starts in.

Cases (all four patches): `test_tb34_bar_lengths_engine.cpp`: a chord symbol
past its bar, mid-score, in the last bar, and from a 4/4 bar into a 6/8 one;
words bars past their own, and before their bar's start, and before the first
bar; a jump past its bar (the score and its MEI twin alike); a rest with no type
of two beats, of one beat before a dotted half, of five beats (no value makes
them), hidden, and one that fills its bar (must not change); a bar of empty
space in 4/4 and 3/4; a 3/4 bar repeated after a 4/4 one, Those Were the Days'
shape (the restated meters in order, 2/4 4/4 2/4 4/4, none where the 4/4 is
written), a clef restated before a repeated bar. Each failing on tb.33. Breaks,
each caught: no placement by time; a jump carried; the next bar's meter
ignored; no carry back before a bar's start; a typeless rest whole-bar again;
a hidden one a bar of space; no ratio for ticks no value makes; mSpace timed
for nothing; nothing restated; the score's first definition unread; the
definitions between two parts not counted; copies not traced to their part.
Native suite 342 of 342; with asserts and the address sanitizer 342 of 342;
the engine with asserts over every live song and the cases (2,768 of 2,768).
Log-format audit: 0 calls. Parity tb.33 to tb.34: 88 of 22,086 outputs differ,
in 42 scores, each judged: the songs of RC55 to RC58 and their superseded
versions play their bars at their files' lengths (Wave, Breathless and v2, I
Know Where I'm Going, Petit Papa Noël, What Makes You Beautiful, Green Onions,
Fatback, Those Were the Days and v2, Caresse sur l'océan); 24 scores add a time
signature to their MIDI where a part played again changes meter (their notes
the same); marks before their bar's start move into it or an earlier bar
(Always Look On The Bright Side Of Life, Non, je ne regrette rien and v2);
bars of empty space are laid out as whole bars (All Through the Night's last
chord symbol, drawn past the final barline, now stands in its bar; Goldfinger
and v2 and an MEI fixture pixel-identical, the drawing's own generated ids of
systems and their clefs moved). The annotation locator: 0 failures, every note
id kept (477,334). Seeded ids 3,093 of 3,093. MuseSync's songs: every note the
same, all 33. MuseNote's guard `bar_times.py`: over list 9, rules 1 to 3 name
56 songs on tb.32 and none on tb.34; its census names 44 (finding W73, open).

**Upstream status**: not offered (patches 69 to 72).

### 68. A bar is the bar it was read in, not its number (tb.33)

Found by MuseNote plan V3_2_0_51 (root cause RC54, finding W59). A MusicXML bar's
number is a token, and it may repeat in a part (MusicXML 2.0 files from Sibelius 6
number a section from 0 or 1 again). The MusicXML reader filed every control
element (chord symbols, words, dynamics, ties, slurs, fermatas, ornaments, pedals,
octave lines, tempos, rehearsal marks, figured bass, the jumps a `<sound>` gives:
31 filing sites) under its bar's number, and placed each into the first bar with
that number; clef changes, a slur stop read before its start and the dynamic that
dashes extend were matched by number too, and a multi-bar rest's closing barline
was filed under its 0-based position. Now each element is filed under the bar
being read (`m_controlElements` holds `const Measure *`, so the compiler refuses a
site that passes a number); `AddMeasure` records which bar of the score each bar
read went into (itself; the first part's bar a later part's bar was merged into;
none where "Mismatching measure number" drops it), and placement, the clef queue,
the slur match and the dashes' search go through that record. The multi-bar
rest's barline is filed under the rest's own bar. Bar numbers stay the bars'
labels (`@n`) and the numbers in warnings; objects are built in the same order.

Cases: `test_tb33_bars_numbered_alike_engine.cpp` (bars numbered 1 2 1 2: a chord
symbol, words and a tie in the second 1; dashes there that extend no dynamic of
the first; a slur stop in the first 1 that does not end the second's slur; clef
changes mid-bar and at the bar's start; a Fine given by a sound, the play ending
there, the score and its MEI twin alike; a later part's chord symbol on its staff;
a fermata on a multi-bar rest's closing barline), each failing on tb.32. Breaks,
each caught: placement by number, the clef queue by number, the slur match by
number, the dashes by number, a later part's bar not resolved, the multi-bar
rest's barline by its position. Native suite 335 of 335; with asserts and the
address sanitizer 335 of 335; the engine with asserts over every live song (2,767
of 2,767). Log-format audit: 0 calls. Parity tb.32 to tb.33: 14 of 22,086 outputs
differ, all of the three scores numbered alike (A Ghost Of A Chance and
Overprotected: their MEI and page; the Star Trek: The Next Generation theme: its
MEI, page, MIDI, pitches and timemap: its ties tie their own notes and its
opening bars play their file's lengths); the annotation locator: 0 failures,
every generated id kept. Seeded ids 3,093 of 3,093. MuseSync's songs: every note
the same, all 33.

**Upstream status**: not offered.

### 67. A repeat played after the jump plays as the first time, and its Fine or To Coda is taken (tb.32)

Found by MuseNote plan V3_2_0_48 (root cause RC47). A Fine (or a To Coda) in a
first ending is reached after a D.C. or a D.S. only where the repeat is played
again after the jump: MusicXML marks that on the repeat sign, `after-jump="yes"`
(its default is no: after the jump the last ending is played, as MuseScore plays
it too). Files so marked were played wrong by the one order-of-play rule
(`WriteOrderOfPlay`, every format):

- Where a jump landed, the repeat start last met was kept: a D.C. into a repeat
  with no start sign, played again after the jump, began its passes at a later
  repeat's start and ran off the end of the parts (Maple Leaf Rag so marked
  ended the process). Now the repeat a jump lands in starts at the last repeat
  start at or before where it lands, or at the score's beginning, as a repeat
  end with no start goes back; its passes are counted from there.
- After a jump back, a Fine or a To Coda in an ending was asked of the endings'
  last part alone, so one in a first ending was passed. Now it is taken the
  last time that ending is played, at the end of the last pass it names (a To
  Coda that names its times keeps its own rule). MuseScore's playback takes them
  on their volta's last playthrough.

Cases: `test_tb32_after_jump_endings_engine.cpp` (each MusicXML score and its MEI
twin played alike: Maple Leaf Rag's shape and King Of The Road's, a Fine in a
first ending marked after-jump; the same unmarked, the standard's default; a To
Coda in a first ending; a Fine in the repeat's own bars, taken on its last pass;
a repeat with no start sign played after the jump), each failing on tb.31 (the
first and the last ended the process). Breaks: each caught (the repeat start
under the sanitizer, a container overflow, and in release, a segfault). Native
suite 328 of 328; with asserts and the address sanitizer 328 of 328; the engine
with asserts over every live song and the six songs marked after-jump (2,773 of 2,773).
Log-format audit: 0 calls. Parity tb.31 to tb.32: 6 of 22,086 outputs differ, all of one score, The Fool On The Hill (its coda ends with a repeat with no start sign; after the coda its pass is counted from the jump, so its second ending is played: its MEI, MIDI, pitches and timemap); the annotation locator: 0 failures, every generated id kept. Seeded ids 3,093 of 3,093. MuseSync's songs:
the same order, all 33..

**Upstream status**: not offered.

### 66. A jump written as its words, or a target as its sign, is played (tb.31)

Found by MuseNote plan V3_2_0_46 (root cause RC42): the MusicXML reader read a
jump only from a `<sound>`. Many exporters write a jump as its words alone
("D.S. al Coda", "To Coda", "Fine") and a segno or coda as its sign alone
(MuseScore 0.9, Sibelius 4 and 5, Finale 2006 to 2012: 134 of MuseNote's
songs), and such a song played straight through. MuseScore's own importer
(`importmusicxmlpass2.cpp`, `handleRepeats`) honours a direction's `<sound>`
always, and reads its words only where it has none, by a fixed vocabulary
(`matchRepeat`).

- Words are a jump when the whole text, lowered and its spaces removed, is
  one phrase of that vocabulary: D.C., Da Capo, D.C. al Fine, D.C. al Coda,
  D.S., Dal Segno, D.S. al Fine, D.S. al Coda, Fine, Segno, To Coda, Coda;
  a roman numeral, or D.S.S., names a second segno or coda (`@label`
  `segnoii`, `codaii`). Text that is no such phrase is no jump ("D.S. and
  fade", "voice tacet on D.S."). The direction is typed by its jump, so the
  score and its exports hold it.
- A segno or coda sign alone is its target.
- A score whose `<sound>`s give a kind of jump or target (a direction's, or
  a measure's own, which MuseScore does not read) plays that kind by them:
  its words and signs of the kind are print. (Chi Mai v2: "Fine" written at
  the end of bar 36 beside the final bar's `<sound fine>`; read as a second
  Fine, the play stopped a bar early.) A sign that is print is drawn in an
  untyped `<dir>`: as a repeat mark it would be a target in the MEI, and
  every export would play to it. A sign drawn in a `<dir>` (print, or a jump
  drawn as its sign, patch 64) is drawn above the staff where its file places
  it nowhere, as a repeat mark is (a direction's own default is below:
  Beautiful's sign moved under the staff).
- A coda sign in a measure that holds a To Coda is that To Coda's sign ("To
  Coda ⊕"): a To Coda never lands in its own bar. MuseScore keeps one coda
  mark in a measure (`measureHasCoda`); here no `<sound>` is overridden.
- Order of play, for every format: a jump that names its target by the
  default label of its kind ("segno", "coda": words name none) goes to the
  first target of its kind where none has that label. (Eight songs give
  their segno by a sound labelled with its bar, `segno="34"`, and the D.S.
  in words; MuseScore labels every segno alike.) One naming another label
  is not taken.
- Changed with it: tb.28's case "a sign that is a sign stays one" expected
  a second segno with only an empty `<sound/>` to stay a repeat mark; it is
  print now, so the MEI no longer holds a target the file does not play.

Cases: `test_tb31_jumps_in_words_engine.cpp` (each MusicXML score and its MEI
twin played alike: every phrase, spacing and case, a sign alone, numerals,
D.S.S., a sound winning, phrases that are none; the kinds a score's sounds
give; a print sign; a To Coda's sign; a print sign placed above; a D.S. and a To Coda in words to
targets labelled by their bars; a label that names another), each failing on
tb.30. Breaks: each caught (one inert: dropping the check that the whole text
is the phrase, which `regex_match` already holds). Native suite 324 of 324; with asserts and
the address sanitizer 324 of 324; the engine with asserts over every live song
(2,765 of 2,765). Log-format audit: 0 calls. Parity tb.30 to tb.31: 736 of 22,086 outputs differ, in 141
scores: 49 live songs play otherwise by design (each judged against its marks:
18 now play their jumps, 31 play their marks as written and their files are
wrong), and 7 superseded versions likewise; the other 85 change in their MEI
and page only (words typed, signs print), timing and pitch the same. Where a
sign becomes print its song's generated ids move (383 of 477,334 notes);
annotation locator: 0 failures. Seeded ids 3,093 of 3,093.
MuseSync's 33 songs: the same order.

**Upstream status**: not offered.

### 65. The score keeps what its file says of its order of play (tb.30)

Found by MuseNote plan V3_2_0_45 (root cause RC41): the engine's MEI kept a
jump's kind and a repeat's sign, not what the file says of them. The times a
jump is taken, the label of the coda or segno it goes to (a coda's or segno's
own), a repeat's count and whether it is taken after a jump lived only in the
order of play (MEI's `<expansion>`); a jump given by a `<sound>` with no
direction left no mark. Every export of the score, and the score read again
without its order, played another order.

- The MusicXML reader keeps on each mark what its sound says: `@label` where
  the label is not "coda" or "segno"; a `@type` token `times-2.3` for a
  jump's named times (dots between the numbers: the engine reads "1-3" as a
  range). The measure of a repeat end keeps `repeat-N` for a count other than
  two, and `after-jump`. A jump that no mark carries (a sound of the measure,
  a direction's second jump, also in the bars of a multi-bar rest) gets a
  `<dir>` of its kind with nothing to draw.
- The rule for a score with no order of its own reads all of them: a mark by
  the first token of its type, a coda or a segno given in words as one (only
  the `repeatMark` signs were), and every jump a measure holds.
- The tokens are written and read in one place (`vrv::order`).

Cases: `test_tb30_order_in_the_score_engine.cpp` (each MusicXML score and its
MEI twin played alike: a jump's times in a repeat and at a repeat's end; two
labelled codas; a coda in words; a count of three; a repeat after the jump;
a jump by a sound alone; a direction's second jump), each failing on tb.29.
Breaks, all nine caught. Native suite 320 of 320; with asserts and the
address sanitizer 320 of 320; the engine with asserts over every live song.
Parity tb.29 to tb.30: of the outputs present in both (the corpus gained the
49 files MuseNote published the same day), 495 differ, in 36 scores: their
MEI by the attributes alone, their page by the labels' tooltips and, in the 5
with a jump by a sound alone, an empty group; timing and pitches the same
(three timemaps by ids only). Annotation locator: 0 failures. Seeded ids
3,093 of 3,093. MuseSync's 24 songs: the same.

**Upstream status**: not offered.

### 64. A jump is what its sound says, taken the times it names, as many as its barline holds (tb.28)

Found by MuseNote plan V3_2_0_44 (every song played in full; root causes RC34,
RC35, RC36, RC38, RC39), each shown on a small correctly marked file before it
was changed:

- MusicXML (RC34): a direction that shows a coda sign or a segno and no words,
  whose sound is a jump (`tocoda`, `dalsegno`, `dacapo="yes"`), is that jump: a
  `<dir>` typed by its sound holding the sign as a symbol, as words with a sign
  are. It was a `repeatMark` of the sign: the score held a second coda or segno
  and no jump, which lived only in the order of play, so every export wrote the
  sign. A sign whose sound is a sign stays a `repeatMark`.
- Order of play (RC35): a jump that names its times (`time-only`, "which times
  through the repeated section the sound applies") is taken the time its part is
  played that it names, every time counted, its repeat's passes too, and as soon
  as that time comes: in a pass of a repeat the rest of it is not played. Only
  the returns to the part were counted, so a To Coda "3rd time" in a passage
  played twice by its repeat and once after the D.S. was never taken, nor a To
  Coda "2nd time" in a repeat. A jump that names no times keeps its meaning (a
  D.C. or D.S. the first time, a To Coda the time after the jump back).
- Order of play (RC36): a part holds every jump its barline is given (MusicXML
  lets a barline hold a To Coda for the second time and another for the third);
  the rule takes the first whose time it is. The reader kept the last.
- Order of play (RC38): a repeat whose closing sign says `after-jump="yes"`
  plays its passes and endings after the jump as the first time. Where the sign
  closed an ending only the last ending was played; the mark was read on a plain
  repeat alone.
- MusicXML (RC39): the bar that begins a multi-bar rest is read on after its
  attributes for its directions and sounds, and so are the bars the rest spans,
  into the rest's measure (a start at its start, a jump at its end, as its
  barlines are its last bar's); also where another part begins the rest. A
  segno, a To Coda, a D.S., a tempo or words there were lost.

Cases: `test_tb28_jump_times_engine.cpp` (the three signs drawn for jumps, in
the order, the MEI and the MEI played without its order; times through a repeat
and a return, in a repeat, at a repeat's end, by the returns alone, and none
named; two jumps at one barline, as two directions and as a sound of its own; a
repeat taken after the jump with endings, and without the mark; a segno and a
D.S. in multi-bar rests; another part's words in one), each failing on tb.27.
Breaks, each caught, but one that cannot change an order (a named-times jump
judged by its returns as well: it is taken by its plays first). Native suite 315
of 315, also under the address sanitizer. Parity tb.27 to tb.28: 81 of 21,743
outputs differ, in the 13 scores above, each judged against its file: the six
signs (MEI, SVG and ids; timemap and pitch map the same but for ids), the seven
rests (the marks added; notes played the same but Superstition's, 151 to 245 by
its jumps; four tempos read). Annotation locator: 0 failures. Seeded ids 3,044
of 3,044. MuseSync's 24 songs: the same order and MEI.

**Amended (tb.29, the same day):** the reader asked every direction's sound
whether it is a D.C. through `HasAttributeWithValue`, which asserts its node,
and a direction with no `<sound>` has none: a build with asserts ended on every
MusicXML file with such a direction (MuseNote root cause RC40, found by
MuseNote's check in its debug app; a release build read it as before, and
every check above was a release build). The sound is asked only of a sign with
no words that has one. Checked since with asserts on: the native suite under
the address sanitizer, and the engine over every corpus song and MuseNote's
corrected files (load, every page, MIDI, timemap, MEI). Parity tb.28 to tb.29:
no output differs.

**Upstream status**: not offered.

### 63. An ending is played in every pass its label names and its signs reach (tb.27)

Found by MuseNote's ABC export gate (plan V3_2_0_42; finding W56, root causes
RC26, RC27 and RC29), all three in `WriteOrderOfPlay` and its `parseInts`:

- `parseInts` read a list only where commas stood between its numbers: an
  ending labelled "1.2." was played in the first pass alone, "2.3." in the
  second, "1-5" in passes 1 and -5. It now reads every number and every range
  whatever stands between them ("1, 2", "1.2.", "6.", "1-5", "1–5"); a range
  past any sense is its first number, as the ABC reader reads one, and a number
  an int does not hold names nothing (it ended the list).
- Read right, "2.3." showed what the misreading hid: the rule played every pass
  the endings name, as if a sign sent the play back for each. Where one ending is
  played in two passes in a row, the play now goes back only for a repeat sign at
  the end of the ending just played, a sign further on, or an ending further on
  still to be played (a file that leaves out the sign before its next ending, as
  before). Otherwise the repeat is left there, and what the endings name after it
  is the time after a jump back, played as their last ending (I Wanna Be Loved By
  You: "2.3." with the Fine in it, its third time after the D.C.; She Will Be
  Loved: "2, 3", wrong since patch 56). The sign at the end of a last ending
  (patch 61) is not followed once the repeat is left.
- A sign further on, after plain bars, brought round an ending for several passes
  even where that ending closes with a sign of its own: its second time was passed
  over and the next ending played in its place (Vaya Con Dios, Happy Together).
  Only an ending with no sign of its own is now brought round so.

Cases: `test_tb27_ending_labels_engine.cpp` (each form of label, in MusicXML and
as MEI with no order of its own; ABC ranges and lists; a last ending for two
passes with a jump, without one, and closed by a sign; a file that leaves out
the sign before its next ending; a repeat left there and an earlier ending's
count; an ending with its own sign and a stray one further on). Native suite 310
of 310, also under the address sanitizer. Parity tb.26 to tb.27: 41 of 21,743
outputs differ, the MEI, MIDI, timemap and pitch map (and locator copies) of A
quoi ça sert l'amour, Chan Chan, It's My Party, Please Help Me I'm Falling,
She Will Be Loved, Vaya Con Dios and Happy Together, each judged against its page;
no page and no note id. Seeded ids 3,044 of 3,044. The guard
`tool/export_gate/returns_by_signs.py` (MuseNote): the play goes back only where
a sign sends it, in every corpus score (tb.26: She Will Be Loved).

**Upstream status**: not offered.

### 62. A tie joins a note to the next note of its voice (tb.26)

Found by MuseNote's ABC gate and measured by its tie guard (plan V3_2_0_39;
finding W55, root cause RC21). The MusicXML reader paired a tie by pitch and
reading order, not by the notes it joins: a stop closed the last open tie of
its pitch in its layer, or else, at the bar's end, any open tie of its pitch
begun earlier; an open tie stayed open across bars, and across parts (the
stack was never cleared between them). So a start whose next note did not stop
it was ended by a later note of its pitch, one to 79 notes on (41 ties in 27
songs: the notes between were not heard as written, an arc was drawn over
them); a one-note tie (MusicXML 4.0: start and stop on one note, into a repeat)
tied its note to itself and silenced it; a part's open tie could be ended by the
next part's first note of its pitch.

- Every note and drawn rest takes the next place of its voice as it is written
  into it: a chord one place, a grace note none of its own (it stands with the
  note it leads to), a rest not drawn and a `<forward>` none (gaps, not
  notation). By places, not by the file's ticks: a `<backup>` inside one voice
  (Angie) lets two notes begin together by the ticks that the engine writes
  and plays one after the other.
- A tie in its own voice ends on the note at the next place after its start;
  into another voice, on a note that begins where its note ends by the ticks,
  or at the next bar's start where its note ended at the bar line. Its own
  voice first. A stop read before its start is matched at the bar's end by
  the same rule.
- A tie that ties nothing is left out, said in the log: its voice went past it,
  or it waited one bar for its voice in vain, or its part ended.

MusicXML: the start on one note, the stop on the next; MEI: notes "of the same
pitch [that] form a single note with their combined rhythmic values". A rule by
ticks alone would have dropped 8 ties the page rightly shows, where a file has
an invisible `<forward>` between a tie's notes.

Cases: `test_tb26_ties_next_note_engine.cpp` (the next note, over the bar line,
in chords; a start its next note does not stop; over a rest; a one-note tie;
into another voice at its end and not; an invisible gap and a hidden rest; a
voice that skips a bar; a part's end; grace notes), and the case of
`test_midi_document_engine.cpp` that pinned the old pairing on purpose,
changed as its comment asked. Nine deliberate breaks, each caught; the suite
under the address sanitizer. The guard (MuseNote's
`tool/export_gate/ties_join_next_notes.py`): every tie of the 3,024 corpus files
ends on the next note of its voice (45 did not); 1 tie into another voice,
judged by hand. Parity tb.25 to tb.26: 1,303 of 21,743 outputs differ: the MEI,
page and MIDI of the 31 files the guard named; and the MEI and page of 287
more, which lose 3,022 ties with a start and no end, each an empty group
(checked element by element: nothing else). Seeded ids 3,044 of 3,044.

### 61. A repeat sign on the page is played (tb.25)

Found by MuseNote's order gates (plan V3_2_0_38; findings W54 and W51, root
causes RC19 and RC20). Two causes behind one symptom, a repeat sign on the
page that the order of play never took (26 songs):

- `WriteOrderOfPlay` took a repeat's passes from the pass numbers its endings
  name, and nothing else. A repeat sign at the end of the last ending of its
  group (a first ending closed by a repeat sign with the second time through
  unmarked, Anastasia's shape; or one standing alone before plain bars) sent
  the play back on the page and never in the music. Now the passes after that
  ending's, up to the repeat's count, play the repeat's music and go on after
  the endings. The count is taken from the pass the repeat is in when it
  reaches the endings: a repeat that has played its passes by endings before
  (a second "1." after "1." and "2.") is not taken again. Passes the endings
  skip between their numbers (a "1." then a "3.") are not made up: nothing in
  such a file says what the missing pass plays.
- At a barline that holds an ending's stop and a repeat sign, the MusicXML
  reader gathered both into one section stop and, where it was an ending's,
  kept the ending's number and dropped the repeat: the part's count fell to
  once. Where no ending began (a stray stop, the file's fault: El Choclo,
  Help!, Without You) the repeat was not played at all; where the sign closes
  an ending its times were lost. The count is now kept with the ending.

Cases: `test_tb25_repeat_signs_engine.cpp` (MusicXML and MEI with no order of
its own: a lone first ending closed by a repeat sign, after plain bars, a lone
"2", no repeat start, `times="3"`, stray stops, and what must not move:
`[1 :| [2`, `[1 :| [3`, three passes, a repeat that has played its passes, a
D.C.), four deliberate breaks each caught, the suite under the address
sanitizer. The guard (MuseNote's `tool/export_gate/repeat_signs_played.py`):
every repeat sign of the 3,024 corpus files is played, or is explained (an
ending's sign whose repeat has played its passes; 8 signs in a coda, found with
it and left for their own plan: the rule keeps "after a jump" for the rest of
the score, W60). Parity tb.24 to tb.25: 186 of 21,743 outputs differ, MEI
`<expansion>`, MIDI, timemap and pitch map of those songs only; seeded ids
3,044 of 3,044. Hand-written ABC tunes: 984 to 988 of 1,037 play as abc2midi
plays them, none worse.

### 60. Every tie, slur and line of the score as played is judged by the order of play (tb.24)

Found by MuseNote's ABC gate (plan V3_2_0_34; finding W53, root cause RC18), and
measured over the corpus before the fix: 330 ends wrong in 217 songs. The
expansion writes the order of play by cloning sections one after another. As
each clone was made, `UpdateIDs` pointed a copied element's end at the latest
copy of its end made so far; an end given as a count of bars (`@tstamp2`)
counts, in the score as played, played bars. Nothing judged either against the
order: (a) where the end's bar was cloned after the start's, a copy ended on an
earlier pass's note; (b) where the play never reaches the end in that pass (a
jump back, a Fine) it still ended on a note already played. 500 Miles held its
last note 1.5 quarters past the end of the song; in Sweet Dreams and Bolero a
tied note attacked again on the second pass.

- `Doc::ExpandExpansions` takes each measure's place in the score as written
  before `Expand` moves anything, and after it calls
  `ExpansionMap::FollowSpansInOrder` over the score as played.
- From an element's start the play goes on while it stays within the bars the
  element spans on the page and its start is not played again (that is the
  next pass, with its own copy). What joins notes (tie, glissando, beam across
  bars) ends on the copy of its end reached there, and is taken out of a pass
  where none is; what spans music ends on the copy reached there, or on the
  last note, chord or rest it covers before the play leaves its bars. A count
  of bars is written again as the count of played bars; a beam's note list
  follows the copies played. Starts were measured right and are not changed.

This is MusicXML 4.0's account of a tie's sound across a repeat (`<tie>` apart
from `<tied>`, `time-only`) and MEI's (one tie per ending, a tie back over a
repeat): a tie sounds where its note is followed, in that pass, by the note it
joins. MEI's two ties into two endings and a tie back to a repeat's start need
no case of their own.

Cases: `test_tb24_spans_in_order_engine.cpp` (ties over a repeat sign, into the
bar after Fine, into each ending, back to a repeat's start, three passes with
the end's bar copied after the start's; octave lines over a repeat and over a
whole repeated passage; slurs and hairpins by count of bars; beams across bars
with their note lists), ten deliberate breaks each caught, the suite under the
address sanitizer. The corpus guard (MuseNote's
`tool/export_gate/expanded_spans.py`) judges every spanning element of every
song as played: 0 wrong (330 before). Parity tb.23 to tb.24: 227 of 21,743
outputs differ, all MIDI, every one a song the guard named for a tie; seeded
ids 3,044 of 3,044. Found with it, not this patch's: a MusicXML file that
numbers two measures alike gets ties in the wrong measure (W59, 5 ties in 2
songs; the reader places a control element by its measure's number).

### 59. An ABC file told by its tunes (tb.23)

Found by MuseNote's check of the rebuilt reader in the running app (plan
V3_2_0_34, step 4; finding W58): the app hands a stored score to the engine
without naming its format, and the engine told ABC by its first character only
(`X`, or `%` and `a`). ABC 2.1 lets a file begin with empty lines, free text,
comments and a file header before its first tune (2.2): the Nottingham Music
Database's files begin with an empty line, many files with a comment. Such a
file was taken for MEI, for PAE ("%" and a space), or for GABC when it held a
"%%" directive (the GABC test's comment said "%%" cannot appear in ABC; ABC's
stylesheet directives begin with it).

- `IdentifyInputFrom`: a text whose first `X:` line comes after nothing but
  empty lines, `%` lines and information fields is ABC. Plaine & Easie has no
  `X:` line; a GABC header's names are words.
- The reader reads the file header (the fields and directives a file begins
  with, up to its first empty line) and sets it at the head of every tune
  (2.2.2: "used to set default values for the tunes in the file").

Cases: `abc_standard.table` rows loaded without naming the format (an empty
line first, a comment first, a file header first) and a case that PAE, GABC
and Humdrum keep their own. Measured: of 1,055 hand-written files and tunes
read by detection alone, 345 were read before, all now; parity 0 of 21,743.

### 58. The ABC reader rebuilt on the standard (tb.22)

MuseNote's root cause RC6 (plan V3_2_0_34): the reader had no model of a tune
and no account of the standard. It wrote each character it met into one layer
of one staff; a voice, an ending, a change of key, a part had no place, and
what it did not know it passed over. Six patches (7, 13, 22, 31, 41, 49) fixed
it symptom by symptom.

**Two halves** (`src/ioabc.cpp`; the header keeps only the class):
- namespace `abc` reads the text into tunes: the fields with their scope (the
  file, a tune, a voice, a point in the music), the voices, each voice's bars,
  and in each bar its events (notes, chords, rests, spaces, bar rests) with what
  belongs to them (lengths, signs, ties, grace groups, tuplets, decorations,
  texts, syllables, changes of key, meter, clef and tempo). A note's sound is
  worked out once, by ABC's rule.
- the writer makes the score: staves from `%%score`/`%%staves` (voices in round
  brackets share a staff; braces and brackets group staves), bars across all
  voices, `<ending>`s, a length as the notes it takes (tied pieces down to a
  1024th, the shortest value with a written form), beams and tuplets as ranges,
  marks where they stand (a jump written before a bar line is its bar's), the
  name of a part as a rehearsal mark, and the order of play by
  `WriteOrderOfPlayFor` (patch 57), with the counts of the colons and the order
  of the parts (P:, single upper-case letters, an order in the standard's
  grammar only).
- Files with no version, or one before 2.1, are read loosely (12.2): `+...+`
  is a decoration of 2.0 or a chord of before 1.6. A file of 2.1 has neither.
- What is not read is said in the log, once a kind; the reserved characters are
  ignored and said (8.1).
- Character tests are ABC's own (ASCII). `isspace` followed the C locale and
  said yes to 0xA0, the second byte of "a grave": a title lost it (MuseNote W46).
- No measure numbers: ABC gives none, and a mark stored on an ABC score knows
  its bar by its notes.

`WriteOrderOfPlayFor` takes the repeat counts and a playlist of stretches; a
part starts after a stretch's end too, so a stretch is played to its end and no
further.

**Cases:** `abc_standard.table` in the monorepo (152 constructs of ABC 2.1: 147
read, 5 knowingly not read with their reasons), and the existing ABC cases, six
of them restated where the standard reads otherwise than the old reader (each
says "since tb.22"). 32 deliberate breaks, all caught. 286 native cases, also
under the address sanitizer.

**Measured:** tool/parity, tb.21 against this: 21,743 outputs, 194 differ, all
the ABC inputs' MEI. MuseNote's ABC export of 2,765 songs read back: 2,359 play
the same in every aspect (56 before). 1,037 hand-written tunes (the Nottingham
Music Database) against abc2midi: 984 the same (603); each of the 53 left read
against the standard's text. Locators of stored ABC scores, old reader against
new: 99.6% of notes kept (hand-written tunes), 93% (MuseNote's exports); every
note that moves is one the old reader misread.

### 57. One rule for the order of play (tb.21)

MuseNote's root cause RC15 (plan V3_2_0_34), found while preparing the
rebuilt ABC reader: the engine worked the order of play out in two places.
`MusicXmlInput::CreateExpansion` had the rule with endings and jumps
(patches 46, 56); every other format went through
`ExpansionMap::GenerateExpansionFor`, which knew repeat signs only and
looked only at measures that stand directly in the score's one section.
So an MEI, ABC or Humdrum score never played an ending in its pass and
took no jump, and an MEI file whose measures stand inside `<ending>`s or
sections of the section played no repeat at all (a trial: 95 notes of a
song of 188).

**The rule is one function**, `WriteOrderOfPlay(play::Sections &)` in
`expansionmap.cpp`: the body of `CreateExpansion`, moved unchanged, over
the score as a run of parts. The part types (`EndingInfo`, `RepeatInfo`,
`JumpInfo`, `FineInfo`, `SectionInfo`) moved with it into namespace `play`
(`expansionmap.h`); namespace `musicxml` names them by alias. The MusicXML
reader gathers its parts while reading, as before, and asks the rule.

**`GenerateExpansionFor` builds the parts from the score as it stands**:
its measures in score order, through sections and endings; a new part
where a container (a section, an ending) or a repeat starts, at a segno or
a coda, and after a repeat end, a jump or a fine. Marks: a `repeatMark` by
its `@func`, and a `dir` typed as the MusicXML reader types the jumps it
reads (`dacapo`, `dalsegno`, `tocoda`, `fine`). Every part of an ending is
the ending's (one a mark divides is played whole in its passes). MEI holds
no repeat count and no labels: a repeat is played twice, a D.C. or D.S. is
taken once, a to-coda on the second visit, the labels are "segno" and
"coda". Each part gets the element the order names: the section or ending
that holds just its measures where that is a child of the score's section,
otherwise a new section there. A score with no repeat end, no ending and
no jump is left as it stands. Still refused: editorial content, and more
than one section directly under `<score>`. `CreateSection` is gone.

**Cases:** `test_tb21_one_order_engine.cpp` in the monorepo
(`corelib/verovio/test/native`): scores said in MusicXML and as MEI with
no order of its own (the engine's MEI, its `<expansion>` removed), which
must play the same; MEI as another program writes it; what the general
rule played rightly before. 13 deliberate breaks, 11 caught, 2 showed
conditions that could not matter, which were removed.

**Measured:** tool/parity, tb.20 against this: 21,743 outputs, 0 differ
(beside songs added to the corpus since). All songs' MEI with the order
taken out, read again: 2,744 of 2,765 play note for note as the song.
The 21: 13 files hold an ending stop at a repeat sign with no ending open,
where the MusicXML reader drops the repeat and the MEI plays it (MuseNote
finding W51); 5 give a jump by `<sound>` alone, which the MEI does not
hold; 2 repeat three times; 1 has endings whose numbers disagree.

**What changes for a user:** a score that is not MusicXML and has no
`<expansion>` (an MEI or Humdrum file, a score made in an editor that
writes MEI) plays its endings in their passes and takes its jumps.

### 56. An ending is played in its own pass (tb.20)

MuseNote's finding W49 (plan V3_2_0_33): in 257 of its songs the first
ending was played on every pass. Their files bracket the first ending
over one bar and put the repeat sign a bar or more later, in no ending
(MuseScore 0.9.5, where a bracket dropped on a bar covers that bar).
`MusicXmlInput::CreateExpansion` gathers the endings of a repeat only
while they stand one directly after the other; with plain bars between,
the first ending was ordinary music to it, and the repeat sign played
everything from the repeat's start again, the ending numbered 1 too.

The files are wrong in themselves (the bracket was meant to reach the
sign) and are corrected in MuseNote's catalog. This patch is the engine's
part: whatever a file brackets, an ending is not played in a pass its
number does not name.

- **The passes of a repeat are counted** from its start (a repeat start,
  one inside an ending, or the beginning). Where a repeat sign or a
  further ending sends the music back, an ending in the bars played again
  is played only if its number names that pass. An ending with no number
  names no pass and is played as before; an ending the repeat itself
  starts in is that repeat's own music and is always played.
- **One ending for several passes** ("1, 2") comes round by the repeat
  sign at its end. Where that sign stands further on, after plain bars,
  the sign brings it round in the passes it names, and the ending's own
  group does not play it a second time. With no sign after it at all it
  is played as before.
- **After a jump back** the last ending of a repeat is the one played, as
  before, also when plain bars stand between the endings: an ending
  followed, before any new repeat start, by one whose numbers go on from
  its own is not the last. One that starts its numbers again belongs to
  another repeat.

Nothing is guessed about where a bracket should end: the bars between a
short first ending and its repeat sign are plain bars and are played in
every pass.

**Measured** (tool/parity, tb.19 against tb.20): 1,627 of 19,930 outputs
differ, all of 272 MusicXML scores: MIDI, timemap, pitch map, and the
`<expansion>` line of their MEI exports. No page differs and no other
line of any MEI. Seeded ids are the same on a fresh thread (2,785 of
2,785). MuseNote's count of the bars in the order they first sound is
the same for every score but one (Song For Guy, whose first ending only
its D.C. pass reached, and no pass reaches now).

**Cases**: `test/native/test_tb20_endings_engine.cpp` in MuseNote's
`corelib/verovio` (9 cases; 7 fail on tb.19, the other two hold what
tb.19 played rightly); 11 deliberate breaks, each caught; clean under
AddressSanitizer with libc++ hardening.

**Upstream status**: not offered.

## Upstream fixes judged (2026-11-19)

MuseNote's review of upstream 6.2.1, 6.3.0, 6.4.0, 6.4.1 and develop to 7f9e3651d (2026-11-18), for its W112, the base
move, which waits for MuseNote 3.3.0 (the owner, 2026-11-19). Taken onto this branch at tb.49: patches 103 to 108.
For the base move: the target is 6.4.1. Tools and notes: `tb14-durable/verovio-upstream-notes` (merge-tree of this
branch onto each release, the conflicts blamed hunk by hunk).

**Taken at tb.49** (none moves a generated id): 6d5d4f85a, 15c17a299, 2387e3807 (patch 103); 8300807c9, the direction
half of af2adb2a7 (104); 8339461f8 (105); c8582c622 (106, develop); 61866183d (107); ad1570a6d (108, develop).

**Upstream has the fault this branch's patch mends (the patch drops or is replaced at the base move):**
- patch 3: beeeb2150 (6.3.0), the same commit;
- patch 7: 485b6018a (6.4.0, an octave without its end); its toolkit half was already replaced by patch 84;
- patch 9: e918f18ec (6.4.0, a pedal without its start), short of the alignment check; the layer check is ours alone;
- patch 68: 1f51c826a (6.3.0), the same fault (RC54) mended by bar count, not by bar; patches 64 to 66, 69, 94, 98,
  99 and some of 80 to 83 take a bar and are re-ported to it;
- patch 18: the `<alter>` series (d11c392fb, 2d0dea2d3, 4d76e16af, e59ec90d6, a761ed4bd in 6.3.0; c2f730f00 in 6.4.0),
  the same rule (the file's alter is the sound) built otherwise: it drops empty and natural accids, so ids move;
  patch 50 stands on 18; 6.4.1 has a regression of it, #4567 (an accidental-mark changes the next note's pitch), mended
  on develop by 6b49a5a66;
- patches 28 and 40: 3fe7f1ad3 and f1998d2b0 (6.4.0), the same faults mended otherwise;
- patch 91: eb00ec45a (develop), the same fault (nested slurs) mended otherwise, and innermost first: its sort taken at
  tb.52 (patch 125);
- patch 93: a96989225 (develop), the same fault (a chord's beam), narrower than ours.

**Upstream goes the other way:** 8817c3033 (6.3.0) drops a trailing forward's space; patch 98 keeps the forward's time.

**Upstream changes that move generated ids (MusicXML):** 7e02fe4ea (no empty accid; 97.9 % of MusicXML note ids, the
fork study), a761ed4bd, 92b03ade9 and 7d781df87 (a verse at an extender's stop), bd1515b1a (a tempo from a bare
`<sound tempo>`), c2f730f00, f1998d2b0. The Score/Part split of 6.4.0 builds the same objects.

**To mend at the base move:** libmei reads numbers with `stof` and `stoi` from 6.3.0 (04cf3e934): an empty or
non-numeric number in an MEI file throws past every catch, and a double is read as a float (patch 90's read side);
6.4.0 draws an SVG placeholder (a circle) at every scoreDef, sb, pb and section inside a system, hidden only by the
stylesheet, which MuseNote's SvgPatcher removes; 6.3.0's new glyphs (Turkish accidentals) for the app's assets.

**Not taken, not needed by MuseNote or MuseSync:** the editor API and cursor; show-hidden; neume, mensural and
tablature work; page-based MEI; `<parts>`; `staffDef@num`; `--convert-attributes`; the `svgAdditionalAttribute` speed
work; 1712ca6f8 and 5e8dc3db8 (thread safety of the custom tuning and the object factory: one engine worker, and none
of the factory's users is used; 1712ca6f8 applies cleanly if it is ever needed); da57e000e (`getElementsAtTime`'s
page, which neither app calls); develop's font rewrite (it moves `data/` and writes text as text).

**Found by the audits of tb.49's classes, upstream develop still has it:** a tablature note's course sized the MIDI
writer's hold list (patch 107, ours); a `<staff>` the score does not have read past `m_octDis` (patch 103, ours); a
part's first `<staff-details print-object>` is never read (MuseNote's W116, open).

**Round 2, taken at tb.52 (2026-11-19, MuseNote plan V3_2_0_78; patches 114 to 128).** First a trial: upstream 6.4.1,
with this branch's patches 2, 88 and 6 only, against MuseNote's 422 native cases: on every one this branch is never
worse than 6.4.1; 6.4.1 mends 33 of them, partly 11, still fails 321, and crashes on 50 of their inputs. So the picks
are upstream fixes for faults the cases never covered, each found in this branch by a probe: 2fee90407, 44c85a898,
75df1a822, e2c687382, e2f797f22, 378720075, 0c9330162, 60a4472dc, 0b4b78a9b, d7b74259b, 20c1936cc (its articulation
half), 3d744d933, 90a440db7, 992d2e644, ede5a0f20, e2e5cd302 (6.3.0 and 6.4.0), 828f7264d, eb00ec45a (its sort),
53584d697 (develop), and f1998d2b0's delete (patch 127). None moves a generated id of MuseNote's songs; in three
shapes of file, ids after the fix move, as it builds what was dropped or drops what was wrong (a single TAB staff's
rests, a stemless tab note's duration sign, a grace group in an MEI tuplet). At the base move they drop, the develop
ones when upstream releases them; ours stay: patches 128, 123's bound, 121's technical guard.

**Left for the base move:** 975f7c28d (harmony text runs: every chord symbol's tspans change, no gain from MusicXML);
57f56d0da (an ending's `@label` drawn: no effect from MusicXML). **Not needed:** d1a5c019a (patch 62 already ties tab
notes), 8497ed330 (it needs 92b03ade9: both taken at tb.56, patch 136), 1712ca6f8.

**Found by the audits of tb.52's classes, upstream develop still has it:** a rest in a tab group timed twice (patch
128, ours; e2f797f22 brings it to MusicXML); a technical mark offered to a tabGrp (patch 121, ours); an ornament's staff
unbounded (patch 123, ours); no triangle notehead is drawn, `isotriangle` or any (MuseNote RC157, W141, open).

**Taken at tb.56 (2026-11-20, MuseNote plan V3_2_0_84; patch 136):** #4339 (92b03ade9, 8497ed330; 6.3.0) and
#4444 (7d781df87; 6.4.0), their lyric rule ported into `FollowSyl` (patch 85), and 20c1936cc's verse half. They
move no generated id of MuseNote's songs or MuseSync's (none has a stop); a user's file with stops reads the ids
after each stop once more.

**Taken at tb.58 (2026-11-21, MuseNote plan V3_2_0_86; patch 138):** bd1515b1a (#4443) and 5c7736156 (#4503), 6.4.0,
ported into `ReadMusicXmlSound`, with ours on them: a bare `<sound tempo>` builds a tempo only where it changes the
tempo in effect. They move no generated id of MuseNote's songs or MuseSync's (none of their bare `<sound tempo>`s
changes the tempo); a user's file whose bare `<sound>` changes it reads its ids after each new tempo once more.

**Taken at tb.59 (2026-11-21, MuseNote plan V3_2_0_88; patch 139):** d751fb2ed and 9473d055a (#4446), 6.4.0, each
conflict kept ours, with ours on them: the Humdrum export writes a syllable joined by a space with `&#160;`. They
move no generated id of MuseNote's songs or MuseSync's (no object is added); 46 elisions in 11 of MuseNote's songs
are drawn with the space their files give.

**Taken at tb.61 (2026-11-22, MuseNote plan V3_2_0_90; patch 144):** 90fc4d036 (#4565, develop, not yet released),
with ours under it, the syllable ink bounds of a09acec80 (#4405, otherwise not taken). It moves no id: the songs' SVG
boxes change, and the page MuseNote draws does not.

## Rules for this branch

- A patch is carried only when the change cannot be made outside the
  engine. Speed work and new features live in the app's own code, not
  here.
- At most about five patches of about 30 lines each. None may change the
  ids or the output of a seeded load on one thread — in practice, nothing
  in drawing (`view_*`), layout functors, or importer object construction.
  Six are carried since 2026-09-30 by the owner's decision: patch 6
  exposes a pitch only the MIDI functor computes, and patch 3 (upstream's
  own fix) drops at the 6.3.0 base move. Seven since 2026-10-11, again by
  the owner's decision: patch 7 stops two crashes on files a user can
  import. Nine since 2026-10-12, by the owner's decision: patches 8 and 9
  finish what patch 7 began. Patch 8 is in the importer; it removes an
  object after it was built, which leaves the order of construction, and
  so every other id, as it was.
  Twelve since 2026-10-12, by the owner's decision: patch 10 makes the
  Humdrum export hold the score (88 lines, well over the 30: eleven
  faults of one converter, none of them in drawing, layout or an
  importer), patch 11 puts a MusicXML octave line where it belongs, and
  patch 12 keeps two exceptions inside the engine. Patch 11 is in the
  importer: it removes an octave line that has no note, after it was
  built, and moves one to its own part; no score of the corpus has
  either, and no id of the corpus changed.
  Fourteen since 2026-10-12, by the owner's decision: patch 13 finishes
  the Humdrum export (91 lines, and the first to add members to a class
  of the engine), and patch 14 mends the engine's own expansion of
  repeats, which that export's files ran into and which any ABC, MEI or
  Humdrum file can. Patch 11 was amended after its review; an amendment
  is not counted as a patch. tb.9 (the same day, by the owner's
  decision) amends patches 11 to 14 after the review of tb.8 and adds
  none.
  Seventeen since 2026-10-13, by the owner's decision (the findings of
  the reviews of tb.9 are mended, and the count may pass fourteen):
  patch 15 reads ABC's repeats as ABC says, patch 16 does not follow an
  order of play that costs more than any score's, patch 17 passes over
  a clef for a staff the part does not have; tb.10 also amends patches
  10 to 14. Patch 15 is in an importer and changes what it builds for
  the ABC files it is about (a measure more at a `::`, a repeat start
  written where ABC means one): 8 of the 194 ABC files of the parity
  set, declared above, and no file of another format.
  Eighteen since 2026-10-13, by the owner's decision of 2026-10-12
  ("Sound now, page next"): patch 18 is no crash fix. It gives a
  MusicXML note with no accidental written the sound its file says,
  which only the importer can do. It is in the importer and changes
  pitches by design; it builds no object and removes none, and no id of
  a document of the corpus changed (on two pages a text the engine makes
  while it draws has another id: see the entry).
  Twenty-two since 2026-10-13, by the owner's decision of 2026-10-12
  ("Build it all"), under an exception to two rules at once: patches 19
  to 22 are features (the Humdrum export says the order a score is
  played in and holds its chord symbols), and they add 615 lines (171,
  115, 30 and 299) where a patch is to be about 30. They are separate
  patches, each with its own cases; patch 22 is the largest because it
  also holds what the review of the first build found in the converter
  (a staff or a beat that a file says wrongly, a hairpin beside a
  dynamic, a mark given by the note it starts on). Patch 21 is in an
  importer and changes what it builds for two kinds of Humdrum file,
  declared in its section: no file MuseNote wrote, and none of the
  parity set. Patch 19 is not shipped without patch 21. One limit is
  declared and pinned by a case, not mended: the chord symbols of a
  re-opened export do not follow a transposition.
  Twenty-three since 2026-10-14: patch 23 is a crash fix (text of a file
  was the format of a log message), of the kind this queue has carried
  since patch 7. It marks four functions in a header and changes the
  output of no score.
  Twenty-nine since 2026-10-14, by the owner's standing decision that
  the findings of a review are mended in the fork when the gates are
  clean, files that end the app first: patches 24 and 28 are crash fixes
  in two importers (24 also bounds one read of the text converter in
  `src/vrv.cpp`, which every format uses) and patch 29 is one in the
  MIDI writer (a meter whose unit is below one), patch 27 bounds the
  time of patch 16's ceiling, and patches 25 and 26 finish what patch 15
  began. Patches 24 to 26 are in an importer; patch 26 changes what it
  builds for an ABC `Z` with no number (an `<mRest>` for a
  `<multiRest>`, with the same id): 14 of the 194 ABC files of the
  parity set, declared above, and no file of another format. Patch 27
  adds a small class to `expansionmap.h`. tb.13 also amends patch 19:
  its export writes no order of play past the limit of patch 27 at its
  least (such an order may be cut short).
  Forty-nine since 2026-10-26, by the owner's decision of 2026-10-20
  ("go for all"; MuseNote's plan V3_2_0_13, its point 6: the count
  passes its budget again for faults a user's file meets): patches 30
  to 40 and 44 to 49 are crash, hang and overflow fixes for files a
  user can open, in the MEI, Humdrum, ABC and MusicXML readers, the
  Humdrum export, the timemap and the engine's fractions, and patches
  41 to 43 give a MusicXML score the order of play its file says. Patch
  39 is in generated code (libmei) and is carried again at a
  regeneration or a base move. Patches 41 to 43 are in an importer and
  change the order of play of 28 scores of the corpus by design, and
  patch 44 the timemap of one (Wave, put in time order): no object is
  built that was not, and no id moves. Patch 45 refuses the Humdrum
  export of neume notation, which held a clef and no note.
  Fifty since 2026-11-11, by the owner's decision of that day
  (MuseNote's plan V3_2_0_22: "choose the best that is not workaround
  that fix the root cause"): patch 50 is a feature, 379 lines where a
  patch is to be about 30, and it is in an importer. It finishes what
  patch 18 began: the page shows the sound the file gives. It sets
  attributes on objects the importer built, and builds and removes none:
  no id of the corpus moved. MuseSync's pages of MusicXML gain the signs
  too.
  Fifty-three since 2026-11-12, by the owner's decision of that day
  (MuseNote's plan V3_2_0_25, "go ahead with W17 and W18", with W19 and
  W20 folded in): patches 51 to 53 are in an importer and change what it
  writes for 31 scores of the corpus by design, so that each plays its
  file's ticks. Patch 51 builds every object as before and attaches two
  in the order their ends say; patches 52 and 53 rewrite after the
  import, keep the objects the import built, and delete only a gap's
  surplus spaces and grace rests: no id of the corpus moved. Patches 51
  and 52 were amended before the tag by what the gate found. Patch 52 is
  199 lines, a walk like patch 50's.
  Fifty-four since 2026-11-12, by the owner's decision of that day
  (MuseNote's plan V3_2_0_26, "MEI's rule everywhere"): patch 54 changes
  how the engine times nested tuplets (in `layerelement.cpp`, the first
  patch outside importers, exports and the MIDI path since patch 6), and
  the MusicXML and ABC importers with it. No score of the corpus is
  nested: no output of a MusicXML, MEI or Humdrum score changed, and no
  id of one. It changes what the ABC importer builds for tuplets (13 of
  the 194 ABC files of the parity set, their ids with them), declared in
  its section.
  Fifty-five since 2026-11-12, by the owner's decision of that day
  (MuseNote's plan V3_2_0_27): patch 55 adds two options, off by default,
  one in drawing (`view_page.cpp`: labels not drawn) and one in the MIDI
  export, and changes which white space the MEI reader takes off a text.
  With the options off no page, MEI, timemap or id of the corpus changed;
  92 MIDI files did, in the spaces of their lyric texts.
- Every patch lists its date, why, test input, output change and
  upstream status in this file.
- Tags `musenote-<base>-tb.<n>` mark exact shipped states. They are
  never moved or deleted.
- A move to a newer upstream release starts a new `musenote/<base>`
  branch from that release's tag and re-applies this series.
