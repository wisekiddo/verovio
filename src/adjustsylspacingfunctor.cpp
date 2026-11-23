/////////////////////////////////////////////////////////////////////////////
// Name:        adjustsylspacingfunctor.cpp
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "adjustsylspacingfunctor.h"

#include <algorithm>
#include <climits>

//----------------------------------------------------------------------------

#include "doc.h"
#include "label.h"
#include "labelabbr.h"
#include "layer.h"
#include "measure.h"
#include "staff.h"
#include "syl.h"
#include "system.h"
#include "verse.h"

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// AdjustSylSpacingFunctor
//----------------------------------------------------------------------------

AdjustSylSpacingFunctor::AdjustSylSpacingFunctor(Doc *doc) : DocFunctor(doc)
{
    m_previousVerse = NULL;
    m_lastSyl = NULL;
    m_previousMeasure = NULL;
    m_freeSpace = 0;
    m_staffSize = 100;
}

int AdjustSylSpacingFunctor::AdjustPreviousVersePosition(int &overlap)
{
    const int previousX = m_previousVerse->GetDrawingXRel();
    const int nextFreeSpace = m_previousVerse->AdjustPosition(overlap, m_freeSpace, m_doc);
    const int shift = m_previousVerse->GetDrawingXRel() - previousX;
    for (Verse *verse : m_simultaneousVerses) {
        verse->SetDrawingXRel(verse->GetDrawingXRel() + shift);
    }
    return nextFreeSpace;
}

