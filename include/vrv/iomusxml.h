/////////////////////////////////////////////////////////////////////////////
// Name:        iomusxml.h
// Author:      Laurent Pugin and Klaus Rettinghaus
// Created:     22/09/2015
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_IOMUSXML_H__
#define __VRV_IOMUSXML_H__

#include <map>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <vector>

//----------------------------------------------------------------------------

#include "attdef.h"
#include "expansionmap.h"
#include "fraction.h"
#include "iobase.h"
#include "metersig.h"
#include "vrvdef.h"

//----------------------------------------------------------------------------

#include "pugixml.hpp"

namespace vrv {

class Arpeg;
class BeamSpan;
class BracketSpan;
class Clef;
class ControlElement;
class Dir;
class Dynam;
class F;
class Fermata;
class Fb;
class Gliss;
class Hairpin;
class Harm;
class InstrDef;
class KeySig;
class Label;
class LabelAbbr;
class Layer;
class LayerElement;
class Measure;
class MeterSigGrp;
class Octave;
class Pedal;
class ScoreDef;
class Section;
class Slur;
class StaffGrp;
class Tempo;
class Tie;
class Trill;

//----------------------------------------------------------------------------
// namespace for local MusicXML classes
//----------------------------------------------------------------------------

#ifndef NO_MUSICXML_SUPPORT

namespace musicxml {

    struct OpenSlur {
        OpenSlur(const std::string &measureNum, short int number, curvature_CURVEDIR curvedir)
        {
            m_measureNum = measureNum;
            m_number = number;
            m_curvedir = curvedir;
        }

        std::string m_measureNum;
        short int m_number;
        curvature_CURVEDIR m_curvedir;
    };

    struct CloseSlur {
        CloseSlur(const std::string &measureNum, short int number, curvature_CURVEDIR curvedir)
        {
            m_measureNum = measureNum;
            m_number = number;
            m_curvedir = curvedir;
        }

        std::string m_measureNum;
        short int m_number;
        curvature_CURVEDIR m_curvedir;
    };

    struct OpenSpanner {
        OpenSpanner(const int &dirN, const int &lastMeasureCount)
        {
            m_dirN = dirN;
            m_lastMeasureCount = lastMeasureCount;
        }

        int m_dirN;
        int m_lastMeasureCount;
    };

    struct OpenArpeggio {
        OpenArpeggio(const int &arpegN, const Fraction &timeStamp)
        {
            m_arpegN = arpegN;
            m_timeStamp = timeStamp;
        }

        int m_arpegN;
        Fraction m_timeStamp;
    };

    // What an order of play is worked out from is every reader's (expansionmap.h): there is one rule for it.
    using EndingInfo = play::EndingInfo;
    using RepeatInfo = play::RepeatInfo;
    using JumpInfo = play::JumpInfo;
    using FineInfo = play::FineInfo;
    using SectionInfo = play::SectionInfo;

    struct ClefChange {
        ClefChange(const std::string &measureNum, Staff *staff, Layer *layer, Clef *clef, const int &scoreOnset,
            bool afterBarline)
        {
            m_measureNum = measureNum;
            m_staff = staff;
            m_layer = layer;
            m_clef = clef;
            m_scoreOnset = scoreOnset;
            m_afterBarline = afterBarline;
        }

        std::string m_measureNum;
        Staff *m_staff;
        Layer *m_layer;
        Clef *m_clef;
        int m_scoreOnset; // the score position of clef change
        bool m_afterBarline = false; // musicXML attribute
    };

    struct OpenDashes {
        OpenDashes(const int dirN, int staffNum, const int measureCount)
        {
            m_dirN = dirN;
            m_staffNum = staffNum;
            m_measureCount = measureCount;
        }

        int m_dirN; // direction number
        int m_staffNum;
        int m_measureCount; // measure number of dashes start
    };

    struct OpenTie {
        OpenTie(Tie *tie, Note *note, int layerNum, int position, const Measure *measure, int end)
        {
            m_tie = tie;
            m_note = note;
            m_layerNum = layerNum;
            m_position = position;
            m_measure = measure;
            m_end = end;
        }

