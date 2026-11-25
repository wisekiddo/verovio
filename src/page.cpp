/////////////////////////////////////////////////////////////////////////////
// Name:        page.cpp
// Author:      Laurent Pugin
// Created:     2005
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "page.h"

//----------------------------------------------------------------------------

#include <cassert>
#include <set>

//----------------------------------------------------------------------------

#include "adjustaccidxfunctor.h"
#include "adjustarpegfunctor.h"
#include "adjustarticfunctor.h"
#include "adjustbeamsfunctor.h"
#include "adjustclefchangesfunctor.h"
#include "adjustdotsfunctor.h"
#include "adjustfloatingpositionerfunctor.h"
#include "adjustgracexposfunctor.h"
#include "adjustharmgrpsspacingfunctor.h"
#include "adjustlayersfunctor.h"
#include "adjustneumexfunctor.h"
#include "adjustossiastaffdeffunctor.h"
#include "adjustslursfunctor.h"
#include "adjuststaffoverlapfunctor.h"
#include "adjustsylspacingfunctor.h"
#include "adjusttempofunctor.h"
#include "adjusttupletsxfunctor.h"
#include "adjusttupletsyfunctor.h"
#include "adjustxoverflowfunctor.h"
#include "adjustxposfunctor.h"
#include "adjustxrelfortranscriptionfunctor.h"
#include "adjustyposfunctor.h"
#include "adjustyrelfortranscriptionfunctor.h"
#include "alignfunctor.h"
#include "bboxdevicecontext.h"
#include "cachehorizontallayoutfunctor.h"
#include "calcalignmentpitchposfunctor.h"
#include "calcalignmentxposfunctor.h"
#include "calcarticfunctor.h"
#include "calcbboxoverflowsfunctor.h"
#include "calcchordnoteheadsfunctor.h"
#include "calcdotsfunctor.h"
#include "calcledgerlinesfunctor.h"
#include "calcligatureorneumeposfunctor.h"
#include "calcslurdirectionfunctor.h"
#include "calcspanningbeamspansfunctor.h"
#include "calcstemfunctor.h"
#include "comparison.h"
#include "doc.h"
#include "functor.h"
#include "justifyfunctor.h"
#include "libmei.h"
#include "miscfunctor.h"
#include "pageelement.h"
#include "pages.h"
#include "pgfoot.h"
#include "pghead.h"
#include "resetfunctor.h"
#include "score.h"
#include "staff.h"
#include "syl.h"
#include "system.h"
#include "verse.h"
#include "view.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// Page
//----------------------------------------------------------------------------

Page::Page() : Object(PAGE)
{
    this->Reset();
}

Page::~Page() {}

void Page::Reset()
{
    Object::Reset();

    m_drawingScoreDef.Reset();
    m_score = NULL;
    m_scoreEnd = NULL;
    m_layoutDone = false;
    this->ResetID();

    // by default we have no values and use the document ones
    m_pageHeight = -1;
    m_pageWidth = -1;
    m_pageMarginBottom = 0;
    m_pageMarginLeft = 0;
    m_pageMarginRight = 0;
    m_pageMarginTop = 0;
    m_PPUFactor = 1.0;

    m_drawingJustifiableHeight = 0;
    m_justificationSum = 0.;
}

bool Page::IsSupportedChild(ClassId classId)
{
    static const std::vector<ClassId> supported{ SYSTEM };

    if (std::find(supported.begin(), supported.end(), classId) != supported.end()) {
        return true;
    }
    else if (Object::IsPageElement(classId)) {
        return true;
    }
    else {
        return false;
    }
}

bool Page::IsFirstOfSelection() const
{
    const Doc *doc = vrv_cast<const Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);
    if (!doc->HasSelection()) return false;

    assert(this->GetParent());
    return (this->GetParent()->GetFirst() == this);
}

bool Page::IsLastOfSelection() const
{
    const Doc *doc = vrv_cast<const Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);
    if (!doc->HasSelection()) return false;

    assert(this->GetParent());
    return (this->GetParent()->GetLast() == this);
}