FunctorCode AdjustSylSpacingFunctor::VisitMeasureEnd(Measure *measure)
{
    // Every voice's verses of the measure, spaced in the order of their alignments (MuseNote RC205: a voice's verses
    // spaced alone ran into another voice's on the same line)
    std::stable_sort(m_measureVerses.begin(), m_measureVerses.end(), [](const Verse *verse1, const Verse *verse2) {
        const int x1 = verse1->GetAlignment() ? verse1->GetAlignment()->GetXRel() : 0;
        const int x2 = verse2->GetAlignment() ? verse2->GetAlignment()->GetXRel() : 0;
        return (x1 < x2);
    });
    for (Verse *verse : m_measureVerses) {
        this->AdjustVerse(verse);
    }
    m_measureVerses.clear();

    // At the end of the measure - pass it along for overlapping verses
    m_previousMeasure = measure;

    // Adjust the position of the alignment according to what we have collected for this verse
    measure->m_measureAligner.AdjustProportionally(m_overlappingSyl);
    m_overlappingSyl.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustSylSpacingFunctor::VisitStaff(Staff *staff)
{
    // Set the staff size for this pass
    m_staffSize = staff->m_drawingStaffSize;

    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustSylSpacingFunctor::VisitSystem(System *system)
{
    // reset it
    m_overlappingSyl.clear();
    m_previousVerse = NULL;
    m_simultaneousVerses.clear();
    m_previousMeasure = NULL;
    m_systemLayers.clear();
    m_measureVerses.clear();
    m_freeSpace = 0;
    m_staffSize = 100;

    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustSylSpacingFunctor::VisitSystemEnd(System *system)
{
    if (!m_previousMeasure) {
        return FUNCTOR_CONTINUE;
    }

    // Here we also need to handle the last syl of the measure - we check the alignment with the right barline
    if (m_previousVerse && m_lastSyl) {
        int overlap = m_lastSyl->GetContentRight() - m_previousMeasure->GetRightBarLine()->GetAlignment()->GetXRel();
        this->AdjustPreviousVersePosition(overlap);

        // If the previous verse was not in the previous measure (but before), ignore the overlap because it is
        // not likely to go over the whole following measure
        if ((m_previousMeasure == m_previousVerse->GetFirstAncestor(MEASURE)) && (overlap > 0)) {
            m_overlappingSyl.push_back(std::make_tuple(
                m_previousVerse->GetAlignment(), m_previousMeasure->GetRightBarLine()->GetAlignment(), overlap));
        }
    }

    // Adjust the position of the alignment according to what we have collected for this verse
    m_previousMeasure->m_measureAligner.AdjustProportionally(m_overlappingSyl);
    m_overlappingSyl.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode AdjustSylSpacingFunctor::VisitVerse(Verse *verse)
{
    // Spaced at the end of the measure, with the other voices' verses (MuseNote RC205)
    m_measureVerses.push_back(verse);

    return FUNCTOR_CONTINUE;
}

void AdjustSylSpacingFunctor::AdjustVerse(Verse *verse)
{
    const Layer *layer = vrv_cast<const Layer *>(verse->GetFirstAncestor(LAYER));
    assert(layer);

    /****** find label / labelAbbr */

    // The abbreviations are tracked for each voice
    LabelAbbr *&currentLabelAbbr = m_currentLabelAbbrs[layer->GetN()];

    // If we have a <label>, reset the previous abbreviation
    if (verse->FindDescendantByType(LABEL)) {
        currentLabelAbbr = NULL;
    }

    bool newLabelAbbr = false;
    verse->SetDrawingLabelAbbr(NULL);
    // Find the labelAbbr (if none previously given)
    if (currentLabelAbbr == NULL) {
        currentLabelAbbr = vrv_cast<LabelAbbr *>(verse->FindDescendantByType(LABELABBR));
        // Keep indication that this is a new abbreviation and that it should not be displayed on this verse
        newLabelAbbr = true;
    }

    /*******/

    ListOfObjects syls = verse->FindAllDescendantsByType(SYL);

    int shift = m_doc->GetDrawingUnit(m_staffSize);
    Syl::AdjustToLyricSize(m_doc, shift);

    int previousSylShift = 0;

    verse->SetDrawingXRel(-1 * shift);

    ListOfObjects::iterator iter = syls.begin();
    while (iter != syls.end()) {
        if ((*iter)->HasContentHorizontalBB()) {
            Syl *syl = vrv_cast<Syl *>(*iter);
            assert(syl);
            syl->SetDrawingXRel(previousSylShift);
            previousSylShift += syl->GetContentX2() + syl->CalcConnectorSpacing(m_doc, m_staffSize);
            ++iter;
        }
        else {
            iter = syls.erase(iter);
        }
    }

    if (syls.empty()) return;

    Syl *firstSyl = vrv_cast<Syl *>(syls.front());
    assert(firstSyl);
    // We keep a pointer to the last syl because we move it (when more than one) and the verse content bounding box is
    // not updated
    Syl *lastSyl = vrv_cast<Syl *>(syls.back());
    assert(lastSyl);

    // The abbreviation is shown on each voice's first verse of the system
    if (m_systemLayers.insert(layer->GetN()).second && !newLabelAbbr && currentLabelAbbr) {
        verse->SetDrawingLabelAbbr(currentLabelAbbr);
    }

    // Not much to do when we hit the first syllable of the system
    if (m_previousVerse == NULL) {
        m_previousVerse = verse;
        m_simultaneousVerses.clear();
        m_lastSyl = lastSyl;

        // No free space because we never move the first one back
        m_freeSpace = 0;
        m_previousMeasure = NULL;
        return;
    }

    // Verses of two voices at one alignment are not spaced against each other: the one reaching furthest right is the
    // previous, and the others move with it (upstream e5f1b2456). Spaced, they would ask the measure for room from
    // their alignment to itself, which widens it and leaves them as they are.
    if (!m_previousMeasure && (verse->GetAlignment() == m_previousVerse->GetAlignment())
        && (layer != m_previousVerse->GetFirstAncestor(LAYER))) {
        if (lastSyl->GetContentRight() > m_lastSyl->GetContentRight()) {
            m_simultaneousVerses.push_back(m_previousVerse);
            m_previousVerse = verse;
            m_lastSyl = lastSyl;
        }
        else {
            m_simultaneousVerses.push_back(verse);
        }
        return;
    }

    int xShift = 0;

    // We have a previous syllable from the previous measure - we need to add the measure width because the measures are
    // not aligned yet
    if (m_previousMeasure) {
        xShift = m_previousMeasure->GetWidth();
    }

    // Use the syl because the content bounding box of the verse might be invalid at this stage
    int overlap = m_lastSyl->GetContentRight() - (firstSyl->GetContentLeft() + xShift);
    overlap += m_lastSyl->CalcConnectorSpacing(m_doc, m_staffSize);

    // Check that we also include the space for the label if the verse has a new label
    Label *label = vrv_cast<Label *>(verse->FindDescendantByType(LABEL));
    if (label) {
        overlap += (label->GetContentX2() - label->GetContentX1()) + m_doc->GetDrawingDoubleUnit(m_staffSize);
    }

    int nextFreeSpace = this->AdjustPreviousVersePosition(overlap);

    if (overlap > 0) {
        // We are adjusting syl in two different measures - move only the right barline of the first measure
        if (m_previousMeasure) {
            m_overlappingSyl.push_back(std::make_tuple(
                m_previousVerse->GetAlignment(), m_previousMeasure->GetRightBarLine()->GetAlignment(), overlap));
            // Do it now
            m_previousMeasure->m_measureAligner.AdjustProportionally(m_overlappingSyl);
            m_overlappingSyl.clear();
        }
        else {
            // Normal case, both in the same measure
            m_overlappingSyl.push_back(
                std::make_tuple(m_previousVerse->GetAlignment(), verse->GetAlignment(), overlap));
        }
    }

    m_previousVerse = verse;
    m_simultaneousVerses.clear();
    m_lastSyl = lastSyl;
    m_freeSpace = nextFreeSpace;
    m_previousMeasure = NULL;
}

//----------------------------------------------------------------------------
// RecordSylSpacingFunctor (MuseNote RC92)
//----------------------------------------------------------------------------

FunctorCode RecordSylSpacingFunctor::VisitMeasureEnd(Measure *measure)
{
    m_measureEnds.emplace_back(m_place++, measure);
    return FUNCTOR_CONTINUE;
}

FunctorCode RecordSylSpacingFunctor::VisitStaff(Staff *staff)
{
    m_staffN = staff->GetN();
    m_staves[m_staffN].emplace_back(m_place++, staff);
    return FUNCTOR_CONTINUE;
}

FunctorCode RecordSylSpacingFunctor::VisitSystem(System *system)
{
    m_systems.emplace_back(m_place++, system);
    return FUNCTOR_CONTINUE;
}

FunctorCode RecordSylSpacingFunctor::VisitSystemEnd(System *system)
{
    m_systemEnds.emplace_back(m_place++, system);
    return FUNCTOR_CONTINUE;
}

FunctorCode RecordSylSpacingFunctor::VisitVerse(Verse *verse)
{
    m_verses[{ m_staffN, verse->GetPlace() == STAFFREL_above, verse->GetN() }].emplace_back(m_place++, verse);
    return FUNCTOR_CONTINUE;
}

std::vector<RecordSylSpacingFunctor::Line> RecordSylSpacingFunctor::GetLines() const
{
    std::vector<Line> lines;
    for (const auto &[line, met] : m_verses) {
        lines.push_back(line);
    }
    return lines;
}

void RecordSylSpacingFunctor::Replay(const Line &line, AdjustSylSpacingFunctor &functor) const
{
    static const Met<Staff> noStaves;
    static const Met<Verse> noVerses;
    const auto staves = m_staves.find(std::get<0>(line));
    const auto verses = m_verses.find(line);
    const Met<Staff> &staffMet = (staves != m_staves.end()) ? staves->second : noStaves;
    const Met<Verse> &verseMet = (verses != m_verses.end()) ? verses->second : noVerses;
    // The five lists, each in the order of the walk, merged by their places in it
    size_t system = 0, measureEnd = 0, systemEnd = 0, staff = 0, verse = 0;
    const auto placeOf = [](const auto &met, size_t i) { return (i < met.size()) ? met[i].first : INT_MAX; };
    while (true) {
        const int next = std::min({ placeOf(m_systems, system), placeOf(m_measureEnds, measureEnd),
            placeOf(m_systemEnds, systemEnd), placeOf(staffMet, staff), placeOf(verseMet, verse) });
        if (next == INT_MAX) break;
        if (next == placeOf(m_systems, system)) {
            functor.VisitSystem(m_systems[system++].second);
        }
        else if (next == placeOf(m_measureEnds, measureEnd)) {
            functor.VisitMeasureEnd(m_measureEnds[measureEnd++].second);
        }
        else if (next == placeOf(m_systemEnds, systemEnd)) {
            functor.VisitSystemEnd(m_systemEnds[systemEnd++].second);
        }
        else if (next == placeOf(staffMet, staff)) {
            functor.VisitStaff(staffMet[staff++].second);
        }
        else {
            functor.VisitVerse(verseMet[verse++].second);
        }
    }
}

} // namespace vrv
