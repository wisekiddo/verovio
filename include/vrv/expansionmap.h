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

/**
 * An order of play while it is written, and the least that following it can cost: each section by the times it is
 * named, squared (it holds one element at the least; the count itself is in Doc::ExpandExpansions). Whoever writes
 * an order stops when that is past what is followed. What it has written by then costs more than is followed too.
 */
class OrderOfPlay {
public:
    /** What following an order of play may cost */
    static constexpr size_t s_maxCost = 1000000;

    /** Adds a reference. False, and nothing added, once the order costs more than is followed */
    bool Add(const std::string &ref);

    const xsdAnyURI_List &GetRefs() const { return m_refs; }

private:
    xsdAnyURI_List m_refs;
    std::map<std::string, size_t> m_timesNamed;
    size_t m_leastCost = 0;
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
    JumpInfo(JUMPTYPE jump, const std::string &label, const std::list<int> &times)
    {
        m_label = label;
        m_jump = jump;
        m_times = times;
    }
    JumpInfo(JUMPTYPE jump, const std::list<int> &times)
    {
        m_jump = jump;
        m_times = times;
    }
    JumpInfo(JUMPTYPE jump) { m_jump = jump; }

    std::string m_label;
    JUMPTYPE m_jump;
    std::list<int> m_times;
};

struct FineInfo {
    FineInfo() { m_fine = false; }
    FineInfo(bool fine) { m_fine = fine; }

    bool m_fine;
};

struct SectionInfo {
    SectionInfo(bool repeatStart = false)
    {
        m_visited = 0;
        m_classId = SECTION;
        m_target = NULL;
        m_repeatStart = repeatStart;
    }
    SectionInfo(const EndingInfo &endingInfo)
    {
        m_visited = 0;
        m_classId = ENDING;
        m_target = NULL;
        m_endingInfo = endingInfo;
        m_repeatStart = false;
    }
    SectionInfo(const RepeatInfo &repeatInfo)
    {
        m_visited = 0;
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
    JumpInfo m_jumpInfo;
    FineInfo m_fineInfo;
    int m_visited;
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

/** The numbers in a list like "1, 2" (an ending's passes, the times a jump is taken) */
std::list<int> parseInts(const std::string &str);

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
    Object *Expand(Expansion *expansion, xsdAnyURI_List &existingList, Object *prevSection,
        xsdAnyURI_List &deletionList, bool deleteList);

    std::vector<std::string> GetExpansionIDsForElement(const std::string &xmlId);

    /**
     * Write the currentexpansionMap to a JSON string
     */
    void ToJson(std::string &output);

    /**
     * Generate an expan for the score analysing the repeats and endings
     */
    void GenerateExpansionFor(Score *score);

    /**
     * @name Setter and getter for the generating attempt flag
     */
    ///@{
    void SetProcessed(bool isProcessed) { m_isProcessed = isProcessed; }
    bool IsProcessed() { return m_isProcessed; }
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

    /** Ads an id string to an original/notated id */
    bool AddExpandedIDToExpansionMap(const std::string &origXmlId, std::string newXmlId);

public:
    /** The expansion map indicates which xmlId has been repeated (expanded) elsewhere */
    std::map<std::string, std::vector<std::string>> m_map;

private:
    /** A flag indicating that the generation processed has been run even if the expansion map is empty  */
    bool m_isProcessed;
    /** The expansions that Expand is in, outermost first, but for the one it expands now */
    std::vector<const Expansion *> m_expanding;
};

} // namespace vrv

#endif /* expansionmap_h */
