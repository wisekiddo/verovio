/////////////////////////////////////////////////////////////////////////////
// Name:        adjustharmgrpsspacingfunctor.cpp
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "adjustharmgrpsspacingfunctor.h"

//----------------------------------------------------------------------------

#include "doc.h"
#include "harm.h"
#include "syl.h"
#include "system.h"
#include "vrv.h"

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// AdjustHarmGrpsSpacingFunctor
//----------------------------------------------------------------------------

AdjustHarmGrpsSpacingFunctor::AdjustHarmGrpsSpacingFunctor(Doc *doc) : DocFunctor(doc)
{
    m_currentGrp = 0;
    m_previousHarmPositioner = NULL;
    m_previousHarmStart = NULL;
    m_previousMeasure = NULL;
    m_currentSystem = NULL;
}

FunctorCode AdjustHarmGrpsSpacingFunctor::VisitHarm(Harm *harm)
{
    int currentGrpId = harm->GetDrawingGrpId();

    // No group ID, nothing to do - should probably never happen
    if (currentGrpId == 0) {
        return FUNCTOR_SIBLINGS;
    }

    // We are filling the array of grp ids for the system
    if (m_currentGrp == 0) {
        // Look if we already have this grpId
        if (std::find(m_grpIds.begin(), m_grpIds.end(), currentGrpId) == m_grpIds.end()) {
            // if not, then just add to the list of grpIds for the system
            m_grpIds.push_back(currentGrpId);
        }
        // This is it for this pass
        return FUNCTOR_SIBLINGS;
    }
    // We are processing harm for a grp Id which is not the current one, skip it
    else if (currentGrpId != m_currentGrp) {
        return FUNCTOR_SIBLINGS;
    }

    // Spaced at the end of the measure, in the order of time (MuseNote RC226). A harm with no start, or none aligned,
    // is not drawn and has no positioner
    if (harm->GetStart() && harm->GetStart()->GetAlignment()) {
        m_measureHarms.push_back(harm);
    }

    return FUNCTOR_SIBLINGS;
}

void AdjustHarmGrpsSpacingFunctor::AdjustHarmSpacing(Harm *harm)
{
    /************** Find the widest positioner **************/

    // Get all the positioners for this object - all of them (all staves) because we can have different staff sizes
    ArrayOfFloatingPositioners positioners;
    m_currentSystem->m_systemAligner.FindAllPositionerPointingTo(&positioners, harm);

    FloatingPositioner *harmPositioner = NULL;
    // Something is probably not right if nothing found - maybe no @staff
    if (positioners.empty()) {
        LogDebug("Something was wrong when searching positioners for %s '%s'", harm->GetClassName().c_str(),
            harm->GetID().c_str());
        return;
    }

    // Keep the one with the lowest left position (this will also be the widest)
    for (auto const &positioner : positioners) {
        if (!harmPositioner || (harmPositioner->GetContentLeft() > positioner->GetContentLeft())) {
            harmPositioner = positioner;
        }
    }

    // If the harm positioner is missing or is empty, do not adjust spacing
    // Nor for a blank harm, whose box has no width: it is not spaced from, and takes no room (MuseNote RC226)
    if (!harmPositioner || !harmPositioner->HasContentBB()
        || (harmPositioner->GetContentX2() <= harmPositioner->GetContentX1())) {
        return;
    }

    // If we have more than one harm at the same position, do not adjust them
    // This situation makes sense when the first of them is right aligned
    // The next harm is spaced from the one reaching furthest right, not from the last one, which can be narrower
    // (MuseNote RC226). A position is an alignment: a harm on a note and one timed at it share it
    if (m_previousHarmStart && (m_previousHarmStart->GetAlignment() == harm->GetStart()->GetAlignment())) {
        if (harmPositioner->GetContentRight() > m_previousHarmPositioner->GetContentRight()) {
            m_previousHarmPositioner = harmPositioner;
        }
        return;
    }

    /************** Calculate the adjustment **************/

    assert(harm->GetStart());
    assert(harmPositioner);

    // First harm in the system
    if (!m_previousMeasure && !m_previousHarmPositioner) {
        // Check that is it not overflowing the beginning of the measure
        int overflow = harm->GetStart()->GetDrawingX() + harmPositioner->GetContentX1();
        Measure *measure = vrv_cast<Measure *>(harm->GetFirstAncestor(MEASURE));
        if ((overflow < 0) && measure && measure->GetLeftBarLine()) {
            m_overlappingHarm.push_back(std::make_tuple(
                measure->GetLeftBarLine()->GetAlignment(), harm->GetStart()->GetAlignment(), -overflow));
        }
    }

    // Not much to do when we hit the first harm of the system
    if (m_previousHarmPositioner == NULL) {
        m_previousHarmStart = harm->GetStart();
        m_previousHarmPositioner = harmPositioner;
        m_previousMeasure = NULL;
        return;
    }

    int xShift = 0;

    // We have a previous harm from a previous measure - we need to add the measure widths because the measures are
    // not aligned yet: of each measure ended since the previous harm, its own and any after it with no harm, not of the
    // last only (MuseNote RC226)
    for (Measure *measure : m_previousMeasures) {
        xShift += measure->GetWidth();
    }

    assert(m_previousHarmStart);
    assert(m_previousHarmPositioner);

    int overlap = m_previousHarmPositioner->GetContentRight() - (harmPositioner->GetContentLeft() + xShift);
    // Two units as default spacing
    int wordSpace = 2 * m_doc->GetDrawingUnit(100);
    Syl::AdjustToLyricSize(m_doc, wordSpace);
    overlap += wordSpace;

    if (overlap > 0) {
        // We are adjusting harms in two different measures - move only the right barline of the first measure, the
        // previous harm's own: the measure ended last can be a later one, with no harm, which the previous harm's start
        // is no alignment of (MuseNote RC226)
        if (!m_previousMeasures.empty()) {
            Measure *previousHarmMeasure = m_previousMeasures.front();
            m_overlappingHarm.push_back(std::make_tuple(m_previousHarmStart->GetAlignment(),
                previousHarmMeasure->GetRightBarLine()->GetAlignment(), overlap));
            // Do it now
            previousHarmMeasure->m_measureAligner.AdjustProportionally(m_overlappingHarm);
            m_overlappingHarm.clear();
        }
        else {
            // Normal case, both in the same measure
            m_overlappingHarm.push_back(
                std::make_tuple(m_previousHarmStart->GetAlignment(), harm->GetStart()->GetAlignment(), overlap));
        }
    }

    m_previousHarmStart = harm->GetStart();
    m_previousHarmPositioner = harmPositioner;
    m_previousMeasure = NULL;
    m_previousMeasures.clear();
}