        Tie *m_tie = NULL;
        Note *m_note = NULL;
        int m_layerNum = 0;
        // its note's place among the notes and rests of its voice; its measure, and where it ends in it
        int m_position = 0;
        const Measure *m_measure = NULL;
        int m_end = 0;
        // the place of the next note or rest of its voice (none yet: -1), and whether its voice went on past it
        int m_nextPosition = -1;
        bool m_passed = false;
        // its voice had nothing more in its measure: it waits for the next measure's; and whether its note ended at
        // the bar line (another voice may then end it at the next measure's start)
        bool m_carried = false;
        bool m_atBarLine = false;
    };

    struct CloseTie {
        CloseTie(Note *note, int layerNum, int position, int onset)
        {
            m_note = note;
            m_layerNum = layerNum;
            m_position = position;
            m_onset = onset;
        }

        Note *m_note = NULL;
        int m_layerNum = 0;
        // its note's place among the notes and rests of its voice, and where it begins in its measure
        int m_position = 0;
        int m_onset = 0;
    };

    /**
     * A gap the import wrote as spaces that do not play its time: its length is no sum of written values (a third of
     * a quarter, say), or it went into an open tuplet, which plays its spaces at the tuplet's ratio. m_scale is the
     * open tuplets' num / numbase: what is written there for a time. MusicXmlInput::WriteExactTime writes the gap
     * again, exactly, after the import.
     */
    struct InexactGap {
        std::vector<LayerElement *> m_spaces;
        int m_ticks;
        int m_ppq;
        Fraction m_scale;
    };

    /**
     * A note, chord or rest timed by a ratio no <tuplet> bracket shows: its <time-modification> (actual notes in the
     * time of normal notes).
     */
    struct UnbracketedRatio {
        LayerElement *m_element;
        int m_num;
        int m_numbase;
    };

    struct Accidental {
        Accidental()
        {
            m_accid = ACCIDENTAL_WRITTEN_NONE;
            m_glyphName = "";
            m_glyphAuth = "";
        }

        Accidental(data_ACCIDENTAL_WRITTEN accid, std::string glyphName, std::string glyphAuth)
        {
            m_accid = accid;
            m_glyphName = glyphName;
            m_glyphAuth = glyphAuth;
        }

        data_ACCIDENTAL_WRITTEN m_accid;
        std::string m_glyphName;
        std::string m_glyphAuth;
    };
} // namespace musicxml

#endif // NO_MUSICXML_SUPPORT

//----------------------------------------------------------------------------
// MusicXmlInput
//----------------------------------------------------------------------------

class MusicXmlInput : public Input {
public:
    // constructors and destructors
    MusicXmlInput(Doc *doc);
    virtual ~MusicXmlInput();

private:
    /*
     * Objects that were not successfully added and should be destroyed at the end of the import
     */
    ListOfObjects m_garbage;

#ifndef NO_MUSICXML_SUPPORT
public:
    bool Import(const std::string &musicxml) override;

    /*
     * @name Public methods for converting MusicXML values to MEI attributes.
     */
    ///@{
    static data_ACCIDENTAL_WRITTEN ConvertAccidentalToAccid(const std::string &value);
    ///@}

private:
    /*
     * Top level method called from ImportFile or Import
     */
    bool ReadMusicXml(pugi::xml_node root);

    /*
     * Method to fill MEI header with title
     */
    void ReadMusicXmlTitle(pugi::xml_node title);

    /*
     * @name Top level methods for reading MusicXML part and measure elements.
     */
    ///@{
    bool ReadMusicXmlPart(pugi::xml_node node, Section *section, short int nbStaves, const short int staffOffset);
    bool ReadMusicXmlMeasure(pugi::xml_node node, Section *section, Measure *measure, short int nbStaves,
        const short int staffOffset, int index);
    ///@}

    /*
     * Methods for reading the first MusicXML attributes element as MEI staffDef.
     * Returns the number of staves in the part.
     */
    short int ReadMusicXmlPartAttributesAsStaffDef(
        pugi::xml_node node, StaffGrp *staffGrp, const short int staffOffset);