RunningElement *Page::GetHeader()
{
    return const_cast<RunningElement *>(std::as_const(*this).GetHeader());
}

const RunningElement *Page::GetHeader() const
{
    assert(m_score);
    assert(m_score->GetScoreDef());

    const Doc *doc = vrv_cast<const Doc *>(this->GetFirstAncestor(DOC));
    if (!doc || (doc->GetOptions()->m_header.GetValue() == HEADER_none)) {
        return NULL;
    }

    const Pages *pages = doc->GetPages();
    assert(pages);

    // If we have the option turned on, return the header without `@func` or with
    // `@func="all"`
    if (doc->GetOptions()->m_usePgHeaderForAll.GetValue()) {
        RunningElement *header = m_score->GetScoreDef()->GetPgHead(PGFUNC_NONE);
        if (!header) {
            header = m_score->GetScoreDef()->GetPgHead(PGFUNC_all);
        }
        return header;
    }
    else {
        RunningElement *header = NULL;
        if (pages->GetFirst() == this) {
            header = m_score->GetScoreDef()->GetPgHead(PGFUNC_first);
        }
        // If we did not find it, or not the first page
        if (!header) {
            header = m_score->GetScoreDef()->GetPgHead(PGFUNC_all);
        }
        return header;
    }
}

RunningElement *Page::GetFooter()
{
    return const_cast<RunningElement *>(std::as_const(*this).GetFooter());
}

const RunningElement *Page::GetFooter() const
{
    assert(m_scoreEnd);
    assert(m_scoreEnd->GetScoreDef());

    const Doc *doc = vrv_cast<const Doc *>(this->GetFirstAncestor(DOC));
    if (!doc || (doc->GetOptions()->m_footer.GetValue() == FOOTER_none)) {
        return NULL;
    }

    const Pages *pages = doc->GetPages();
    assert(pages);

    // If we have the option turned on, return the footer without `@func` or with
    // `@func="all"`
    if (doc->GetOptions()->m_usePgFooterForAll.GetValue()) {
        RunningElement *footer = m_score->GetScoreDef()->GetPgFoot(PGFUNC_NONE);
        if (!footer) {
            footer = m_score->GetScoreDef()->GetPgFoot(PGFUNC_all);
        }
        return footer;
    }
    else {
        RunningElement *footer = NULL;
        if (pages->GetFirst() == this) {
            footer = m_score->GetScoreDef()->GetPgFoot(PGFUNC_first);
        }
        // If we did not find it, or not the first page
        if (!footer) {
            footer = m_score->GetScoreDef()->GetPgFoot(PGFUNC_all);
        }
        return footer;
    }
}

void Page::LayOut()
{
    if (m_layoutDone) {
        // We only need to reset the header - this will adjust the page number if necessary
        if (this->GetHeader()) this->GetHeader()->SetDrawingPage(this);
        if (this->GetFooter()) this->GetFooter()->SetDrawingPage(this);
        return;
    }

    // A new layout starts from the systems' own spacing and lyric size (MuseNote RC211)
    ListOfObjects systems = this->FindAllDescendantsByType(SYSTEM, false, 1);
    for (Object *object : systems) {
        System *system = vrv_cast<System *>(object);
        system->m_drawingSpacingFactor = 1.0;
        system->m_drawingLyricScale = 1.0;
    }

    this->LayOutHorizontally();
    this->FitHorizontally();
    this->JustifyHorizontally();
    this->LayOutVertically();
    this->JustifyVertically();

    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);
    if (doc->GetOptions()->m_svgBoundingBoxes.GetValue()) {
        View view;
        view.SetDoc(doc);
        BBoxDeviceContext bBoxDC(&view, 0, 0);
        // Do not do the layout in this view - otherwise we will loop...
        view.SetPage(this, false);
        view.DrawCurrentPage(&bBoxDC, false);
    }

    m_layoutDone = true;
}