FunctorCode AdjustHarmGrpsSpacingFunctor::VisitMeasureEnd(Measure *measure)
{
    // The harms in the order of time, not as written: each is spaced from the one before it, and one written before
    // an earlier harm was spaced from a harm to its right, the room made running backwards (MuseNote RC226). Harms of
    // one time keep their written order
    std::stable_sort(m_measureHarms.begin(), m_measureHarms.end(), [](const Harm *harm1, const Harm *harm2) {
        return (harm1->GetStart()->GetAlignment()->GetTime() < harm2->GetStart()->GetAlignment()->GetTime());
    });
    for (Harm *harm : m_measureHarms) {
        this->AdjustHarmSpacing(harm);
    }
    m_measureHarms.clear();

    // At the end of the measure - pass it along for overlapping verses
    m_previousMeasure = measure;
    // and keep it until the next harm, with any measure after it: they stand between the previous harm and it
    // (MuseNote RC226)
    if (m_previousHarmPositioner) {
        m_previousMeasures.push_back(measure);
    }

    // Adjust the position of the alignment according to what we have collected for this harm grp
    measure->m_measureAligner.AdjustProportionally(m_overlappingHarm);
    m_overlappingHarm.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustHarmGrpsSpacingFunctor::VisitSystem(System *system)
{
    // reset it, but not the current grpId!
    m_currentSystem = system;
    m_overlappingHarm.clear();
    m_measureHarms.clear();
    m_previousHarmPositioner = NULL;
    m_previousHarmStart = NULL;
    m_previousMeasure = NULL;
    m_previousMeasures.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustHarmGrpsSpacingFunctor::VisitSystemEnd(System *system)
{
    // End of the first pass - loop over for each group id
    if (m_currentGrp == 0) {
        for (auto grpId : m_grpIds) {
            m_currentGrp = grpId;
            system->Process(*this);
        }
        // Make sure we reset it for the next system
        m_currentGrp = 0;
        return FUNCTOR_CONTINUE;
    }

    /************** End of a system when actually adjusting **************/

    if (!m_previousMeasure) {
        return FUNCTOR_CONTINUE;
    }

    // Here we also need to handle the last harm of the measure - we check the alignment with the right barline
    // of the last measure, which the last harm can stand measures before, with no harm in them: the widths of the
    // measures from its own to the last, and the room in its own (MuseNote RC226)
    Measure *roomMeasure = m_previousMeasure;
    if (m_previousHarmPositioner && !m_previousMeasures.empty()) {
        roomMeasure = m_previousMeasures.front();
        int lineEnd = m_previousMeasure->GetRightBarLine()->GetAlignment()->GetXRel();
        for (size_t i = 0; i + 1 < m_previousMeasures.size(); ++i) {
            lineEnd += m_previousMeasures.at(i)->GetWidth();
        }
        int overlap = m_previousHarmPositioner->GetContentRight() - lineEnd;

        if (overlap > 0) {
            m_overlappingHarm.push_back(std::make_tuple(m_previousHarmStart->GetAlignment(),
                roomMeasure->GetRightBarLine()->GetAlignment(), overlap));
        }
    }

    // Adjust the position of the alignment according to what we have collected for this harm group id
    roomMeasure->m_measureAligner.AdjustProportionally(m_overlappingHarm);
    m_overlappingHarm.clear();

    return FUNCTOR_CONTINUE;
}

} // namespace vrv
