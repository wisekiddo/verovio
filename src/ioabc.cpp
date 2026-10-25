/////////////////////////////////////////////////////////////////////////////
// Name:        ioabc.cpp
// Author:      Klaus Rettinghaus
// Created:     2017
// Copyright (c) Klaus Rettinghaus. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ioabc.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>

//----------------------------------------------------------------------------

#include "annot.h"
#include "artic.h"
#include "beam.h"
#include "chord.h"
#include "doc.h"
#include "dynam.h"
#include "editorial.h"
#include "fermata.h"
#include "hairpin.h"
#include "harm.h"
#include "layer.h"
#include "mdiv.h"
#include "measure.h"
#include "metersig.h"
#include "mordent.h"
#include "mrest.h"
#include "multirest.h"
#include "note.h"
#include "pb.h"
#include "pghead.h"
#include "rend.h"
#include "repeatmark.h"
#include "rest.h"
#include "sb.h"
#include "score.h"
#include "scoredef.h"
#include "section.h"
#include "slur.h"
#include "space.h"
#include "staff.h"
#include "staffdef.h"
#include "staffgrp.h"
#include "syl.h"
#include "tempo.h"
#include "text.h"
#include "tie.h"
#include "trill.h"
#include "tuplet.h"
#include "turn.h"
#include "verse.h"
#include "vrv.h"

//----------------------------------------------------------------------------

#ifndef NO_ABC_SUPPORT
#endif /* NO_ABC_SUPPORT */