void Page::LayOutTranscription(bool force)
{
    if (m_layoutDone && !force) {
        return;
    }

    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    // Reset the horizontal alignment
    ResetHorizontalAlignmentFunctor resetHorizontalAlignment;
    this->Process(resetHorizontalAlignment);

    // Reset the vertical alignment
    ResetVerticalAlignmentFunctor resetVerticalAlignment;
    this->Process(resetVerticalAlignment);

    // Align the content of the page using measure aligners
    // After this:
    // - each LayerElement object will have its Alignment pointer initialized
    AlignHorizontallyFunctor alignHorizontally(doc);
    this->Process(alignHorizontally);

    // Align the content of the page using system aligners
    // After this:
    // - each Staff object will then have its StaffAlignment pointer initialized
    AlignVerticallyFunctor alignVertically(doc);
    this->Process(alignVertically);

    // Set the pitch / pos alignment
    CalcAlignmentPitchPosFunctor calcAlignmentPitchPos(doc);
    this->Process(calcAlignmentPitchPos);

    CalcLigatureOrNeumePosFunctor calcLigatureOrNeumePos(doc);
    this->Process(calcLigatureOrNeumePos);

    CalcStemFunctor calcStem(doc);
    this->Process(calcStem);

    CalcChordNoteHeadsFunctor calcChordNoteHeads(doc);
    this->Process(calcChordNoteHeads);

    CalcDotsFunctor calcDots(doc);
    this->Process(calcDots);

    if (!m_layoutDone) {
        // Render it for filling the bounding box
        View view;
        view.SetDoc(doc);
        BBoxDeviceContext bBoxDC(&view, 0, 0, BBOX_HORIZONTAL_ONLY);
        // Do not do the layout in this view - otherwise we will loop...
        view.SetPage(this, false);
        view.DrawCurrentPage(&bBoxDC, false);
    }

    AdjustXRelForTranscriptionFunctor adjustXRelForTranscription;
    this->Process(adjustXRelForTranscription);
    AdjustYRelForTranscriptionFunctor adjustYRelForTranscription;
    this->Process(adjustYRelForTranscription);

    CalcLedgerLinesFunctor calcLedgerLines(doc);
    this->Process(calcLedgerLines);

    m_layoutDone = true;
}

void Page::ResetAligners()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    // Reset the horizontal alignment
    ResetHorizontalAlignmentFunctor resetHorizontalAlignment;
    this->Process(resetHorizontalAlignment);

    // Reset the vertical alignment
    ResetVerticalAlignmentFunctor resetVerticalAlignment;
    this->Process(resetVerticalAlignment);

    // Align the content of the page using measure aligners
    // After this:
    // - each LayerElement object will have its Alignment pointer initialized
    AlignHorizontallyFunctor alignHorizontally(doc);
    this->Process(alignHorizontally);

    // Align the content of the page using system aligners
    // After this:
    // - each Staff object will then have its StaffAlignment pointer initialized
    AlignVerticallyFunctor alignVertically(doc);
    this->Process(alignVertically);

    // Unless duration-based spacing is disabled, set the X position of each Alignment.
    // Does non-linear spacing based on the duration space between two Alignment objects.
    if (!doc->GetOptions()->m_evenNoteSpacing.GetValue()) {
        data_DURATION longestActualDur = DURATION_4;

        // Detect the longest duration in order to adjust the spacing (false by default)
        if (doc->GetOptions()->m_spacingDurDetection.GetValue()) {
            // Get the longest duration in the piece
            AttDurExtremeComparison durExtremeComparison(LONGEST);
            Object *longestDur = this->FindDescendantExtremeByComparison(&durExtremeComparison);
            if (longestDur) {
                DurationInterface *interface = longestDur->GetDurationInterface();
                assert(interface);
                longestActualDur = interface->GetActualDur();
                // LogDebug("Longest duration is DUR_* code %d", longestActualDur);
            }
        }

        CalcAlignmentXPosFunctor calcAlignmentXPos(doc);
        calcAlignmentXPos.SetLongestActualDur(longestActualDur);
        this->Process(calcAlignmentXPos);
    }

    // Set the pitch / pos alignment
    CalcAlignmentPitchPosFunctor calcAlignmentPitchPos(doc);
    this->Process(calcAlignmentPitchPos);

    CalcLigatureOrNeumePosFunctor calcLigatureOrNeumePos(doc);
    this->Process(calcLigatureOrNeumePos);

    CalcStemFunctor calcStem(doc);
    this->Process(calcStem);

    CalcChordNoteHeadsFunctor calcChordNoteHeads(doc);
    this->Process(calcChordNoteHeads);

    CalcDotsFunctor calcDots(doc);
    this->Process(calcDots);

    // Adjust the position of outside articulations
    CalcArticFunctor calcArtic(doc);
    this->Process(calcArtic);

    CalcSlurDirectionFunctor calcSlurDirection(doc);
    this->Process(calcSlurDirection);

    CalcSpanningBeamSpansFunctor calcSpanningBeamSpans(doc);
    this->Process(calcSpanningBeamSpans);
}