    /*
     * @name Methods for reading the content of a MusicXML measure.
     */
    ///@{
    void ReadMusicXmlAttributes(pugi::xml_node, Section *section, Measure *measure, const std::string &measureNum);
    void ReadMusicXmlBackup(pugi::xml_node, Measure *measure, const std::string &measureNum);
    void ReadMusicXmlBarLine(pugi::xml_node, Measure *measure, const std::string &measureNum);
    void ReadMusicXmlDirection(
        pugi::xml_node, Measure *measure, const std::string &measureNum, const short int staffOffset, Section *section);
    void ReadMusicXmlFigures(pugi::xml_node, Measure *measure, const std::string &measureNum);
    void ReadMusicXmlForward(pugi::xml_node, Measure *measure, const std::string &measureNum);
    void ReadMusicXmlHarmony(pugi::xml_node, Measure *measure, const std::string &measureNum);
    void ReadMusicXmlNote(
        pugi::xml_node, Measure *measure, const std::string &measureNum, const short int staffOffset, Section *section);
    void ReadMusicXmlPrint(pugi::xml_node, Section *section);
    void ReadMusicXmlSound(pugi::xml_node, Measure *measure, Section *section);
    /**
     * The directions and sounds of the [bars] bars a multi-bar rest stands for, from [from] in the first, [bar], on:
     * read into [measure], the rest's (a start at its start, a jump at its end, as its barlines are its last bar's)
     */
    void ReadMultiRestMarks(pugi::xml_node from, pugi::xml_node bar, int bars, Measure *measure,
        const std::string &measureNum, const short int staffOffset, Section *section);
    /**
     * What [sound] says of the jump or sign of [kind] that [mark] is, kept on the mark (MuseNote RC41): the label of
     * its coda or segno, or its own, where it is not the one the rule takes without one; the times it is taken, where
     * the file names them
     */
    void KeepWhatTheSoundSays(ControlElement *mark, const pugi::xml_node sound, const std::string &kind) const;
    /**
     * A mark for each jump [sound] gives that no mark carries ([carried] is the kind the direction's own mark is): a
     * direction of its kind with nothing to draw, at [tstamp] of [measure] (MuseNote RC41)
     */
    void MarkJumpsOfSound(
        const pugi::xml_node sound, const std::string &measureNum, const std::string &carried, double tstamp);
    /**
     * The jump or sign the words of a direction with no <sound> for one are (MuseNote RC42): its kind (dacapo,
     * dalsegno, tocoda, fine, coda, segno) and the label of its segno or coda, when the whole text is one phrase of
     * the vocabulary MuseScore's importer reads; none otherwise
     */
    static std::optional<std::pair<std::string, std::string>> JumpOfWords(const std::string &text);
    /** What a jump or sign read from its words or its sign does to the order of play, as its <sound> would */
    void PlayJumpOfWords(const std::string &kind, const std::string &label);
    /**
     * Whether a <sound> anywhere in the score of [node] gives a jump or sign of [kind] (a measure's own, or a
     * direction's): then the score gives that kind's playback, and its words and signs of the kind are its print
     */
    static bool SoundOfScoreGives(const pugi::xml_node node, const std::string &kind);
    /** Whether [measure] holds a To Coda: by a <sound>, or in its words */
    static bool MeasureHoldsToCoda(const pugi::xml_node measure);
    bool ReadMusicXmlBeamsAndTuplets(const pugi::xml_node &node, Layer *layer, bool isChord);
    /**
     * Opens a tuplet level for [tupletStart] with its own ratio (MEI's rule: the ratios of nested tuplets multiply):
     * the numbers the notation writes, else [timedBy]'s time-modification over the levels open around it.
     */
    void ReadMusicXmlTupletStart(const pugi::xml_node &tupletStart, Layer *layer, const pugi::xml_node &timedBy);
    /**
     * Every tuplet that starts on [node] is a level: those around the innermost open here, outer first (the one that
     * stops later; an outer one with no numbers is timed by the first note it holds directly). Returns the innermost
     * start, which the caller opens as one tuplet always was.
     */
    pugi::xml_node OpenOuterTupletLevels(
        const pugi::xml_node &node, Layer *layer, bool isChord, const std::vector<pugi::xml_node> &measureNodes);
    /** The index on the layer's stack of the innermost open tuplet of [number], or -1 */
    int DepthOfTuplet(Layer *layer, const std::string &number) const;
    /** The product of the ratios (num / numbase) of the tuplets open in [layer] */
    Fraction OpenTupletRatio(Layer *layer) const;
    /**
     * A beam and a tuplet that start on one note nest by where each really ends in the note's voice: the beam on the
     * note the import closes it on (an "end" with no "continue"), the tuplet on its stop. The two have just been
     * built, empty, in the order the import has always built them (their ids); they swap when the other nesting is
     * the right one.
     */
    void NestBeamAndTupletByTheirEnds(const pugi::xml_node &node, const pugi::xml_node &tupletStart, Layer *layer,
        const std::vector<pugi::xml_node> &measureNodes);
    /**
     * Closes the open tuplet of [number] (the innermost one when none has it), with what was opened inside it: a
     * tuplet ends on its stop and must not take in the notes after it.
     */
    void CloseTuplet(Layer *layer, const std::string &measureNum, const std::string &number);
    void ReadMusicXmlBeamStart(const pugi::xml_node &node, const pugi::xml_node &beamStart, Layer *layer);
    void ReadMusicXMLMeterSig(const pugi::xml_node &node, Object *parent);
    void ReadMusicXmlTies(const pugi::xml_node &node, Layer *layer, Note *note, const Measure *measure,
        const std::string &measureNum, int position, int onset, int duration);
    ///@}

