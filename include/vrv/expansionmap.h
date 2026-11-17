/////////////////////////////////////////////////////////////////////////////
// Name:        expansionmap.h
// Author:      Werner Goebl
// Created:     2019
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_EXPANSION_MAP_H__
#define __VRV_EXPANSION_MAP_H__

#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

//----------------------------------------------------------------------------

#include "expansion.h"
#include "options.h"

namespace vrv {

class Measure;
class Object;
class Score;
class Section;

//----------------------------------------------------------------------------
// WrittenContext
//----------------------------------------------------------------------------

/**
 * What the score definitions in effect say at a point of the score as written: the time signature, the key, each
 * staff's clef and transposition, as the elements that said them last. A part played after another bar than the one
 * written before it is played under its own (MuseNote RC58)
 */
struct WrittenContext {
    const Object *meterSig = NULL;
    const Object *keySig = NULL;
    std::map<int, const Object *> clefs;
    std::map<int, const Object *> transpositions;
};

/**
 * An order of play while it is written, and the least that following it can cost: one element for each reference (a
 * section holds one at the least; the count itself is in Doc::ExpandExpansions). Whoever writes an order stops when
 * that is past what is followed. What it has written by then costs more than is followed too, and is refused.
 * (The cost was each section by the times it is named, squared, as the expansion map's lists grew: MuseNote RC93)
 */
class OrderOfPlay {
public:
    /**
     * What following an order of play may cost: the elements the score holds as played. The dearest of MuseNote's
     * 3,571 scores and inputs holds 7,819 (Piano Man); 100,000 take some 350 MB and two and a half seconds on a
     * desktop, every played note some 17 KB. (It was 1,000,000 by the square of the times a section is played:
     * MuseNote RC93)
     */
    static constexpr size_t s_maxCost = 100000;

    /** Adds a reference. False, and nothing added, once the order costs more than is followed */
    bool Add(const std::string &ref);

    const xsdAnyURI_List &GetRefs() const { return m_refs; }

private:
    xsdAnyURI_List m_refs;
};

/**
 * What an order of play is worked out from: the score as a run of parts, each a plain section or an ending, with what
 * its barlines and marks say (where a repeat starts and how often it is played, an ending's number, a jump, a fine,
 * the labels a jump names). Every reader that works out an order gives it in this form to WriteOrderOfPlay.
 */
namespace play {

struct EndingInfo {
    EndingInfo() {}

    EndingInfo(const std::string &number, const std::string &type, const std::string &text)
    {
        m_number = number;
        m_type = type;
        m_text = text;
    }

    std::string m_number;
    std::string m_type;
    std::string m_text;
};

struct RepeatInfo {
    RepeatInfo()
    {
        m_times = 1;
        m_afterJump = false;
    }

    RepeatInfo(int times, bool afterJump)
    {
        m_times = times;
        m_afterJump = afterJump;
    }

    int m_times;
    bool m_afterJump;
};

struct JumpInfo {
    typedef enum { NONE = 0, DALSEGNO, DACAPO, TOCODA } JUMPTYPE;

    JumpInfo() { m_jump = NONE; }
    JumpInfo(JUMPTYPE jump, const std::string &label, const std::list<int> &times, bool timesGiven = false)
    {
        m_label = label;
        m_jump = jump;
        m_times = times;
        m_timesGiven = timesGiven;
    }
    JumpInfo(JUMPTYPE jump, const std::list<int> &times, bool timesGiven = false)
    {
        m_jump = jump;
        m_times = times;
        m_timesGiven = timesGiven;
    }
    JumpInfo(JUMPTYPE jump) { m_jump = jump; }

    std::string m_label;
    JUMPTYPE m_jump;
    std::list<int> m_times;
    // Whether the score names the times (MusicXML time-only): they are counted by every time the part is played,
    // its repeat's passes too. Times it does not name are what the jump means without them, counted by the returns
    // to the part: a D.C. or a D.S. the first time, a To Coda the time after the jump back
    bool m_timesGiven = false;
    // Whether the repeats are played again after this jump ("D.S. al Coda with repeat"; MuseScore's "play repeats";
    // MuseNote RC75): its MEI type's token "repeats"
    bool m_playRepeats = false;
};

struct FineInfo {
    FineInfo() { m_fine = false; }
    FineInfo(bool fine) { m_fine = fine; }
    // The passes the score names for it (MusicXML time-only): it ends the play the time its part is played that it
    // names, a jump back or none. Without them it ends the play after a jump back (MuseNote RC79)
    std::list<int> m_times;
    bool m_timesGiven = false;

    bool m_fine;
};

struct SectionInfo {
    SectionInfo(bool repeatStart = false)
    {
        m_visited = 0;
        m_played = 0;
        m_classId = SECTION;
        m_target = NULL;
        m_repeatStart = repeatStart;
    }
    SectionInfo(const EndingInfo &endingInfo)
    {
        m_visited = 0;
        m_played = 0;
        m_classId = ENDING;
        m_target = NULL;
        m_endingInfo = endingInfo;
        m_repeatStart = false;
    }
    SectionInfo(const RepeatInfo &repeatInfo)
    {
        m_visited = 0;
        m_played = 0;
        m_classId = SECTION;
        m_target = NULL;
        m_repeatInfo = repeatInfo;
        m_repeatStart = false;
    }