namespace vrv {

#ifndef NO_ABC_SUPPORT

// Global variables:
std::string abcLine;
#define MAX_DATA_LEN 1024 // One line of the abc file would not be that long!
char dataKey[MAX_DATA_LEN];
char dataValue[MAX_DATA_LEN]; // ditto as above

const std::string pitch = "FCGDAEB";
const std::string shorthandDecoration = ".~HLMOPSTuv";
std::string keyPitchAlter = "";
int keyPitchAlterAmount = 0;

//----------------------------------------------------------------------------
// ABCInput
//----------------------------------------------------------------------------

ABCInput::ABCInput(Doc *doc) : Input(doc)
{
    m_layoutInformation = LAYOUT_ENCODED;
}

ABCInput::~ABCInput() {}

//////////////////////////////////////////////////////////////////////////

bool ABCInput::Import(const std::string &abc)
{
    std::istringstream in_stream(abc);
    return this->ParseABC(in_stream);
}

//////////////////////////////
//
// parseABC --
//

bool ABCInput::ParseABC(std::istream &infile)
{
    // initialize doc
    m_doc->Reset();
    m_doc->SetType(Raw);

    Score *score = NULL;
    Section *section = NULL;
    this->CreateHeader();
    while (!infile.eof()) {
        std::getline(infile, abcLine);
        ++m_lineNum;
        if (!abcLine.empty() && (abcLine.at(0) == 'X')) {
            // A tune begins at its X, with a number after it or without, and its header runs to its key: that may
            // be the file's last line, and may say nothing ("K:"). (An X with no number began no tune, a line was
            // read only while another followed it, and one of two characters was never a field: the header's end
            // then read the tune that was not there, or the lines of music were taken for the header)
            // A tune's key is its own. (The steps a key alters are kept outside the reader, and a key with no sharps or
            // flats did not empty them: a tune in C read after a tune in D, in the same file or by the next load, kept
            // D's sharps.) They are emptied where a tune begins, not by an X field: one inside a tune ("[X:2]") would
            // take its key from the notes after it.
            keyPitchAlter.clear();
            bool hasKey = false;
            while (true) {
                const char field = abcLine.empty() ? '\0' : abcLine.at(0);
                const bool hasValue = (abcLine.length() >= 3);
                if (hasValue || (field == 'X')) {
                    this->ReadInformationField(field, hasValue ? abcLine.substr(2) : "");
                }
                hasKey = (field == 'K') && (hasValue || (abcLine == "K:"));
                if (hasKey || infile.eof()) break;
                std::getline(infile, abcLine);
                ++m_lineNum;
            }
            if (!hasKey) break;
            if (m_title.empty()) {
                LogWarning("ABC import: Title field missing, creating empty title");
                m_title.push_back(std::make_pair("", 0));
            }
            // add work entry to meiHead
            this->CreateWorkEntry();

            this->InitScoreAndSection(score, section);

            continue;
        }
        else if (!m_mdiv || !score || !section) {
            // if m_div is not initialized - we didn't read X element, so continue until we do
            continue;
        }
        if (abcLine.empty() || (abcLine.find_first_not_of(' ') == std::string::npos)) {
            // abc tunes are separated from each other by empty lines
            this->FlushControlElements(score, section);
            continue;
        }
        else if (abcLine.at(0) == '%')
            // skipping comments and stylesheet directives
            continue;
        // (a line that begins with "::" is music, a double repeat first, and no field: it was read as one, and its
        // measures were lost. Any other first character before a colon is still taken for a field's name)
        else if ((abcLine.length() >= 3) && (abcLine.at(1) == ':') && (abcLine.at(0) != '|')
            && (abcLine.at(0) != ':')) {
            if (abcLine.at(0) != 'K') {
                this->ReadInformationField(abcLine.at(0), abcLine.substr(2));
            }
            else {
                LogWarning("ABC import: Key changes not supported");
            }
        }
        else {
            m_verseNumber = 1;
            m_lineNoteArray.clear();
            this->ReadMusicCode(abcLine, section);
        }
    }
    // A file with no tune in it (an X, and no key after it) is refused. (The tune that was not there was read)
    if (!score || !section) return false;
    this->FlushControlElements(score, section);

    if (section && score && !section->GetParent()) {
        score->AddChild(section);
    }

    m_composer.clear();
    m_info.clear();
    m_title.clear();

    m_doc->ConvertToPageBasedDoc();
    return true;
}

/**********************************
 *
 * SetBarLine
 * Translation from ABC to Verovio representation:
 *
 BARRENDITION_single     |
 BARRENDITION_end        |]
 BARRENDITION_rptboth    ::
 BARRENDITION_rptend     :|
 BARRENDITION_rptstart   |:
 BARRENDITION_dbl        ||
 */

// "::" as a bar line: two colons with no "|" after them ("::|" is a ":|" with a colon more; the colons of "|::"
// are read with their "|")
static bool IsColonPair(const std::string &musicCode, int i)
{
    const int length = (int)musicCode.length();
    if ((musicCode.at(i) != ':') || (i + 1 >= length) || (musicCode.at(i + 1) != ':')) return false;
    return (i + 2 >= length) || (musicCode.at(i + 2) != '|');
}

// "A repeat end with no start goes back to the beginning of the tune, or to the latest double bar line or end of
// repeated section" (ABC 2.1, 4.8, in other words): tunes are written "A :| B :|" and mean A A B B. The start
// that is meant is written on the measure it stands before, so that the tune plays so whoever reads it next.
// (Left unwritten, the second end went back to the beginning, the rule of the other formats: A A B A B)
void ABCInput::ImplyRepeatStart(Measure *measure)
{
    const auto isEnd = [](data_BARRENDITION sign) {
        return (sign == BARRENDITION_rptend) || (sign == BARRENDITION_rptboth);
    };
    const auto isStart = [](data_BARRENDITION sign) {
        return (sign == BARRENDITION_rptstart) || (sign == BARRENDITION_rptboth);
    };
    const auto isDouble = [](data_BARRENDITION sign) {
        return (sign == BARRENDITION_dbl) || (sign == BARRENDITION_end);
    };
    // The start is written over whatever bar line the measure begins with: a tune whose lines begin with "|" is
    // the ordinary way to write one, and its measures have a left bar line. (Only a measure with none was given
    // the start.) An end that stands there, of the repeat before, stays with it
    const auto endRepeat = [&]() {
        if (!m_hasRepeatStart && m_repeatsFrom) {
            m_repeatsFrom->SetLeft(isEnd(m_repeatsFrom->GetLeft()) ? BARRENDITION_rptboth : BARRENDITION_rptstart);
        }
        m_hasRepeatStart = false;
    };
    const data_BARRENDITION left = measure->GetLeft();
    const data_BARRENDITION right = measure->GetRight();
    // A sign met before the measure's first note is on its left, and is the end of what stands before it: an end
    // there ("C | :| E") ends a repeat, and it or a double bar ("C |" and "|| E" on the next line, or "[|") is a
    // place to go back to. (Only signs on the right were looked at)
    if (isEnd(left)) endRepeat();
    if (m_repeatsFromNext || isEnd(left) || isDouble(left)) {
        m_repeatsFrom = measure;
        m_repeatsFromNext = false;
    }
    if (isStart(left)) m_hasRepeatStart = true;
    if (isEnd(right)) endRepeat();
    if (isStart(right)) m_hasRepeatStart = true;
    if (isEnd(right) || isDouble(right)) m_repeatsFromNext = true;
}

int ABCInput::SetBarLine(const std::string &musicCode, int i)
{
    data_BARRENDITION barLine = BARRENDITION_NONE;
    // A "|" with nothing more to it; after a "[" it is the thick-thin double bar (which was read as a bar line of
    // one stroke)
    const data_BARRENDITION alone
        = ((i >= 1) && (musicCode.at(i - 1) == '[')) ? BARRENDITION_dbl : BARRENDITION_single;
    // "::", which has no "|" of its own, or a "|" with a ":" before it
    if ((musicCode.at(i) == ':') || (i >= 1 && musicCode.at(i - 1) == ':'))
        barLine = BARRENDITION_rptend;
    else if (i + 1 < (int)musicCode.length()) {
        switch (musicCode.at(i + 1)) {
            case ':':
                barLine = BARRENDITION_rptstart;
                ++i;
                break;
            case '|':
                barLine = BARRENDITION_dbl;
                ++i;
                break;
            case ']':
                barLine = BARRENDITION_end;
                ++i;
                break;
            default: barLine = alone; break;
        }
    }
    else {
        barLine = alone;
    }
    // if the measure is still empty, put the bar line on the left
    if (!m_layer->GetChildCount()) {
        m_barLines.first = barLine;
    }
    else {
        m_barLines.second = barLine;
    }
    return i;
}

void ABCInput::CalcUnitNoteLength()
{
    MeterSig *meterSig = vrv_cast<MeterSig *>(m_doc->GetFirstScoreDef()->FindDescendantByType(METERSIG));
    if (!meterSig || !meterSig->HasUnit() || double(meterSig->GetTotalCount()) / double(meterSig->GetUnit()) >= 0.75) {
        m_unitDur = 8;
        m_durDefault = DURATION_8;
    }
    else {
        m_unitDur = 16;
        m_durDefault = DURATION_16;
    }
}

void ABCInput::AddLayerElement()
{
    // exit if there is nothing to add
    if (!m_noteStack.size()) return;
    // if just one note in the stack - add it do the layer directly
    if (m_noteStack.size() == 1) {
        if (m_containerElement.m_element && (ElementType::Tuplet == m_containerElement.m_type)) {
            m_containerElement.m_element->AddChild(m_noteStack.back());
            if (!--m_containerElement.m_count) {
                m_layer->AddChild(m_containerElement.m_element);
                m_containerElement = {};
            }
        }
        else {
            m_layer->AddChild(m_noteStack.back());
        }
        m_noteStack.clear();
        return;
    }
    // otherwise we can have beam or tuplet (for now)
    // Nothing to beam where no note is among them (two changes of clef in a row): they go to the layer as they are.
    // (They were put into a beam first, and so had two parents.) A tuplet that waits for the rest of its notes is
    // closed with those it has: it was deleted with them, and the line's lyrics then read a note that was gone
    const bool hasNote = std::any_of(m_noteStack.begin(), m_noteStack.end(),
        [](LayerElement *element) { return element->Is(NOTE) || element->FindDescendantByType(NOTE); });
    if (!hasNote) {
        if (m_containerElement.m_element && m_containerElement.m_element->GetChildCount()) {
            m_layer->AddChild(m_containerElement.m_element);
            m_containerElement.m_element = NULL;
        }
        for (LayerElement *element : m_noteStack) {
            m_layer->AddChild(element);
        }
    }
    else {
        Beam *beam = new Beam();
        // add stacked notes to the current element
        for (LayerElement *element : m_noteStack) {
            beam->AddChild(element);
        }
        LayerElement *element = NULL;
        if (m_containerElement.m_element && (ElementType::Tuplet == m_containerElement.m_type)) {
            element = m_containerElement.m_element;
            element->AddChild(beam);
            m_containerElement.m_element = NULL;
        }
        // otherwise default to it being beam
        else {
            element = beam;
        }
        m_layer->AddChild(element);
    }
    // clean-up leftover data, if any
    if (m_containerElement.m_element) delete m_containerElement.m_element;

    m_containerElement = {};
    m_noteStack.clear();
}

// The number of a tuplet [text] begins with, 0 where it has none, or -1 where it is too long for an int. (atoi kept
// what 32 bits hold of such a number: "(99999999999" was a tuplet of 1215752191 notes)
static int TupletNumber(const std::string &text)
{
    const long long number = std::strtoll(text.c_str(), NULL, 10);
    return (number > INT_MAX) ? -1 : static_cast<int>(number);
}

int ABCInput::ParseTuplet(const std::string &musicCode, int index)
{
    constexpr std::string_view tupletElements = "(:0123456789 ";
    const size_t tupletEnd = musicCode.find_first_not_of(tupletElements, ++index);
    const std::string tupletStr = musicCode.substr(index, tupletEnd - index);

    size_t separator = tupletStr.find_first_of(":");
    // Get tuplet number first 9:_:_
    int tupletNum = 0;
    if (separator != std::string::npos) {
        const std::string tupletNumStr = tupletStr.substr(0, separator);
        tupletNum = TupletNumber(tupletNumStr);
        ++separator;
    }
    else {
        tupletNum = TupletNumber(tupletStr);
    }
    // Get tuplet number base _:3:_
    int tupletNumbase = 0;
    if (separator != std::string::npos) {
        size_t secondSeparator = tupletStr.find_first_of(":", separator);
        if (secondSeparator != std::string::npos) {
            if (secondSeparator != separator) {
                std::string tupletNumbaseStr = tupletStr.substr(separator, secondSeparator - separator);
                tupletNumbase = TupletNumber(tupletNumbaseStr);
                separator = secondSeparator + 1;
            }
        }
        else {
            std::string tupletNumbaseStr = tupletStr.substr(separator);
            tupletNumbase = TupletNumber(tupletNumbaseStr);
            separator = secondSeparator + 1;
        }
    }
    // return index of the last element in tuplet, so that we point to the actual notes when incrementing 'i'
    // (the end of the line, where the tuplet is the last thing on it: "nowhere" was returned as -2, and the line
    // was read from before its start)
    const int last = static_cast<int>((tupletEnd == std::string::npos) ? musicCode.length() : tupletEnd) - 1;
    // A tuplet ABC cannot mean is none, and its notes are read as they are written: one of no notes, and one with a
    // number too long for an int. (Their durations ran past what the engine's fractions hold: the MIDI writer refused
    // them, the Humdrum export wrote negative durations, and a load of the tune's MEI or Humdrum did not always come
    // back.) A second number of 0 is read as none written, as before
    if ((tupletNum < 1) || (tupletNumbase < 0)) {
        LogWarning("ABC import: '(%s' is no tuplet, its notes are read as written", tupletStr.c_str());
        return last;
    }
    Tuplet *tuplet = new Tuplet();
    // List of tuplets with default base of 3
    const std::unordered_set<int> threeBase = { 2, 4, 8, 9 };
    if (!tupletNumbase) {
        tupletNumbase = threeBase.contains(tupletNum) ? 3 : 2;
    }
    // Get number of elements supposed to be in the tuplet _:_:9
    // Ignore this for the time being
    tuplet->SetNum(tupletNum);
    tuplet->SetNumbase(tupletNumbase);
    m_containerElement = { ElementType::Tuplet, tuplet, tupletNum };

    return last;
}

void ABCInput::AddAnnot(const std::string &remark)
{
    // remarks
    Annot *annot = new Annot();
    Text *text = new Text();
    text->SetText(UTF8to32(remark));
    annot->AddChild(text);
    // todo: add to correct place
    m_layer->AddChild(annot);
}

void ABCInput::AddArticulation(LayerElement *element)
{
    assert(element);

    Artic *artic = new Artic();
    artic->SetArtic(m_artic);
    element->AddChild(artic);

    m_artic.clear();
}

void ABCInput::AddChordSymbol(LayerElement *element)
{
    assert(element);

    // there should always only be one element in the harmony stack
    if (!m_harmStack.empty() && !m_harmStack.back()->HasStartid()) {
        m_harmStack.back()->SetStartid("#" + element->GetID());
        m_harmStack.clear();
    }

    m_harmStack.clear();
}

void ABCInput::AddDynamic(LayerElement *element)
{
    assert(element);

    for (const std::string &str : m_dynam) {
        Dynam *dynam = new Dynam();
        dynam->SetStartid("#" + element->GetID());
        Text *text = new Text();
        text->SetText(UTF8to32(str));
        dynam->AddChild(text);
        m_controlElements.push_back(std::make_pair(m_layer->GetID(), dynam));
    }

    m_dynam.clear();
}

void ABCInput::AddFermata(LayerElement *element)
{
    assert(element);

    Fermata *fermata = new Fermata();
    fermata->SetStartid("#" + element->GetID());
    fermata->SetPlace(m_fermata);
    m_controlElements.push_back(std::make_pair(m_layer->GetID(), fermata));

    m_fermata = STAFFREL_NONE;
}

void ABCInput::AddOrnaments(LayerElement *element)
{
    assert(element);

    std::string refId = "#" + element->GetID();
    // note->SetOrnam(m_ornam);
    if (m_ornam.find("m") != std::string::npos) {
        Mordent *mordent = new Mordent();
        mordent->SetStartid(refId);
        mordent->SetForm(mordentLog_FORM_lower);
        m_controlElements.push_back(std::make_pair(m_layer->GetID(), mordent));
    }
    if (m_ornam.find("M") != std::string::npos) {
        Mordent *mordent = new Mordent();
        mordent->SetStartid(refId);
        mordent->SetForm(mordentLog_FORM_upper);
        m_controlElements.push_back(std::make_pair(m_layer->GetID(), mordent));
    }
    if (m_ornam.find("s") != std::string::npos) {
        Turn *turn = new Turn();
        turn->SetStartid(refId);
        turn->SetForm(turnLog_FORM_lower);
        m_controlElements.push_back(std::make_pair(m_layer->GetID(), turn));
    }
    if (m_ornam.find("S") != std::string::npos) {
        Turn *turn = new Turn();
        turn->SetStartid(refId);
        turn->SetForm(turnLog_FORM_upper);
        m_controlElements.push_back(std::make_pair(m_layer->GetID(), turn));
    }
    if (m_ornam.find("T") != std::string::npos) {
        Trill *trill = new Trill();
        trill->SetStartid(refId);
        m_controlElements.push_back(std::make_pair(m_layer->GetID(), trill));
    }

    m_ornam.clear();
}

void ABCInput::AddRepeatMark(LayerElement *element)
{
    assert(element);

    RepeatMark *rm = new RepeatMark();
    rm->SetStartid("#" + element->GetID());
    rm->SetFunc(m_repeatMark);
    m_controlElements.push_back(std::make_pair(m_layer->GetID(), rm));

    m_repeatMark = repeatMarkLog_FUNC_NONE;
}

void ABCInput::AddTie()
{
    if (!m_tieStack.empty()) {
        LogWarning("ABC import: '%s' already tied", m_ID.c_str());
        return;
    }
    if (!m_ID.empty()) {
        Tie *tie = new Tie();
        tie->SetStartid(m_ID);
        m_tieStack.push_back(tie);
        m_controlElements.push_back(std::make_pair(m_layer->GetID(), tie));
    }
}

void ABCInput::StartSlur()
{
    Slur *openSlur = new Slur();
    m_slurStack.push_back(openSlur);
    m_controlElements.push_back(std::make_pair(m_layer->GetID(), openSlur));
}

void ABCInput::EndSlur()
{
    if (!m_slurStack.empty()) {
        if (!m_slurStack.back()->HasStartid()) {
            LogError("ABC import: Empty slur found");
            m_slurStack.pop_back();
            return;
        }
        for (auto riter = m_slurStack.rbegin(); riter != m_slurStack.rend(); ++riter) {
            if (((*riter)->GetStartid()).find(m_ID) == std::string::npos) {
                (*riter)->SetEndid("#" + m_ID);
                m_slurStack.erase((riter + 1).base());
                break;
            }
        }
        return;
    }
    LogWarning("ABC import: Closing slur for element '%s' could not be matched", m_ID.c_str());
}

void ABCInput::ParseDecoration(const std::string &decorationString)
{
    // shorthand decorations hard-coded !
    if (isdigit(decorationString[0])) {
        LogWarning("ABC import: Fingering not supported", decorationString.c_str());
        return;
    }
    if (decorationString == ".") {
        m_artic.push_back(ARTICULATION_stacc);
    }
    else if ((decorationString == "~") || (decorationString == "roll")) {
        m_ornam.push_back('S');
    }
    else if ((decorationString == "trill") || (decorationString == "T")) {
        m_ornam.push_back('T');
    }
    else if ((decorationString == "mordent") || (decorationString == "lowermordent") || (decorationString == "M")) {
        m_ornam.push_back('m');
    }
    else if ((decorationString == "pralltriller") || (decorationString == "uppermordent")
        || (decorationString == "P")) {
        m_ornam.push_back('M');
    }
    else if (decorationString == "turn") {
        m_ornam.push_back('S');
    }
    else if (decorationString == "invertedturn") {
        m_ornam.push_back('s');
    }
    else if ((decorationString == ">") || (decorationString == "accent") || (decorationString == "emphasis")) {
        m_artic.push_back(ARTICULATION_acc);
    }
    else if ((decorationString == "^") || (decorationString == "marcato")) {
        m_artic.push_back(ARTICULATION_marc);
    }
    else if ((decorationString == "fermata") || (decorationString == "H")) {
        m_fermata = STAFFREL_above;
    }
    else if ((decorationString == "invertedfermata")) {
        m_fermata = STAFFREL_below;
    }
    else if (decorationString == "tenuto") {
        m_artic.push_back(ARTICULATION_ten);
    }
    else if ((decorationString == "+") || (decorationString == "plus")) {
        m_artic.push_back(ARTICULATION_stop);
    }
    else if (decorationString == "snap") {
        m_artic.push_back(ARTICULATION_snap);
    }
    else if (decorationString == "slide") {
        m_artic.push_back(ARTICULATION_scoop);
    }
    else if (decorationString == "wedge") {
        m_artic.push_back(ARTICULATION_stacciss);
    }
    else if ((decorationString == "upbow") || (decorationString == "u")) {
        m_artic.push_back(ARTICULATION_upbow);
    }
    else if ((decorationString == "downbow") || (decorationString == "v")) {
        m_artic.push_back(ARTICULATION_dnbow);
    }
    else if (decorationString == "open") {
        m_artic.push_back(ARTICULATION_open);
    }
    else if ((decorationString == "pppp") || (decorationString == "ppp") || (decorationString == "pp")
        || (decorationString == "p") || (decorationString == "mp") || (decorationString == "mf")
        || (decorationString == "f") || (decorationString == "ff") || (decorationString == "fff")
        || (decorationString == "ffff") || (decorationString == "sfz")) {
        m_dynam.push_back(decorationString);
    }
    else if (decorationString == "segno") {
        m_repeatMark = repeatMarkLog_FUNC_segno;
    }
    else if (decorationString == "coda") {
        m_repeatMark = repeatMarkLog_FUNC_coda;
    }
    else if (decorationString == "D.S.") {
        m_repeatMark = repeatMarkLog_FUNC_dalSegno;
    }
    else if (decorationString == "D.C.") {
        m_repeatMark = repeatMarkLog_FUNC_daCapo;
    }
    else {
        LogWarning("ABC import: Decoration %s not supported", decorationString.c_str());
    }
}

//////////////////////////////
//
// parse information fields
//

void ABCInput::ParseInstruction(const std::string &instruction)
{
    if (!strncmp(instruction.c_str(), "abc-include", 11)) {
        LogWarning("ABC import: Include field is ignored");
    }
    else if (!strncmp(instruction.c_str(), "linebreak", 9)) {
        if (instruction.find("<none>") != std::string::npos) {
            m_linebreak = '\0';
            m_layoutInformation = LAYOUT_NONE;
        }
        else {
            m_linebreak = '$';
            m_layoutInformation = LAYOUT_ENCODED;
            LogWarning("ABC import: Default linebreaks are used for now.");
        }
    }
    else if (!strncmp(instruction.c_str(), "decoration", 10)) {
        // (with nothing after the word, the character was read past the end of the text)
        if (instruction.length() > 11) m_decoration = instruction[11];
    }
}

void ABCInput::ParseKey(std::string &keyString)
{
    int i = 0;
    m_ID = "";
    short int accidNum = 0;
    data_MODE mode = MODE_NONE;
    m_key = new KeySig();
    m_key->IsAttribute(true);
    m_clef = new Clef();
    while (isspace(keyString[i])) ++i;

    // set key.pname
    if (pitch.find(keyString[i]) != std::string::npos) {
        accidNum = int(pitch.find(keyString[i])) - 1;
        keyString[i] = tolower(keyString[i]);
        m_key->SetPname(m_key->AttPitch::StrToPitchname(keyString.substr(i, 1)));
        ++i;
    }
    while (isspace(keyString[i])) ++i;

    // set key.accid
    switch (keyString[i]) {
        case '#':
            m_key->SetAccid(ACCIDENTAL_GESTURAL_basic_s);
            accidNum += 7;
            ++i;
            break;
        case 'b':
            m_key->SetAccid(ACCIDENTAL_GESTURAL_basic_f);
            accidNum -= 7;
            ++i;
            break;
        default: break;
    }

    // set key.mode
    if (m_key->HasPname()) {
        // when no mode is indicated, major is assumed
        mode = MODE_major;
        while (isspace(keyString[i])) ++i;

        if (keyString[i]) {
            std::string modeString(&keyString[i]);
            // capitalization is ignored for the modes
            // and in fact only the first three letters of each mode are parsed
            modeString.resize(3);
            for (char &c : modeString) {
                c = tolower(c);
            }

            if (modeString == "min" || (modeString[0] == 'm' && !isalpha(modeString[1]))) {
                mode = MODE_minor;
                accidNum -= 3;
            }
            else if (modeString == "ion") {
                mode = MODE_dorian;
            }
            else if (modeString == "dor") {
                mode = MODE_dorian;
                accidNum -= 2;
            }
            else if (modeString == "phr") {
                mode = MODE_phrygian;
                accidNum -= 4;
            }
            else if (modeString == "lyd") {
                mode = MODE_lydian;
                accidNum += 1;
            }
            else if (modeString == "mix") {
                mode = MODE_mixolydian;
                accidNum -= 1;
            }
            else if (modeString == "aeo") {
                mode = MODE_aeolian;
                accidNum -= 3;
            }
            else if (modeString == "loc") {
                mode = MODE_locrian;
                accidNum -= 5;
            }
        }
    }
    m_key->SetMode(mode);

    // we need set @key.sig for correct rendering
    if (accidNum != 0) {

        std::string keySig;
        unsigned long posStart = 0;
        // A key of more than seven flats or sharps (F flat, D flat minor) alters all seven steps. (For flats the
        // steps were read from before the first of the seven, which threw)
        const size_t posEnd = std::min<size_t>(abs(accidNum), pitch.size());

        if (accidNum < 0) {
            keySig = StringFormat("%df", abs(accidNum));
            posStart = pitch.size() - posEnd;
            keyPitchAlterAmount = -1;
        }
        else {
            keySig = StringFormat("%ds", accidNum);
            keyPitchAlterAmount = 1;
        }

        // m_doc->m_scoreDef.SetSig(keySig);
        m_key->SetSig(m_key->AttKeySigLog::StrToKeysignature(keySig));
        keyPitchAlter = pitch.substr(posStart, posEnd);
    }

    // set clef
    // <clef name> - may be treble, alto, tenor, bass, perc or none. perc selects the drum clef. clef= may be omitted.
    // [<line number>] - indicates on which staff line the base clef is written. Defaults are: treble: 2; alto: 3;
    // tenor: 4; bass: 4.
    // [+8 | -8] - draws '8' above or below the staff. The player will transpose the notes one octave higher or lower.
    if (keyString.find("alto") != std::string::npos) {
        m_clef->SetShape(CLEFSHAPE_C);
        i += 4;
        m_clef->SetLine(3);
    }
    else if (keyString.find("tenor") != std::string::npos) {
        m_clef->SetShape(CLEFSHAPE_C);
        i += 5;
        m_clef->SetLine(4);
    }
    else if (keyString.find("bass") != std::string::npos) {
        m_clef->SetShape(CLEFSHAPE_F);
        i += 4;
        m_clef->SetLine(4);
    }
    else if (keyString.find("perc") != std::string::npos) {
        LogWarning("ABC Input: Drum clef is not supported");
    }
    else if (keyString.find("none") != std::string::npos) {
        i += 4;
        m_clef->SetShape(CLEFSHAPE_NONE);
    }
    else {
        m_clef->SetShape(CLEFSHAPE_G);
        m_clef->SetLine(2);
    }

    if (keyString.find("transpose=", i) != std::string::npos) {
        i = int(keyString.find("transpose=", i)) + 10;
        std::string transStr;
        while (keyString[i] == '-' || isdigit(keyString[i])) {
            transStr.push_back(keyString[i]);
            ++i;
        }
        m_transpose = atoi(transStr.c_str());
    }

    // stafflines
    if (keyString.find("stafflines=", i) != std::string::npos) {
        int pos = int(keyString.find("stafflines=", i)) + 11;
        m_stafflines = atoi(keyString.substr(pos, keyString.find_first_not_of("0123456789", pos)).c_str());
    }
}

void ABCInput::ParseUnitNoteLength(const std::string &unitNoteLength)
{
    if (unitNoteLength.find('/'))
        m_unitDur = atoi(&unitNoteLength[unitNoteLength.find('/') + 1]);
    else if (atoi(unitNoteLength.c_str()) == 1)
        m_unitDur = 1;
    switch (m_unitDur) {
        case 1: m_durDefault = DURATION_1; break;
        case 2: m_durDefault = DURATION_2; break;
        case 4: m_durDefault = DURATION_4; break;
        case 8: m_durDefault = DURATION_8; break;
        case 16: m_durDefault = DURATION_16; break;
        case 32: m_durDefault = DURATION_32; break;
        case 64: m_durDefault = DURATION_64; break;
        case 128: m_durDefault = DURATION_128; break;
        case 256: m_durDefault = DURATION_256; break;
        default: break;
    }
}

void ABCInput::ParseMeter(const std::string &meterString)
{
    m_meter = new MeterSig();
    if (meterString.find('C') != std::string::npos) {
        if (meterString[meterString.find('C') + 1] == '|') {
            m_meter->SetSym(METERSIGN_cut);
            m_meter->SetCount({ { 2 }, MeterCountSign::None });
            m_meter->SetUnit(2);
        }
        else {
            m_meter->SetSym(METERSIGN_common);
            m_meter->SetCount({ { 4 }, MeterCountSign::None });
            m_meter->SetUnit(4);
        }
    }
    else if (meterString.find('/')) {
        std::string meterCount = meterString.substr(0, meterString.find('/'));
        if (meterCount.front() == '(' && meterCount.back() == ')')
            meterCount = meterCount.substr(1, meterCount.length() - 1);
        // this is a little "hack", until libMEI is fixed
        m_meter->SetCount({ { atoi(meterCount.c_str()) }, MeterCountSign::None });
        // (a unit below zero, as a number too long for an int comes out, is none: the MIDI writer never came back
        // from working out which power of two it is)
        m_meter->SetUnit(std::max(0, atoi(&meterString[meterString.find('/') + 1])));
    }
}

void ABCInput::ParseTempo(const std::string &tempoString)
{
    Tempo *tempo = new Tempo();
    if (tempoString.find('=') != std::string::npos) {
        // (std::stof threw where no number follows the "=", as in "Q:1/4=fast", or one too large. No number is
        // read as 0, which is no tempo)
        const float mm = std::strtof(tempoString.c_str() + tempoString.find('=') + 1, NULL);
        if (std::isfinite(mm)) tempo->SetMm(mm);
    }
    if (tempoString.find('\"') != std::string::npos) {
        std::string tempoWord = tempoString.substr(tempoString.find('\"') + 1);
        // to the closing quote, or to the end where there is none (resize threw for "no position")
        tempoWord = tempoWord.substr(0, tempoWord.find('\"'));
        if (!tempoWord.empty()) {
            Text *text = new Text();
            text->SetText(UTF8to32(tempoWord));
            tempo->AddChild(text);
        }
    }
    // this has to be fixed
    tempo->SetTstamp(1);
    m_tempoStack.push_back(tempo);
    LogWarning("ABC import: Tempo definitions are not fully supported yet");
}

void ABCInput::ParseReferenceNumber(const std::string &referenceNumberString)
{
    // The X: field is also used to indicate the start of the tune
    m_mdiv = new Mdiv();
    m_mdiv->SetVisibility(Visible);
    if (!referenceNumberString.empty()) {
        const int mdivNum = atoi(referenceNumberString.c_str());
        if (mdivNum < 1) {
            LogError("ABC import: reference number should be a positive integer");
        }
        m_mdiv->SetN(std::to_string(mdivNum));
    }
    m_doc->AddChild(m_mdiv);

    // reset unit note length
    m_durDefault = DURATION_NONE;

    // a tune's repeats are its own
    m_repeatsFrom = NULL;
    m_repeatsFromNext = false;
    m_hasRepeatStart = false;

    // reset information fields
    m_composer.clear();
    m_history.clear();
    m_info.clear();
    m_origin.clear();
    m_title.clear();
}

void ABCInput::PrintInformationFields(Score *score)
{
    PgHead *pgHead = new PgHead();
    pgHead->SetFunc(PGFUNC_first);
    for (const auto &it : m_title) {
        Rend *titleRend = new Rend();
        titleRend->SetHalign(HORIZONTALALIGNMENT_center);
        titleRend->SetValign(VERTICALALIGNMENT_middle);
        if (it != m_title.front()) {
            data_FONTSIZE fontsize;
            fontsize.SetTerm(FONTSIZETERM_small);
            titleRend->SetFontsize(fontsize);
        }
        Text *text = new Text();
        text->SetText(UTF8to32(it.first));
        titleRend->AddChild(text);
        pgHead->AddChild(titleRend);
    }
    for (const auto &it : m_composer) {
        Rend *compRend = new Rend();
        compRend->SetHalign(HORIZONTALALIGNMENT_right);
        compRend->SetValign(VERTICALALIGNMENT_bottom);
        Text *composer = new Text();
        composer->SetText(UTF8to32(it.first));
        compRend->AddChild(composer);
        if (!m_origin.empty()) {
            Text *origin = new Text();
            origin->SetText(UTF8to32(" (" + m_origin.front().first + ")"));
            compRend->AddChild(origin);
        }
        pgHead->AddChild(compRend);
    }
    if (m_composer.empty() && !m_origin.empty()) {
        Rend *originRend = new Rend();
        originRend->SetHalign(HORIZONTALALIGNMENT_right);
        originRend->SetValign(VERTICALALIGNMENT_bottom);
        Text *origin = new Text();
        origin->SetText(UTF8to32("(" + m_origin.front().first + ")"));
        originRend->AddChild(origin);
        pgHead->AddChild(originRend);
    }
    assert(score->GetScoreDef());
    score->GetScoreDef()->AddChild(pgHead);
}

void ABCInput::CreateHeader()
{
    pugi::xml_node meiHead = m_doc->m_header.append_child("meiHead");

    // <fileDesc> //
    pugi::xml_node fileDesc = meiHead.append_child("fileDesc");
    pugi::xml_node fileTitleStmt = fileDesc.append_child("titleStmt");
    pugi::xml_node fileTitle = fileTitleStmt.append_child("title");
    fileTitle.text().set(m_filename.c_str());
    if (!m_composer.empty()) {
        for (const auto &it : m_composer) {
            pugi::xml_node composer = fileTitleStmt.append_child("composer");
            composer.text().set((it.first).c_str());
            composer.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
            composer.append_attribute("analog").set_value("abc:C");
        }
    }

    pugi::xml_node pubStmt = fileDesc.append_child("pubStmt");
    pubStmt.append_child(pugi::node_pcdata);

    // <notesStmt> //
    if (!m_notes.empty()) {
        pugi::xml_node notes = fileDesc.append_child("notesStmt");
        for (const auto &it : m_notes) {
            pugi::xml_node annot = notes.append_child("annot");
            annot.text().set((it.first).c_str());
            annot.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
            annot.append_attribute("analog").set_value("abc:N");
        }
    }

    // <encodingDesc> //
    pugi::xml_node encodingDesc = meiHead.append_child("encodingDesc");
    pugi::xml_node appInfo = encodingDesc.append_child("appInfo");
    pugi::xml_node app = appInfo.append_child("application");
    pugi::xml_node appName = app.append_child("name");
    appName.text().set("Verovio");
    pugi::xml_node appText = app.append_child("p");
    appText.text().set("Transcoded from abc music");

    // isodate and version //
    const time_t t = time(0); // get time now
    struct tm *now = localtime(&t);
    std::string dateStr = StringFormat("%d-%02d-%02dT%02d:%02d:%02d", now->tm_year + 1900, now->tm_mon + 1,
        now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
    app.append_attribute("isodate").set_value(dateStr.c_str());
    app.append_attribute("version").set_value(GetVersion().c_str());

    m_workList = meiHead.append_child("workList");
}

void ABCInput::CreateWorkEntry()
{
    // <work> //
    pugi::xml_node work = m_workList.append_child("work");
    work.append_attribute("n").set_value(m_mdiv->GetN().c_str());
    work.append_attribute("data").set_value(StringFormat("#%s", m_mdiv->GetID().c_str()).c_str());
    for (const auto &it : m_title) {
        pugi::xml_node title = work.append_child("title");
        title.text().set((it.first).c_str());
        if (it.second != 0) title.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
        title.append_attribute("analog").set_value("abc:T");
        if (it == m_title.front()) {
            title.append_attribute("type").set_value("main");
        }
        else {
            title.append_attribute("type").set_value("alternative");
        }
    }
    if (!m_composer.empty()) {
        for (const auto &it : m_composer) {
            pugi::xml_node composer = work.append_child("composer");
            composer.text().set((it.first).c_str());
            composer.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
            composer.append_attribute("analog").set_value("abc:C");
        }
    }
    if (!m_history.empty()) {
        pugi::xml_node history = work.append_child("history");
        history.append_attribute("analog").set_value("abc:H");
        for (const auto &it : m_history) {
            pugi::xml_node histLine = history.append_child("p");
            histLine.text().set((it.first).c_str());
            histLine.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
        }
    }
    if (!m_info.empty()) {
        pugi::xml_node notes = work.append_child("notesStmt");
        for (const auto &it : m_info) {
            pugi::xml_node annot = notes.append_child("annot");
            annot.text().set((it.first).first.c_str());
            annot.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.first.second).c_str());
            annot.append_attribute("analog").set_value(StringFormat("abc:%c", it.second).c_str());
        }
    }
}