    /**
     * Process all clef change queue and add clefs to corresponding places in the score
     */
    void ProcessClefChangeQueue(Section *section);

    /**
     * Add clef changes to all layers of a given measure, staff, and time stamp
     */
    void AddClefs(Measure *measure, const musicxml::ClefChange &clefChange);

    /**
     * Add clef as layer element to specified layer and #sameas clefs to previous layers, if needed
     */
    void InsertClefToLayer(Staff *staff, Layer *layer, Clef *clef, int scoreOnSet);

    /*
     * @name Helper function to insert clef into correct position in layer/other parent based on the insertAfter
     * variable.
     */
    ///@{
    void InsertClefIntoObject(Object *layerElement, Clef *clef, Layer *layer, int scoreOnset, bool insertAfter);
    void InsertClefIntoObject(Object *parent, Clef *clef, Object *relevantChild, bool insertAfter);
    ///@}

    /*
     * Add a Measure to the section.
     * If the measure already exists it will move all its content.
     * The measure can contain only staves. Other elements must be stacked on m_floatingElements.
     */
    void AddMeasure(Section *section, Measure *measure, int i);

    /*
     * Add a Layer element to the layer or to the LayerElement at the top of m_elementStack.
     */
    void AddLayerElement(Layer *layer, LayerElement *element, int duration = 0);

    /*
     * Add an accidental to a note.
     */
    void AddAccidental(pugi::xml_node accidental, Note *note);

    /*
     * Returns the appropriate layer for a node looking at its MusicXML staff and voice elements.
     */
    Layer *SelectLayer(pugi::xml_node node, Measure *measure);

    /*
     * Returns the appropriate first layer of a staff.
     */
    Layer *SelectLayer(short int staffNb, Measure *measure);

    /*
     * Returns the layer with \@n=layerNb on the staff.
     * Creates the layer if not found.
     */
    Layer *SelectLayer(short int layerNb, Staff *staff);

    /*
     * @name Methods for converting the content of MusicXML attributes.
     */
    ///@{
    Clef *ConvertClef(const pugi::xml_node &clef);
    KeySig *ConvertKey(const pugi::xml_node &key);
    ///@}

    /*
     * Remove the last ClassId element on top of m_elementStack.
     * For example, when closing a beam, we need to remove it from the stack, but it is not
     * necessary the top one (for example we can have an opened chord there).
     */
    void RemoveLastFromStack(ClassId classId, Layer *layer);

    /**
     * Check if element of classId type is present on the specified layer
     */
    bool IsInStack(ClassId classId, Layer *layer);

    /*
     * @name Helper methods for checking presence of values of attributes or elements
     */
    ///@{
    bool HasAttributeWithValue(const pugi::xml_node node, const std::string &attribute, const std::string &value) const;
    bool IsElement(const pugi::xml_node node, const std::string &name) const;
    bool HasContentWithValue(const pugi::xml_node node, const std::string &value) const;
    ///@}

    /*
     * @name Helper methods to work with sections and expansions.
     */
    ///@{
    bool MeasureInExistingSection(const Measure *measure) const;
    void CreateExpansion(Section *section);
    ///@}

    /*
     * @name Helper methods for retrieving attribute values or element content
     */
    ///@{
    std::string GetContent(const pugi::xml_node node) const;
    std::string GetContentOfChild(const pugi::xml_node node, const std::string &child) const;
    ///@}