    void merge(const EndingInfo &endingInfo)
    {
        m_classId = ENDING;
        m_endingInfo = endingInfo;
    }

    ClassId m_classId;
    Object *m_target;
    // what a dalsegno and a tocoda name: a segno's label and a coda's are two kinds (MusicXML pairs dalsegno
    // with segno, tocoda with coda)
    std::string m_segnoLabel;
    std::string m_codaLabel;
    EndingInfo m_endingInfo;
    RepeatInfo m_repeatInfo;
    bool m_repeatStart;
    // the ending written in the score this part is of, counted from 0 (-1: none): one ending a sign or a repeat sign
    // inside it divides is one ending element still, its parts sections inside it (MuseNote RC62)
    int m_endingGroup = -1;
    // the jumps at its end, each for its times: a barline can hold several (MusicXML: a To Coda the second time,
    // another the third). One was kept, the last read (MuseNote RC36)
    std::vector<JumpInfo> m_jumps;
    FineInfo m_fineInfo;
    // the returns to the part (the order's visits), and every time it is played (its repeat's passes too)
    int m_visited;
    int m_played;
};

/** The parts in score order, each with its measures */
typedef std::vector<std::pair<SectionInfo, std::vector<Measure *>>> Sections;

} // namespace play

/**
 * The order of play of [sections]: repeats, endings in the passes their numbers name, jumps (da capo, dal segno, to
 * coda) and fine. The one rule of the engine, for every format: each part's m_target is the element named in the
 * order. Counts its visits in the parts.
 */
xsdAnyURI_List WriteOrderOfPlay(play::Sections &sections);

/**
 * Writes the order of play of [score] into it, from the score as it stands: its measures in score order through
 * sections and endings, their repeat signs, and the marks of a jump (WriteOrderOfPlay is asked). Nothing is written
 * for a score with no repeat end, no ending and no jump. [repeatTimes] says how often the repeat that ends at a
 * measure is played, where a reader knows of more than twice. [playlist], where it is not empty, is the stretches of
 * the score (each from its first measure to its last) in the order they are played, a stretch as often as it is
 * listed (the parts of an ABC tune): the order is then each stretch's own, one after the other.
 */
void WriteOrderOfPlayFor(Score *score, const std::map<const Measure *, int> &repeatTimes,
    const std::vector<std::pair<const Measure *, const Measure *>> &playlist);

/** The numbers and ranges in a list like "1, 2", "1.2." or "1-3" (an ending's passes, the times a jump is taken) */
std::list<int> parseInts(const std::string &str);

/**
 * What a score keeps of its file's order of play beyond its marks (MuseNote RC41), as tokens of MEI's @type, after a
 * mark's own kind: the times a jump is taken ("times-2.3") on its mark; a repeat's count other than two ("repeat-3")
 * and a repeat taken after a jump ("after-jump") on the measure its end stands in. The label of a jump's coda or segno,
 * and a coda's or segno's own, is the mark's @label. They are written where the file says them and read by the rule
 * for a score with no order of its own, so the score plays so without its order, and its exports can say it.
 */
namespace order {
    std::string TimesToken(const std::list<int> &times);
    std::string RepeatToken(int times);
    inline const std::string kAfterJump = "after-jump";
    /** The tokens of an @type, in order */
    std::vector<std::string> TokensOf(const std::string &type);
    /** The times a token names, or none */
    std::optional<std::list<int>> TimesOf(const std::string &token);
    /** The count a token names, or none */
    std::optional<int> RepeatOf(const std::string &token);
} // namespace order

class ExpansionMap {

public:
    /**
     * @name Constructors, destructors, reset methods
     * Reset method resets all attribute classes
     */
    ///@{
    ExpansionMap();
    virtual ~ExpansionMap();
    ///@}

    /*
     * Clear the content of the expansion map.
     */
    virtual void Reset();

    /**
     * Check if m_expansionMap has been filled
     */
    bool HasExpansionMap();

    /**
     * Expand expansion recursively
     */
    Object *Expand(Expansion *expansion, std::set<std::string> &existingList, Object *prevSection,
        xsdAnyURI_List &deletionList, bool deleteList);

    /**
     * The objects of the document under [root] as written, by their ids, for Expand to find what an order names;
     * cleared after (MuseNote RC93: each reference was looked for in the whole document, which grows with every copy)
     */
    void RecordWrittenIds(Object *root);
    void ClearWrittenIds();

