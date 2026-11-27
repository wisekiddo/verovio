/////////////////////////////////////////////////////////////////////////////
// Name:        castofffunctor.cpp
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "castofffunctor.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

//----------------------------------------------------------------------------

#include "clef.h"
#include "div.h"
#include "doc.h"
#include "editorial.h"
#include "ending.h"
#include "keysig.h"
#include "metersig.h"
#include "page.h"
#include "pageelement.h"
#include "pagemilestone.h"
#include "pages.h"
#include "pb.h"
#include "sb.h"
#include "score.h"
#include "setscoredeffunctor.h"
#include "smufl.h"
#include "staff.h"
#include "staffdef.h"
#include "system.h"

//----------------------------------------------------------------------------

namespace vrv {

namespace {

// As Staff::IsTablature and Staff::GetDrawingStaffNotationSize, from the staff's notation type
bool IsTablatureType(data_NOTATIONTYPE type)
{
    return (type == NOTATIONTYPE_tab) || (type == NOTATIONTYPE_tab_guitar)
        || (type == NOTATIONTYPE_tab_lute_italian) || (type == NOTATIONTYPE_tab_lute_french)
        || (type == NOTATIONTYPE_tab_lute_german);
}

int StaffNotationSize(const StaffDef *staffDef, int staffSize)
{
    if (staffDef->GetNotationtype() == NOTATIONTYPE_tab_lute_german) {
        return std::round(staffSize / GERMAN_TAB_STAFF_RATIO);
    }
    return IsTablatureType(staffDef->GetNotationtype()) ? std::round(staffSize / TABLATURE_STAFF_RATIO) : staffSize;
}

} // namespace

//----------------------------------------------------------------------------
// CastOffSystemsFunctor
//----------------------------------------------------------------------------

FunctorCode CastOffSystemsFunctor::VisitDiv(Div *div)
{
    // If we have a previous a Measure or a Div in the System, add a new one
    if ((m_currentSystem->GetChildCount(MEASURE) > 0) || (m_currentSystem->GetChildCount(DIV)) > 0) {
        m_currentSystem = new System();
        m_page->AddChild(m_currentSystem);
    }

    div->MoveItselfTo(m_currentSystem);

    // Always add a System after an Div
    m_currentSystem = new System();
    m_page->AddChild(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

CastOffSystemsFunctor::CastOffSystemsFunctor(Page *page, Doc *doc, bool smart) : DocFunctor(doc)
{
    m_page = page;
    m_contentSystem = NULL;
    m_currentSystem = NULL;
    m_leftoverSystem = NULL;
    m_shift = 0;
    m_systemWidth = 0;
    m_currentScoreDefWidth = 0;
    m_heldLineStartWidth = 0;
    m_heldCautionaryWidth = 0;
    m_smart = smart;
}

FunctorCode CastOffSystemsFunctor::VisitEditorialElement(EditorialElement *editorialElement)
{
    // Since the functor returns FUNCTOR_SIBLINGS we should never go lower than the system children
    assert(dynamic_cast<System *>(editorialElement->GetParent()));

    // Special case where we use the Relinquish method.
    // We want to move the editorial element to the currentSystem. However, we cannot use DetachChild
    // from the contentSystem because this screws up the iterator. Relinquish gives up
    // the ownership of the element - the contentSystem will be deleted afterwards.
    editorialElement = vrv_cast<EditorialElement *>(m_contentSystem->Relinquish(editorialElement->GetIdx()));
    assert(editorialElement);
    // move as pending since we want it at the beginning of the system in case of system break coming
    m_pendingElements.push_back(editorialElement);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitEnding(Ending *ending)
{
    // Since the functor returns FUNCTOR_SIBLINGS we should never go lower than the system children
    assert(dynamic_cast<System *>(ending->GetParent()));

    // Special case where we use the Relinquish method.
    // We want to move the ending to the currentSystem. However, we cannot use DetachChild
    // from the contentSystem because this screws up the iterator. Relinquish gives up
    // the ownership of the ending - the contentSystem will be deleted afterwards.
    ending = dynamic_cast<Ending *>(m_contentSystem->Relinquish(ending->GetIdx()));
    // move as pending since we want it at the beginning of the system in case of system break coming
    m_pendingElements.push_back(ending);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitMeasure(Measure *measure)
{
    const bool hasCache = measure->HasCachedHorizontalLayout();
    int overflow = hasCache ? measure->GetCachedOverflow() : measure->GetDrawingOverflow();
    int width = hasCache ? measure->GetCachedWidth() : measure->GetWidth();
    int drawingXRel = measure->GetDrawingXRel();
    // How far right the measure's harms reach on the one-line layout, for each line it is counted in (MuseNote RC231)
    const int harmReach = hasCache ? measure->GetCachedHarmReach() : measure->GetDrawingHarmReach();
    m_harmRights[measure] = (harmReach == VRV_UNSET) ? VRV_UNSET : drawingXRel + harmReach;

    Object *nextMeasure = m_contentSystem->GetNext(measure, MEASURE);
    const bool isLeftoverMeasure = ((NULL == nextMeasure) && m_doc->GetOptions()->m_breaksNoWidow.GetValue()
        && (m_doc->GetOptions()->m_breaks.GetValue() != BREAKS_encoded));
    if (m_currentSystem->GetChildCount() > 0) {
        // A line starting with this measure, and the change a line ending before it ends with (MuseNote RC216)
        const int lineStartWidth = this->CalcLineStartWidth();
        const int cautionaryWidth = this->CalcChangeWidth(
            m_upcomingScoreDef, vrv_cast<const Measure *>(m_currentSystem->GetLast(MEASURE)));
        // We have overflowing content (dir, dynam, tempo) larger than 5 units, keep it as pending
        if (overflow > (m_doc->GetDrawingUnit(100) * 5)) {
            measure = dynamic_cast<Measure *>(m_contentSystem->Relinquish(measure->GetIdx()));
            assert(measure);
            // A line may start with the first measure held
            if (std::none_of(m_pendingElements.begin(), m_pendingElements.end(),
                    [](const Object *element) { return element->Is(MEASURE); })) {
                m_heldLineStartWidth = lineStartWidth;
                m_heldCautionaryWidth = cautionaryWidth;
            }
            // move as pending since we want it not to be broken with the next measure
            m_pendingElements.push_back(measure);
            this->ApplyMeasure(m_upcomingScoreDef, measure);
            // continue
            return FUNCTOR_SIBLINGS;
        }
        // Break it if necessary: the line ending with this measure before a change ends with that change too
        // (MuseNote RC216), and with the room past its last barline that its harms take, those of the measures held
        // with them (MuseNote RC231), and the line holds the room its first syllables take (MuseNote RC239)
        else if (drawingXRel + width + m_currentScoreDefWidth - m_shift + this->CalcCautionaryWidth(measure)
                + this->CalcHarmEndWidth(this->CalcHarmRight(m_currentSystem, true, measure), measure)
                + this->CalcSylRoom(m_currentSystem, true, measure)
            > m_systemWidth) {
            int firstHeld = -1;
            int lastHeld = -1;
            for (int i = 0; i < (int)m_pendingElements.size(); ++i) {
                if (!m_pendingElements.at(i)->Is(MEASURE)) continue;
                if (firstHeld < 0) firstHeld = i;
                lastHeld = i;
            }
            // The line closing here ends with the change before the next line's first measure, cautionary (MuseNote
            // RC216), and with the room past its last barline that its harms take (MuseNote RC231), and holds the room
            // its first syllables take (MuseNote RC239)
            m_currentSystem->m_castOffStartWidth += ((firstHeld >= 0) ? m_heldCautionaryWidth : cautionaryWidth)
                + this->CalcHarmEndWidth(this->CalcHarmRight(m_currentSystem, false, NULL),
                    vrv_cast<const Measure *>(m_currentSystem->GetLast(MEASURE)))
                + this->CalcSylRoom(m_currentSystem, false, NULL);
            m_currentSystem = new System();
            m_page->AddChild(m_currentSystem);
            m_shift = drawingXRel;
            // The start the new line draws (MuseNote RC216), recorded for the spacing estimate (MuseNote RC215)
            m_currentScoreDefWidth = (firstHeld >= 0) ? m_heldLineStartWidth : lineStartWidth;
            m_currentSystem->m_castOffStartWidth = m_currentScoreDefWidth;
            // If last measure requires separate system - mark that system as leftover for the future CastOffPages call
            if (isLeftoverMeasure) {
                m_leftoverSystem = m_currentSystem;
            }
            // The line begins with the first measure held
            if (firstHeld >= 0) {
                Measure *firstPendingMeasure = vrv_cast<Measure *>(m_pendingElements.at(firstHeld));
                assert(firstPendingMeasure);
                m_shift = firstPendingMeasure->GetCachedXRel();
                m_leftoverSystem = NULL;
            }
            // The measures held and this one must fit the new line together (MuseNote RC214): if they do not, the
            // held measures take it alone, and this one begins the next
            if ((lastHeld >= 0)
                && (drawingXRel + width + m_currentScoreDefWidth - m_shift + this->CalcCautionaryWidth(measure)
                        + this->CalcHarmEndWidth(this->CalcHarmRight(NULL, true, measure), measure)
                        + this->CalcSylRoom(NULL, true, measure)
                    > m_systemWidth)) {
                for (int i = 0; i <= lastHeld; ++i) {
                    m_currentSystem->AddChild(m_pendingElements.at(i));
                }
                // The held line ends with what overhangs its last measure, which the layout makes room for
                // (AdjustXOverflowFunctor), with the change before this measure, cautionary, and with the room past
                // its last barline that its harms take (MuseNote RC231): all counted with the line's widths for the
                // spacing estimate
                Measure *lastHeldMeasure = vrv_cast<Measure *>(m_currentSystem->GetLast(MEASURE));
                assert(lastHeldMeasure);
                m_currentSystem->m_castOffStartWidth += this->CalcChangeWidth(m_upcomingScoreDef, lastHeldMeasure)
                    + (lastHeldMeasure->HasCachedHorizontalLayout() ? lastHeldMeasure->GetCachedOverflow()
                                                                    : lastHeldMeasure->GetDrawingOverflow())
                    + this->CalcHarmEndWidth(this->CalcHarmRight(m_currentSystem, false, NULL), lastHeldMeasure)
                    + this->CalcSylRoom(m_currentSystem, false, NULL);
                m_pendingElements.erase(m_pendingElements.begin(), m_pendingElements.begin() + lastHeld + 1);
                m_currentSystem = new System();
                m_page->AddChild(m_currentSystem);
                m_shift = drawingXRel;
                m_currentScoreDefWidth = lineStartWidth;
                m_currentSystem->m_castOffStartWidth = m_currentScoreDefWidth;
                if (isLeftoverMeasure) {
                    m_leftoverSystem = m_currentSystem;
                }
            }
        }
    }

    // First add all pending objects
    for (Object *pendingElement : m_pendingElements) {
        m_currentSystem->AddChild(pendingElement);
    }
    m_pendingElements.clear();

    // Special case where we use the Relinquish method.
    measure = dynamic_cast<Measure *>(m_contentSystem->Relinquish(measure->GetIdx()));
    assert(measure);
    m_currentSystem->AddChild(measure);
    this->ApplyMeasure(m_upcomingScoreDef, measure);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitPageElement(PageElement *pageElement)
{
    assert(m_page);
    pageElement->MoveItselfTo(m_page);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd)
{
    assert(m_page);
    pageMilestoneEnd->MoveItselfTo(m_page);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitSb(Sb *sb)
{
    if (m_smart) {
        // Get the last measure of the currentSystem
        Measure *measure = dynamic_cast<Measure *>(m_currentSystem->GetChild(m_currentSystem->GetChildCount() - 1));
        if (measure != NULL) {
            int measureRightX = measure->GetDrawingX() + measure->GetWidth() - m_shift;
            double smartSbThresh = m_doc->GetOptions()->m_breaksSmartSb.GetValue();
            if (measureRightX > m_systemWidth * smartSbThresh) {
                // Use this system break.
                m_currentSystem = new System();
                m_page->AddChild(m_currentSystem);
                m_shift += measureRightX;
            }
        }
    }
    // Keep the <sb> in the internal MEI, even if we're not using it to break the system.
    sb->MoveItselfTo(m_currentSystem);
    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitScoreDef(ScoreDef *scoreDef)
{
    // Since the functor returns FUNCTOR_SIBLINGS we should never go lower than the system children
    assert(dynamic_cast<System *>(scoreDef->GetParent()));

    // Special case where we use the Relinquish method.
    // We want to move the scoreDef to the currentSystem. However, we cannot use DetachChild
    // from the contentSystem because this screws up the iterator. Relinquish gives up
    // the ownership of the scoreDef - the contentSystem will be deleted afterwards.
    scoreDef = dynamic_cast<ScoreDef *>(m_contentSystem->Relinquish(scoreDef->GetIdx()));
    // move as pending since we want it at the beginning of the system in case of system break coming
    m_pendingElements.push_back(scoreDef);
    // The scoreDef a line starting after this one draws, kept apart from the start of the current line (MuseNote
    // RC216: the current line was counted with this intermediate scoreDef's own width), and set as
    // ScoreDefSetCurrentFunctor sets it: its draw flags are the change the next measure draws
    if (scoreDef->HasClefInfo(UNLIMITED_DEPTH) || scoreDef->HasKeySigInfo(UNLIMITED_DEPTH)
        || scoreDef->HasMensurInfo(UNLIMITED_DEPTH) || scoreDef->HasMeterSigGrpInfo(UNLIMITED_DEPTH)
        || scoreDef->HasMeterSigInfo(UNLIMITED_DEPTH)) {
        m_upcomingScoreDef.ReplaceDrawingValues(scoreDef);
    }
    ListOfObjects staffDefs = scoreDef->FindAllDescendantsByType(STAFFDEF);
    for (Object *object : staffDefs) {
        m_upcomingScoreDef.ReplaceDrawingValues(vrv_cast<StaffDef *>(object));
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitSystem(System *system)
{
    // We are starting a new system we need to cast off
    m_contentSystem = system;
    // We also need to create a new target system and add it to the page
    System *targetSystem = new System();
    m_page->AddChild(targetSystem);
    m_currentSystem = targetSystem;

    m_shift = -system->GetDrawingLabelsWidth();
    // The first line's start (clefs, keys, meter) is laid out in its first measure, whose width holds it; a later
    // line's is counted when it begins, from the scoreDef in effect there (MuseNote RC216)
    m_currentScoreDefWidth = 0;
    const Page *contentPage = vrv_cast<const Page *>(system->GetFirstAncestor(PAGE));
    assert(contentPage);
    if (contentPage) m_upcomingScoreDef = contentPage->m_drawingScoreDef;
    // No change is pending before the first measure
    m_upcomingScoreDef.SetRedrawFlags(StaffDefRedrawFlags::FORCE_REDRAW);
    m_heldLineStartWidth = 0;
    m_heldCautionaryWidth = 0;
    // What the first line is counted with beyond its measures: its labels (MuseNote RC215)
    targetSystem->m_castOffStartWidth = system->GetDrawingLabelsWidth();

    return FUNCTOR_CONTINUE;
}

int CastOffSystemsFunctor::CalcScoreDefEnd(const std::vector<StaffScoreDefExtents> &staves) const
{
    // As AdjustXPosFunctor::VisitMeasure runs, a staff after another: each alignment first takes the shift the staff's
    // elements before it made; an element drawn moves it, and every one after it, so that its left margin clears what
    // the staff drew before, and leaves its right margin clear after it; one drawn empty clears that with no margin
    // and takes no room; the alignment after the run, drawn empty, is moved clear of it all
    std::array<int, 3> x = { 0, 0, 0 };
    int end = 0;
    for (const StaffScoreDefExtents &staff : staves) {
        int shift = 0;
        int minPos = staff.minPos;
        for (size_t i = 0; i < x.size(); ++i) {
            x.at(i) += shift;
            const ScoreDefExtent &element = staff.elements.at(i);
            if (!element.present) continue;
            const int left = element.drawn ? (x.at(i) + element.left - element.leftMargin) : x.at(i);
            if (left < minPos) {
                shift += minPos - left;
                x.at(i) += minPos - left;
            }
            minPos = element.drawn ? (x.at(i) + element.right + element.rightMargin) : x.at(i);
        }
        end = std::max(end + shift, minPos);
    }
    return end;
}

int CastOffSystemsFunctor::CalcLineStartWidth() const
{
    // What the line's start draws, and what the first measure's stored width holds already: the change right before
    // it, as drawn within the line, where the measure is not the first of its system
    std::vector<StaffScoreDefExtents> start;
    std::vector<StaffScoreDefExtents> change;
    const bool hasSystemStartLine = m_upcomingScoreDef.HasSystemStartLine();
    const ListOfConstObjects staffDefs = m_upcomingScoreDef.FindAllDescendantsByType(STAFFDEF);
    for (const Object *object : staffDefs) {
        const StaffDef *staffDef = vrv_cast<const StaffDef *>(object);
        assert(staffDef);
        if (staffDef->GetDrawingVisibility() == OPTIMIZATION_HIDDEN) continue;
        StaffScoreDefExtents staffStart = this->CalcStaffScoreDefExtents(staffDef, true);
        // The room a line's first measure keeps clear of a system start line (AdjustXPosFunctor::VisitMeasure), which
        // the one-line layout keeps on its first measure only
        if (hasSystemStartLine) {
            const int staffSize = staffDef->HasScale() ? staffDef->GetScale() : 100;
            staffStart.minPos = m_doc->GetDrawingBarLineWidth(staffSize);
        }
        start.push_back(staffStart);
        change.push_back(this->CalcStaffScoreDefExtents(staffDef, false));
    }
    return this->CalcScoreDefEnd(start) - this->CalcScoreDefEnd(change)
        + m_contentSystem->GetDrawingAbbrLabelsWidth();
}

int CastOffSystemsFunctor::CalcChangeWidth(const ScoreDef &scoreDef, const Measure *measure) const
{
    // A staff's cautionary elements start where its right barline ends: the barline's right and its right margin,
    // from the barline's alignment, where they all start (AdjustXPosFunctor::VisitLayerElement); without them the
    // measure ends there
    std::vector<StaffScoreDefExtents> staves;
    int barLineEnd = 0;
    bool hasChange = false;
    const ListOfConstObjects staffDefs = scoreDef.FindAllDescendantsByType(STAFFDEF);
    for (const Object *object : staffDefs) {
        const StaffDef *staffDef = vrv_cast<const StaffDef *>(object);
        assert(staffDef);
        if (staffDef->GetDrawingVisibility() == OPTIMIZATION_HIDDEN) continue;
        StaffScoreDefExtents extents = this->CalcStaffScoreDefExtents(staffDef, false);
        for (const ScoreDefExtent &element : extents.elements) {
            hasChange = hasChange || element.present;
        }
        if (measure) {
            const BarLine *barLine = measure->GetRightBarLine();
            const int staffSize = staffDef->HasScale() ? staffDef->GetScale() : 100;
            const int right = (barLine->HasSelfBB() && !barLine->HasEmptyBB()) ? barLine->GetSelfX2() : 0;
            extents.minPos = right + m_doc->GetRightMargin(barLine) * m_doc->GetDrawingUnit(staffSize);
        }
        barLineEnd = std::max(barLineEnd, extents.minPos);
        staves.push_back(extents);
    }
    return hasChange ? (this->CalcScoreDefEnd(staves) - barLineEnd) : 0;
}

int CastOffSystemsFunctor::CalcCautionaryWidth(Measure *measure) const
{
    // The scoreDef in effect after the measure and the changes between it and the next measure (an ending's end or
    // another milestone may stand among them), its draw flags theirs
    ScoreDef changed = m_upcomingScoreDef;
    bool hasChange = false;
    for (const Object *next = m_contentSystem->GetNext(measure); next && !next->Is(MEASURE);
         next = m_contentSystem->GetNext(next)) {
        if (!next->Is(SCOREDEF)) continue;
        if (!hasChange) {
            this->ApplyMeasure(changed, measure);
            hasChange = true;
        }
        const ScoreDef *scoreDef = vrv_cast<const ScoreDef *>(next);
        if (scoreDef->HasClefInfo(UNLIMITED_DEPTH) || scoreDef->HasKeySigInfo(UNLIMITED_DEPTH)
            || scoreDef->HasMensurInfo(UNLIMITED_DEPTH) || scoreDef->HasMeterSigGrpInfo(UNLIMITED_DEPTH)
            || scoreDef->HasMeterSigInfo(UNLIMITED_DEPTH)) {
            changed.ReplaceDrawingValues(scoreDef);
        }
        const ListOfConstObjects changedStaffDefs = scoreDef->FindAllDescendantsByType(STAFFDEF);
        for (const Object *object : changedStaffDefs) {
            changed.ReplaceDrawingValues(vrv_cast<const StaffDef *>(object));
        }
    }
    return hasChange ? this->CalcChangeWidth(changed, measure) : 0;
}

int CastOffSystemsFunctor::CalcHarmRight(const System *system, bool held, const Measure *measure) const
{
    int right = VRV_UNSET;
    const auto take = [this, &right](const Object *object) {
        if (!object->Is(MEASURE)) return;
        const auto found = m_harmRights.find(vrv_cast<const Measure *>(object));
        if (found != m_harmRights.end()) right = std::max(right, found->second);
    };
    if (system) {
        for (const Object *child : system->GetChildren()) take(child);
    }
    if (held) {
        for (const Object *element : m_pendingElements) take(element);
    }
    if (measure) take(measure);
    return right;
}

int CastOffSystemsFunctor::CalcHarmEndWidth(int harmRight, const Measure *measure) const
{
    // As AdjustHarmGrpsSpacingFunctor::VisitSystemEnd: the furthest a harm reaches, against the line's last barline,
    // where the one-line layout put it (the cache restores no alignment)
    if ((harmRight == VRV_UNSET) || !measure) return 0;
    const int rightBarLineXRel = measure->HasCachedHorizontalLayout() ? measure->GetCachedRightBarLineXRel()
                                                                       : measure->GetRightBarLineXRel();
    return std::max(0, harmRight - (measure->GetDrawingXRel() + rightBarLineXRel));
}

int CastOffSystemsFunctor::CalcSylRoom(const System *system, bool held, const Measure *measure) const
{
    // As AdjustSylSpacingFunctor lays the line out: each line of verses' first verse on the line keeps its place,
    // where the one-line layout moved it back into the measures before it, and its measure takes that room
    std::set<std::tuple<int, bool, int>> lines;
    int room = 0;
    const auto take = [&lines, &room](const Object *object) {
        if (!object->Is(MEASURE)) return;
        const Measure *current = vrv_cast<const Measure *>(object);
        const std::map<std::tuple<int, bool, int>, int> &moveBacks = current->HasCachedHorizontalLayout()
            ? current->GetCachedSylMoveBacks()
            : current->GetDrawingSylMoveBacks();
        for (const auto &[line, moveBack] : moveBacks) {
            // The first verse on the line in this line of verses: those after it keep the room before them
            if (lines.insert(line).second) room = std::max(room, moveBack);
        }
    };
    if (system) {
        for (const Object *child : system->GetChildren()) take(child);
    }
    if (held) {
        for (const Object *element : m_pendingElements) take(element);
    }
    if (measure) take(measure);
    return room;
}

CastOffSystemsFunctor::StaffScoreDefExtents CastOffSystemsFunctor::CalcStaffScoreDefExtents(
    const StaffDef *staffDef, bool lineStart) const
{
    assert(staffDef);
    // At a system's start every staff draws its clef and key (ScoreDef::SetRedrawFlags), a meter where its draw flags
    // set one, as within a line
    StaffScoreDefExtents extents;
    if (lineStart || staffDef->DrawClef()) extents.elements.at(0) = this->CalcClefExtent(staffDef);
    if (lineStart || staffDef->DrawKeySig()) extents.elements.at(1) = this->CalcKeyExtent(staffDef, lineStart);
    if (staffDef->DrawMeterSig()) extents.elements.at(2) = this->CalcMeterExtent(staffDef);
    return extents;
}

CastOffSystemsFunctor::ScoreDefExtent CastOffSystemsFunctor::CalcClefExtent(const StaffDef *staffDef) const
{
    assert(staffDef);
    ScoreDefExtent extent;
    extent.present = true;
    const Clef *clef = staffDef->GetCurrentClef();
    if (!clef || (clef->GetVisible() == BOOLEAN_false)) return extent;
    const bool tablature = IsTablatureType(staffDef->GetNotationtype());
    // View::DrawClef draws no clef without a line to stand on, but a percussion clef
    if (!tablature && !clef->HasLine() && (clef->GetShape() != CLEFSHAPE_perc)) return extent;
    const int staffSize = staffDef->HasScale() ? staffDef->GetScale() : 100;
    // The staffDef's clef is in no document: its glyph is looked up in the document's resources
    const char32_t glyph = clef->GetClefGlyph(staffDef->GetNotationtype(), &m_doc->GetResources());
    if (glyph == 0) return extent;
    this->AddGlyphsExtent(
        std::u32string(1, glyph), 0, tablature ? StaffNotationSize(staffDef, staffSize) : staffSize, extent);
    const int unit = m_doc->GetDrawingUnit(staffSize);
    extent.leftMargin = m_doc->GetLeftMargin(CLEF) * unit;
    extent.rightMargin = m_doc->GetRightMargin(CLEF) * unit;
    return extent;
}

CastOffSystemsFunctor::ScoreDefExtent CastOffSystemsFunctor::CalcKeyExtent(
    const StaffDef *staffDef, bool lineStart) const
{
    assert(staffDef);
    ScoreDefExtent extent;
    extent.present = true;
    // View::DrawKeySig draws no key on tablature
    if (IsTablatureType(staffDef->GetNotationtype())) return extent;
    const KeySig *keySig = staffDef->GetCurrentKeySig();
    if (!keySig || (keySig->GetVisible() == BOOLEAN_false)) return extent;
    const int count = keySig->GetAccidCount();
    // The naturals cancelling the key before, where View::DrawKeySig draws them: within a line or at its end, for a
    // key with @cancelaccid or a key of none
    int naturals = 0;
    bool naturalsAfter = false;
    if (!lineStart && !keySig->m_skipCancellation
        && ((keySig->HasCancelaccid() && (keySig->GetCancelaccid() != CANCELACCID_none)) || (count == 0))) {
        const bool sameType = (keySig->GetAccidType() == keySig->m_drawingCancelAccidType);
        naturalsAfter = sameType && (keySig->GetCancelaccid() == CANCELACCID_after);
        naturals = std::max(0, keySig->m_drawingCancelAccidCount - (sameType ? count : 0));
    }
    if ((count <= 0) && (naturals == 0)) return extent;
    const int staffSize = staffDef->HasScale() ? staffDef->GetScale() : 100;
    const int unit = m_doc->GetDrawingUnit(staffSize);
    // As View::DrawKeySig steps: a natural by its width and a step of its own (View::DrawKeySigCancellation), an
    // accidental by its advance and a step (View::DrawKeyAccid)
    const std::u32string natural(1, SMUFL_E261_accidentalNatural);
    const int naturalStep
        = m_doc->GetGlyphWidth(SMUFL_E261_accidentalNatural, staffSize, false) + unit * TEMP_KEYSIG_NATURAL_STEP;
    const std::u32string accid(1,
        (keySig->GetAccidType() == ACCIDENTAL_WRITTEN_f) ? SMUFL_E260_accidentalFlat : SMUFL_E262_accidentalSharp);
    const int accidStep = this->CalcGlyphsAdvance(accid, staffSize) + unit * TEMP_KEYSIG_STEP;
    int x = 0;
    for (int i = 0; !naturalsAfter && (i < naturals); ++i) {
        this->AddGlyphsExtent(natural, x, staffSize, extent);
        x += naturalStep;
    }
    for (int i = 0; i < count; ++i) {
        this->AddGlyphsExtent(accid, x, staffSize, extent);
        x += accidStep;
    }
    for (int i = 0; naturalsAfter && (i < naturals); ++i) {
        this->AddGlyphsExtent(natural, x, staffSize, extent);
        x += naturalStep;
    }
    extent.leftMargin = m_doc->GetLeftMargin(KEYSIG) * unit;
    extent.rightMargin = m_doc->GetRightMargin(KEYSIG) * unit;
    return extent;
}

CastOffSystemsFunctor::ScoreDefExtent CastOffSystemsFunctor::CalcMeterExtent(const StaffDef *staffDef) const
{
    assert(staffDef);
    ScoreDefExtent extent;
    extent.present = true;
    const MeterSig *meterSig = staffDef->GetCurrentMeterSig();
    if (!meterSig || (meterSig->GetVisible() == BOOLEAN_false)) return extent;
    const int staffSize = staffDef->HasScale() ? staffDef->GetScale() : 100;
    const int glyphSize = StaffNotationSize(staffDef, staffSize);
    // As View::DrawMeterSig draws it: an enclosing glyph and a symbol each stepped by its width; or its lines of
    // figures (View::DrawMeterSigFigures), the summands joined by their sign, each centred on the advance of the line
    // with the more glyphs
    const auto figures = [](int number) {
        std::u32string glyphs;
        for (const char digit : std::to_string(number)) {
            glyphs += (char32_t)(SMUFL_E080_timeSig0 + (digit - '0'));
        }
        return glyphs;
    };
    const bool smallEnclosing = (meterSig->HasSym() || (meterSig->GetForm() == METERFORM_num));
    const auto [front, back] = meterSig->GetEnclosingGlyphs(smallEnclosing);
    int x = 0;
    if (front) {
        this->AddGlyphsExtent(std::u32string(1, front), x, glyphSize, extent);
        x += m_doc->GetGlyphWidth(front, glyphSize, false);
    }
    if (meterSig->HasSym() || meterSig->HasGlyphNum() || meterSig->HasGlyphName()) {
        const char32_t code = meterSig->GetSymbolGlyph(&m_doc->GetResources());
        this->AddGlyphsExtent(std::u32string(1, code), x, glyphSize, extent);
        x += m_doc->GetGlyphWidth(code, glyphSize, false);
    }
    else if (meterSig->HasCount()) {
        const auto [summands, sign] = meterSig->GetCount();
        std::u32string numerator;
        for (const int summand : summands) {
            if (!numerator.empty()) {
                switch (sign) {
                    case MeterCountSign::Slash: numerator += SMUFL_E08E_timeSigFractionalSlash; break;
                    case MeterCountSign::Minus: numerator += SMUFL_E090_timeSigMinus; break;
                    case MeterCountSign::Asterisk: numerator += SMUFL_E091_timeSigMultiply; break;
                    case MeterCountSign::Plus: numerator += SMUFL_E08D_timeSigPlusSmall; break;
                    case MeterCountSign::None:
                    default: break;
                }
            }
            numerator += figures(summand);
        }
        const std::u32string denominator
            = ((meterSig->GetForm() == METERFORM_num) || !meterSig->HasUnit()) ? U"" : figures(meterSig->GetUnit());
        const int width = this->CalcGlyphsAdvance(
            (numerator.length() > denominator.length()) ? numerator : denominator, glyphSize);
        const int centre = x + width / 2;
        this->AddGlyphsExtent(numerator, centre - this->CalcGlyphsAdvance(numerator, glyphSize) / 2, glyphSize, extent);
        if (!denominator.empty()) {
            this->AddGlyphsExtent(
                denominator, centre - this->CalcGlyphsAdvance(denominator, glyphSize) / 2, glyphSize, extent);
        }
        x += width;
    }
    if (back) {
        this->AddGlyphsExtent(std::u32string(1, back), x, glyphSize, extent);
    }
    const int unit = m_doc->GetDrawingUnit(staffSize);
    extent.leftMargin = m_doc->GetLeftMargin(METERSIG) * unit;
    extent.rightMargin = m_doc->GetRightMargin(METERSIG) * unit;
    return extent;
}

void CastOffSystemsFunctor::AddGlyphsExtent(
    const std::u32string &glyphs, int x, int staffSize, ScoreDefExtent &extent) const
{
    const int pointSize = m_doc->GetDrawingSmuflFont(staffSize, false)->GetPointSize();
    for (const char32_t code : glyphs) {
        const Glyph *glyph = m_doc->GetResources().GetGlyph(code);
        if (!glyph || (glyph->GetUnitsPerEm() <= 0)) continue;
        int glyphX, glyphY, glyphWidth, glyphHeight;
        glyph->GetBoundingBox(glyphX, glyphY, glyphWidth, glyphHeight);
        const int left = x + glyphX * pointSize / glyph->GetUnitsPerEm();
        const int right = left + glyphWidth * pointSize / glyph->GetUnitsPerEm();
        extent.left = extent.drawn ? std::min(extent.left, left) : left;
        extent.right = extent.drawn ? std::max(extent.right, right) : right;
        extent.drawn = true;
        x += glyph->GetHorizAdvX() * pointSize / glyph->GetUnitsPerEm();
    }
}

int CastOffSystemsFunctor::CalcGlyphAdvance(char32_t code, int staffSize) const
{
    const Glyph *glyph = m_doc->GetResources().GetGlyph(code);
    if (!glyph || (glyph->GetUnitsPerEm() <= 0)) return 0;
    const double pointSize = m_doc->GetDrawingSmuflFont(staffSize, false)->GetPointSize();
    const int advance = std::ceil(glyph->GetHorizAdvX() * pointSize / glyph->GetUnitsPerEm());
    if (advance > 0) return advance;
    int x, y, width, height;
    glyph->GetBoundingBox(x, y, width, height);
    return std::ceil(width * pointSize / glyph->GetUnitsPerEm());
}

int CastOffSystemsFunctor::CalcGlyphsAdvance(const std::u32string &glyphs, int staffSize) const
{
    int advance = 0;
    for (const char32_t code : glyphs) {
        advance += this->CalcGlyphAdvance(code, staffSize);
    }
    return advance;
}

void CastOffSystemsFunctor::ApplyMeasure(ScoreDef &scoreDef, Measure *measure) const
{
    // As ScoreDefSetCurrentFunctor::VisitClef and VisitKeySig set them
    ListOfObjects clefs = measure->FindAllDescendantsByType(CLEF);
    for (Object *object : clefs) {
        LayerElement *elementOrLink = vrv_cast<LayerElement *>(object)->ThisOrSameasLink();
        if (!elementOrLink || !elementOrLink->Is(CLEF)) continue;
        Clef *clef = vrv_cast<Clef *>(elementOrLink);
        if (clef->IsScoreDefElement()) continue;
        const Staff *staff = vrv_cast<const Staff *>(object->GetFirstAncestor(STAFF));
        if (!staff) continue;
        const int n = clef->m_crossStaff ? clef->m_crossStaff->GetN() : staff->GetN();
        StaffDef *staffDef = scoreDef.GetStaffDef(n);
        if (staffDef) staffDef->SetCurrentClef(clef);
    }
    ListOfObjects keySigs = measure->FindAllDescendantsByType(KEYSIG);
    for (Object *object : keySigs) {
        KeySig *keySig = vrv_cast<KeySig *>(object);
        if (keySig->IsScoreDefElement()) continue;
        const Staff *staff = vrv_cast<const Staff *>(object->GetFirstAncestor(STAFF));
        if (!staff) continue;
        StaffDef *staffDef = scoreDef.GetStaffDef(staff->GetN());
        if (staffDef) staffDef->SetCurrentKeySig(keySig);
    }
    // The change before the measure is drawn there, as ScoreDefSetCurrentFunctor::VisitMeasure clears it
    scoreDef.SetRedrawFlags(StaffDefRedrawFlags::FORCE_REDRAW);
}

FunctorCode CastOffSystemsFunctor::VisitSystemEnd(System *system)
{
    // The last line ends here: the room its first syllables take, with the measures still held, which go to it, is
    // counted with its widths for the spacing estimate (MuseNote RC239)
    m_currentSystem->m_castOffStartWidth += this->CalcSylRoom(m_currentSystem, true, NULL);

    if (m_pendingElements.empty()) return FUNCTOR_CONTINUE;

    // Otherwise add all pending objects
    for (Object *pendingElement : m_pendingElements) {
        m_currentSystem->AddChild(pendingElement);
    }
    m_pendingElements.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode CastOffSystemsFunctor::VisitSystemElement(SystemElement *systemElement)
{
    // Since the functor returns FUNCTOR_SIBLINGS we should never go lower than the system children
    assert(dynamic_cast<System *>(systemElement->GetParent()));

    // Special case where we use the Relinquish method.
    systemElement = dynamic_cast<SystemElement *>(m_contentSystem->Relinquish(systemElement->GetIdx()));
    // move as pending since we want it at the beginning of the system in case of system break coming
    m_pendingElements.push_back(systemElement);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffSystemsFunctor::VisitSystemMilestone(SystemMilestoneEnd *systemMilestoneEnd)
{
    // Since the functor returns FUNCTOR_SIBLINGS we should never go lower than the system children
    assert(dynamic_cast<System *>(systemMilestoneEnd->GetParent()));

    // Special case where we use the Relinquish method.
    // We want to move the milestone to the currentSystem. However, we cannot use DetachChild
    // from the contentSystem because this screws up the iterator. Relinquish gives up
    // the ownership of the milestone - the contentSystem will be deleted afterwards.
    systemMilestoneEnd = dynamic_cast<SystemMilestoneEnd *>(m_contentSystem->Relinquish(systemMilestoneEnd->GetIdx()));
    // End milestones are not added to the pending objects because we do not want them to be placed at the beginning of
    // the next system but only if the pending object array is empty (otherwise it will mess up the MEI tree)
    if (m_pendingElements.empty()) {
        m_currentSystem->AddChild(systemMilestoneEnd);
    }
    else {
        m_pendingElements.push_back(systemMilestoneEnd);
    }

    return FUNCTOR_SIBLINGS;
}

//----------------------------------------------------------------------------
// CastOffPagesFunctor
//----------------------------------------------------------------------------

CastOffPagesFunctor::CastOffPagesFunctor(Page *contentPage, Doc *doc, Page *currentPage) : DocFunctor(doc)
{
    m_contentPage = contentPage;
    m_currentPage = currentPage;
    m_firstCastOffPage = true;
    m_shift = VRV_UNSET;
    m_pageHeight = 0;
    m_pgHeadHeight = 0;
    m_pgFootHeight = 0;
    m_pgHead2Height = 0;
    m_pgFoot2Height = 0;
    m_leftoverSystem = NULL;
}

FunctorCode CastOffPagesFunctor::VisitPageEnd(Page *page)
{
    if (m_pendingPageElements.empty()) return FUNCTOR_CONTINUE;

    // Otherwise add all pending objects
    for (Object *pendingElement : m_pendingPageElements) {
        m_currentPage->AddChild(pendingElement);
    }
    m_pendingPageElements.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode CastOffPagesFunctor::VisitPageElement(PageElement *pageElement)
{
    pageElement = dynamic_cast<PageElement *>(m_contentPage->Relinquish(pageElement->GetIdx()));
    assert(pageElement);
    // move as pending since we want it at the beginning of the page in case of system break coming
    m_pendingPageElements.push_back(pageElement);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffPagesFunctor::VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd)
{
    assert(m_currentPage);

    pageMilestoneEnd = dynamic_cast<PageMilestoneEnd *>(m_contentPage->Relinquish(pageMilestoneEnd->GetIdx()));
    // End milestones can be added to the page only if the pending list is empty
    // Otherwise we are going to mess up the order
    if (m_pendingPageElements.empty()) {
        m_currentPage->AddChild(pageMilestoneEnd);
    }
    else {
        m_pendingPageElements.push_back(pageMilestoneEnd);
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffPagesFunctor::VisitScore(Score *score)
{
    this->VisitPageElement(score);

    m_pgHeadHeight = score->m_drawingPgHeadHeight;
    m_pgFootHeight = score->m_drawingPgFootHeight;
    m_pgHead2Height = score->m_drawingPgHead2Height;
    m_pgFoot2Height = score->m_drawingPgFoot2Height;

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffPagesFunctor::VisitSystem(System *system)
{
    // Check if this is the first system
    if (m_shift == VRV_UNSET) {
        m_shift = system->GetDrawingYRel();
    }

    const int systemMaxPerPage = m_doc->GetOptions()->m_systemMaxPerPage.GetValue();
    const int systemChildCount = m_currentPage->GetChildCount(SYSTEM);
    if ((systemMaxPerPage && (systemMaxPerPage == systemChildCount))
        || ((systemChildCount > 0)
            && (m_shift - system->GetDrawingYRel() + system->GetHeight() > this->GetAvailableDrawingHeight()))) {
        // If this is the last system in the list, it doesn't fit the page and it's a leftover system (has just one
        // measure) => add the system content to the previous system
        Object *nextSystem = m_contentPage->GetNext(system, SYSTEM);
        Object *lastSystem = m_currentPage->GetLast(SYSTEM);
        if (!nextSystem && lastSystem && (system == m_leftoverSystem)) {
            ArrayOfObjects &children = system->GetChildrenForModification();
            for (Object *child : children) {
                child->MoveItselfTo(lastSystem);
            }
            return FUNCTOR_SIBLINGS;
        }

        m_currentPage = new Page();
        assert(m_doc->GetPages());
        m_doc->GetPages()->AddChild(m_currentPage);
        m_shift = system->GetDrawingYRel();
        m_firstCastOffPage = false;
    }

    // First add all pending objects
    for (Object *pendingElement : m_pendingPageElements) {
        m_currentPage->AddChild(pendingElement);
    }
    m_pendingPageElements.clear();

    // Special case where we use the Relinquish method.
    // We want to move the system to the currentPage. However, we cannot use DetachChild
    // from the contentPage because this screws up the iterator. Relinquish gives up
    // the ownership of the system - the contentPage itself will be deleted afterwards.
    system = vrv_cast<System *>(m_contentPage->Relinquish(system->GetIdx()));
    assert(system);
    m_currentPage->AddChild(system);

    return FUNCTOR_SIBLINGS;
}

int CastOffPagesFunctor::GetAvailableDrawingHeight() const
{
    const int pageHeadAndFootHeight
        = m_firstCastOffPage ? (m_pgHeadHeight + m_pgFootHeight) : (m_pgHead2Height + m_pgFoot2Height);
    return m_pageHeight - pageHeadAndFootHeight;
}

//----------------------------------------------------------------------------
// CastOffEncodingFunctor
//----------------------------------------------------------------------------

CastOffEncodingFunctor::CastOffEncodingFunctor(Doc *doc, Page *currentPage, bool usePages) : DocFunctor(doc)
{
    m_currentPage = currentPage;
    m_currentSystem = NULL;
    m_contentSystem = NULL;
    m_usePages = usePages;
}

FunctorCode CastOffEncodingFunctor::VisitDiv(Div *div)
{
    assert(m_currentSystem);
    div->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitEditorialElement(EditorialElement *editorialElement)
{
    // Only move editorial elements that are a child of the system
    if (editorialElement->GetParent() && editorialElement->GetParent()->Is(SYSTEM)) {
        assert(m_currentSystem);
        editorialElement->MoveItselfTo(m_currentSystem);
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitEnding(Ending *ending)
{
    assert(m_currentSystem);
    ending->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitMeasure(Measure *measure)
{
    assert(m_currentSystem);
    measure->MoveItselfTo(m_currentSystem);

    return FUNCTOR_CONTINUE;
}

FunctorCode CastOffEncodingFunctor::VisitPageElement(PageElement *pageElement)
{
    assert(m_currentPage);
    pageElement->MoveItselfTo(m_currentPage);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd)
{
    if (pageMilestoneEnd->GetStart() && pageMilestoneEnd->GetStart()->Is(SCORE)) {
        // This is the end of a score, which means that the current system has to be added
        // to the current page
        assert(m_currentSystem);
        m_currentPage->AddChild(m_currentSystem);
        m_currentSystem = NULL;
    }

    assert(m_currentPage);
    pageMilestoneEnd->MoveItselfTo(m_currentPage);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitPb(Pb *pb)
{
    // We look if the current system has a pb or at least one measure, or the current page has at least a system
    // if yes, we assume that the <pb> is not the one at the beginning of the content.
    // This is not very robust but at least make it work when rendering a <mdiv> that does not start with a <pb> (which
    // we cannot force)
    if ((m_currentSystem->GetChildCount(PB) > 0) || (m_currentSystem->GetChildCount(MEASURE) > 0)
        || (m_currentPage->GetChildCount(SYSTEM) > 0)) {
        m_currentPage->AddChild(m_currentSystem);
        m_currentSystem = new System();
        if (m_usePages) {
            m_currentPage = new Page();
            assert(m_doc->GetPages());
            m_doc->GetPages()->AddChild(m_currentPage);
        }
    }

    assert(m_currentSystem);
    pb->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitSb(Sb *sb)
{
    // We look if the current system has at least one measure - if yes, we assume that the <sb>
    // is not the one at the beginning of the content (<mdiv>). This is not very robust but at least make it
    // work when rendering a <mdiv> that does not start with a <pb> or a <sb> (which we cannot enforce)
    if (m_currentSystem->GetChildCount(MEASURE) > 0 || m_currentSystem->GetChildCount(DIV) > 0) {
        m_currentPage->AddChild(m_currentSystem);
        m_currentSystem = new System();
    }

    assert(m_currentSystem);
    sb->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitScoreDef(ScoreDef *scoreDef)
{
    assert(m_currentSystem);
    scoreDef->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitStaff(Staff *staff)
{
    // Staff alignments must be reset, otherwise they would dangle whenever they belong to a deleted system
    staff->SetAlignment(NULL);
    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffEncodingFunctor::VisitSystem(System *system)
{
    // We are starting a new system we need to cast off
    m_contentSystem = system;
    // Create the new system but do not add it to the page yet.
    // It will be added when reaching a pb / sb or at the end of the score in PageMilestoneEnd::CastOffEncoding
    assert(!m_currentSystem);
    m_currentSystem = new System();

    return FUNCTOR_CONTINUE;
}

FunctorCode CastOffEncodingFunctor::VisitSystemElement(SystemElement *systemElement)
{
    assert(m_currentSystem);
    systemElement->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

//----------------------------------------------------------------------------
// UnCastOffFunctor
//----------------------------------------------------------------------------

UnCastOffFunctor::UnCastOffFunctor(Page *page) : Functor()
{
    m_page = page;
    m_currentSystem = NULL;
    m_resetCache = true;
}

FunctorCode UnCastOffFunctor::VisitFloatingObject(FloatingObject *floatingObject)
{
    floatingObject->SetCurrentFloatingPositioner(NULL);

    return FUNCTOR_SIBLINGS;
}

FunctorCode UnCastOffFunctor::VisitMeasure(Measure *measure)
{
    if (m_resetCache) {
        measure->ResetCachedXRel();
        measure->ResetCachedWidth();
        measure->ResetCachedOverflow();
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode UnCastOffFunctor::VisitPageElement(PageElement *pageElement)
{
    pageElement->MoveItselfTo(m_page);

    return FUNCTOR_SIBLINGS;
}

FunctorCode UnCastOffFunctor::VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd)
{
    if (pageMilestoneEnd->GetStart() && pageMilestoneEnd->GetStart()->Is(SCORE)) {
        // This is the end of a score, which means that nothing else should be added to
        // the current system and we set it to NULL;
        assert(m_currentSystem);
        m_currentSystem = NULL;
    }

    pageMilestoneEnd->MoveItselfTo(m_page);

    return FUNCTOR_SIBLINGS;
}

FunctorCode UnCastOffFunctor::VisitScore(Score *score)
{
    this->VisitPageElement(score);

    assert(!m_currentSystem);
    System *system = new System();
    m_currentSystem = system;
    m_page->AddChild(system);

    return FUNCTOR_SIBLINGS;
}

FunctorCode UnCastOffFunctor::VisitSystem(System *system)
{
    // Just move all the content of the system to the continuous one
    // Use the MoveChildrenFrom method that moves and relinquishes them
    // See Object::Relinquish
    m_currentSystem->MoveChildrenFrom(system);

    return FUNCTOR_CONTINUE;
}

//----------------------------------------------------------------------------
// CastOffToSelectionFunctor
//----------------------------------------------------------------------------

CastOffToSelectionFunctor::CastOffToSelectionFunctor(
    Page *page, Doc *doc, const std::string &start, const std::string &end)
    : DocFunctor(doc)
{
    m_page = page;
    m_contentSystem = NULL;
    m_currentSystem = NULL;
    m_start = start;
    m_end = end;
    m_isSelection = false;
}

FunctorCode CastOffToSelectionFunctor::VisitDiv(Div *div)
{
    assert(m_currentSystem);
    div->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffToSelectionFunctor::VisitEditorialElement(EditorialElement *editorialElement)
{
    assert(m_currentSystem);
    editorialElement->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffToSelectionFunctor::VisitMeasure(Measure *measure)
{
    const bool startSelection = !m_isSelection && (measure->GetID() == m_start);

    if (startSelection) {
        m_page = new Page();
        m_doc->GetPages()->AddChild(m_page);
        m_currentSystem = new System();
        m_page->AddChild(m_currentSystem);
        m_isSelection = true;
    }

    const bool endSelection = m_isSelection && (measure->GetID() == m_end);

    assert(m_currentSystem);
    measure->MoveItselfTo(m_currentSystem);

    if (endSelection) {
        m_page = new Page();
        m_doc->GetPages()->AddChild(m_page);
        m_currentSystem = new System();
        m_page->AddChild(m_currentSystem);
        m_isSelection = false;
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffToSelectionFunctor::VisitPageElement(PageElement *pageElement)
{
    assert(m_page);
    pageElement->MoveItselfTo(m_page);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffToSelectionFunctor::VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd)
{
    assert(m_page);
    pageMilestoneEnd->MoveItselfTo(m_page);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffToSelectionFunctor::VisitScoreDef(ScoreDef *scoreDef)
{
    assert(m_currentSystem);
    scoreDef->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffToSelectionFunctor::VisitSystem(System *system)
{
    // We are starting a new system we need to cast off
    m_contentSystem = system;
    // We also need to create a new target system and add it to the page
    System *targetSystem = new System();
    m_page->AddChild(targetSystem);
    m_currentSystem = targetSystem;

    return FUNCTOR_CONTINUE;
}

FunctorCode CastOffToSelectionFunctor::VisitSystemElement(SystemElement *systemElement)
{
    assert(m_currentSystem);
    systemElement->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

FunctorCode CastOffToSelectionFunctor::VisitSystemMilestone(SystemMilestoneEnd *systemMilestoneEnd)
{
    assert(m_currentSystem);
    systemMilestoneEnd->MoveItselfTo(m_currentSystem);

    return FUNCTOR_SIBLINGS;
}

} // namespace vrv
