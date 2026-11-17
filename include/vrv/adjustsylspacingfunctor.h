/////////////////////////////////////////////////////////////////////////////
// Name:        adjustsylspacingfunctor.h
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_ADJUSTSYLSPACINGFUNCTOR_H__
#define __VRV_ADJUSTSYLSPACINGFUNCTOR_H__

#include "functor.h"

#include <tuple>

namespace vrv {

//----------------------------------------------------------------------------
// AdjustSylSpacingFunctor
//----------------------------------------------------------------------------

/**
 * This class adjusts the spacing of the syl processing verse by verse.
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
    //
public:
    //
private:
    // List of adjustment tuples (Alignment start|Alignment end|distance)
    ArrayOfAdjustmentTuples m_overlappingSyl;
    // The previous verse
    Verse *m_previousVerse;
    // The previous syl
    Syl *m_lastSyl;
    // The previous measure
    Measure *m_previousMeasure;
    // The current LabelAbbr
    LabelAbbr *m_currentLabelAbbr;
    // Amount of free space
    int m_freeSpace;
    // The staff size
    int m_staffSize;
};

//----------------------------------------------------------------------------
// RecordSylSpacingFunctor
//----------------------------------------------------------------------------

/**
 * This class records, in one walk of a page, what AdjustSylSpacingFunctor meets in the walk of each verse: the
 * systems and the ends of the measures, which every verse meets, the staves of each number, and the verses of each
 * staff, layer and verse number; each in the order of the page. Replay gives a verse's own to the functor in that
 * order. (A filtered walk of the whole page for each verse: as many walks as verses, and a Humdrum token of 3,000
 * words took seconds to lay out. MuseNote RC92)
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

    /** Gives [functor] what the walk of verse [verseN] of layer [layerN] of the staves numbered [staffN] meets */
    void Replay(int staffN, int layerN, int verseN, AdjustSylSpacingFunctor &functor) const;

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitLayer(Layer *layer) override;
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
    // The verses, by staff, layer and verse number
    std::map<std::tuple<int, int, int>, Met<Verse>> m_verses;
    // The place in the walk, and the numbers of the staff and the layer it is in
    int m_place = 0;
    int m_staffN = 0;
    int m_layerN = 0;
};

} // namespace vrv

#endif // __VRV_ADJUSTSYLSPACINGFUNCTOR_H__
