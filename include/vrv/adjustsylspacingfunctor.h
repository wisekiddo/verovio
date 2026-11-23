/////////////////////////////////////////////////////////////////////////////
// Name:        adjustsylspacingfunctor.h
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_ADJUSTSYLSPACINGFUNCTOR_H__
#define __VRV_ADJUSTSYLSPACINGFUNCTOR_H__

#include "functor.h"

#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace vrv {

//----------------------------------------------------------------------------
// AdjustSylSpacingFunctor
//----------------------------------------------------------------------------

/**
 * This class adjusts the spacing of the syl processing verse by verse.
 * A line of verses holds every voice of a staff, spaced from left to right (MuseNote RC205, the idea of upstream
 * e5f1b2456).
 */
class AdjustSylSpacingFunctor : public DocFunctor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    AdjustSylSpacingFunctor(Doc *doc);
    virtual ~AdjustSylSpacingFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return true; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitMeasureEnd(Measure *measure) override;
    FunctorCode VisitStaff(Staff *staff) override;
    FunctorCode VisitSystem(System *system) override;
    FunctorCode VisitSystemEnd(System *system) override;
    FunctorCode VisitVerse(Verse *verse) override;
    ///@}

protected:
    //
private:
    /** Adjusts the spacing between the previous verse and this one */
    void AdjustVerse(Verse *verse);

    /** Moves the previous verse back into the free space, and the verses at its alignment with it */
    int AdjustPreviousVersePosition(int &overlap);

public:
    //
private:
    // List of adjustment tuples (Alignment start|Alignment end|distance)
    ArrayOfAdjustmentTuples m_overlappingSyl;
    // The previous verse
    Verse *m_previousVerse;
    // The verses of other voices at the previous verse's alignment, moved with it
    std::vector<Verse *> m_simultaneousVerses;
    // The previous syl
    Syl *m_lastSyl;
    // The previous measure
    Measure *m_previousMeasure;
    // The current LabelAbbr of each voice, by layer @n
    std::map<int, LabelAbbr *> m_currentLabelAbbrs;
    // The voices, by layer @n, with a verse already in the current system
    std::set<int> m_systemLayers;
    // The verses of the current measure, spaced at its end in the order of their alignments
    std::vector<Verse *> m_measureVerses;
    // Amount of free space
    int m_freeSpace;
    // The staff size
    int m_staffSize;
};

//----------------------------------------------------------------------------
// RecordSylSpacingFunctor
//----------------------------------------------------------------------------

/**
 * This class records, in one walk of a page, what AdjustSylSpacingFunctor meets in the walk of each line of verses:
 * the systems and the ends of the measures, which every line meets, the staves of each number, and the verses of each
 * line, a staff's side (above or below) and verse number, every voice of the staff together (MuseNote RC205); each in
 * the order of the page. Replay gives a line's own to the functor in that order. (A filtered walk of the whole page
 * for each verse: as many walks as verses, and a Humdrum token of 3,000 words took seconds to lay out. MuseNote RC92)
 */
class RecordSylSpacingFunctor : public Functor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    RecordSylSpacingFunctor() {}
    virtual ~RecordSylSpacingFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return true; }

    /** A line of verses: the staff @n, whether the verses are above it, the verse @n */
    using Line = std::tuple<int, bool, int>;

    /** The lines met, in the order of their keys */
    std::vector<Line> GetLines() const;

    /** Gives [functor] what the walk of the verses of [line] meets */
    void Replay(const Line &line, AdjustSylSpacingFunctor &functor) const;

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitMeasureEnd(Measure *measure) override;
    FunctorCode VisitStaff(Staff *staff) override;
    FunctorCode VisitSystem(System *system) override;
    FunctorCode VisitSystemEnd(System *system) override;
    FunctorCode VisitVerse(Verse *verse) override;
    ///@}

private:
    // What a walk meets, by its place in the walk
    template <class OBJECT> using Met = std::vector<std::pair<int, OBJECT *>>;
    // Met by every verse
    Met<System> m_systems;
    Met<Measure> m_measureEnds;
    Met<System> m_systemEnds;
    // The staves, by their number
    std::map<int, Met<Staff>> m_staves;
    // The verses, by line
    std::map<Line, Met<Verse>> m_verses;
    // The place in the walk, and the number of the staff it is in
    int m_place = 0;
    int m_staffN = 0;
};

} // namespace vrv

#endif // __VRV_ADJUSTSYLSPACINGFUNCTOR_H__
