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
| 19 | 2026-10-13 | Humdrum export: write the order a score is played in, as a label where each section and ending starts and one expansion list (09542e944) | `src/hum/humlib.cpp`, +171 / -1 | `Toolkit::ConvertMEIToHumdrum` only. A score whose one `<section>` holds an `<expansion>` that names its `<section>` and `<ending>` children (every score that came as MusicXML: 2785 of the 2785 of the corpus) gains one line `*>[A,A1,A,A2,B]`, one line `*>A` for each of them that holds a note or a rest, and a line `!LO:TX:t=` right above the label of an ending whose bracket says something else than its number. Read back, it is played in that order, and its endings are endings. A measure the converter finds nothing in is no longer written; before, it took the barline of the measure before it. A carriage return in the words of a `<dir>` is a space. None for a score with no `<expansion>`, or with one the labels cannot say. The notes and the syllables of every export are as they were (2785 of 2785); three exports gain a barline that was lost. **Not to be shipped without patch 21.** No SVG, timemap, MIDI, MEI or id changes | Not offered upstream; `develop` of 2026-10-08 writes neither |
| 20 | 2026-10-13 | Humdrum export: write the chord symbols, as text in a spine beside their staff (c92e8fa32) | `src/hum/humlib.cpp`, +115 / -67 | `Toolkit::ConvertMEIToHumdrum` only. A score with `<harm>` elements (2565 of the 2785) gains a `**cdata` spine after each staff that has one, a second line that says `*above` in it, and a line with nothing in the staves for each chord symbol that stands between two notes. Two symbols at one place are one token; the same text twice at one place is written once. A score with none is written as before. `HumGrid::setHarmonyCount` no longer writes one past the end of its counts for the hundred-and-first staff. **Declared limit: the chord symbols of a re-opened export do not follow a transposition.** No SVG, timemap, MIDI, MEI or id changes | Not offered upstream; `develop` of 2026-10-08 has the stub and the bound |
| 21 | 2026-10-13 | Humdrum reader: do not fold a whole-measure rest into the rest before it across a section label, nor one that stands under a chord symbol (c3a3eedb4) | `src/iohumdrum.cpp`, +30 / -2 | **Changes what a seeded load builds, for two kinds of Humdrum file.** A file in which the reader made one `<multiRest>` of a run of whole-measure rests that has (1) a non-null token of a `**cdata` spine on the line of a rest after the run's first, or (2) a section label (`*>A`) in its first spine between two of its rests, now keeps a measure of its own from each such rest on: it has more `<measure>` elements, every measure after that place has another ordinal, and the seeded ids of what is built after it move. Its chord symbols and its sections are no longer dropped. The patch makes no rest of several measures that was not made before. None for a file with neither: all 189 Humdrum files of the parity gate and all 2785 exports of tb.11 give the same MEI, byte for byte | Not offered upstream; `develop` of 2026-10-08 has both |
| 22 | 2026-10-13 | Humdrum export: a double barline, a dynamic at its beat and beside its hairpin, words where no note starts, the segno and coda signs, a mark given by the note it starts on, and a staff, a beat or a text the file says wrongly (22ebddf9e) | `src/hum/humlib.cpp`, +299 / -68 | `Toolkit::ConvertMEIToHumdrum` only. `right="dbl"` is written `=\|\|` (850 exports of the corpus hold one); a `<dynam>` is on the note of its beat where it was on the first note of its measure, and stays when a hairpin starts or ends on its note (17 exports of the corpus gain dynamics or hairpin signs that tb.11 lost, 38 dynamics and 19 signs; none loses one that tb.11 wrote); the words of a `<dir>` at a time no note starts (every "D.C. al Fine" and "To Coda") are written on the note before, where they were left out; `<repeatMark func="segno">` and `"coda"` are written `[segno]` and `[coda]`; a `<dir>` or a `<repeatMark>` with no `@tstamp` is written on the note, rest or chord its `@startid` names (MuseNote's own navigation marks). A `<dir>`, `<repeatMark>`, `<dynam>`, `<hairpin>` or `<staff>` for a staff the score does not have, and one whose beat is no number, is left out with a warning where it read past the end of a vector or lost the whole export; a measure that holds nothing but a change of clef keeps the barline before it. No SVG, timemap, MIDI, MEI or id changes | Not offered upstream; `develop` of 2026-10-08 has none of it |
| 23 | 2026-10-14 | What the engine says of a file is text, not the format of a message (57188f37b) | `include/vrv/vrv.h`, `src/toolkit.cpp`, `src/iohumdrum.cpp`, `src/iopae.cpp`, `src/convertfunctor.cpp`, +60 / -45 | None for any score. Forty-one log calls pass their text as the argument of `"%s"` where they passed it as the format: a percent sign in a score's words no longer ends the process when the score is exported to Humdrum, nor one in a Humdrum header line when the file is opened. The four log functions are marked as printf-like | Not offered upstream; `develop` not looked at for it |

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
- Every patch lists its date, why, test input, output change and
  upstream status in this file.
- Tags `musenote-<base>-tb.<n>` mark exact shipped states. They are
  never moved or deleted.
- A move to a newer upstream release starts a new `musenote/<base>`
  branch from that release's tag and re-applies this series.