    /*
     * @name Methods for opening and closing ties and slurs.
     * Opened ties and slurs are stacked together with musicxml::OpenTie
     * and musicxml::OpenSlur objects.
     * Slur starts and ends are matched based on its number.
     */
    ///@{
    void OpenTie(Note *note, Tie *tie, int layerNum, int position, const Measure *measure, int end);
    void CloseTie(Note *note, int layerNum, int position, int onset);
    /** A note or rest at the place [position] of the layer [layerNum]: the next of its voice for the ties before it */
    void PassTies(int layerNum, int position);
    /** Whether [tie] is ended by the note at the place [position] of the layer [layerNum], beginning at [onset] in
     * [measure] */
    static bool EndsAt(
        const musicxml::OpenTie &tie, int layerNum, int position, const Measure *measure, int onset);
    /** Leaves out the open tie at [tie], which no note continues */
    void DropOpenTie(std::vector<musicxml::OpenTie>::iterator tie, const std::string &measureNum);
    void OpenSlur(Measure *measure, short int number, Slur *slur, curvature_CURVEDIR dir);
    void CloseSlur(Measure *measure, short int number, LayerElement *element, curvature_CURVEDIR dir);
    void CloseBeamSpan(Staff *staff, Layer *layer, LayerElement *element);
    void MatchTies(const Measure *measure);
    ///@}

    /**
     * Writes the sign of every note whose page would not show its sound: the importer gives a note with no
     * <accidental> the sound its <alter> says, and the sign a reader of the page needs is the importer's to write.
     */
    void WriteSignsOfSound();

    /**
     * Writes exactly, after the import, the time it could not: a gap whose length is no sum of written values, and a
     * note, chord or rest timed by a ratio no bracket shows, each in a tuplet that shows neither bracket nor number.
     * Every object the import built keeps its place in the build order (its id); what this makes comes after them.
     */
    void WriteExactTime();

    /** Keeps [element] for WriteExactTime when [node] is timed by a ratio no open tuplet bracket gives it */
    void KeepUnbracketedRatio(const pugi::xml_node &node, Layer *layer, LayerElement *element);

    /**
     * The written value and dots of [node]'s ticks when they are not the time its type and dots give (a "half" rest
     * holding three beats): a value with up to two dots, or false when they make none. Not for what a ratio times (a
     * tuplet's, or its own), a grace note, or a note of a fingered tremolo (written at the whole tremolo's value).
     */
    bool ValueOfItsTicks(const pugi::xml_node &node, Layer *layer, const std::string &typeStr, int dots, int ticks,
        data_DURATION &duration, int &valueDots);

    /**
     * Whether a rest written [typeStr] and [dots], [ticks] long, fills its whole bar with another time than it is
     * written for (a whole rest in a bar of 3/4): it is the bar's rest.
     */
    bool FillsItsBarOtherwise(const pugi::xml_node &node, Layer *layer, const std::string &typeStr, int dots, int ticks);

    /*
     * @name Helper methods for rendering text elements
     */
    ///@{
    std::string GetWordsOrDynamicsText(const pugi::xml_node node) const;
    void TextRendition(const pugi::xpath_node_set words, ControlElement *element) const;
    std::string StyleLabel(pugi::xml_node display);
    void PrintMetronome(pugi::xml_node metronome, Tempo *tempo);
    ///@}

    /*
     * @name Helper methods for filling in space elements
     */
    ///@{
    void FillSpace(Layer *layer, int dur, bool withClefs = false, int processed = 0);
    ///@}

    /*
     * @name Helper method for generating additional IDs
     */
    ///@{
    void GenerateID(pugi::xml_node node);
    ///@}

    /*
     * @name Helper method for meterSigGrp. Separates beat/beat-type into MeterSig and adds them to the MeterSigGrp.
     * Returns total meterCount and meterUnit for the group
     */
    ///@{
    std::pair<std::vector<int>, int> GetMeterSigGrpValues(const pugi::xml_node &node, MeterSigGrp *parent);
    ///@}

    /*
     * @name Helper method for multirests. Returns number of measure hidden by MRest before
     * measure with certain index
     */
    ///@{
    int GetMrestMeasuresCountBeforeIndex(int index) const;
    ///@}

    /*
     * @name Helper method for multirests. Checks whether measure should be hidden as part of MRest
     */
    ///@{
    bool IsMultirestMeasure(int index) const;
    ///@}

    /*
     * @name Helper method for styling fermatas
     */
    ///@{
    void SetFermataExternalSymbols(Fermata *fermata, const std::string &shape);
    void ShapeFermata(Fermata *fermata, pugi::xml_node node);
    ///@}

    /*
     * @name Helper method for getting glyph numbers for ornaments based on approach/depart attributes
     */
    ///@{
    std::string GetOrnamentGlyphNumber(int attributes) const;
    ///@}

