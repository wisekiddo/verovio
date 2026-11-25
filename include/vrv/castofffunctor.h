/////////////////////////////////////////////////////////////////////////////
// Name:        castofffunctor.h
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_CASTOFFFUNCTOR_H__
#define __VRV_CASTOFFFUNCTOR_H__

#include <array>

#include "functor.h"
#include "scoredef.h"

namespace vrv {

//----------------------------------------------------------------------------
// CastOffSystemsFunctor
//----------------------------------------------------------------------------

/**
 * This class fills a page by adding systems with the appropriate length.
 * Adds all the pending objects when reaching the end.
 */
class CastOffSystemsFunctor : public DocFunctor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    CastOffSystemsFunctor(Page *page, Doc *doc, bool smart);
    virtual ~CastOffSystemsFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return true; }

    /*
     * Retrieve the leftover system
     */
    System *GetLeftoverSystem() const { return m_leftoverSystem; }

    /*
     * Set the system width
     */
    void SetSystemWidth(int width) { m_systemWidth = width; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitDiv(Div *div) override;
    FunctorCode VisitEditorialElement(EditorialElement *editorialElement) override;
    FunctorCode VisitEnding(Ending *ending) override;
    FunctorCode VisitMeasure(Measure *measure) override;
    FunctorCode VisitPageElement(PageElement *pageElement) override;
    FunctorCode VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd) override;
    FunctorCode VisitSb(Sb *sb) override;
    FunctorCode VisitScoreDef(ScoreDef *scoreDef) override;
    FunctorCode VisitSystem(System *system) override;
    FunctorCode VisitSystemEnd(System *system) override;
    FunctorCode VisitSystemElement(SystemElement *systemElement) override;
    FunctorCode VisitSystemMilestone(SystemMilestoneEnd *systemMilestoneEnd) override;
    ///@}

protected:
    //
private:
    //
public:
    //
private:
    /**
     * What a staff has in one of a measure's scoreDef alignments, as AdjustXPosFunctor places it (MuseNote RC216):
     * nothing; an element drawn empty, which keeps the alignment clear of what the staff drew before it but takes no
     * room and no margin; or a drawing, its left and right from the alignment, with its margins.
     */
    struct ScoreDefExtent {
        bool present = false;
        bool drawn = false;
        int left = 0;
        int right = 0;
        int leftMargin = 0;
        int rightMargin = 0;
    };

    /**
     * A staff's clef, key and meter in a run of scoreDef alignments (a mensur is not counted), and where the first may
     * start on that staff: past a system start line, or past the barline before a cautionary change (MuseNote RC216).
     */
    struct StaffScoreDefExtents {
        int minPos = 0;
        std::array<ScoreDefExtent, 3> elements;
    };

    /**
     * Where the alignment after a run of scoreDef alignments goes (a line's first measure's left barline, drawn empty;
     * or a measure's end after a cautionary change), the run's alignments all starting at 0, as
     * AdjustXPosFunctor::VisitMeasure places them: staff by staff, each element moving its alignment, and every one
     * after it, as far as it must to clear what its staff drew before (MuseNote RC216).
     */
    int CalcScoreDefEnd(const std::vector<StaffScoreDefExtents> &staves) const;

    /**
     * The width a line starting with the next measure takes beyond that measure's stored width (MuseNote RC216): what
     * its start draws (each staff's clef and key, a meter changing there), less what the stored width holds already of
     * a change right before the measure, drawn there within the line; and the abbreviated labels.
     */
    int CalcLineStartWidth() const;

    /**
     * The width a change of clef, key or meter takes at the end of a line closing with the measure, cautionary: what
     * each staff whose draw flags the scoreDef set draws after the measure's right barline (MuseNote RC216).
     */
    int CalcChangeWidth(const ScoreDef &scoreDef, const Measure *measure) const;

    /**
     * The width of what a line ending with this measure ends with: the change right after it, if any, cautionary
     * (MuseNote RC216).
     */
    int CalcCautionaryWidth(Measure *measure) const;

    /**
     * What a staff draws in the scoreDef alignments of a line's start (its clef and key, and a meter where its draw
     * flags set one), or of a change within a line or at its end (what its draw flags set) (MuseNote RC216).
     */
    StaffScoreDefExtents CalcStaffScoreDefExtents(const StaffDef *staffDef, bool lineStart) const;

    /**
     * A staff's clef, key and meter in effect, as View::DrawClef (View::DrawTabClef), View::DrawKeySig and
     * View::DrawMeterSig draw them and BBoxDeviceContext bounds them, with their margins; at a line's start a key
     * draws no naturals (MuseNote RC216).
     */
    ///@{
    ScoreDefExtent CalcClefExtent(const StaffDef *staffDef) const;
    ScoreDefExtent CalcKeyExtent(const StaffDef *staffDef, bool lineStart) const;
    ScoreDefExtent CalcMeterExtent(const StaffDef *staffDef) const;
    ///@}

    /**
     * Glyphs drawn from x as BBoxDeviceContext::DrawMusicText bounds them (each glyph at the advances before it,
     * truncated, from its own left), added to an extent (MuseNote RC216).
     */
    void AddGlyphsExtent(const std::u32string &glyphs, int x, int staffSize, ScoreDefExtent &extent) const;

    /**
     * A glyph's advance as a device context measures text it draws (DeviceContext::AddGlyphToTextExtend: rounded up),
     * its width where it has none; and a string's, as DeviceContext::GetSmuflTextExtent sums them (MuseNote RC216)
     */
    ///@{
    int CalcGlyphAdvance(char32_t code, int staffSize) const;
    int CalcGlyphsAdvance(const std::u32string &glyphs, int staffSize) const;
    ///@}

    /**
     * A measure's clefs and keys into a scoreDef, as ScoreDefSetCurrentFunctor::VisitClef and VisitKeySig set them,
     * and its draw flags cleared, the change before the measure being drawn there (MuseNote RC216).
     */
    void ApplyMeasure(ScoreDef &scoreDef, Measure *measure) const;