void ABCInput::FlushControlElements(Score *score, Section *section)
{
    Layer *layer = NULL;
    Measure *measure = NULL;
    for (auto &iter : m_controlElements) {
        if (!measure || (layer && layer->GetID() != iter.first)) {
            layer = dynamic_cast<Layer *>(section->FindDescendantByID(iter.first));
        }
        if (!layer) {
            LogWarning("ABC import: Element '%s' could not be assigned to layer '%s'",
                iter.second->GetClassName().c_str(), iter.first.c_str());
            // It may still wait for its note or its end (a chord symbol, an open slur or a tie in a last measure
            // that has no bar line, before the empty line that ends the tune): the next note then read it, or
            // wrote its end into it, after it was deleted
            std::erase(m_harmStack, iter.second);
            std::erase(m_slurStack, iter.second);
            std::erase(m_tieStack, iter.second);
            delete iter.second;
            iter.second = NULL;
            continue;
        }
        measure = vrv_cast<Measure *>(layer->GetFirstAncestor(MEASURE));
        assert(measure);
        measure->AddChild(iter.second);
    }
    if (!section->GetParent()) {
        score->AddChild(section);
    }

    m_controlElements.clear();
}

void ABCInput::InitScoreAndSection(Score *&score, Section *&section)
{
    // create score
    assert(m_mdiv != NULL);
    score = new Score();
    m_mdiv->AddChild(score);

    StaffGrp *staffGrp = new StaffGrp();
    // create staff
    StaffDef *staffDef = new StaffDef();
    staffDef->SetN(1);
    staffDef->SetLines(m_stafflines);
    staffDef->SetTransSemi(m_transpose);
    if (m_clef) {
        staffDef->AddChild(m_clef);
        m_clef = NULL;
    }
    if (m_meter) {
        staffDef->AddChild(m_meter);
        m_meter = NULL;
    }
    staffGrp->AddChild(staffDef);
    // create page head
    this->PrintInformationFields(score);
    assert(score->GetScoreDef());
    score->GetScoreDef()->AddChild(staffGrp);
    if (m_key) {
        score->GetScoreDef()->AddChild(m_key);
        m_key = NULL;
    }

    // create section
    section = new Section();
    // start with a new page
    if (m_linebreak != '\0') {
        Pb *pb = new Pb();
        pb->SetID(StringFormat("abcLine%02d", m_lineNum + 1));
        section->AddChild(pb);
    }
    // calculate default unit note length
    if (m_durDefault == DURATION_NONE) {
        this->CalcUnitNoteLength();
    }
    assert(score->GetScoreDef());
    score->GetScoreDef()->SetDurDefault(m_durDefault);
    m_durDefault = DURATION_NONE;

    // read music code
    m_layer = new Layer();
    m_layer->SetN(1);
}