    /*
     * @name Helper method for setting \@staff attribute for chords
     */
    ///@{
    void SetChordStaff(Layer *layer);
    ///@}

    /*
     * @name Update the carried-over accidentals for each pitch class based on key signature
     */
    ///@{
    void ResetAccidentals(const KeySig *keySig = NULL);
    ///@}

    /**
     * @name Find or create a ScoreDef before the given measure
     */
    ///@{
    ScoreDef *GetOrCreateLastScoreDef(Section *section);
    ///@}

    /*
     * @name Helper for detecting the slur curve direction
     */
    static curvature_CURVEDIR CombineCurvedir(curvature_CURVEDIR startDir, curvature_CURVEDIR stopDir);

    /*
     * @name Methods for converting MusicXML values to MEI attributes.
     */
    ///@{
    static data_ACCIDENTAL_GESTURAL ConvertAlterToAccid(const float value);
    static data_ARTICULATION ConvertArticulations(const std::string &value);
    static data_BARRENDITION ConvertStyleToRend(const std::string &value, const bool repeat);
    static data_BOOLEAN ConvertWordToBool(const std::string &value);
    static data_DURATION ConvertTypeToDur(const std::string &value);
    static data_HEADSHAPE_list ConvertNotehead(const std::string &value);
    static data_LINESTARTENDSYMBOL ConvertLineEndSymbol(const std::string &value);
    static data_MIDIVALUE ConvertDynamicsToMidiVal(const float dynamics);
    static data_PITCHNAME ConvertStepToPitchName(const std::string &value);
    static data_TEXTRENDITION ConvertEnclosure(const std::string &value);
    static beamRend_FORM ConvertBeamFanToForm(const std::string &value);
    static curvature_CURVEDIR InferCurvedir(const pugi::xml_node slurOrTie);
    static fermataVis_SHAPE ConvertFermataShape(const std::string &value);
    static pedalLog_DIR ConvertPedalTypeToDir(const std::string &value);
    static repeatMarkLog_FUNC ConvertJumpType(const std::string &value);
    static tupletVis_NUMFORMAT ConvertTupletNumberValue(const std::string &value);
    static std::u32string ConvertTypeToVerovioText(const std::string &value);
    static std::string ConvertAlterToSymbol(const std::string &value, bool plusMinus = false);
    static std::string ConvertKindToSymbol(const std::string &value);
    static std::string ConvertKindToText(const std::string &value);
    static std::string ConvertDegreeToText(const pugi::xml_node harmony);
    static std::string ConvertFigureGlyph(const std::string &value);
    ///@}

