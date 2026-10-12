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
- Every patch lists its date, why, test input, output change and
  upstream status in this file.
- Tags `musenote-<base>-tb.<n>` mark exact shipped states. They are
  never moved or deleted.
- A move to a newer upstream release starts a new `musenote/<base>`
  branch from that release's tag and re-applies this series.