void Page::LayOutHorizontally()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    this->ResetAligners();

    // Render it for filling the bounding box
    View view;
    view.SetDoc(doc);
    view.SetSlurHandling(SlurHandling::Ignore);
    BBoxDeviceContext bBoxDC(&view, 0, 0, BBOX_HORIZONTAL_ONLY);
    // Do not do the layout in this view - otherwise we will loop...
    view.SetPage(this, false);
    view.DrawCurrentPage(&bBoxDC, false);

    AdjustOssiaStaffDefFunctor adjustOssiaStaffDef(doc);
    this->Process(adjustOssiaStaffDef);

    // Adjust the position of outside articulations
    AdjustArticFunctor adjustArtic(doc);
    this->Process(adjustArtic);

    // Adjust the x position of the LayerElement where multiple layers collide
    // Look at each LayerElement and change the m_xShift if the bounding box is overlapping
    // For the first iteration align elements without taking dots into consideration
    AdjustLayersFunctor adjustLayers(doc);
    this->Process(adjustLayers);

    // Adjust dots for the multiple layers. Try to align dots that can be grouped together when layers collide,
    // otherwise keep their relative positioning
    AdjustDotsFunctor adjustDots(doc);
    this->Process(adjustDots);

    // Adjust the X position of the neume and syllables
    AdjustNeumeXFunctor adjustNeumeX(doc);
    this->Process(adjustNeumeX);

    // Adjust layers again, this time including dots positioning
    AdjustLayersFunctor adjustLayersWithDots(doc);
    adjustLayersWithDots.IgnoreDots(false);
    this->Process(adjustLayersWithDots);

    // Adjust the X position of the accidentals, including in chords
    AdjustAccidXFunctor adjustAccidX(doc);
    this->Process(adjustAccidX);

    // Adjust the X shift of the Alignment looking at the bounding boxes
    // Look at each LayerElement and change the m_xShift if the bounding box is overlapping
    AdjustXPosFunctor adjustXPos(doc);
    adjustXPos.SetExcluded({ TABDURSYM });
    this->Process(adjustXPos);

    // Adjust tabRhythm separately
    adjustXPos.ClearExcluded();
    adjustXPos.SetIncluded({ BARLINE, KEYSIG, METERSIG, TABDURSYM });
    adjustXPos.SetRightBarLinesOnly(true);
    this->Process(adjustXPos);

    // Adjust the X shift of the Alignment looking at the bounding boxes
    // Look at each LayerElement and change the m_xShift if the bounding box is overlapping
    AdjustGraceXPosFunctor adjustGraceXPos(doc);
    this->Process(adjustGraceXPos);

    // Adjust the spacing of clef changes since they are skipped in AdjustXPos
    // Look at each clef change and  move them to the left and add space if necessary
    AdjustClefChangesFunctor adjustClefChanges(doc);
    this->Process(adjustClefChanges);

    // We need to populate processing lists for processing the document by Layer (for matching @tie) and
    // by Verse (for matching syllable connectors)
    InitProcessingListsFunctor initProcessingLists;
    this->Process(initProcessingLists);

    this->AdjustSylSpacingByVerse(initProcessingLists.GetVerseTree(), doc);

    AdjustHarmGrpsSpacingFunctor adjustHarmGrpsSpacing(doc);
    this->Process(adjustHarmGrpsSpacing);

    // Adjust the arpeg
    AdjustArpegFunctor adjustArpeg(doc);
    this->Process(adjustArpeg);

    // Adjust the tempo
    AdjustTempoFunctor adjustTempo(doc);
    this->Process(adjustTempo);

    // Adjust the position of the tuplets
    AdjustTupletsXFunctor adjustTupletsX(doc);
    this->Process(adjustTupletsX);

    // Prevent a margin overflow
    AdjustXOverflowFunctor adjustXOverflow(doc->GetDrawingUnit(100));
    this->Process(adjustXOverflow);

    // Adjust measure X position
    AlignMeasuresFunctor alignMeasures(doc);
    this->Process(alignMeasures);
}

