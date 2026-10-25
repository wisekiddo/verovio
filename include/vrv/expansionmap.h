/////////////////////////////////////////////////////////////////////////////
// Name:        expansionmap.h
// Author:      Werner Goebl
// Created:     2019
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_EXPANSION_MAP_H__
#define __VRV_EXPANSION_MAP_H__

#include <map>

//----------------------------------------------------------------------------

#include "expansion.h"
#include "options.h"

namespace vrv {

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

    std::string CreateSection(
        Section *section, const ListOfObjects::iterator &first, const ListOfObjects::iterator &last);

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