void ABCInput::ParseLyrics()
{
    std::vector<std::pair<Syl *, int>> syllables;
    constexpr std::string_view delimiters = "-_*~ ";
    // skipping w:, so start from third element
    std::size_t start = 2;
    std::size_t found = abcLine.find_first_of(delimiters, 2);
    while (found != std::string::npos) {
        // Counter indicates for how many notes verse should be held. This defaults to 1, unless '_' is found
        int counter = 1;
        std::string syllable = "";
        sylLog_CON sylType = sylLog_CON_NONE;
        if (abcLine.at(found) == '_') {
            while ((found < abcLine.size()) && (abcLine.at(found) == '_')) {
                ++counter;
                ++found;
            }
            --found;
            sylType = sylLog_CON_u;
        }
        else if (abcLine.at(found) == '~') {
            counter = 0;
            sylType = sylLog_CON_s;
        }
        else if (abcLine.at(found) == '-') {
            if (abcLine.at(found - 1) == '\\') {
                counter = 0;
            }
            sylType = sylLog_CON_d;
        }
        else if (abcLine.at(found) == '*') {
            // skip one note
            ++counter;
        }
        // separate syllable from delimiters to form syl that we want to add
        syllable = abcLine.substr(start, found - start);
        syllable.erase(
            std::remove_if(syllable.begin(), syllable.end(), [](unsigned char x) { return (x == '_') || (x == '\\'); }),
            syllable.end());
        if (!syllable.empty()) {
            Text *sylText = new Text();
            sylText->SetText(UTF8to32(syllable));
            Syl *syl = new Syl();
            syl->AddChild(sylText);
            syl->SetCon(sylType);
            if (sylType == sylLog_CON_d) {
                syl->SetWordpos(sylLog_WORDPOS_m);
            }
            syllables.push_back({ syl, counter });
        }

        // find next delimeter in the string
        start = found + 1;
        found = abcLine.find_first_of(delimiters, start);
        // if none found, the rest of the string is going to serve as last syl
        if ((found == std::string::npos) && (start < abcLine.size())) {
            std::string syllable = abcLine.substr(start);
            if (!syllable.empty() && syllable[syllable.size() - 1] == '\r') syllable.erase(syllable.size() - 1);
            Text *sylText = new Text();
            sylText->SetText(UTF8to32(syllable));
            Syl *syl = new Syl();
            syl->AddChild(sylText);
            syl->SetCon(sylType);
            syllables.push_back({ syl, counter });
        }
    }

    // Iterate over notes and syllables simultaneously. Move through note array using counters for each syllable, moving
    // for several notes if syllable needs to be held
    for (int i = 0, j = 0; (i < (int)m_lineNoteArray.size()) && (j < (int)syllables.size()); ++j) {
        // (the note was read before the index was looked at: a line that ends in grace notes threw)
        while ((i < (int)m_lineNoteArray.size()) && m_lineNoteArray.at(i)->IsGraceNote()) {
            ++i;
        }
        if (i >= (int)m_lineNoteArray.size()) break;
        Verse *verse = NULL;
        verse = vrv_cast<Verse *>(m_lineNoteArray.at(i)->GetChild(0, VERSE));
        if (!verse) {
            verse = new Verse();
            verse->SetN(m_verseNumber);
            m_lineNoteArray.at(i)->AddChild(verse);
        }
        verse->AddChild(syllables.at(j).first);
        i += syllables.at(j).second;
    }
    // clean up syllables that were not added to any of the layer elements
    for (const auto &syl : syllables) {
        if (!syl.first->GetParent()) delete syl.first;
    }

    // increment verse number, in case next line in file is also w:
    ++m_verseNumber;
}