void Page::LayOutHorizontallyWithCache(bool restore)
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    CacheHorizontalLayoutFunctor cacheHorizontalLayout(doc);
    cacheHorizontalLayout.SetRestore(restore);
    this->Process(cacheHorizontalLayout);
}

void Page::LayOutVertically()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    // Reset the vertical alignment
    ResetVerticalAlignmentFunctor resetVerticalAlignment;
    this->Process(resetVerticalAlignment);

    CalcLedgerLinesFunctor calcLedgerLines(doc);
    this->Process(calcLedgerLines);

    // Align the content of the page using system aligners
    // After this:
    // - each Staff object will then have its StaffAlignment pointer initialized
    AlignVerticallyFunctor alignVertically(doc);
    this->Process(alignVertically);

    // Render it for filling the bounding box
    View view;
    BBoxDeviceContext bBoxDC(&view, 0, 0);
    view.SetDoc(doc);
    // Do not do the layout in this view - otherwise we will loop...
    view.SetPage(this, false);
    view.DrawCurrentPage(&bBoxDC, false);

    // Adjust the position of outside articulations with slurs end and start positions
    AdjustArticWithSlursFunctor adjustArticWithSlurs(doc);
    this->Process(adjustArticWithSlurs);

    // Adjust the position of the beams in regards of layer elements
    AdjustBeamsFunctor adjustBeams(doc);
    this->Process(adjustBeams);

    // Adjust the position of the tuplets
    AdjustTupletsYFunctor adjustTupletsY(doc);
    this->Process(adjustTupletsY);

    // Adjust the position of the slurs
    AdjustSlursFunctor adjustSlurs(doc);
    this->Process(adjustSlurs);

    // At this point slurs must not be reinitialized, otherwise the adjustment we just did was in vain
    view.SetSlurHandling(SlurHandling::Drawing);
    view.SetPage(this, false);
    view.DrawCurrentPage(&bBoxDC, false);

    // Adjust the position of tuplets by slurs
    AdjustTupletWithSlursFunctor adjustTupletWithSlurs(doc);
    this->Process(adjustTupletWithSlurs);

    // Fill the arrays of bounding boxes (above and below) for each staff alignment for which the box overflows.
    CalcBBoxOverflowsFunctor calcBBoxOverflows(doc);
    this->Process(calcBBoxOverflows);

    // Adjust the positioners of floating elements (slurs, hairpin, dynam, etc)
    AdjustFloatingPositionersFunctor adjustFloatingPositioners(doc);
    this->Process(adjustFloatingPositioners);

    // Adjust the overlap of the staff alignments by looking at the overflow bounding boxes
    AdjustStaffOverlapFunctor adjustStaffOverlap(doc);
    this->Process(adjustStaffOverlap);

    // Set the Y position of each StaffAlignment
    // Adjust the Y shift to make sure there is a minimal space (staffMargin) between each staff
    AdjustYPosFunctor adjustYPos(doc);
    this->Process(adjustYPos);

    // Adjust the positioners of floating elements placed between staves
    AdjustFloatingPositionersBetweenFunctor adjustFloatingPositionersBetween(doc);
    this->Process(adjustFloatingPositionersBetween);

    AdjustCrossStaffYPosFunctor adjustCrossStaffYPos(doc);
    this->Process(adjustCrossStaffYPos);

    // Redraw are re-adjust the position of the slurs when we have cross-staff ones
    if (adjustSlurs.HasCrossStaffSlurs()) {
        view.SetSlurHandling(SlurHandling::Initialize);
        view.SetPage(this, false);
        view.DrawCurrentPage(&bBoxDC, false);
        this->Process(adjustSlurs);
    }

    if (this->GetHeader()) {
        this->GetHeader()->AdjustRunningElementYPos();
    }

    if (this->GetFooter()) {
        this->GetFooter()->AdjustRunningElementYPos();
    }

    // Adjust system Y position
    AlignSystemsFunctor alignSystems(doc);
    alignSystems.SetShift(doc->m_drawingPageContentHeight);
    alignSystems.SetSystemSpacing(doc->GetOptions()->m_spacingSystem.GetValue() * doc->GetDrawingUnit(100));
    this->Process(alignSystems);
}