private:
    // The system we are taking the content from
    System *m_contentSystem;
    // The page we are adding the system to
    Page *m_page;
    // The current system
    System *m_currentSystem;
    // The cumulated shift (m_drawingXRel of the first measure of the current system)
    int m_shift;
    // The system width
    int m_systemWidth;
    // The width counted for the start of the current line before its first measure (MuseNote RC216: it was the last
    // scoreDef's own drawing width, and 0 until the line breaker met one)
    int m_currentScoreDefWidth;
    // The scoreDef in effect at this point of the content; its draw flags, the change right before the next measure
    // (MuseNote RC216)
    ScoreDef m_upcomingScoreDef;
    // For a line starting with the first measure held: its start, and the change the line before it ends with
    // (MuseNote RC216)
    int m_heldLineStartWidth;
    int m_heldCautionaryWidth;
    // The pending elements (ScoreDef, Endings, etc.) to be placed at the beginning of a system
    ArrayOfObjects m_pendingElements;
    // Indicates to smartly use encoded system breaks
    bool m_smart;
    // The leftover system (last system with only one measure)
    System *m_leftoverSystem;
};

//----------------------------------------------------------------------------
// CastOffPagesFunctor
//----------------------------------------------------------------------------

/**
 * This class fills a document by adding pages with the appropriate length.
 */
class CastOffPagesFunctor : public DocFunctor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    CastOffPagesFunctor(Page *contentPage, Doc *doc, Page *currentPage);
    virtual ~CastOffPagesFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return true; }

    /*
     * Set the leftover system
     */
    void SetLeftoverSystem(System *system) { m_leftoverSystem = system; }

    /*
     * Set the page height
     */
    void SetPageHeight(int height) { m_pageHeight = height; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitPageEnd(Page *page) override;
    FunctorCode VisitPageElement(PageElement *pageElement) override;
    FunctorCode VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd) override;
    FunctorCode VisitScore(Score *score) override;
    FunctorCode VisitSystem(System *system) override;
    ///@}

protected:
    //
private:
    /*
     * Returns the available height for system drawing on the current page
     */
    int GetAvailableDrawingHeight() const;

public:
    //
private:
    // The page we are taking the content from
    Page *m_contentPage;
    // The current page
    Page *m_currentPage;
    // Indicates whether the current page is the first
    bool m_firstCastOffPage;
    // The cumulated shift (m_drawingYRel of the first system of the current page)
    int m_shift;
    // The page heights
    int m_pageHeight;
    int m_pgHeadHeight;
    int m_pgFootHeight;
    int m_pgHead2Height;
    int m_pgFoot2Height;
    // The leftover system (last system with only one measure)
    System *m_leftoverSystem;
    // The pending elements (Mdiv, Score) to be placed at the beginning of a page
    ArrayOfObjects m_pendingPageElements;
};