//////////////////////////////
//
// readInformationField --
// information fields always
// start with a letter
// followed by a single colon
//

void ABCInput::ReadInformationField(const char &dataKey, std::string value)
{
    // remove comments and trim
    if (dataKey == '%' || dataKey == '\0') {
        return;
    }
    std::size_t comment = value.find('%');
    if (comment != std::string::npos) {
        value.resize(comment);
    }
    // (a value of white space only was read one byte before its start)
    while (!value.empty() && isspace((unsigned char)value.back())) value.pop_back();
    if (value.empty()) {
        // (an X begins a tune with no number after it too)
        if (dataKey == 'X') this->ParseReferenceNumber(value);
        return;
    }
    while (isspace((unsigned char)value.front())) value.erase(0, 1);

    if (dataKey == '+') {
        LogWarning("ABC import: Field continuation (+) is not supported");
        return;
    }

    switch (dataKey) {
        case 'B': m_info.push_back(std::make_pair(std::make_pair(value, m_lineNum), dataKey)); break;
        case 'C': m_composer.push_back(std::make_pair(value, m_lineNum)); break;
        case 'D': m_info.push_back(std::make_pair(std::make_pair(value, m_lineNum), dataKey)); break;
        case 'F': m_info.push_back(std::make_pair(std::make_pair(value, m_lineNum), dataKey)); break;
        case 'H': m_history.push_back(std::make_pair(value, m_lineNum)); break;
        case 'I': this->ParseInstruction(value); break;
        case 'K': this->ParseKey(value); break;
        case 'L': this->ParseUnitNoteLength(value); break;
        case 'M': this->ParseMeter(value); break;
        case 'N': m_info.push_back(std::make_pair(std::make_pair(value, m_lineNum), dataKey)); break;
        case 'O': m_origin.push_back(std::make_pair(value, m_lineNum)); break;
        case 'Q': this->ParseTempo(value); break;
        case 'S': m_info.push_back(std::make_pair(std::make_pair(value, m_lineNum), dataKey)); break;
        case 'T': m_title.push_back(std::make_pair(value, m_lineNum)); break;
        case 'U': LogWarning("ABC import: User defined sympols are not supported"); break;
        case 'V': LogWarning("ABC import: Multi-voice music is not supported"); break;
        case 'w': this->ParseLyrics(); break;
        case 'W': LogWarning("ABC import: Lyrics are not supported yet"); break;
        case 'X': this->ParseReferenceNumber(value); break;
        case 'Z': m_info.push_back(std::make_pair(std::make_pair(value, m_lineNum), dataKey)); break;
        default: LogWarning("ABC import: Information field %c is ignored", dataKey);
    }
}