    /*
     * @name Methods for converting between MusicXML <pitch> and MIDI note numbers.
     */
    ///@{
    static int PitchToMidi(const std::string &step, int alter, int octave);
    static void MidiToPitch(int midi, std::string &step, int &alter, int &octave);
    ///@}

public:
    //
private:
    /* octave offset */
    std::vector<int> m_octDis;
    /* measure repeats */
    bool m_mRpt = false;
    /* measure repeats */
    bool m_slash = false;
    /* MIDI ticks */
    int m_ppq = -1;
    /* measure time */
    int m_durTotal = 0;
    /* measure time */
    int m_durFb = 0;
    /* meter signature */
    std::vector<int> m_meterCount = { 4 };
    int m_meterUnit = 4;
    MeterCountSign m_meterSign = MeterCountSign::None;
    /* part information */
    Label *m_label = NULL;
    LabelAbbr *m_labelAbbr = NULL;
    InstrDef *m_instrdef = NULL;
    /* LastElementID */
    std::string m_ID;
    /* The same for each staff: an octave line ends on the last note of its own staff */
    std::map<int, std::string> m_lastIDOnStaff;
    /* A map of stacks for piling open LayerElements (beams, tuplets, chords, btrem, ftrem) separately per layer */
    std::map<Layer *, std::vector<LayerElement *>> m_elementStackMap;
    /* A maps of time stamps (score time) to indicate write pointer of a given layer */
    std::map<Layer *, int> m_layerEndTimes;
    std::map<Layer *, std::multimap<int, LayerElement *>> m_layerTimes;
    /** The gaps and the ratios the import could not write exactly, for WriteExactTime */
    std::vector<musicxml::InexactGap> m_inexactGaps;
    std::vector<musicxml::UnbracketedRatio> m_unbracketedRatios;
    /** Grace rests: no time in the file, and MEI has no rest without one; left out after the import */
    std::vector<LayerElement *> m_graceRests;
    /** The number each tuplet level was opened with, which its stop names */
    std::map<const LayerElement *, std::string> m_tupletNumbers;
    /** The tuplet stops the chord being read has given (its later notes may write them again) */
    std::set<std::string> m_chordTupletStops;
    /* To remember layer of last element (note) to handle chords */
    Layer *m_prevLayer = NULL;
    /* To remember current layer to properly handle layers/staves/cross-staff elements */
    Layer *m_currentLayer = NULL;
    bool m_isLayerInitialized = false;
    /* The stack for open slurs */
    std::vector<std::pair<Slur *, musicxml::OpenSlur>> m_slurStack;
    /* The stack for slur stops that might come before the slur has been opened */
    std::vector<std::pair<LayerElement *, musicxml::CloseSlur>> m_slurStopStack;
    /* The stack for open ties */
    std::vector<musicxml::OpenTie> m_tieStack;
    // the notes and drawn rests of each layer of the part so far (a chord counts once, a grace note not): the place
    // of a note in its voice, by which a tie finds the next note of its voice
    std::map<int, int> m_voicePositions;
    /* The stack for tie stops that might come before that tie was opened */
    std::vector<musicxml::CloseTie> m_tieStopStack;
    /* The stack for hairpins */
    std::vector<std::pair<Hairpin *, musicxml::OpenSpanner>> m_hairpinStack;
    /* The stack for hairpin stops that might occur before a hairpin was started staffNumber, tStamp2, (hairpinNumber,
     * measureCount) */
    std::vector<std::tuple<int, double, musicxml::OpenSpanner>> m_hairpinStopStack;
    /* The stack for the beamspans with numbers of staff/layer where starting element is located*/
    std::vector<std::pair<BeamSpan *, std::pair<int, int>>> m_beamspanStack;
    std::vector<std::pair<BracketSpan *, musicxml::OpenSpanner>> m_bracketStack;
    std::vector<std::pair<Trill *, musicxml::OpenSpanner>> m_trillStack;
    /* Current section/ending info for start/stop */
    std::optional<musicxml::SectionInfo> m_sectionStart;
    std::optional<musicxml::SectionInfo> m_sectionStop;
    /* The jumps of the measure being read, each for its times: a barline can hold several (MuseNote RC36) */
    std::vector<musicxml::JumpInfo> m_jumps;
    std::optional<musicxml::FineInfo> m_fineInfo;
    /* The list of sections/endings to be inserted at the end of XML import */
    std::vector<std::pair<musicxml::SectionInfo, std::vector<Measure *>>> m_sections;
    /* The stack of open dashes (direction-type) containing *ControlElement, OpenDashes */
    std::vector<std::pair<ControlElement *, musicxml::OpenDashes>> m_openDashesStack;
    /* The stacks for ControlElements */
    std::vector<Dir *> m_dirStack;
    std::vector<Dynam *> m_dynamStack;
    std::vector<Gliss *> m_glissStack;
    std::vector<Harm *> m_harmStack;
    std::vector<Octave *> m_octaveStack;
    std::vector<Pedal *> m_pedalStack;
    std::vector<Tempo *> m_tempoStack;
    /*
     * The stack of floating elements (tie, slur, etc.) to be added at the
     * end of each measure
     */
    std::vector<std::pair<std::string, ControlElement *>> m_controlElements;
    /* stack of clef changes to be inserted to all layers of a given staff */
    std::deque<musicxml::ClefChange> m_clefChangeQueue;
    /* stack of new arpeggios that get more notes added. */
    std::vector<std::pair<Arpeg *, musicxml::OpenArpeggio>> m_ArpeggioStack;
    /* a map for the measure counts storing the index of each measure created */
    std::map<Measure *, int> m_measureCounts;
    /* measure rests */
    std::map<int, int> m_multiRests;
    /* a map of current accidental for each pitch class */
    std::map<data_PITCHNAME, std::vector<musicxml::Accidental>> m_currentAccids;
    /* current key signature */
    KeySig *m_currentKeySig = NULL;
    /* A flag indicating we had a clef change */
    int m_clefChanged = 0;

#endif // NO_MUSICXML_SUPPORT
};

} // namespace vrv

#endif // __VRV_IOMUSXML_H__
