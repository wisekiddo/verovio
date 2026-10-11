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
| 11 | 2026-10-12 | Read a MusicXML octave line on its own part, and do not leave one waiting or open (1f1c88522) | `src/iomusxml.cpp`, +40 / -15 | A MusicXML `octave-shift` with no `<staff>` in a part that is not the first is on its own part (it was drawn over the first part, and its notes sounded an octave off); one with no note under it is not built (it took the next part's first note); one never stopped runs to the last note of its part (it was not drawn, and its notes sounded an octave off). None for a score with none of the three | Not offered upstream; `develop` of 2026-10-08 has all three |
| 12 | 2026-10-12 | Do not end the process on what the Humdrum reader or the MEI to Humdrum converter throws (15cd1c51a) | `src/iohumdrum.cpp`, `src/toolkit.cpp`, +19 / -1 | A Humdrum file the reader cannot parse is refused, and an MEI document the converter cannot read a number from gives no Humdrum, where either ended the process. None for a file that did not throw | Not offered upstream; `develop` of 2026-10-08 has both |

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

tb.7 against tb.6 (same toolchain, MuseNote's `tool/parity`, 2785
scores): 19930 outputs compared, 0 differ; seeded ids deterministic on a
fresh thread for 2785 of 2785.

The gate does not look at the Humdrum export, which patch 10 changes for
every score. Over the same 2785 scores, each exported to MEI and that
converted: all convert and none ends the process; 5 still hold a "ZZZ"
token. The text, read by a kern reader written for this check (not the
engine's), holds every note at its place and pitch for 2716; 68 differ
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
- Every patch lists its date, why, test input, output change and
  upstream status in this file.
- Tags `musenote-<base>-tb.<n>` mark exact shipped states. They are
  never moved or deleted.
- A move to a newer upstream release starts a new `musenote/<base>`
  branch from that release's tag and re-applies this series.
