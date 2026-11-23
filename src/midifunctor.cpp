/////////////////////////////////////////////////////////////////////////////
// Name:        midifunctor.cpp
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "midifunctor.h"

//----------------------------------------------------------------------------

#include <algorithm>

//----------------------------------------------------------------------------

#include "arpeg.h"
#include "beatrpt.h"
#include "btrem.h"
#include "doc.h"
#include "featureextractor.h"
#include "ftrem.h"
#include "gracegrp.h"
#include "instrdef.h"
#include "layer.h"
#include "mrest.h"
#include "multirest.h"
#include "octave.h"
#include "pedal.h"
#include "rest.h"
#include "staff.h"
#include "syl.h"
#include "tabgrp.h"
#include "tempo.h"
#include "text.h"
#include "tie.h"
#include "timemap.h"
#include "tuplet.h"
#include "verse.h"
#include "vrv.h"

//----------------------------------------------------------------------------

#include "MidiFile.h"
#include "Tunings.h"

namespace vrv {

//----------------------------------------------------------------------------
// InitOnsetOffsetFunctor
//----------------------------------------------------------------------------

InitOnsetOffsetFunctor::InitOnsetOffsetFunctor(Doc *doc) : DocFunctor(doc)
{
    static const std::map<int, data_DURATION> durationEq{
        { DURATION_EQ_brevis, DURATION_brevis }, //
        { DURATION_EQ_semibrevis, DURATION_semibrevis }, //
        { DURATION_EQ_minima, DURATION_minima }, //
    };

    m_currentScoreTime = 0;
    m_currentRealTimeSeconds = 0.0;
    m_notationType = NOTATIONTYPE_cmn;
    m_currentTempo = MIDI_TEMPO;
    m_currentMeasure = NULL;
    m_meterParams.equivalence = durationEq.at(m_doc->GetOptions()->m_durationEquivalence.GetValue());
}

FunctorCode InitOnsetOffsetFunctor::VisitChordEnd(Chord *chord)
{
    LayerElement *element = chord->ThisOrSameasLink();

    Fraction incrementScoreTime = element->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
    // (each piece of the bar at its tempo: MuseNote RC189)
    const double realTimeIncrementSeconds
        = this->RealTimeSecondsAt(m_currentScoreTime + incrementScoreTime) - m_currentRealTimeSeconds;

    m_currentScoreTime = m_currentScoreTime + incrementScoreTime;
    m_currentRealTimeSeconds += realTimeIncrementSeconds;

    return FUNCTOR_CONTINUE;
}

FunctorCode InitOnsetOffsetFunctor::VisitLayer(Layer *layer)
{
    m_currentScoreTime = 0;
    m_currentRealTimeSeconds = 0.0;

    m_meterParams.mensur = layer->GetCurrentMensur();
    m_meterParams.meterSig = layer->GetCurrentMeterSig();
    m_meterParams.proport = layer->GetCurrentProport();

    return FUNCTOR_CONTINUE;
}

FunctorCode InitOnsetOffsetFunctor::VisitLayerElement(LayerElement *layerElement)
{
    if (layerElement->IsScoreDefElement()) return FUNCTOR_SIBLINGS;

    LayerElement *element = layerElement->ThisOrSameasLink();

    Fraction incrementScoreTime;

    if (element->Is(REST) || element->Is(SPACE)) {
        Rest *rest = element->Is(REST) ? vrv_cast<Rest *>(element) : NULL;
        // A rest inside a tab group is timed by its group, as the group's notes are: the group's end adds the time
        // (VisitTabGrpEnd), and the rest adds none of its own. (It added its own as well: what followed it was played
        // late, and the bar ran long. MuseNote RC156)
        TabGrp *tabGrp = rest ? vrv_cast<TabGrp *>(rest->GetFirstAncestor(TABGRP, MAX_TABGRP_DEPTH)) : NULL;
        if (tabGrp && !rest->HasDur() && !rest->HasDurGes()) {
            incrementScoreTime = tabGrp->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
        }
        else {
            incrementScoreTime = element->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
        }
        // For rests to be possibly added to the timemap (each piece of the bar at its tempo: MuseNote RC189)
        if (rest) {
            const double realTimeIncrementSeconds
                = this->RealTimeSecondsAt(m_currentScoreTime + incrementScoreTime) - m_currentRealTimeSeconds;
            rest->SetScoreTimeOnset(m_currentScoreTime);
            rest->SetRealTimeOnsetSeconds(m_currentRealTimeSeconds);
            rest->SetScoreTimeOffset(m_currentScoreTime + incrementScoreTime);
            rest->SetRealTimeOffsetSeconds(m_currentRealTimeSeconds + realTimeIncrementSeconds);
        }
        if (!tabGrp) {
            m_currentScoreTime = m_currentScoreTime + incrementScoreTime;
            m_currentRealTimeSeconds = this->RealTimeSecondsAt(m_currentScoreTime);
        }
    }
    else if (element->Is(NOTE)) {
        Note *note = vrv_cast<Note *>(element);
        assert(note);

        if (note->IsGraceNote()) {
            // Just store the current onset - only used for grace notes at the end of the layer
            note->SetScoreTimeOnset(m_currentScoreTime);
            note->SetRealTimeOnsetSeconds(m_currentRealTimeSeconds);
            return FUNCTOR_CONTINUE;
        }

        Chord *chord = note->IsChordTone();
        TabGrp *tabGrp = note->IsTabGrpNote();

        // If the note has a @dur or a @dur.ges, take it into account
        // This means that overwriting only @dots or @dots.ges will not be taken into account
        if (chord && !note->HasDur() && !note->HasDurGes()) {
            incrementScoreTime = chord->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
        }
        else if (tabGrp && !note->HasDur() && !note->HasDurGes()) {
            incrementScoreTime = tabGrp->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
        }
        else {
            incrementScoreTime = note->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
        }
        // (each piece of the bar at its tempo: MuseNote RC189)
        const double realTimeIncrementSeconds
            = this->RealTimeSecondsAt(m_currentScoreTime + incrementScoreTime) - m_currentRealTimeSeconds;

        // LogDebug("Note Alignment Duration %f - Dur %d - Diatonic Pitch %d - Track %d", GetAlignmentDuration(),
        // note->GetNoteOrChordDur(element), note->GetDiatonicPitch(), *midiTrack);
        // LogDebug("Oct %d - Pname %d - Accid %d", note->GetOct(), note->GetPname(), note->GetAccid());

        // When we have a @sameas, do store the onset / offset values of the pointed note in the pointing note
        Note *storeNote = (layerElement == element) ? note : dynamic_cast<Note *>(layerElement);
        if (storeNote) {
            storeNote->SetScoreTimeOnset(m_currentScoreTime);
            storeNote->SetRealTimeOnsetSeconds(m_currentRealTimeSeconds);
            storeNote->SetScoreTimeOffset(m_currentScoreTime + incrementScoreTime);
            storeNote->SetRealTimeOffsetSeconds(m_currentRealTimeSeconds + realTimeIncrementSeconds);
        }

        // increase the currentTime accordingly, but only if not in a chord or tabGrp
        if (!note->IsChordTone() && !note->IsTabGrpNote()) {
            m_currentScoreTime = m_currentScoreTime + incrementScoreTime;
            m_currentRealTimeSeconds += realTimeIncrementSeconds;
        }
    }
    else if (element->Is(BEATRPT)) {
        BeatRpt *rpt = vrv_cast<BeatRpt *>(element);
        assert(rpt);

        incrementScoreTime = rpt->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
        rpt->SetScoreTimeOnset(m_currentScoreTime);
        m_currentScoreTime = m_currentScoreTime + incrementScoreTime;
        m_currentRealTimeSeconds = this->RealTimeSecondsAt(m_currentScoreTime);
    }
    else if (layerElement->Is({ BEAM, LIGATURE, FTREM, TUPLET }) && layerElement->HasSameasLink()) {
        incrementScoreTime
            = layerElement->GetSameAsContentAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
        m_currentScoreTime = m_currentScoreTime + incrementScoreTime;
        m_currentRealTimeSeconds = this->RealTimeSecondsAt(m_currentScoreTime);
    }
    else if (layerElement->Is(MENSUR)) {
        this->m_meterParams.mensur = vrv_cast<Mensur *>(layerElement);
    }
    else if (layerElement->Is(METERSIG)) {
        this->m_meterParams.meterSig = vrv_cast<MeterSig *>(layerElement);
    }
    else if (layerElement->Is(PROPORT)) {
        if (layerElement->GetType() == "cmme_tempo_change") return FUNCTOR_SIBLINGS;
        // replace the current proport
        const Proport *previous = (m_meterParams.proport) ? (m_meterParams.proport) : NULL;
        m_meterParams.proport = vrv_cast<Proport *>(layerElement);
        assert(m_meterParams.proport);
        if (previous) {
            m_meterParams.proport->Cumulate(previous);
        }
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode InitOnsetOffsetFunctor::VisitMeasure(Measure *measure)
{
    m_currentTempo = measure->GetCurrentTempo();
    m_currentMeasure = measure;

    return FUNCTOR_CONTINUE;
}

double InitOnsetOffsetFunctor::RealTimeSecondsAt(const Fraction &scoreTime) const
{
    return m_currentMeasure ? m_currentMeasure->GetRealTimeSecondsAt(scoreTime)
                            : scoreTime.ToDouble() * 60.0 / m_currentTempo;
}

FunctorCode InitOnsetOffsetFunctor::VisitStaff(Staff *staff)
{
    // A hidden staff is not timed, as it is not aligned (AlignHorizontallyFunctor): its content is passed over, and
    // a hidden ossia staff has no staffDef (ScoreDefSetOssiaFunctor sets none), which ended the process here
    if (staff->IsHidden()) return FUNCTOR_SIBLINGS;

    StaffDef *drawingStaffDef = staff->m_drawingStaffDef;
    assert(drawingStaffDef);

    m_notationType = (drawingStaffDef->HasNotationtype()) ? drawingStaffDef->GetNotationtype() : NOTATIONTYPE_cmn;

    return FUNCTOR_CONTINUE;
}

FunctorCode InitOnsetOffsetFunctor::VisitTabGrpEnd(TabGrp *tabGrp)
{
    LayerElement *element = tabGrp->ThisOrSameasLink();

    Fraction incrementScoreTime = element->GetAlignmentDuration(m_meterParams, true, m_notationType) * SCORE_TIME_UNIT;
    // (each piece of the bar at its tempo: MuseNote RC189)
    const double realTimeIncrementSeconds
        = this->RealTimeSecondsAt(m_currentScoreTime + incrementScoreTime) - m_currentRealTimeSeconds;

    m_currentScoreTime = m_currentScoreTime + incrementScoreTime;
    m_currentRealTimeSeconds += realTimeIncrementSeconds;

    return FUNCTOR_CONTINUE;
}

//----------------------------------------------------------------------------
// InitMaxMeasureDurationFunctor
//----------------------------------------------------------------------------

InitMaxMeasureDurationFunctor::InitMaxMeasureDurationFunctor() : Functor()
{
    m_currentScoreTime = 0;
    m_currentRealTimeSeconds = 0.0;
    m_currentTempo = MIDI_TEMPO;
    m_tempoAdjustment = 1.0;
    m_multiRestFactor = 1;
    m_currentMeasure = NULL;
}

FunctorCode InitMaxMeasureDurationFunctor::VisitLayerElement(LayerElement *layerElement)
{
    if (layerElement->Is(MULTIREST)) {
        MultiRest *multiRest = vrv_cast<MultiRest *>(layerElement);
        assert(multiRest);
        m_multiRestFactor = multiRest->GetNum();
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode InitMaxMeasureDurationFunctor::VisitMeasure(Measure *measure)
{
    // The bar's tempos: the one in effect at its start, then each change in it (MuseNote RC189)
    m_currentMeasure = measure;
    measure->ClearTempos();
    measure->AddTempo(0, this->GetAdjustedTempo());

    measure->ClearScoreTimeOnset();
    measure->AddScoreTimeOnset(m_currentScoreTime);

    measure->ClearRealTimeOnsetMilliseconds();
    measure->AddRealTimeOnsetMilliseconds(m_currentRealTimeSeconds * 1000.0);

    return FUNCTOR_CONTINUE;
}

FunctorCode InitMaxMeasureDurationFunctor::VisitMeasureEnd(Measure *measure)
{
    // The tempo carried on, the last one met in the bar
    const double tempo = this->GetAdjustedTempo();
    measure->SetCurrentTempo(tempo);

    // Each piece of the bar at its own tempo (MuseNote RC189: the bar was timed by the last one met in it); a multi-bar
    // rest's factor times the whole
    const Fraction barScoreTime = measure->m_measureAligner.GetRightAlignment()->GetTime() * SCORE_TIME_UNIT;
    const Fraction scoreTimeIncrement = barScoreTime * m_multiRestFactor;
    m_currentScoreTime = m_currentScoreTime + scoreTimeIncrement;
    m_currentRealTimeSeconds += measure->GetRealTimeSecondsAt(barScoreTime) * m_multiRestFactor;

    measure->ClearScoreTimeOffset();
    measure->AddScoreTimeOffset(m_currentScoreTime);

    measure->ClearRealTimeOffsetMilliseconds();
    measure->AddRealTimeOffsetMilliseconds(m_currentRealTimeSeconds * 1000.0);

    m_multiRestFactor = 1;
    m_currentMeasure = NULL;

    return FUNCTOR_CONTINUE;
}

FunctorCode InitMaxMeasureDurationFunctor::VisitScoreDef(ScoreDef *scoreDef)
{
    // (as every tempo is read: MuseNote RC191)
    if (const double given = Tempo::PlaysAt(scoreDef, scoreDef); given > 0) {
        m_currentTempo = given;
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode InitMaxMeasureDurationFunctor::VisitTempo(Tempo *tempo)
{
    if (const double given = Tempo::PlaysAt(tempo, tempo); given > 0) {
        m_currentTempo = given;
        // It plays from where it stands in its bar, by its start's alignment (the layout is done before the timemap);
        // one with no place there, from the bar's start (MuseNote RC189)
        if (m_currentMeasure) {
            Fraction offset = 0;
            if (tempo->GetStart() && tempo->GetStart()->GetAlignment()) {
                offset = tempo->GetStart()->GetAlignment()->GetTime() * SCORE_TIME_UNIT;
                if (offset < 0) offset = 0;
            }
            m_currentMeasure->AddTempo(offset, this->GetAdjustedTempo());
        }
    }

    return FUNCTOR_CONTINUE;
}

//----------------------------------------------------------------------------
// InitTimemapTiesFunctor
//----------------------------------------------------------------------------

InitTimemapTiesFunctor::InitTimemapTiesFunctor() : Functor() {}

FunctorCode InitTimemapTiesFunctor::VisitTie(Tie *tie)
{
    Note *note1 = dynamic_cast<Note *>(tie->GetStart());
    Note *note2 = dynamic_cast<Note *>(tie->GetEnd());

    if (!note1 || !note2) {
        return FUNCTOR_CONTINUE;
    }

    Fraction sttd2 = note2->GetScoreTimeTiedDuration();
    Fraction std2 = note2->GetScoreTimeDuration();

    if (sttd2 > 0) {
        note1->SetScoreTimeTiedDuration(sttd2 + std2);
    }
    else {
        note1->SetScoreTimeTiedDuration(std2);
    }
    note2->SetScoreTimeTiedDuration(-1);

    return FUNCTOR_SIBLINGS;
}

//----------------------------------------------------------------------------
// InitTimemapAdjustNotesFunctor
//----------------------------------------------------------------------------

InitTimemapAdjustNotesFunctor::InitTimemapAdjustNotesFunctor() : Functor()
{
    m_noCue = false;
    m_lastNote = NULL;
    m_currentTempo = MIDI_TEMPO;
    m_currentMeasure = NULL;
}

FunctorCode InitTimemapAdjustNotesFunctor::VisitArpeg(Arpeg *arpeg)
{
    // Sort the involved notes by playing order
    const bool playTopDown = (arpeg->GetOrder() == arpegLog_ORDER_down);
    std::set<Note *> notes = arpeg->GetNotes();
    if (notes.empty()) return FUNCTOR_CONTINUE;

    std::vector<Note *> sortedNotes;
    std::copy(notes.begin(), notes.end(), std::back_inserter(sortedNotes));
    std::sort(sortedNotes.begin(), sortedNotes.end(), [playTopDown](const Note *note1, const Note *note2) {
        const int pitch1 = note1->GetMIDIPitch();
        const int pitch2 = note2->GetMIDIPitch();
        return playTopDown ? (pitch1 > pitch2) : (pitch1 < pitch2);
    });

    // Defer the notes in playing order
    Fraction shift = 0;
    Fraction startTime = sortedNotes.front()->GetScoreTimeOnset();
    const Fraction increment = UNACC_GRACENOTE_FRACTION * (int)this->TempoAt(startTime);
    for (Note *note : sortedNotes) {
        if (shift != 0) this->SetNoteStartStop(note, startTime + shift);
        shift = shift + increment;
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode InitTimemapAdjustNotesFunctor::VisitChord(Chord *chord)
{
    if (chord->IsGraceNote()) {
        std::list<Note *> notes;
        const ListOfObjects &chordNotes = chord->GetList();
        for (Object *obj : chordNotes) {
            Note *note = vrv_cast<Note *>(obj);
            assert(note);
            notes.push_back(note);
        }

        m_accentedGraceNote = (chord->GetGrace() == GRACE_acc);
        double time = chord->HasGraceTime() ? chord->GetGraceTime() : 50.0;
        const GraceGrp *graceGrp = vrv_cast<const GraceGrp *>(chord->GetFirstAncestor(GRACEGRP));
        if (graceGrp) {
            if (graceGrp->GetGrace() == GRACE_acc) m_accentedGraceNote = true;
            time = (graceGrp->HasGraceTime()) ? graceGrp->GetGraceTime() : 50.0;
        }
        m_graces.push_back({ notes, chord->GetActualDur(), time });

        return FUNCTOR_SIBLINGS;
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode InitTimemapAdjustNotesFunctor::VisitGraceGrpEnd(GraceGrp *graceGrp)
{
    // Handling of Nachschlag
    if (!m_graces.empty() && (graceGrp->GetAttach() == graceGrpLog_ATTACH_pre) && !m_accentedGraceNote && m_lastNote) {
        Fraction startTime = m_lastNote->GetScoreTimeOffset();
        const Fraction graceNoteDur = UNACC_GRACENOTE_FRACTION * (int)this->TempoAt(startTime);
        const Fraction totalDur = graceNoteDur * (int)m_graces.size();
        startTime = startTime - totalDur;
        startTime = (startTime < 0) ? 0 : startTime;

        for (const auto &grace : m_graces) {
            const Fraction stopTime = startTime + graceNoteDur;
            for (const auto &note : grace.notes) {
                // Set the start (onset) and end (offset) of the grace note
                this->SetNoteStartStop(note, startTime, stopTime);
            }
            startTime = stopTime;
        }

        m_graces.clear();
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode InitTimemapAdjustNotesFunctor::VisitLayerEnd(Layer *layer)
{
    // Process grace notes at the end of the layer - always treat them as "unacc"
    if (!m_graces.empty() && !m_graces.front().notes.empty()) {
        Fraction startTime = m_graces.front().notes.front()->GetScoreTimeOnset();
        Fraction graceNoteDur = UNACC_GRACENOTE_FRACTION * (int)this->TempoAt(startTime);
        const Fraction totalDur = graceNoteDur * (int)m_graces.size();
        startTime = startTime - totalDur;
        startTime = (startTime < 0) ? 0 : startTime;

        // If we have a previous note ending after the start, correct its duration
        if (m_lastNote && (m_lastNote->GetScoreTimeOffset() > startTime)
            && m_lastNote->GetScoreTimeOnset() < startTime) {
            this->SetNoteOrChordStartStop(m_lastNote, m_lastNote->GetScoreTimeOnset(), startTime);
        }

        for (const auto &grace : m_graces) {
            const Fraction stopTime = startTime + graceNoteDur;
            for (const auto &note : grace.notes) {
                // Set the start (onset) and end (offset) of the grace note
                this->SetNoteStartStop(note, startTime, stopTime);
            }
            startTime = stopTime;
        }
        m_graces.clear();
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode InitTimemapAdjustNotesFunctor::VisitMeasure(Measure *measure)
{
    // Update the current tempo
    m_currentTempo = measure->GetCurrentTempo();
    m_currentMeasure = measure;

    return FUNCTOR_CONTINUE;
}

double InitTimemapAdjustNotesFunctor::TempoAt(const Fraction &scoreTime) const
{
    return m_currentMeasure ? m_currentMeasure->GetTempoAt(scoreTime) : m_currentTempo;
}

double InitTimemapAdjustNotesFunctor::RealTimeSecondsAt(const Fraction &scoreTime) const
{
    return m_currentMeasure ? m_currentMeasure->GetRealTimeSecondsAt(scoreTime)
                            : scoreTime.ToDouble() * 60.0 / m_currentTempo;
}

FunctorCode InitTimemapAdjustNotesFunctor::VisitNote(Note *note)
{
    // Skip linked notes
    if (note->HasSameasLink()) {
        return FUNCTOR_SIBLINGS;
    }

    // Skip cue notes when midiNoCue is activated
    if ((note->GetCue() == BOOLEAN_true) && m_noCue) {
        return FUNCTOR_SIBLINGS;
    }

    // If the note is a secondary tied note, then ignore it
    if (note->GetScoreTimeTiedDuration() < 0) {
        return FUNCTOR_SIBLINGS;
    }

    // Handle grace notes
    if (note->IsGraceNote()) {
        m_accentedGraceNote = (note->GetGrace() == GRACE_acc);
        double time = note->HasGraceTime() ? note->GetGraceTime() : 50.0;
        const GraceGrp *graceGrp = vrv_cast<const GraceGrp *>(note->GetFirstAncestor(GRACEGRP));
        if (graceGrp) {
            if (graceGrp->GetGrace() == GRACE_acc) m_accentedGraceNote = true;
            time = (graceGrp->HasGraceTime()) ? graceGrp->GetGraceTime() : 50.0;
        }
        m_graces.push_back({ { note }, note->GetDur(), time });

        return FUNCTOR_SIBLINGS;
    }

    // Check if some grace notes must be performed
    if (!m_graces.empty()) {
        this->SetGraceNotesFor(note);
        m_graces.clear();
    }

    // Store reference, i.e. for Nachschlag
    m_lastNote = note;

    return FUNCTOR_SIBLINGS;
}

void InitTimemapAdjustNotesFunctor::SetGraceNotesFor(Note *refNote)
{
    Fraction startTime = refNote->GetScoreTimeOnset();

    Fraction graceNoteDur = 0;
    if (m_accentedGraceNote && !m_graces.empty()) {
        // Arbitrarily looks at the first note, not sure what to do if we have contradictory values
        double percent = m_graces.front().time;
        // Arbitrarily constraint the time between 5% and 95%
        percent = std::min(95.0, std::max(5.0, percent));
        const Fraction totalDur = refNote->GetScoreTimeDuration() * (int)percent / 100;
        // Adjust the start of the main note
        this->SetNoteOrChordStartStop(refNote, startTime + totalDur);
        graceNoteDur = totalDur / (int)m_graces.size();
    }
    else {
        graceNoteDur = UNACC_GRACENOTE_FRACTION * (int)this->TempoAt(startTime);
        const Fraction totalDur = graceNoteDur * (int)m_graces.size();
        if (startTime >= totalDur) {
            startTime = startTime - totalDur;
        }
        else {
            // Adjust the start of the main note or chord
            this->SetNoteOrChordStartStop(refNote, startTime + totalDur);
        }
    }

    for (const auto &grace : m_graces) {
        const Fraction stopTime = startTime + graceNoteDur;
        for (const auto &note : grace.notes) {
            // Set the start (onset) and end (offset) of the grace note
            this->SetNoteStartStop(note, startTime, stopTime);
        }
        startTime = stopTime;
    }
}

void InitTimemapAdjustNotesFunctor::SetNoteOrChordStartStop(
    Note *note, const Fraction &startTime, const Fraction &stopTime)
{
    if (note->IsChordTone()) {
        Chord *chord = note->IsChordTone();
        ListOfObjects notes = chord->FindAllDescendantsByType(NOTE);
        for (Object *child : notes) {
            Note *chordNote = vrv_cast<Note *>(child);
            this->SetNoteStartStop(chordNote, startTime, stopTime);
        }
    }
    else {
        this->SetNoteStartStop(note, startTime, stopTime);
    }
}

void InitTimemapAdjustNotesFunctor::SetNoteStartStop(Note *note, const Fraction &startTime, const Fraction &stopTime)
{
    assert(note);

    note->SetScoreTimeOnset(startTime);
    // (each piece of the bar at its tempo: MuseNote RC189)
    const double startRealTimeSeconds = this->RealTimeSecondsAt(startTime);
    note->SetRealTimeOnsetSeconds(startRealTimeSeconds);
    if (stopTime != VRV_UNSET) {
        note->SetScoreTimeOffset(stopTime);
        const double stopRealTimeSeconds = this->RealTimeSecondsAt(stopTime);
        note->SetRealTimeOffsetSeconds(stopRealTimeSeconds);
    }
}

//----------------------------------------------------------------------------
// InitMIDIFunctor
//----------------------------------------------------------------------------

InitMIDIFunctor::InitMIDIFunctor() : ConstFunctor()
{
    m_currentTempo = MIDI_TEMPO;
}

FunctorCode InitMIDIFunctor::VisitMeasure(const Measure *measure)
{
    m_currentTempo = measure->GetCurrentTempo();

    return FUNCTOR_CONTINUE;
}

FunctorCode InitMIDIFunctor::VisitOctave(const Octave *octave)
{
    // An octave line without both of its ends is not drawn (View::HasValidTimeSpanningOrder), and
    // GenerateMIDIFunctor::HandleOctave reads the alignment of both: leave it out
    if (!octave->GetStart() || !octave->GetEnd()) return FUNCTOR_CONTINUE;

    const Measure *measure = vrv_cast<const Measure *>(octave->GetFirstAncestor(MEASURE));
    assert(measure);
    std::vector<const Staff *> staffList = octave->GetTstampStaves(measure, octave);
    if (staffList.size() != 1) return FUNCTOR_CONTINUE;
    const Staff *staff = staffList[0];

    const bool raisePitch = (octave->GetDisPlace() != STAFFREL_basic_below);
    int shift = 0;
    switch (octave->GetDis()) {
        case OCTAVE_DIS_8: shift = 1; break;
        case OCTAVE_DIS_15: shift = 2; break;
        case OCTAVE_DIS_22: shift = 3; break;
        default: break;
    }

    const Layer *layer = vrv_cast<const Layer *>(raisePitch ? staff->GetFirst(LAYER) : staff->GetLast(LAYER));
    // A staff with no layer in the octave's measure: there is no note there for it to move
    if (!layer) return FUNCTOR_CONTINUE;

    m_octaves.push_back({ octave, staff->GetN(), layer->GetN(), (raisePitch ? shift : -shift), false });

    return FUNCTOR_CONTINUE;
}

//----------------------------------------------------------------------------
// GenerateMIDIFunctor
//----------------------------------------------------------------------------

GenerateMIDIFunctor::GenerateMIDIFunctor(smf::MidiFile *midiFile) : ConstFunctor()
{
    m_midiFile = midiFile;
    m_midiTrack = 1;
    m_midiChannel = 0;
    m_totalTime = 0.0;
    m_staffN = 0;
    m_transSemi = 0;
    m_octaveShift = 0;
    m_currentTempo = MIDI_TEMPO;
    m_lastNote = NULL;
    m_noCue = false;
    m_controlEvents = false;
    m_instrDef = NULL;
    m_customTuning = NULL;
    m_notePitches = NULL;
}

FunctorCode GenerateMIDIFunctor::VisitBeatRpt(const BeatRpt *beatRpt)
{
    // Sameas not taken into account for now
    AlignMeterParams params;
    double beatLength = beatRpt->GetAlignmentDuration(params).ToDouble() * SCORE_TIME_UNIT;
    double startTime = m_totalTime + beatRpt->GetScoreTimeOnset().ToDouble();
    int tpq = m_midiFile->getTPQ();

    // filter last beat and copy all notes
    smf::MidiEvent event;
    int eventCount = m_midiFile->getEventCount(m_midiTrack);
    for (int i = 0; i < eventCount; ++i) {
        event = m_midiFile->getEvent(m_midiTrack, i);
        if (event.tick > startTime * tpq)
            break;
        else if (event.tick >= (startTime - beatLength) * tpq) {
            if (((event[0] & 0xf0) == 0x80) || ((event[0] & 0xf0) == 0x90)) {
                m_midiFile->addEvent(m_midiTrack, event.tick + beatLength * tpq, event);
            }
        }
    }

    for (int i = 0; i < beatLength * tpq; ++i) {
        // LogWarning("%i", i);
        // smf::MidiEvent event = m_midiFile->getEvent(m_midiTrack, startTime * tpq);
        // event.clearVariables();
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitBTrem(const BTrem *bTrem)
{
    // Do nothing if the tremolo is unmeasured
    if (bTrem->GetForm() == tremForm_FORM_unmeas) {
        return FUNCTOR_CONTINUE;
    }

    // Adjust duration of the bTrem if it's nested within tuplet
    int num = 0;
    const Tuplet *tuplet = vrv_cast<const Tuplet *>(bTrem->GetFirstAncestor(TUPLET, MAX_TUPLET_DEPTH));
    if (tuplet) {
        num = (tuplet->GetNum() > 0) ? tuplet->GetNum() : 0;
    }
    // Get num value if it's set
    if (bTrem->HasNum()) {
        num = std::clamp(bTrem->GetNum(), 0, 256);
    }

    // Calculate duration of individual note in tremolo
    const data_DURATION individualNoteDur = bTrem->CalcIndividualNoteDuration();
    if ((individualNoteDur == DURATION_NONE) || (individualNoteDur > DURATION_2048)) return FUNCTOR_CONTINUE;
    const double noteInQuarterDur = pow(2.0, (DURATION_4 - individualNoteDur));

    // Define lambda which expands one note into multiple individual notes of the same pitch
    auto expandNote = [this, noteInQuarterDur, num](const Object *obj) {
        const Note *note = vrv_cast<const Note *>(obj);
        assert(note);
        // Secondary tied notes are not played, their duration is already added to the first note
        if (note->GetScoreTimeTiedDuration() < 0) return;
        const int pitch = this->GetMIDIPitch(note);
        const double totalInQuarterDur
            = note->GetScoreTimeDuration().ToDouble() + note->GetScoreTimeTiedDuration().ToDouble();
        int multiplicity = totalInQuarterDur / noteInQuarterDur;
        double noteDuration = noteInQuarterDur;
        // if NUM has been set for the bTrem, override calculated values
        if (num) {
            multiplicity = num;
            noteDuration = totalInQuarterDur / double(num);
        }
        if (multiplicity < 1 || totalInQuarterDur <= 0.0) return;
        m_expandedNotes[note] = MIDINoteSequence(multiplicity, { pitch, noteDuration });
    };

    // Apply expansion either to all notes in chord or to first note
    const Chord *chord = vrv_cast<const Chord *>(bTrem->FindDescendantByType(CHORD));
    if (chord) {
        ListOfConstObjects notes = chord->FindAllDescendantsByType(NOTE, false);
        std::for_each(notes.begin(), notes.end(), expandNote);
    }
    else {
        const Object *note = bTrem->FindDescendantByType(NOTE);
        if (note) {
            expandNote(note);
        }
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitChord(const Chord *chord)
{
    this->HandleOctave(chord);

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitFTrem(const FTrem *fTrem)
{
    this->HandleOctave(fTrem);

    if (fTrem->HasUnitdur()) {
        LogWarning("FTrem produces incorrect MIDI output");
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitHalfmRpt(const HalfmRpt *halfmRpt)
{
    LogWarning("HalfmRpt produces empty MIDI output");

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitLayer(const Layer *layer)
{
    if ((layer->GetCue() == BOOLEAN_true) && m_noCue) return FUNCTOR_SIBLINGS;

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitLayerEnd(const Layer *layer)
{
    // stop all previously held notes
    for (auto &[course, held] : m_heldNotes) {
        if (held.m_pitch > 0) {
            m_midiFile->addNoteOff(
                m_midiTrack, std::max(0.0, held.m_stopTime * m_midiFile->getTPQ() - 1), m_midiChannel, held.m_pitch);
        }
    }

    m_heldNotes.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitLayerElement(const LayerElement *layerElement)
{
    if (layerElement->IsScoreDefElement()) return FUNCTOR_SIBLINGS;

    this->HandleOctave(layerElement);

    // Only resolve simple sameas links to avoid infinite recursion
    const LayerElement *sameas = dynamic_cast<const LayerElement *>(layerElement->GetSameasLink());
    if (sameas && !sameas->HasSameasLink()) {
        sameas->Process(*this);
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitMeasure(const Measure *measure)
{
    // Here we need to update the m_totalTime from the starting time of the measure.
    m_totalTime = measure->GetScoreTimeOnset().ToDouble();

    // Each of the bar's tempos at its own tick: the one at its start, then each change in it, so the MIDI plays what the
    // timemap times (MuseNote RC189: the bar's last tempo was written at its first tick). The score's first tempo is
    // its first bar's at tick 0.
    std::vector<std::pair<Fraction, double>> tempos = measure->GetTempos();
    if (tempos.empty()) tempos.push_back({ 0, measure->GetCurrentTempo() });
    for (const auto &[offset, tempo] : tempos) {
        const bool scoreStart = (m_totalTime == 0.0) && (offset == 0);
        if (!scoreStart && (tempo == m_currentTempo)) continue;
        m_currentTempo = tempo;
        const int tick = (m_totalTime + offset.ToDouble()) * m_midiFile->getTPQ();
        // Check if there was already a tempo event added for the given tick
        if (m_tempoEventTicks.insert(tick).second) {
            m_midiFile->addTempo(0, tick, m_currentTempo);
        }
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitMRpt(const MRpt *mRpt)
{
    LogWarning("MRpt produces empty MIDI output");

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitNote(const Note *note)
{
    this->HandleOctave(note);

    // Record the pitch the note is played at, also for the notes skipped below (e.g. secondary tied notes)
    if (m_notePitches) {
        const auto expanded = m_expandedNotes.find(note);
        const bool isExpanded = (expanded != m_expandedNotes.end()) && !expanded->second.empty();
        (*m_notePitches)[note->GetID()] = isExpanded ? expanded->second.front().pitch : this->GetMIDIPitch(note);
    }

    // Skip linked notes
    if (note->HasSameasLink()) {
        return FUNCTOR_SIBLINGS;
    }

    // Skip cue notes when midiNoCue is activated
    if ((note->GetCue() == BOOLEAN_true) && m_noCue) {
        return FUNCTOR_SIBLINGS;
    }

    // If the note is a secondary tied note, then ignore it
    if (note->GetScoreTimeTiedDuration() < 0) {
        return FUNCTOR_SIBLINGS;
    }

    const int velocity = (note->HasVel()) ? note->GetVel() : MIDI_VELOCITY;
    if (!velocity) return FUNCTOR_SIBLINGS;

    const int channel = m_midiChannel;
    const int tpq = m_midiFile->getTPQ();
    double startTime = m_totalTime + note->GetScoreTimeOnset().ToDouble();

    // Check if note is deferred
    if (m_deferredNotes.find(note) != m_deferredNotes.end()) {
        startTime += m_deferredNotes.at(note);
        m_deferredNotes.erase(note);
    }

    // Check if note was expanded into sequence of short notes due to trills/tremolandi
    // Play either the expanded note sequence or a single note
    if (m_expandedNotes.find(note) != m_expandedNotes.end()) {
        for (const auto &midiNote : m_expandedNotes.at(note)) {
            const double stopTime = startTime + midiNote.duration;

            m_midiFile->addNoteOn(m_midiTrack, startTime * tpq, channel, midiNote.pitch, velocity);
            m_midiFile->addNoteOff(m_midiTrack, std::max(0.0, stopTime * tpq - 1), channel, midiNote.pitch);

            startTime = stopTime;
        }
    }
    else {
        const int pitch = this->GetMIDIPitch(note);

        if (note->HasTabCourse() && (note->GetTabCourse() >= 1)) {
            // Tablature 'rule of holds'.  A note on a course is held until the next note
            // on that course is required, or until a default hold duration is reached.

            const int course = note->GetTabCourse();
            MIDIHeldNote &onCourse = m_heldNotes[course];

            // if a previously held note on this course is already sounding, end it now.
            if (onCourse.m_pitch > 0) {
                onCourse.m_stopTime = startTime; // stop now
            }

            // end all previously held notes that have reached their stoptime
            // or if the new pitch is already sounding, on any course
            for (auto &[heldCourse, held] : m_heldNotes) {
                if ((held.m_pitch > 0) && ((held.m_stopTime <= startTime) || (held.m_pitch == pitch))) {
                    m_midiFile->addNoteOff(
                        m_midiTrack, std::max(0.0, held.m_stopTime * tpq - 1), channel, held.m_pitch);
                    held.m_pitch = 0;
                    held.m_stopTime = 0;
                }
            }

            // hold this note until the greater of its rhythm sign and the default duration.
            // TODO optimize the default hold duration
            const double defaultHoldTime = 4; // quarter notes
            onCourse.m_pitch = pitch;
            onCourse.m_stopTime = m_totalTime
                + std::max(defaultHoldTime,
                    note->GetScoreTimeOffset().ToDouble() + note->GetScoreTimeTiedDuration().ToDouble());

            // start this note
            m_midiFile->addNoteOn(m_midiTrack, startTime * tpq, channel, pitch, velocity);
        }
        else {
            const double stopTime
                = m_totalTime + note->GetScoreTimeOffset().ToDouble() + note->GetScoreTimeTiedDuration().ToDouble();

            m_midiFile->addNoteOn(m_midiTrack, startTime * tpq, channel, pitch, velocity);
            m_midiFile->addNoteOff(m_midiTrack, std::max(0.0, stopTime * tpq - 1), channel, pitch);
        }
    }

    // Store reference, i.e. for Nachschlag
    m_lastNote = note;

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitPedal(const Pedal *pedal)
{
    if (!pedal->HasDir()) return FUNCTOR_CONTINUE;

    // Check the functor flag - filters should always be there, but just in case we change how it is called
    if (!m_controlEvents || !this->GetFilters()) return FUNCTOR_CONTINUE;

    // Check if the pedal applies to the staff filtered
    const Measure *measure = vrv_cast<const Measure *>(pedal->GetFirstAncestor(MEASURE));
    assert(measure);
    std::vector<const Staff *> staffList = pedal->GetTstampStaves(measure, pedal);
    bool applies = false;
    for (const Staff *staff : staffList) {
        applies = (applies || this->GetFilters()->Apply(staff));
    }
    if (!applies) return FUNCTOR_CONTINUE;

    // A pedal whose start was not found (a @startid that names nothing) has no time to be played at
    if (!pedal->GetStart() || !pedal->GetStart()->GetAlignment()) return FUNCTOR_CONTINUE;

    double pedalTime = pedal->GetStart()->GetAlignment()->GetTime().ToDouble() * SCORE_TIME_UNIT;
    double startTime = m_totalTime + pedalTime;
    int tpq = m_midiFile->getTPQ();

    // todo: check pedal @func to switch between sustain/soften/damper pedals?
    switch (pedal->GetDir()) {
        case pedalLog_DIR_down: m_midiFile->addSustainPedalOn(m_midiTrack, (startTime * tpq), m_midiChannel); break;
        case pedalLog_DIR_up: m_midiFile->addSustainPedalOff(m_midiTrack, (startTime * tpq), m_midiChannel); break;
        case pedalLog_DIR_bounce:
            m_midiFile->addSustainPedalOff(m_midiTrack, (startTime * tpq), m_midiChannel);
            m_midiFile->addSustainPedalOn(m_midiTrack, (startTime * tpq) + 0.1, m_midiChannel);
            break;
        default: return FUNCTOR_CONTINUE;
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitScoreDef(const ScoreDef *scoreDef)
{
    double totalTime = m_totalTime;
    // check next measure for the time offset
    const Object *parent = scoreDef->GetParent();
    if (parent && (parent->GetLast() != scoreDef)) {
        const Object *next = parent->GetNext(scoreDef);
        if (next && next->Is(MEASURE)) {
            const Measure *nextMeasure = vrv_cast<const Measure *>(next);
            totalTime = nextMeasure->GetScoreTimeOnset().ToDouble();
        }
    }
    const double currentTick = totalTime * m_midiFile->getTPQ();

    smf::MidiEvent midiEvent;
    midiEvent.tick = currentTick;
    // calculate reference pitch class based on @tune.pname
    int referencePitchClass = 0;
    if (scoreDef->HasTunePname()) {
        referencePitchClass = Note::PnameToPclass(scoreDef->GetTunePname());
    }
    // set and remember custom tuning if available
    if (scoreDef->GetCustomTuning().IsValid()) {
        m_customTuning = &scoreDef->GetCustomTuning();
        const int program = m_instrDef && m_instrDef->HasMidiInstrnum() ? m_instrDef->GetMidiInstrnum() : 0;
        const Tunings::Tuning &tuneCustom = scoreDef->GetCustomTuning().GetTuning();
        std::vector<std::pair<int, double>> mapping;
        // sysex messages should not have the high bit (0x80) set, which means that the full 128 MIDI notes
        // cannot be retuned in the same sysex message. For now, we skip MIDI key 0.
        for (int i = 1; i < 128; i++) {
            mapping.push_back(std::make_pair(i, tuneCustom.frequencyForMidiNote(i)));
        }
        midiEvent.makeMts2_KeyTuningsByFrequency(mapping, program);
        m_midiFile->addEvent(m_midiTrack, midiEvent);
    }
    // set temperament event if corresponding attribute present
    else if (scoreDef->HasTuneTemper()) {
        switch (scoreDef->GetTuneTemper()) {
            case TEMPERAMENT_equal: midiEvent.makeTemperamentEqual(referencePitchClass); break;
            case TEMPERAMENT_just: midiEvent.makeTemperamentBad(100.0, referencePitchClass); break;
            case TEMPERAMENT_mean: midiEvent.makeTemperamentMeantone(referencePitchClass); break;
            case TEMPERAMENT_pythagorean: midiEvent.makeTemperamentPythagorean(referencePitchClass); break;
            default: break;
        }
        m_midiFile->addEvent(m_midiTrack, midiEvent);
    }
    // set tuning
    if (scoreDef->HasTuneHz()) {
        const double tuneHz = scoreDef->GetTuneHz();
        // Add tuning for all keys from 0 to 127
        std::vector<std::pair<int, double>> tuneFrequencies;
        for (int i = 0; i < 127; ++i) {
            double freq = pow(2.0, (i - 69.0) / 12.0) * tuneHz;
            tuneFrequencies.push_back(std::make_pair(i, freq));
        }
        midiEvent.makeMts2_KeyTuningsByFrequency(tuneFrequencies);
        m_midiFile->addEvent(m_midiTrack, midiEvent);
    }
    // set MIDI key signature
    if (scoreDef->HasKeySigInfo()) {
        const KeySig *keySig = vrv_cast<const KeySig *>(scoreDef->GetKeySig());
        if (keySig && keySig->HasSig()) {
            // m_midiFile->addKeySignature(
            //     m_midiTrack, currentTick, keySig->GetFifthsInt(), (keySig->GetMode() == MODE_minor));
        }
    }
    // set MIDI time signature
    if (scoreDef->HasMeterSigInfo()) {
        const MeterSig *meterSig = vrv_cast<const MeterSig *>(scoreDef->GetMeterSig());
        if (meterSig && meterSig->HasCount() && meterSig->HasUnit()) {
            m_midiFile->addTimeSignature(m_midiTrack, currentTick, meterSig->GetTotalCount(), meterSig->GetUnit());
        }
        else if (meterSig && meterSig->HasUnit()) {
            m_midiFile->addTimeSignature(
                m_midiTrack, currentTick, meterSig->GetTotalCount(), meterSig->GetSymImplicitUnit());
        }
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitStaff(const Staff *staff)
{
    if (staff->IsOssia()) return FUNCTOR_SIBLINGS;

    m_expandedNotes.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitStaffDef(const StaffDef *staffDef)
{
    if (staffDef->GetN() == m_staffN) {
        // Update the semitone transposition
        if (staffDef->HasTransSemi()) m_transSemi = staffDef->GetTransSemi();
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateMIDIFunctor::VisitSyl(const Syl *syl)
{
    if (syl->IsEmpty()) return FUNCTOR_CONTINUE;

    const Note *note = NULL;
    if (syl->GetFirstAncestor(CHORD)) {
        const Chord *parentChord = vrv_cast<const Chord *>(syl->GetFirstAncestor(CHORD));
        note = vrv_cast<const Note *>(parentChord->GetFirst(NOTE));
    }
    else {
        note = vrv_cast<const Note *>(syl->GetFirstAncestor(NOTE));
    }
    if (!note) return FUNCTOR_CONTINUE;

    const double startTime = m_totalTime + note->GetScoreTimeOnset().ToDouble();
    const std::string sylText = UTF32to8(syl->GetText());

    m_midiFile->addLyric(m_midiTrack, startTime * m_midiFile->getTPQ(), sylText);

    return FUNCTOR_SIBLINGS;
}

FunctorCode GenerateMIDIFunctor::VisitVerse(const Verse *verse)
{
    const LayerElement *parent = vrv_cast<const Note *>(verse->GetFirstAncestor(NOTE));
    if (!parent) parent = vrv_cast<const Chord *>(verse->GetFirstAncestor(CHORD));
    assert(parent);

    const Verse *previousVerse = vrv_cast<const Verse *>(parent->GetPrevious(verse, VERSE));

    if (previousVerse) return FUNCTOR_SIBLINGS;

    return FUNCTOR_CONTINUE;
}

void GenerateMIDIFunctor::DeferMIDINote(const Note *refNote, double shift, bool includeChordSiblings)
{
    // Recursive call for chords
    const Chord *chord = refNote->IsChordTone();
    if (chord && includeChordSiblings) {
        const ListOfConstObjects &notes = chord->GetList();

        for (const Object *obj : notes) {
            const Note *note = vrv_cast<const Note *>(obj);
            assert(note);
            this->DeferMIDINote(note, shift, false);
        }
        return;
    }

    // Register the shift
    if (shift < refNote->GetScoreTimeDuration().ToDouble() + refNote->GetScoreTimeTiedDuration().ToDouble()) {
        m_deferredNotes[refNote] = shift;
    }
}

void GenerateMIDIFunctor::HandleOctave(const LayerElement *layerElement)
{
    if (!layerElement->GetAlignment()) return;

    const auto isBefore = [](const LayerElement *element, const LayerElement *other) {
        const Alignment &alignment = *element->GetAlignment();
        const Alignment &otherAlignment = *other->GetAlignment();
        if (alignment == otherAlignment) return element->IsGraceNote() && !other->IsGraceNote();
        return alignment < otherAlignment;
    };

    // Handle octave end
    for (auto octaveIter = m_octaves.begin(); octaveIter != m_octaves.end();) {
        if (octaveIter->isActive && (octaveIter->staffN == m_staffN) && (octaveIter->layerN == m_layerN)) {
            const LayerElement *end = octaveIter->octave->GetEnd();
            if (end->GetAlignment() && isBefore(end, layerElement)) {
                m_octaveShift -= octaveIter->octaveShift;
                octaveIter = m_octaves.erase(octaveIter);
                continue;
            }
        }
        ++octaveIter;
    }

    // Handle octave begin
    for (OctaveInfo &octave : m_octaves) {
        if (!octave.isActive && (octave.staffN == m_staffN) && (octave.layerN == m_layerN)) {
            const LayerElement *start = octave.octave->GetStart();
            const LayerElement *end = octave.octave->GetEnd();
            if (start->GetAlignment() && end->GetAlignment() && !isBefore(layerElement, start)
                && !isBefore(end, layerElement)) {
                m_octaveShift += octave.octaveShift;
                octave.isActive = true;
            }
        }
    }
}

int GenerateMIDIFunctor::GetMIDIPitch(const Note *note)
{
    // The MIDI key of unpitched notes is neither transposed nor tuned
    if (note->HasPnum() && note->HasLoc() && !note->HasPname()) return note->GetPnum();
    if (m_customTuning && m_customTuning->IsValid()) {
        return m_customTuning->GetMIDIPitch(note, m_transSemi, m_octaveShift);
    }
    return note->GetMIDIPitch(m_transSemi, m_octaveShift);
}

//----------------------------------------------------------------------------
// GenerateTimemapFunctor
//----------------------------------------------------------------------------

GenerateTimemapFunctor::GenerateTimemapFunctor(Timemap *timemap) : ConstFunctor()
{
    m_currentScoreTime = 0;
    m_currentRealTimeMilliseconds = 0.0;
    m_currentTempo = MIDI_TEMPO;
    m_noCue = false;
    m_timemap = timemap;
    m_currentMeasure = NULL;
}

FunctorCode GenerateTimemapFunctor::VisitLayerElement(const LayerElement *layerElement)
{
    if (layerElement->IsScoreDefElement()) return FUNCTOR_SIBLINGS;

    // Only resolve simple sameas links to avoid infinite recursion
    const LayerElement *sameas = dynamic_cast<const LayerElement *>(layerElement->GetSameasLink());
    if (sameas && !sameas->HasSameasLink()) {
        sameas->Process(*this);
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateTimemapFunctor::VisitMeasure(const Measure *measure)
{
    m_currentScoreTime = measure->GetScoreTimeOnset();
    m_currentRealTimeMilliseconds = measure->GetRealTimeOnsetMilliseconds();
    m_currentTempo = measure->GetCurrentTempo();
    m_currentMeasure = measure;

    this->AddTimemapEntry(measure);

    // Each change of tempo in the bar has its own place in the timemap, where no note may start (MuseNote RC189); a
    // mark that gives the tempo already playing changes nothing
    double before = 0.0;
    for (const auto &[offset, tempo] : measure->GetTempos()) {
        const bool same = (tempo == before);
        before = tempo;
        if ((offset == 0) || same) continue;
        TimemapEntry &entry = m_timemap->GetEntry(m_currentScoreTime + offset);
        entry.tstamp = round(m_currentRealTimeMilliseconds + measure->GetRealTimeSecondsAt(offset) * 1000.0);
        entry.tempo = tempo;
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode GenerateTimemapFunctor::VisitMRest(const MRest *mRest)
{
    this->AddTimemapEntry(mRest);

    return FUNCTOR_SIBLINGS;
}

FunctorCode GenerateTimemapFunctor::VisitMultiRest(const MultiRest *multiRest)
{
    this->AddTimemapEntry(multiRest);

    return FUNCTOR_SIBLINGS;
}

FunctorCode GenerateTimemapFunctor::VisitNote(const Note *note)
{
    // Skip cue notes when midiNoCue is activated
    if ((note->GetCue() == BOOLEAN_true) && m_noCue) {
        return FUNCTOR_SIBLINGS;
    }

    note = dynamic_cast<const Note *>(note->ThisOrSameasLink());
    assert(note);

    this->AddTimemapEntry(note);

    return FUNCTOR_SIBLINGS;
}

FunctorCode GenerateTimemapFunctor::VisitRest(const Rest *rest)
{
    this->AddTimemapEntry(rest);

    return FUNCTOR_SIBLINGS;
}

FunctorCode GenerateTimemapFunctor::VisitStaff(const Staff *staff)
{
    if (staff->IsOssia()) return FUNCTOR_SIBLINGS;

    return FUNCTOR_CONTINUE;
}

void GenerateTimemapFunctor::AddTimemapEntry(const Object *object)
{
    assert(object);

    if (object->Is({ NOTE, REST })) {
        const DurationInterface *interface = object->GetDurationInterface();
        assert(interface);

        Fraction scoreTimeStart = m_currentScoreTime + interface->GetScoreTimeOnset();
        double realTimeStart = round(m_currentRealTimeMilliseconds + interface->GetRealTimeOnsetMilliseconds());

        Fraction scoreTimeEnd = m_currentScoreTime + interface->GetScoreTimeOffset();
        double realTimeEnd = round(m_currentRealTimeMilliseconds + interface->GetRealTimeOffsetMilliseconds());

        bool isRest = (object->Is(REST));

        /*********** start values ***********/

        TimemapEntry &startEntry = m_timemap->GetEntry(scoreTimeStart);

        // Should check if value for realTimeStart already exists and if so, then
        // ensure that it is equal to scoreTimeStart:
        startEntry.tstamp = realTimeStart;

        // Store the element ID in list to turn on at given time - note or rest
        if (!isRest) startEntry.notesOn.push_back(object->GetID());
        if (isRest) startEntry.restsOn.push_back(object->GetID());

        // Also add the tempo, the one at its onset in its bar (MuseNote RC189: the bar's last one)
        startEntry.tempo
            = m_currentMeasure ? m_currentMeasure->GetTempoAt(interface->GetScoreTimeOnset()) : m_currentTempo;

        /*********** end values ***********/

        TimemapEntry &endEntry = m_timemap->GetEntry(scoreTimeEnd);

        // Should check if value for realTimeEnd already exists and if so, then
        // ensure that it is equal to scoreTimeEnd:
        endEntry.tstamp = realTimeEnd;

        // Store the element ID in list to turn off at given time - notes or rest
        if (!isRest) endEntry.notesOff.push_back(object->GetID());
        if (isRest) endEntry.restsOff.push_back(object->GetID());
    }
    else if (object->Is(MEASURE)) {
        const Measure *measure = vrv_cast<const Measure *>(object);
        assert(measure);

        // Deal with repeated music later, for now get the last times.
        Fraction scoreTimeStart = m_currentScoreTime;
        double realTimeStart = round(m_currentRealTimeMilliseconds);

        TimemapEntry &startEntry = m_timemap->GetEntry(scoreTimeStart);

        // Should check if value for realTimeStart already exists and if so, then
        // ensure that it is equal to scoreTimeStart:
        startEntry.tstamp = realTimeStart;

        // Add the measureOn
        startEntry.measureOn = measure->GetID();
    }
    else if (object->Is({ MREST, MULTIREST })) {
        // Get the ancestor measure
        const Measure *measure = vrv_cast<const Measure *>(object->GetFirstAncestor(MEASURE));
        assert(measure);

        Fraction scoreTimeStart = m_currentScoreTime;
        double realTimeStart = round(m_currentRealTimeMilliseconds);
        Fraction scoreTimeEnd = measure->GetScoreTimeOffset();
        double realTimeEnd = round(measure->GetRealTimeOffsetMilliseconds());

        /*********** start values ***********/

        TimemapEntry &startEntry = m_timemap->GetEntry(scoreTimeStart);

        // Should check if a value for realTimeStart already exists and if so, then
        // ensure that it is equal to scoreTimeStart:
        startEntry.tstamp = realTimeStart;

        // Store the element ID in list to turn on at given time
        startEntry.restsOn.push_back(object->GetID());

        /*********** end values ***********/

        TimemapEntry &endEntry = m_timemap->GetEntry(scoreTimeEnd);

        endEntry.tstamp = realTimeEnd;

        // Store the element ID in list to turn off at given time
        endEntry.restsOff.push_back(object->GetID());
    }
}

//----------------------------------------------------------------------------
// GenerateFeaturesFunctor
//----------------------------------------------------------------------------

GenerateFeaturesFunctor::GenerateFeaturesFunctor(FeatureExtractor *extractor) : ConstFunctor()
{
    m_extractor = extractor;
}

FunctorCode GenerateFeaturesFunctor::VisitObject(const Object *object)
{
    m_extractor->Extract(object);

    return FUNCTOR_CONTINUE;
}

} // namespace vrv