    /**
     * The ids of [xmlId]'s group: the one the others copy first, then each copy as it was made; [xmlId] alone where
     * it has none. A copy of the whole list, for the toolkit's own calls: the engine reads FirstOf, LastOf and CountOf
     */
    std::vector<std::string> GetExpansionIDsForElement(const std::string &xmlId) const;
    /** The first id of [xmlId]'s group, the last, and how many it holds (MuseNote RC93) */
    ///@{
    std::string FirstOf(const std::string &xmlId) const;
    std::string LastOf(const std::string &xmlId) const;
    size_t CountOf(const std::string &xmlId) const;
    ///@}

    /**
     * Write the currentexpansionMap to a JSON string
     */
    void ToJson(std::string &output);

    /**
     * Generate an expan for the score analysing the repeats and endings
     */
    void GenerateExpansionFor(Score *score);

    /**
     * Judges every spanning element of the score as it is played (once Expand has written it out) by the order of
     * play, originals and copies alike. A copy is given its end when it is made (the latest copy of the end so far),
     * or by a count of bars, which in the score as played counts played bars: across a jump that is another pass's
     * note, or a note already played. [writtenBars] is each measure's place in the score as written, taken before
     * Expand moved anything.
     * What joins notes (a tie, a glissando, a beam across bars) holds in a pass only where the play goes from its
     * start to its end within the bars it joins: its end is that copy, else it is taken out of that pass. What spans
     * music covers the bars played from its start while the play stays within the bars it spans on the page: its end
     * is the copy the play reaches there, else the last note, chord or rest it covers before the play leaves them.
     */
    void FollowSpansInOrder(Object *root, const std::map<std::string, int> &writtenBars);

    /**
     * The score definitions in effect at the start and the end of each part an order can play (a section, an ending,
     * a measure, an editorial element) in [root] as written, read before it is expanded; cleared after (MuseNote RC58)
     */
    void RecordWrittenContexts(const Object *root);
    void ClearWrittenContexts();

    /**
     * @name Setter and getter for the generating attempt flag
     */
    ///@{
    void SetProcessed(bool isProcessed) { m_isProcessed = isProcessed; }
    bool IsProcessed() { return m_isProcessed; }
    ///@}

    /**
     * Whether the order of play was refused: more than the engine plays (Doc::ExpandExpansions). Such a score is
     * not played at all, rather than played otherwise than it says (MuseNote RC91)
     */
    ///@{
    void SetRefused(bool isRefused) { m_isRefused = isRefused; }
    bool IsRefused() const { return m_isRefused; }
    ///@}

    //----------------//
    // Static methods //
    //----------------//

    /**
     * @name Methods to check if a measure yields a repeat start or end
     */
    ///@{
    static bool IsRepeatStart(Measure *measure);
    static bool IsRepeatEnd(Measure *measure);
    static bool IsNextRepeatStart(Measure *measure);
    static bool IsPreviousRepeatEnd(Measure *measure);
    ///@}

private:
    bool UpdateIDs(Object *object);

    void GetIDList(Object *object, std::vector<std::string> &idList);

    void GeneratePredictableIDs(Object *source, Object *target);
    /** What parent->FindDescendantByID(id) finds, from the ids recorded where they say (MuseNote RC93) */
    Object *FindWritten(Object *parent, const std::string &id);
    /** [object] and what it holds, made by Expand, recorded with the document's ids while they are */
    void RecordMade(Object *object);
    /** Where [part] is placed after [previous], what of its own score definitions differs from those in effect there
     * is said again just before it (MuseNote RC58) */
    void RestateWrittenContext(Object *previous, Object *part);
    void RecordWrittenContexts(const Object *object, WrittenContext &context);

    /** Ads an id string to an original/notated id */
    bool AddExpandedIDToExpansionMap(const std::string &origXmlId, std::string newXmlId);

    /**
     * The expansion map indicates which xmlId has been repeated (expanded) elsewhere: each id copied, or a copy, by
     * its group in m_groups, the one they copy first and each copy in the order made. (Each id held a list of its
     * own, and every list was written to at each copy: k copies cost k times k. MuseNote RC93)
     */
    std::map<std::string, size_t> m_groupOf;
    std::vector<std::vector<std::string>> m_groups;
    /* the objects of the document as written by their ids: every one, and those a walk of what is shown meets */
    std::map<std::string, std::vector<Object *>> m_writtenIds;
    std::map<std::string, std::vector<Object *>> m_shownIds;
    /* the ids held twice that an order named, each said once */
    std::set<std::string> m_idsHeldTwice;

private:
    /** A flag indicating that the generation processed has been run even if the expansion map is empty  */
    /* the score definitions in effect at the start and the end of each part as written, by its id (RC58) */
    std::map<std::string, WrittenContext> m_startContexts;
    std::map<std::string, WrittenContext> m_endContexts;
    /* each copy the expansion made, by its id: the id of the part it copies (RC58) */
    std::map<std::string, std::string> m_copyOf;
    bool m_isProcessed;
    bool m_isRefused = false;
    /** The expansions that Expand is in, outermost first, but for the one it expands now */
    std::vector<const Expansion *> m_expanding;
};

} // namespace vrv

#endif /* expansionmap_h */