namespace {

// A line's words are set no smaller than two fifths of their size (the owner's choice, 2026-11-24): where they cannot
// fit even then, JustifyHorizontally squeezes the line, as before
constexpr double MIN_LYRIC_SCALE = 0.4;

// The lowest ratio JustifyXFunctor may squeeze a system by and keep each word clear of the next, and each hyphen its
// dash (MuseNote RC211): 0 when the system has no two words to close
double SafeSqueezeRatio(System *system, Doc *doc)
{
    // The syllables of each line of verses: a staff's side and verse number (as AdjustSylSpacingFunctor spaces them)
    std::map<std::tuple<int, bool, int>, std::vector<Syl *>> lines;
    ListOfObjects syls = system->FindAllDescendantsByType(SYL);
    for (Object *object : syls) {
        Syl *syl = vrv_cast<Syl *>(object);
        const Staff *staff = vrv_cast<const Staff *>(syl->GetFirstAncestor(STAFF));
        const Verse *verse = vrv_cast<const Verse *>(syl->GetFirstAncestor(VERSE));
        if (!staff || !verse || !syl->HasContentHorizontalBB()) continue;
        lines[{ staff->GetN(), verse->GetPlace() == STAFFREL_above, verse->GetN() }].push_back(syl);
    }
    double safe = 0.0;
    for (auto &[line, lineSyls] : lines) {
        std::stable_sort(lineSyls.begin(), lineSyls.end(),
            [](const Syl *syl1, const Syl *syl2) { return (syl1->GetContentLeft() < syl2->GetContentLeft()); });
        for (int i = 0; i + 1 < (int)lineSyls.size(); ++i) {
            Syl *syl = lineSyls.at(i);
            const Syl *next = lineSyls.at(i + 1);
            // JustifyXFunctor moves the notes; a syllable keeps its place by its note
            const LayerElement *note = syl->GetStart();
            const LayerElement *nextNote = next->GetStart();
            if (!note || !nextNote) continue;
            const int span = nextNote->GetDrawingX() - note->GetDrawingX();
            if (span <= 0) continue;
            const int staffSize = vrv_cast<const Staff *>(syl->GetFirstAncestor(STAFF))->m_drawingStaffSize;
            const int minGap = ((syl->GetWordpos() == sylLog_WORDPOS_i) || (syl->GetWordpos() == sylLog_WORDPOS_m))
                ? syl->CalcHyphenLength(doc, staffSize)
                : 0;
            const int gap = next->GetContentLeft() - syl->GetContentRight();
            safe = std::max(safe, 1.0 - (double)(gap - minGap) / (double)span);
        }
    }
    return std::min(safe, 1.0);
}

} // namespace