//////////////////////////////
//
// readMusicCode --
// parse abc music code
//

void ABCInput::ReadMusicCode(const std::string &musicCode, Section *section)
{
    assert(section);

    int i = 0;
    bool sysBreak = true;

    data_GRACE grace = GRACE_NONE;
    Chord *chord = NULL;

    while (i < (int)musicCode.length()) {
        // eat the input...

        if (musicCode.at(i) == '`') {
            // keeps a beam
        }
        if (isspace(musicCode.at(i))) {
            // always ends a beam
            this->AddLayerElement();
        }

        // comments
        else if (musicCode.at(i) == '%') {
            break;
        }

        // endings
        else if ((i + 2 < (int)musicCode.length()) && musicCode.at(i) == '[' && isdigit(musicCode.at(i + 1))) {
            // Ending *ending = new Ending;
            // ending->SetN(musicCode.at(i));
            // The sign is "[" and its number, or its numbers with "-" between them ("[1-3"), and no more. (The
            // character after the first digit went with it: "[1C" lost its note, and '[1"D"d' its quote, after
            // which the chord symbols of the line were read as its notes and its notes as chord symbols)
            const size_t after = musicCode.find_first_not_of("0123456789-", i + 1);
            i = static_cast<int>((after == std::string::npos) ? musicCode.length() : after) - 1;
        }

        // inline fields: "[", a letter, ":" (ABC 2.1, 3.3). "[|:" is a bar line; it was read as a field named "|"
        else if ((i + 2 < (int)musicCode.length()) && musicCode.at(i) == '[' && musicCode.at(i + 2) == ':'
            && isalpha((unsigned char)musicCode.at(i + 1))) {
            ++i;
            char dataKey = musicCode.at(i);
            ++i;
            ++i;
            std::string information;
            // to its "]", or to the end of the line where it has none (the line was read past its end)
            while ((i < (int)musicCode.length()) && (musicCode.at(i) != ']')) {
                information.push_back(musicCode.at(i));
                ++i;
            }
            if (dataKey == 'r') {
                this->AddAnnot(information);
            }
            else {
                this->ReadInformationField(dataKey, information);
            }
        }

        // linebreaks
        else if (musicCode.at(i) == m_linebreak) {
            this->AddLayerElement();
            Sb *sb = new Sb();
            section->AddChild(sb);
        }

        // decorations
        else if (shorthandDecoration.find(musicCode.at(i)) != std::string::npos) {
            std::string shorthandDecorationString;
            shorthandDecorationString.push_back(musicCode.at(i));
            this->ParseDecoration(shorthandDecorationString);
        }
        else if (musicCode.at(i) == m_decoration) {
            // A "!" that no other closes on its line is the line-break sign of older ABC, and is passed over. (The
            // line was read past its end for the other one, and a "!" that ends a line is how old tune books are
            // written)
            const size_t closing = musicCode.find(m_decoration, i + 1);
            if ((i + 1 < (int)musicCode.length()) && isspace(musicCode.at(i + 1))) {
                ++i;
            }
            else if (closing != std::string::npos) {
                this->ParseDecoration(musicCode.substr(i + 1, closing - i - 1));
                i = static_cast<int>(closing);
            }
        }

        // tuplets
        else if ((i + 2 < (int)musicCode.length()) && musicCode.at(i) == '(' && isdigit(musicCode.at(i + 1))) {
            i = ParseTuplet(musicCode, i);
        }

        // slurs and ties
        else if (musicCode.at(i) == '(') {
            this->StartSlur();
        }
        else if (musicCode.at(i) == ')') {
            this->EndSlur();
        }
        else if (musicCode.at(i) == '-') {
            this->AddTie();
        }

        // chords
        else if ((i + 2 < (int)musicCode.length()) && musicCode.at(i) == '[' && musicCode.at(i + 1) != '|') {
            // start chord
            chord = new Chord();

            // add articulation
            if (!m_artic.empty()) {
                this->AddArticulation(chord);
            }

            // add chord symbols
            if (!m_harmStack.empty()) {
                this->AddChordSymbol(chord);
            }

            // add dynamics
            if (!m_dynam.empty()) {
                this->AddDynamic(chord);
            }

            // add fermata
            if (m_fermata != STAFFREL_NONE) {
                this->AddFermata(chord);
            }

            // add repeat mark
            if (m_repeatMark != repeatMarkLog_FUNC_NONE) {
                this->AddRepeatMark(chord);
            }
        }
        // (a "]" that closes no chord is passed over: the chord that was not there was read)
        else if (chord && (i >= 1) && musicCode.at(i) == ']' && musicCode.at(i - 1) != '|') {
            // end chord
            if (chord->GetDur() < DURATION_8) {
                // if chord cannot be beamed, write it directly to the layer
                if (m_noteStack.size() > 0) AddLayerElement();
                m_layer->AddChild(chord);
                m_lineNoteArray.push_back(chord);
            }
            else {
                m_noteStack.push_back(chord);
                m_lineNoteArray.push_back(chord);
            }
            chord = NULL;
        }

        // grace notes
        else if ((i + 2 < (int)musicCode.length()) && ((musicCode.at(i) == '{') || (musicCode.at(i) == '}'))) {
            // !to be refined when graceGrp is added!
            // start grace group
            if (musicCode.at(i) == '{') {
                grace = GRACE_acc;
                if (musicCode.at(i + 1) == '/') {
                    grace = GRACE_unacc;
                    ++i;
                }
            }
            // end grace group
            else {
                if ((m_gracecount > 1) || (grace == GRACE_unacc)) AddLayerElement();
                grace = GRACE_NONE;
                m_gracecount = 0;
            }
        }

        // note
        else if (pitch.find(toupper(musicCode.at(i))) != std::string::npos) {
            int oct = 0;
            Note *note = new Note;
            m_ID = note->GetID();

            // accidentals
            if (i >= 1) {
                data_ACCIDENTAL_WRITTEN abcAccid = ACCIDENTAL_WRITTEN_NONE;
                switch (musicCode.at(i - 1)) {
                    case '^':
                        i > 1 && musicCode.at(i - 2) == '^' ? abcAccid = ACCIDENTAL_WRITTEN_x
                                                            : abcAccid = ACCIDENTAL_WRITTEN_s;
                        break;
                    case '=': abcAccid = ACCIDENTAL_WRITTEN_n; break;
                    case '_':
                        i > 1 && musicCode.at(i - 2) == '_' ? abcAccid = ACCIDENTAL_WRITTEN_ff
                                                            : abcAccid = ACCIDENTAL_WRITTEN_f;
                        break;
                    default: break;
                }
                if (abcAccid != ACCIDENTAL_WRITTEN_NONE) {
                    Accid *accid = new Accid();
                    accid->SetAccid(abcAccid);
                    note->AddChild(accid);
                }
            }

            if (keyPitchAlter.find(static_cast<char>(toupper(musicCode.at(i)))) != std::string::npos) {
                auto accid = vrv_cast<Accid *>(note->GetFirst(ACCID));
                if (!accid) {
                    accid = new Accid();
                    note->AddChild(accid);
                    accid->IsAttribute(true);
                    accid->SetAccidGes((keyPitchAlterAmount < 0) ? ACCIDENTAL_GESTURAL_f : ACCIDENTAL_GESTURAL_s);
                }
            }

            // set pitch name
            if (isupper(musicCode.at(i))) {
                oct = 4;
            }
            else {
                oct = 5;
            }
            note->SetPname(note->AttPitch::StrToPitchname(std::string(1, tolower(musicCode.at(i)))));

            // set octave
            while (i + 1 < (int)musicCode.length() && (musicCode.at(i + 1) == '\'' || musicCode.at(i + 1) == ',')) {
                if (musicCode.at(i + 1) == ',') {
                    oct -= 1;
                }
                else {
                    oct += 1;
                }
                ++i;
            }
            note->SetOct(oct);

            // set duration
            std::string numStr, numbaseStr;
            int dots = 0;
            int numbase = 1;
            if ((m_broken < 0) && (grace == GRACE_NONE)) {
                dots = -m_broken;
                m_broken = 0;
            }
            while (i + 1 < (int)musicCode.length() && isdigit(musicCode.at(i + 1))) {
                ++i;
                numStr.push_back(musicCode.at(i));
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '/') {
                ++i;
                numbase *= 2;
            }
            while (i + 1 < (int)musicCode.length() && isdigit(musicCode.at(i + 1))) {
                ++i;
                numbaseStr.push_back(musicCode.at(i));
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '>') {
                ++i;
                ++m_broken;
                ++dots;
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '<') {
                ++i;
                --m_broken;
            }
            int num = (numStr.empty()) ? 1 : std::atoi(numStr.c_str());
            numbase = (numbaseStr.empty()) ? numbase : std::atoi(numbaseStr.c_str());
            // (a number too long for an int comes out negative, and this never ended)
            while ((num > 0) && ((num & (num - 1)) != 0)) {
                ++dots;
                // won't work for num > 12
                num = num - num / 3;
            }
            if ((numbase & (numbase - 1)) != 0) LogError("ABC import: note length divider must be power of 2");
            // (a length of 0, or one too long for an int, which comes out below zero: divided by, it ends the
            // process on a processor that traps the division)
            int dur = (num < 1) ? 4 : m_unitDur * numbase / num;

            // set grace
            if (grace != GRACE_NONE) {
                ++m_gracecount;
                note->SetGrace(grace);
                // "The unit duration to use for gracenotes is not specified by the abc file"
                // setting it to an eighth by default for now
                note->SetDur(DURATION_8);
                if (grace == GRACE_unacc) note->SetStemMod(STEMMODIFIER_1slash);
            }

            // add articulation
            if (!m_artic.empty()) {
                this->AddArticulation(note);
            }

            // add chord symbols
            if (!m_harmStack.empty()) {
                this->AddChordSymbol(note);
            }

            // add dynamics
            if (!m_dynam.empty()) {
                this->AddDynamic(note);
            }

            // add fermata
            if (m_fermata != STAFFREL_NONE) {
                this->AddFermata(note);
            }

            // add ornaments
            if (!m_ornam.empty()) {
                this->AddOrnaments(note);
            }

            // add repeat mark
            if (m_repeatMark != repeatMarkLog_FUNC_NONE) {
                this->AddRepeatMark(note);
            }

            if ((m_broken < 0) && (grace == GRACE_NONE)) {
                for (int i = 0; i != -m_broken; ++i) dur = dur * 2;
            }
            else if ((dots == 0) && (m_broken > 0) && (grace == GRACE_NONE)) {
                for (; m_broken != 0; --m_broken) dur = dur * 2;
            }
            data_DURATION meiDur
                = (dur == 0) ? DURATION_breve : note->AttDurationLog::StrToDuration(std::to_string(dur));

            if (chord) {
                chord->AddChild(note);
                if (!chord->HasDur()) {
                    if (dots > 0) chord->SetDots(dots);
                    if (num == 0) chord->SetStemVisible(BOOLEAN_false);
                    chord->SetDur(meiDur);
                }
            }
            else {
                if (dots > 0) note->SetDots(dots);
                if (num == 0) note->SetStemVisible(BOOLEAN_false);
                note->SetDur(meiDur);
                if (note->GetDur() < DURATION_8) {
                    // if note cannot be beamed, write it directly to the layer
                    if (m_noteStack.size() > 0) AddLayerElement();
                    m_layer->AddChild(note);
                    m_lineNoteArray.push_back(note);
                }
                else {
                    m_noteStack.push_back(note);
                    m_lineNoteArray.push_back(note);
                }
            }

            if (!m_tieStack.empty()) {
                m_tieStack.back()->SetEndid("#" + m_ID);
                m_tieStack.clear();
            }
            for (Slur *slur : m_slurStack) {
                if (!slur->HasStartid()) {
                    slur->SetStartid("#" + m_ID);
                }
            }
        }

        // spaces
        else if (musicCode.at(i) == 'x') {
            Space *space = new Space();
            m_ID = space->GetID();

            // add chord symbols
            if (!m_harmStack.empty()) {
                this->AddChordSymbol(space);
            }

            // set duration
            std::string numStr, numbaseStr;
            int dots = 0;
            int numbase = 1;
            if ((m_broken < 0) && (grace == GRACE_NONE)) {
                dots = -m_broken;
                m_broken = 0;
            }
            while (i + 1 < (int)musicCode.length() && isdigit(musicCode.at(i + 1))) {
                ++i;
                numStr.push_back(musicCode.at(i));
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '/') {
                ++i;
                numbase *= 2;
            }
            while (i + 1 < (int)musicCode.length() && isdigit(musicCode.at(i + 1))) {
                ++i;
                numbaseStr.push_back(musicCode.at(i));
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '>') {
                ++i;
                ++m_broken;
                ++dots;
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '<') {
                ++i;
                --m_broken;
            }
            int num = (numStr.empty()) ? 1 : std::atoi(numStr.c_str());
            numbase = (numbaseStr.empty()) ? numbase : std::atoi(numbaseStr.c_str());
            // (a number too long for an int comes out negative, and this never ended)
            while ((num > 0) && ((num & (num - 1)) != 0)) {
                ++dots;
                // won't work for num > 12
                num = num - num / 3;
            }
            if ((numbase & (numbase - 1)) != 0) LogError("ABC import: note length divider must be power of 2");
            // (a length of 0: the division ends the process on a processor that traps it; elsewhere it gave 0)
            int dur = (num < 1) ? 0 : m_unitDur * numbase / num;

            if (m_broken < 0) {
                for (int i = 0; i != -m_broken; ++i) dur = dur * 2;
            }
            else if ((dots == 0) && (m_broken > 0)) {
                for (; m_broken != 0; --m_broken) dur = dur * 2;
            }
            data_DURATION meiDur
                = (dur == 0) ? DURATION_breve : space->AttDurationLog::StrToDuration(std::to_string(dur));

            if (dots > 0) space->SetDots(dots);
            space->SetDur(meiDur);

            // spaces cannot be beamed
            this->AddLayerElement();
            m_layer->AddChild(space);
        }

        // padding
        else if (musicCode.at(i) == 'y') {
            // Pad *pad = new Pad;
            LogWarning("ABC import: Extra space not supported");
        }

        // rests
        else if (musicCode.at(i) == 'z') {
            Rest *rest = new Rest();
            m_ID = rest->GetID();

            // add chord symbols
            if (!m_harmStack.empty()) {
                this->AddChordSymbol(rest);
            }

            // add fermata
            if (m_fermata != STAFFREL_NONE) {
                this->AddFermata(rest);
            }

            // add repeat mark
            if (m_repeatMark != repeatMarkLog_FUNC_NONE) {
                this->AddRepeatMark(rest);
            }

            // set duration
            std::string numStr, numbaseStr;
            int dots = 0;
            int numbase = 1;
            if ((m_broken < 0) && (grace == GRACE_NONE)) {
                dots = -m_broken;
                m_broken = 0;
            }
            while (i + 1 < (int)musicCode.length() && isdigit(musicCode.at(i + 1))) {
                ++i;
                numStr.push_back(musicCode.at(i));
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '/') {
                ++i;
                numbase *= 2;
            }
            while (i + 1 < (int)musicCode.length() && isdigit(musicCode.at(i + 1))) {
                ++i;
                numbaseStr.push_back(musicCode.at(i));
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '>') {
                ++i;
                ++m_broken;
                ++dots;
            }
            while (i + 1 < (int)musicCode.length() && musicCode.at(i + 1) == '<') {
                ++i;
                --m_broken;
            }
            int num = (numStr.empty()) ? 1 : std::atoi(numStr.c_str());
            numbase = (numbaseStr.empty()) ? numbase : std::atoi(numbaseStr.c_str());
            // (a number too long for an int comes out negative, and this never ended)
            while ((num > 0) && ((num & (num - 1)) != 0)) {
                ++dots;
                // won't work for num > 12
                num = num - num / 3;
            }
            if ((numbase & (numbase - 1)) != 0) LogError("ABC import: note length divider must be power of 2");
            // (a length of 0: the division ends the process on a processor that traps it; elsewhere it gave 0)
            int dur = (num < 1) ? 0 : m_unitDur * numbase / num;

            if (m_broken < 0) {
                for (int i = 0; i != -m_broken; ++i) dur = dur * 2;
            }
            else if ((dots == 0) && (m_broken > 0)) {
                for (; m_broken != 0; --m_broken) dur = dur * 2;
            }
            data_DURATION meiDur
                = (dur == 0) ? DURATION_breve : rest->AttDurationLog::StrToDuration(std::to_string(dur));

            if (dots > 0) rest->SetDots(dots);
            rest->SetDur(meiDur);

            // rests cannot be beamed
            this->AddLayerElement();
            m_layer->AddChild(rest);
        }

        // multi-measure rests
        else if (musicCode.at(i) == 'Z') {
            std::string numString;
            while (i + 1 < (int)musicCode.length() && isdigit(musicCode.at(i + 1))) {
                numString.push_back(musicCode.at(i + 1));
                ++i;
            }
            // "Z" with no number is one measure of rest (ABC 2.1, 4.5), and is written as an empty bar is, an
            // <mRest>; so is a number that is none (0, or one too long for an int, which comes out below zero).
            // (No number was read as 0, and a <multiRest> of no measures has no time: the tune was played a measure
            // short for each. MuseNote's own ABC export writes "Z" for an empty bar)
            const int num = atoi(numString.c_str());
            if (num < 1) {
                m_layer->AddChild(new MRest());
            }
            else {
                MultiRest *multiRest = new MultiRest();
                multiRest->SetNum(num);
                m_layer->AddChild(multiRest);
            }
        }

        // text elements
        else if (musicCode.at(i) == '\"') {
            ++i;
            // (a quote with nothing after it, or one that is not closed: the line was read past its end. The text
            // then runs to the end of the line)
            const int length = (int)musicCode.length();
            if ((i < length)
                && (musicCode.at(i) == '^' || musicCode.at(i) == '_' || musicCode.at(i) == '<' || musicCode.at(i) == '>'
                    || musicCode.at(i) == '@')) {
                LogWarning("ABC import: Annotations are not fully support yet");
                ++i;
            }
            std::string chordSymbol;
            while ((i < length) && (musicCode.at(i) != '\"')) {
                chordSymbol.push_back(musicCode.at(i));
                ++i;
            }
            Harm *harm = new Harm();
            Text *text = new Text();
            text->SetText(UTF8to32(chordSymbol));
            harm->AddChild(text);
            m_harmStack.push_back(harm);
            m_controlElements.push_back(std::make_pair(m_layer->GetID(), harm));
        }

        // suppressing score line-breaks
        else if (musicCode.at(i) == '\\') {
            sysBreak = false;
        }

        // barLine ("::" is one that has no "|" in it)
        else if ((musicCode.at(i) == '|') || IsColonPair(musicCode, i)) {
            // add stacked elements to layer
            this->AddLayerElement();
            // "::" and ":|:" end one repeat and start the next, as ":||:" does (ABC 2.1, 4.8). The start was dropped
            // from ":|:", and "::" was no bar line at all: what stood on either side of it was one measure
            const bool startsNext = (musicCode.at(i) == ':')
                || ((i >= 1) && (musicCode.at(i - 1) == ':') && (i + 1 < (int)musicCode.length())
                    && (musicCode.at(i + 1) == ':'));
            i = SetBarLine(musicCode, i);

            if (m_barLines.second != BARRENDITION_NONE) {
                Measure *measure = new Measure();
                measure->SetLeft(m_barLines.first);
                measure->SetRight(m_barLines.second);
                this->ImplyRepeatStart(measure);
                m_barLines = std::make_pair(BARRENDITION_NONE, BARRENDITION_NONE);
                Staff *staff = new Staff();

                staff->AddChild(m_layer);
                measure->AddChild(staff);
                section->AddChild(measure);
                m_layer = new Layer();
                m_layer->SetN(1);
                for (ControlElement *tempo : m_tempoStack) {
                    measure->AddChild(tempo);
                }
                m_tempoStack.clear();
            }
            if (startsNext) {
                // (an end that found its measure empty waits on the left as well)
                m_barLines.first
                    = (m_barLines.first == BARRENDITION_rptend) ? BARRENDITION_rptboth : BARRENDITION_rptstart;
                ++i;
            }
        }

        ++i;

        // check if there is a clef change
        if (m_clef) {
            m_noteStack.push_back(m_clef);
            m_clef = NULL;
        }

        // check if there is a change in meter
        if (m_meter) {
            // todo: apply meter changes to staves
            ScoreDef *scoreDef = new ScoreDef();
            m_meter->IsAttribute(true);
            scoreDef->AddChild(m_meter);
            section->AddChild(scoreDef);
            m_meter = NULL;
        }

        if (m_durDefault != DURATION_NONE) {
            ScoreDef *scoreDef = new ScoreDef();
            scoreDef->SetDurDefault(m_durDefault);
            section->AddChild(scoreDef);
            m_durDefault = DURATION_NONE;
        }
    }

    // by default, line-breaks in the code generate line-breaks in the score
    // Verovio does not support line-breaks within a layer
    // has to be refined later
    // (a section with nothing in it yet has no last child: line breaks set to "<none>" in the header and set back
    // after it)
    if (sysBreak && (m_linebreak != '\0') && !(section->GetLast() && section->GetLast()->Is(SB))) {
        this->AddLayerElement();
        Sb *sb = new Sb();
        sb->SetID(StringFormat("abcLine%02d", m_lineNum + 1));
        section->AddChild(sb);
    }
}

#endif /* NO_ABC_SUPPORT */

} // namespace vrv