//----------------------------------------------------------------------------
// CastOffEncodingFunctor
//----------------------------------------------------------------------------

/**
 * This class casts off the document according to the encoding provided (pb and sb).
 */
class CastOffEncodingFunctor : public DocFunctor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    CastOffEncodingFunctor(Doc *doc, Page *currentPage, bool usePages = true);
    virtual ~CastOffEncodingFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return false; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitDiv(Div *div) override;
    FunctorCode VisitEditorialElement(EditorialElement *editorialElement) override;
    FunctorCode VisitEnding(Ending *ending) override;
    FunctorCode VisitMeasure(Measure *measure) override;
    FunctorCode VisitPageElement(PageElement *pageElement) override;
    FunctorCode VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd) override;
    FunctorCode VisitPb(Pb *pb) override;
    FunctorCode VisitSb(Sb *sb) override;
    FunctorCode VisitScoreDef(ScoreDef *scoreDef) override;
    FunctorCode VisitStaff(Staff *staff) override;
    FunctorCode VisitSystem(System *system) override;
    FunctorCode VisitSystemElement(SystemElement *systemElement) override;
    ///@}

protected:
    //
private:
    //
public:
    //
private:
    // The current page
    Page *m_currentPage;
    // The system we are taking the content from
    System *m_contentSystem;
    // The current system
    System *m_currentSystem;
    // Indicates if we want to use the pageBreaks from the document
    bool m_usePages;
};

//----------------------------------------------------------------------------
// UnCastOffFunctor
//----------------------------------------------------------------------------

/**
 * This class undoes the cast off for both pages and systems.
 * This is used by Doc::UnCastOffDoc for putting all pages / systems continously.
 */
class UnCastOffFunctor : public Functor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    UnCastOffFunctor(Page *page);
    virtual ~UnCastOffFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return false; }

    /*
     * Set the reset cache flag
     */
    void SetResetCache(bool resetCache) { m_resetCache = resetCache; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitFloatingObject(FloatingObject *floatingObject) override;
    FunctorCode VisitMeasure(Measure *measure) override;
    FunctorCode VisitPageElement(PageElement *pageElement) override;
    FunctorCode VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd) override;
    FunctorCode VisitScore(Score *score) override;
    FunctorCode VisitSystem(System *system) override;
    ///@}

protected:
    //
private:
    //
public:
    //
private:
    // The page we are adding systems to
    Page *m_page;
    // The system we are adding content to
    System *m_currentSystem;
    // Indicates if we need to reset the horizontal layout cache
    bool m_resetCache;
};

//----------------------------------------------------------------------------
// CastOffToSelectionFunctor
//----------------------------------------------------------------------------

/**
 * This class casts off a document to selection.
 * Moves everything before the selection to the first page, the selection to a second page,
 * and everything after the selection to a third page.
 */
class CastOffToSelectionFunctor : public DocFunctor {
public:
    /**
     * @name Constructors, destructors
     */
    ///@{
    CastOffToSelectionFunctor(Page *page, Doc *doc, const std::string &start, const std::string &end);
    virtual ~CastOffToSelectionFunctor() = default;
    ///@}

    /*
     * Abstract base implementation
     */
    bool ImplementsEndInterface() const override { return false; }

    /*
     * Functor interface
     */
    ///@{
    FunctorCode VisitDiv(Div *div) override;
    FunctorCode VisitEditorialElement(EditorialElement *editorialElement) override;
    FunctorCode VisitMeasure(Measure *measure) override;
    FunctorCode VisitPageElement(PageElement *pageElement) override;
    FunctorCode VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd) override;
    FunctorCode VisitScoreDef(ScoreDef *scoreDef) override;
    FunctorCode VisitSystem(System *system) override;
    FunctorCode VisitSystemElement(SystemElement *systemElement) override;
    FunctorCode VisitSystemMilestone(SystemMilestoneEnd *systemMilestoneEnd) override;
    ///@}

protected:
    //
private:
    //
public:
    //
private:
    // The system we are taking the content from
    System *m_contentSystem;
    // The page we are adding systems to
    Page *m_page;
    // The current system
    System *m_currentSystem;
    // The start measure ID of the selection
    std::string m_start;
    // The end measure ID of the selection
    std::string m_end;
    // Indicates whether we are currently in the selection range
    bool m_isSelection;
};

} // namespace vrv

#endif // __VRV_CASTOFFFUNCTOR_H__