void Page::FitHorizontally()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    if ((doc->GetOptions()->m_breaks.GetValue() == BREAKS_none) || doc->GetOptions()->m_noJustification.GetValue()
        || doc->GetOptions()->m_adjustPageWidth.GetValue()) {
        return;
    }

    ListOfObjects systems = this->FindAllDescendantsByType(SYSTEM, false, 1);
    // Each system's width when last laid out; the systems whose words are being set smaller; and the systems left to
    // JustifyHorizontally
    std::map<const System *, int> laidOutWidths;
    std::set<const System *> wordsSmaller;
    std::set<const System *> settled;
    for (int pass = 0; pass < 8; ++pass) {
        bool again = false;
        for (Object *object : systems) {
            System *system = vrv_cast<System *>(object);
            assert(system);
            if (settled.count(system)) continue;
            const double ratio = system->GetJustificationRatio(doc);
            if (ratio >= 1.0) continue;
            // A squeeze that keeps the words apart is left to JustifyHorizontally, as before
            if (ratio >= SafeSqueezeRatio(system, doc)) continue;
            const int width = system->m_drawingTotalWidth;
            const auto laidOut = laidOutWidths.find(system);
            // The notes tighter while that narrows the system by a hundredth at least
            const bool narrowed
                = (laidOut == laidOutWidths.end()) || (laidOut->second - width > laidOut->second / 100);
            if (!wordsSmaller.count(system) && narrowed) {
                system->m_drawingSpacingFactor *= ratio;
            }
            else if (!system->FindDescendantByType(SYL) || (system->m_drawingLyricScale <= MIN_LYRIC_SCALE)) {
                // No words to set smaller, or at two fifths already: JustifyHorizontally squeezes it, as before
                settled.insert(system);
                continue;
            }
            else {
                // Then the words smaller, while the squeeze would still draw one over the next: smaller words open
                // the gaps between them, whether or not the system narrows
                wordsSmaller.insert(system);
                system->m_drawingLyricScale = std::max(MIN_LYRIC_SCALE, system->m_drawingLyricScale * ratio);
            }
            laidOutWidths[system] = width;
            again = true;
        }
        if (!again) break;
        this->LayOutHorizontally();
    }
}

void Page::JustifyHorizontally()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    if ((doc->GetOptions()->m_breaks.GetValue() == BREAKS_none) || doc->GetOptions()->m_noJustification.GetValue()) {
        return;
    }

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    if ((doc->GetOptions()->m_adjustPageWidth.GetValue())) {
        doc->m_drawingPageContentWidth = this->GetContentWidth();
        doc->m_drawingPageWidth
            = doc->m_drawingPageContentWidth + doc->m_drawingPageMarginLeft + doc->m_drawingPageMarginRight;
    }
    else {
        // Justify X position
        JustifyXFunctor justifyX(doc);
        justifyX.SetSystemFullWidth(doc->m_drawingPageContentWidth);
        this->Process(justifyX);
    }
}

void Page::JustifyVertically()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    // Nothing to justify
    if (m_drawingJustifiableHeight <= 0 || m_justificationSum <= 0) {
        return;
    }

    // Vertical justification is not enabled
    if (!doc->GetOptions()->m_justifyVertically.GetValue()) {
        return;
    }

    this->ReduceJustifiableHeight(doc);

    // Justify Y position
    JustifyYFunctor justifyY(doc);
    justifyY.SetJustificationSum(m_justificationSum);
    justifyY.SetSpaceToDistribute(m_drawingJustifiableHeight);
    this->Process(justifyY);

    if (!justifyY.GetShiftForStaff().empty()) {
        // Adjust cross staff content which is displaced through vertical justification
        JustifyYAdjustCrossStaffFunctor justifyYAdjustCrossStaff(doc);
        justifyYAdjustCrossStaff.SetShiftForStaff(justifyY.GetShiftForStaff());
        this->Process(justifyYAdjustCrossStaff);
    }
}

void Page::ReduceJustifiableHeight(const Doc *doc)
{
    const Pages *pages = doc->GetPages();
    assert(pages);

    double maxRatio = doc->GetOptions()->m_justificationMaxVertical.GetValue();
    // Special handling for justification of last page
    if (pages->GetLast() == this) {
        const System *firstSystem = vrv_cast<const System *>(this->GetFirst(SYSTEM));
        const System *lastSystem = vrv_cast<const System *>(this->GetLast(SYSTEM));
        if (firstSystem && lastSystem) {
            const int usedDrawingHeight
                = firstSystem->GetDrawingY() - lastSystem->GetDrawingY() + lastSystem->GetHeight();
            maxRatio *= (double)usedDrawingHeight / (double)doc->m_drawingPageHeight;
        }
    }

    m_drawingJustifiableHeight = std::min<int>(doc->m_drawingPageHeight * maxRatio, m_drawingJustifiableHeight);
}

void Page::LayOutPitchPos()
{
    Doc *doc = vrv_cast<Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    // Set the pitch / pos alignment
    CalcAlignmentPitchPosFunctor calcAlignmentPitchPos(doc);
    this->Process(calcAlignmentPitchPos);

    CalcStemFunctor calcStem(doc);
    this->Process(calcStem);

    CalcLedgerLinesFunctor calcLedgerLines(doc);
    this->Process(calcLedgerLines);
}

int Page::GetContentHeight() const
{
    const Doc *doc = vrv_cast<const Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    if (!this->GetChildCount()) {
        return 0;
    }

    const System *last = vrv_cast<const System *>(this->GetLast(SYSTEM));
    assert(last);
    int height = doc->m_drawingPageContentHeight - last->GetDrawingYRel() + last->GetHeight();

    if (this->GetFooter()) {
        height += this->GetFooter()->GetTotalHeight(doc);
    }

    return height;
}

int Page::GetContentWidth() const
{
    const Doc *doc = vrv_cast<const Doc *>(this->GetFirstAncestor(DOC));
    assert(doc);
    // in non debug
    if (!doc) return 0;

    // Make sure we have the correct page size
    assert(doc->CheckPageSize(this));

    int maxWidth = 0;
    for (const Object *child : this->GetChildren()) {
        const System *system = dynamic_cast<const System *>(child);
        if (system) {
            // we include the left margin and the right margin
            int systemWidth = system->m_drawingTotalWidth + system->m_systemLeftMar + system->m_systemRightMar;
            maxWidth = std::max(systemWidth, maxWidth);
        }
    }
    // For avoiding unused variable warning in non debug mode
    doc = NULL;

    return maxWidth;
}

void Page::AdjustSylSpacingByVerse(const IntTree &verseTree, Doc *doc)
{
    if (verseTree.child.empty()) return;

    // Same for the lyrics, but line by line since Syl are TimeSpanningInterface elements for handling connectors. A
    // line is a staff's side and verse number, every voice of the staff together: spaced voice by voice, the verses
    // of two voices drawn on one line ran into each other (MuseNote RC205). Each line is given what the walk of the
    // page filtered to it met, from one walk that records it (a walk for each verse took seconds for a token of 3,000
    // words: MuseNote RC92)
    RecordSylSpacingFunctor recordSylSpacing;
    this->Process(recordSylSpacing);
    for (const RecordSylSpacingFunctor::Line &line : recordSylSpacing.GetLines()) {
        AdjustSylSpacingFunctor adjustSylSpacing(doc);
        recordSylSpacing.Replay(line, adjustSylSpacing);
    }
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

FunctorCode Page::Accept(Functor &functor)
{
    return functor.VisitPage(this);
}

FunctorCode Page::Accept(ConstFunctor &functor) const
{
    return functor.VisitPage(this);
}

FunctorCode Page::AcceptEnd(Functor &functor)
{
    return functor.VisitPageEnd(this);
}

FunctorCode Page::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitPageEnd(this);
}

} // namespace vrv
