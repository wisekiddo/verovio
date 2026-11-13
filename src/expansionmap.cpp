/////////////////////////////////////////////////////////////////////////////
// Name:        expansionmap.cpp
// Author:      Werner Goebl
// Created:     2019
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "expansionmap.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <cassert>
#include <climits>
#include <optional>
#include <sstream>
#include <iostream>

//----------------------------------------------------------------------------

#include "dir.h"
#include "editorial.h"
#include "ending.h"
#include "expansion.h"
#include "lem.h"
#include "linkinginterface.h"
#include "measure.h"
#include "plistinterface.h"
#include "rdg.h"
#include "repeatmark.h"
#include "score.h"
#include "section.h"
#include "timeinterface.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// OrderOfPlay
//----------------------------------------------------------------------------

bool OrderOfPlay::Add(const std::string &ref)
{
    if (m_leastCost > s_maxCost) return false;
    m_refs.push_back(ref);
    // named t times, then once more: (t + 1) squared is 2t + 1 more than t squared
    m_leastCost += 2 * m_timesNamed[ref]++ + 1;
    return true;
}

//----------------------------------------------------------------------------
// ExpansionMap
//----------------------------------------------------------------------------

ExpansionMap::ExpansionMap()
{
    this->Reset();
}

ExpansionMap::~ExpansionMap() {}

void ExpansionMap::Reset()
{
    m_map.clear();
    m_isProcessed = false;
}

Object *ExpansionMap::Expand(Expansion *expansion, xsdAnyURI_List &existingList, Object *prevSect,
    xsdAnyURI_List &deletionList, bool deleteList = false)
{
    assert(expansion);
    Object *parent = expansion->GetParent();
    assert(parent);

    xsdAnyURI_List expansionPlist = expansion->GetPlist();
    if (expansionPlist.empty()) {
        LogWarning("ExpansionMap::Expand: Expansion element %s has empty @plist. Nothing expanded.",
            expansion->GetID().c_str());
        return prevSect;
    }

    assert(prevSect);
    assert(prevSect->GetParent());

    Object *insertHere = nullptr; // cloned parent container

    // If expansion parent already exists, create a new empty such element
    if (std::find(existingList.begin(), existingList.end(), parent->GetID()) != existingList.end()) {
        Object *newContainer;
        // check type of expansion parent
        if (parent->Is(SECTION)) {
            newContainer = static_cast<Object *>(new Section());
        }
        else if (parent->Is(ENDING)) {
            newContainer = static_cast<Object *>(new Ending());
        }
        else if (parent->Is(LEM)) {
            newContainer = static_cast<Object *>(new Lem());
        }
        else if (parent->Is(RDG)) {
            newContainer = static_cast<Object *>(new Rdg());
        }
        else {
            LogWarning(
                "ExpansionMap::Expand: Expansion element %s has unsupported parent type.", expansion->GetID().c_str());
            return prevSect;
        }

        assert(parent->GetParent());
        Object *referenceChild = parent->GetDirectChild(parent->GetParent(), prevSect);
        assert(referenceChild);
        parent->GetParent()->InsertAfter(referenceChild, newContainer);
        GeneratePredictableIDs(parent, newContainer);
        LogDebug("Creating new container <%s> for expansion element %s", newContainer->GetClassName().c_str(),
            newContainer->GetID().c_str());

        insertHere = newContainer;
    }
    else if (std::find(existingList.begin(), existingList.end(), parent->GetID()) == existingList.end()) {
        existingList.push_back(parent->GetID());
    }

    // find and add all relevant (and new) expansion sibling ids to deletionList
    for (Object *siblings : parent->GetChildren()) {
        if (siblings->Is({ SECTION, ENDING, LEM, RDG })
            && std::count(deletionList.begin(), deletionList.end(), siblings->GetID()) == 0) {
            deletionList.push_back(siblings->GetID());
        }
    }

    // iterate over expansion plist
    for (std::string id : expansionPlist) {
        LogDebug("Looking for element in @plist: %s", id.c_str());
        if (id.rfind("#", 0) == 0) id = id.substr(1, id.size() - 1); // remove leading hash from id
        Object *currSect = parent->FindDescendantByID(id); // find section pointer for id string
        if (currSect == NULL) {
            // Warn about referenced element not found and continue
            LogWarning("ExpansionMap::Expand: Element referenced in @plist not found: %s", id.c_str());
            continue;
        }
        // An order plays sections, endings, measures and what an edition marks of them (a reading, a choice and
        // what it chooses between, an addition...), and nothing inside a measure: anything else is passed over as
        // an element that is not there. (A layer named again after a section was put into the section, where the
        // timemap ended the process)
        if (!(currSect->Is({ SECTION, ENDING, MEASURE, EXPANSION }) || currSect->IsEditorialElement())
            || currSect->GetFirstAncestor(MEASURE)) {
            LogWarning("ExpansionMap::Expand: Element referenced in @plist is not played: %s", id.c_str());
            continue;
        }
        if (currSect->Is(EXPANSION)) { // if id is itself an expansion, resolve it recursively
            Expansion *currExpansion = vrv_cast<Expansion *>(currSect);
            assert(currExpansion);
            // An expansion that names itself, or one that is being expanded, has no end: it is passed over as an
            // element that is not there. (It was followed until the stack ran out)
            if ((currExpansion == expansion)
                || (std::find(m_expanding.begin(), m_expanding.end(), currExpansion) != m_expanding.end())) {
                LogWarning("ExpansionMap::Expand: Expansion in @plist named within itself: %s", id.c_str());
                continue;
            }
            m_expanding.push_back(expansion);
            prevSect = this->Expand(currExpansion, existingList, prevSect, deletionList);
            m_expanding.pop_back();
        }
        else {
            // id already in existingList or currSect is not in expansion parent, clone object, update ids, insert it
            if (std::find(existingList.begin(), existingList.end(), id) != existingList.end()
                || (insertHere != NULL && currSect->GetParent() != insertHere)) {

                // clone current section/ending/rdg/lem and rename it, adding -"rend2" for the first repetition etc.
                Object *clonedObject = currSect->Clone();
                clonedObject->CloneReset();
                this->GeneratePredictableIDs(currSect, clonedObject);

                // get IDs of old and new sections and add them to m_map
                std::vector<std::string> oldIds;
                oldIds.push_back(currSect->GetID());
                this->GetIDList(currSect, oldIds);
                std::vector<std::string> clonedIds;
                clonedIds.push_back(clonedObject->GetID());
                this->GetIDList(clonedObject, clonedIds);
                for (int i = 0; (i < (int)oldIds.size()) && (i < (int)clonedIds.size()); ++i) {
                    this->AddExpandedIDToExpansionMap(oldIds.at(i), clonedIds.at(i));
                }

                // go through cloned objects, find TimePointing/SpanningInterface, PListInterface, LinkingInterface
                this->UpdateIDs(clonedObject);

                LogDebug("Cloning element in @plist: %s", clonedObject->GetID().c_str());

                if (insertHere != NULL) {
                    insertHere->AddChild(clonedObject); // add to new container, if it exists
                }
                else {
                    prevSect->GetParent()->InsertAfter(prevSect, clonedObject); // or add after previous section
                }

                prevSect = clonedObject;
                existingList.push_back(clonedObject->GetID());
            }
            else { // add to existingList, remember previous element, re-order if necessary

                bool moveCurrentElement = false;
                int prevIdx = prevSect->GetIdx();
                int childCount = prevSect->GetParent()->GetChildCount();
                int currIdx = currSect->GetIdx();

                // check re-order when within same parent
                if (currSect->GetParent()->GetID() == prevSect->GetParent()->GetID()) {
                    // If prevSect has a next element and if it is different than the currSect or has no next element,
                    // move it to after the currSect.
                    if (prevIdx < childCount - 1) {
                        Object *nextElement = prevSect->GetParent()->GetChild(prevIdx + 1);
                        assert(nextElement);
                        if (nextElement->Is({ SECTION, ENDING, LEM, RDG }) && nextElement != currSect) {
                            moveCurrentElement = true;
                        }
                    }
                    else {
                        moveCurrentElement = true;
                    }
                }

                // move prevSect to after currSect
                if (moveCurrentElement && currIdx < prevIdx && prevIdx < childCount) {
                    LogDebug(
                        "Re-ordering element %s to after %s", currSect->GetID().c_str(), prevSect->GetID().c_str());
                    currSect->GetParent()->RotateChildren(currIdx, currIdx + 1, prevIdx + 1);
                }
                else {
                    LogDebug("Leaving existing element %s", currSect->GetID().c_str());
                }

                prevSect = currSect;
                existingList.push_back(id);
            }
        }
    }

    // at the very end, remove unused sections from structure if not in existingList
    if (deleteList) {
        for (std::string del : deletionList) {
            long cnt = std::count(existingList.begin(), existingList.end(), del);
            if (cnt == 0) {
                Object *currSect = parent->FindDescendantByID(del); // find section pointer for id string
                // Removed already, with a section that holds it and was not played either (it was read)
                if (!currSect) continue;

                int idx = currSect->GetIdx();
                LogDebug("ExpansionMap::Expand: Removing unused section/ending/rdg/lem with id %s", del.c_str());
                currSect->GetParent()->DetachChild(idx);
            }
        }
    }

    return prevSect;
}

bool ExpansionMap::UpdateIDs(Object *object)
{
    for (Object *o : object->GetChildren()) {
        o->IsExpansion(true);
        if (o->HasInterface(INTERFACE_TIME_POINT)) {
            TimePointInterface *interface = o->GetTimePointInterface();
            assert(interface);
            // @startid
            std::string oldStartId = interface->GetStartid();
            if (oldStartId.rfind("#", 0) == 0) oldStartId = oldStartId.substr(1, oldStartId.size() - 1);
            std::string newStartId = this->GetExpansionIDsForElement(oldStartId).back();
            if (!newStartId.empty()) interface->SetStartid("#" + newStartId);
        }
        if (o->HasInterface(INTERFACE_TIME_SPANNING)) {
            TimeSpanningInterface *interface = o->GetTimeSpanningInterface();
            assert(interface);
            // @startid
            std::string oldStartId = interface->GetStartid();
            if (oldStartId.rfind("#", 0) == 0) oldStartId = oldStartId.substr(1, oldStartId.size() - 1);
            std::string newStartId = this->GetExpansionIDsForElement(oldStartId).back();
            if (!newStartId.empty()) interface->SetStartid("#" + newStartId);
            // @endid
            oldStartId = interface->GetEndid();
            if (oldStartId.rfind("#", 0) == 0) oldStartId = oldStartId.substr(1, oldStartId.size() - 1);
            std::string newEndId = this->GetExpansionIDsForElement(oldStartId).back();
            if (!newEndId.empty()) interface->SetEndid("#" + newEndId);
        }
        if (o->HasInterface(INTERFACE_PLIST)) {
            PlistInterface *interface = o->GetPlistInterface(); // @plist
            assert(interface);
            xsdAnyURI_List oldList = interface->GetPlist();
            xsdAnyURI_List newList;
            for (std::string oldRefString : oldList) {
                if (oldRefString.rfind("#", 0) == 0) oldRefString = oldRefString.substr(1, oldRefString.size() - 1);
                newList.push_back("#" + this->GetExpansionIDsForElement(oldRefString).back());
            }
            interface->SetPlist(newList);
        }
        else if (o->HasInterface(INTERFACE_LINKING)) {
            LinkingInterface *interface = o->GetLinkingInterface();
            assert(interface);
            // @sameas
            std::string oldIdString = interface->GetSameas();
            if (oldIdString.rfind("#", 0) == 0) oldIdString = oldIdString.substr(1, oldIdString.size() - 1);
            std::string newIdString = this->GetExpansionIDsForElement(oldIdString).back();
            if (!newIdString.empty()) interface->SetSameas("#" + newIdString);
            // @next
            oldIdString = interface->GetNext();
            if (oldIdString.rfind("#", 0) == 0) oldIdString = oldIdString.substr(1, oldIdString.size() - 1);
            newIdString = this->GetExpansionIDsForElement(oldIdString).back();
            if (!newIdString.empty()) interface->SetNext("#" + newIdString);
            // @prev
            oldIdString = interface->GetPrev();
            if (oldIdString.rfind("#", 0) == 0) oldIdString = oldIdString.substr(1, oldIdString.size() - 1);
            newIdString = this->GetExpansionIDsForElement(oldIdString).back();
            if (!newIdString.empty()) interface->SetPrev("#" + newIdString);
            // @copyof
            oldIdString = interface->GetCopyof();
            if (oldIdString.rfind("#", 0) == 0) oldIdString = oldIdString.substr(1, oldIdString.size() - 1);
            newIdString = this->GetExpansionIDsForElement(oldIdString).back();
            if (!newIdString.empty()) interface->SetCopyof("#" + newIdString);
            // @synch
            oldIdString = interface->GetSynch();
            if (oldIdString.rfind("#", 0) == 0) oldIdString = oldIdString.substr(1, oldIdString.size() - 1);
            newIdString = this->GetExpansionIDsForElement(oldIdString).back();
            if (!newIdString.empty()) interface->SetSynch("#" + newIdString);
            // @corresp is already handle by the Object::Clone and LinkingInterface::AddBackLink
        }
        this->UpdateIDs(o);
    }
    return true;
}

bool ExpansionMap::AddExpandedIDToExpansionMap(const std::string &origXmlId, std::string newXmlId)
{
    auto list = m_map.find(origXmlId);
    if (list != m_map.end()) {
        list->second.push_back(newXmlId); // add to existing key
        for (std::string s : list->second) {
            // (Every key but the one the copy is of: a section that holds a copy is a copy's copy, whose own list
            // was written to while it was read, and the original's was not written)
            if (s != origXmlId && s != list->second.back()) {
                m_map.at(s).push_back(newXmlId); // add to middle keys
            }
        }
        m_map.insert({ newXmlId, m_map.at(origXmlId) }); // add new as key
    }
    else {
        std::vector<std::string> s;
        s.push_back(origXmlId);
        s.push_back(newXmlId);
        m_map.insert({ origXmlId, s });
        m_map.insert({ newXmlId, s });
    }
    return true;
}

std::vector<std::string> ExpansionMap::GetExpansionIDsForElement(const std::string &xmlId)
{
    if (m_map.contains(xmlId)) {
        return m_map.at(xmlId);
    }
    else {
        std::vector<std::string> ids;
        ids.push_back(xmlId.c_str());
        return ids;
    }
}

bool ExpansionMap::HasExpansionMap()
{
    return (m_map.empty()) ? false : true;
}

void ExpansionMap::GetIDList(Object *object, std::vector<std::string> &idList)
{
    for (Object *o : object->GetChildren()) {
        idList.push_back(o->GetID());
        this->GetIDList(o, idList);
    }
}

void ExpansionMap::GeneratePredictableIDs(Object *source, Object *target)
{
    target->SetID(
        source->GetID() + "-rend" + std::to_string(this->GetExpansionIDsForElement(source->GetID()).size() + 1));

    ArrayOfObjects sourceObjects = source->GetChildren();
    ArrayOfObjects targetObjects = target->GetChildren();
    if (sourceObjects.size() <= 0 || sourceObjects.size() != targetObjects.size()) return;

    unsigned i = 0;
    for (Object *s : sourceObjects) {
        this->GeneratePredictableIDs(s, targetObjects.at(i++));
    }
}

void ExpansionMap::ToJson(std::string &output)
{
    jsonxx::Object expansionmap;
    for (auto &[id, ids] : m_map) {
        jsonxx::Array expandedIds;
        for (auto i : ids) expandedIds << i;
        expansionmap << id << expandedIds;
        ;
    }
    output = expansionmap.json();
}

namespace {

    /** What the marks in a measure say of the order of play */
    struct PlayMarks {
        bool m_segno = false;
        bool m_coda = false;
        bool m_fine = false;
        play::JumpInfo::JUMPTYPE m_jump = play::JumpInfo::NONE;
    };

    /**
     * The marks of [measure] that say something of the order: a repeatMark by its function, and a direction typed
     * as the MusicXML reader types the jumps it reads ("dacapo", "dalsegno", "tocoda", "fine").
     */
    PlayMarks MarksOf(const Measure *measure)
    {
        PlayMarks marks;
        for (const Object *child : measure->GetChildren()) {
            if (child->Is(REPEATMARK)) {
                switch (vrv_cast<const RepeatMark *>(child)->GetFunc()) {
                    case repeatMarkLog_FUNC_segno: marks.m_segno = true; break;
                    case repeatMarkLog_FUNC_coda: marks.m_coda = true; break;
                    case repeatMarkLog_FUNC_fine: marks.m_fine = true; break;
                    case repeatMarkLog_FUNC_daCapo:
                    case repeatMarkLog_FUNC_daCapoAlFine:
                    case repeatMarkLog_FUNC_daCapoAlCoda: marks.m_jump = play::JumpInfo::DACAPO; break;
                    case repeatMarkLog_FUNC_dalSegno:
                    case repeatMarkLog_FUNC_dalSegnoAlFine:
                    case repeatMarkLog_FUNC_dalSegnoAlCoda: marks.m_jump = play::JumpInfo::DALSEGNO; break;
                    default: break;
                }
            }
            else if (child->Is(DIR)) {
                const std::string type = vrv_cast<const Dir *>(child)->GetType();
                if (type == "dacapo") marks.m_jump = play::JumpInfo::DACAPO;
                if (type == "dalsegno") marks.m_jump = play::JumpInfo::DALSEGNO;
                if (type == "tocoda") marks.m_jump = play::JumpInfo::TOCODA;
                if (type == "fine") marks.m_fine = true;
            }
        }
        return marks;
    }

    /** A measure in score order, the ending it stands in (or none) and the element it is a child of */
    struct PlayedMeasure {
        Measure *m_measure;
        Ending *m_ending;
        Object *m_parent;
    };

    void GatherMeasures(Object *parent, Ending *ending, std::vector<PlayedMeasure> &measures)
    {
        for (Object *child : parent->GetChildren()) {
            if (child->Is(MEASURE)) {
                measures.push_back({ vrv_cast<Measure *>(child), ending, parent });
            }
            else if (child->Is(SECTION)) {
                GatherMeasures(child, ending, measures);
            }
            else if (child->Is(ENDING)) {
                GatherMeasures(child, vrv_cast<Ending *>(child), measures);
            }
        }
    }

    /** Whether [container] holds [measures] and no other measure, section or ending */
    bool HoldsOnly(const Object *container, const std::vector<Measure *> &measures)
    {
        size_t count = 0;
        for (const Object *child : container->GetChildren()) {
            if (child->Is({ SECTION, ENDING })) return false;
            if (child->Is(MEASURE)) ++count;
        }
        return (count == measures.size());
    }

} // namespace

void ExpansionMap::GenerateExpansionFor(Score *score)
{
    m_isProcessed = true;

    if (score->HasEditorialContent()) {
        LogWarning("An expansion cannot be generated with editorial content");
        return;
    }

    // The score's own sections (a section inside another is part of it: an order is worked out through them)
    int topSections = 0;
    for (const Object *child : score->GetChildren()) {
        if (child->Is(SECTION)) ++topSections;
    }
    if (topSections > 1) {
        LogWarning("An expansion cannot be generated with more than one section");
        return;
    }

    Section *section = vrv_cast<Section *>(score->FindDescendantByType(SECTION, 1));
    // A score with no section (nothing after its definition, or an ending that stands alone) has no order to work
    // out. (Only an assert said so: without it the null section was read)
    if (!section) return;

    // The order is worked out by the rule every format has (WriteOrderOfPlay), from the score as it stands: its
    // measures in score order, the endings they stand in, their repeat signs and the marks of a jump. (This worked
    // out an order of its own, from repeat signs alone and only for measures that stood directly in the section:
    // an ending was played in every pass, a jump was not taken, and a score whose measures stood in endings or in
    // sections of the section played no repeat at all.)
    std::vector<PlayedMeasure> measures;
    GatherMeasures(section, NULL, measures);
    if (measures.empty()) return;

    // The parts: a new one where a container (a section, an ending) or a repeat starts, at a segno or a coda, and
    // after a part was closed by a repeat end, a jump or a fine. As the MusicXML reader parts its score.
    play::Sections parts;
    bool isOrdered = false; // whether anything in the score says more than "once through"
    bool startsPart = true;
    bool startsRepeat = false;
    const size_t count = measures.size();
    for (size_t i = 0; i < count; ++i) {
        const PlayedMeasure &current = measures[i];
        Measure *measure = current.m_measure;
        const PlayMarks marks = MarksOf(measure);
        const bool isNewContainer = (i == 0) || (measures[i - 1].m_parent != current.m_parent);
        const bool isRepeatStart = startsRepeat || ExpansionMap::IsRepeatStart(measure);
        if (startsPart || isNewContainer || isRepeatStart || marks.m_segno || marks.m_coda) {
            play::SectionInfo info(isRepeatStart);
            // Every part of an ending is the ending's (one that a mark or a repeat start divides is played whole
            // in its passes, as one ending written as two brackets is)
            if (current.m_ending) info.merge(play::EndingInfo(current.m_ending->GetN(), "", ""));
            if (marks.m_segno) info.m_segnoLabel = "segno";
            if (marks.m_coda) info.m_codaLabel = "coda";
            parts.push_back({ info, {} });
        }
        parts.back().second.push_back(measure);
        play::SectionInfo &part = parts.back().first;

        // A repeat end on this measure's right, or on the left of the next. (One on the left of the first measure
        // has nothing before it to repeat.)
        const bool isRepeatEnd = ExpansionMap::IsRepeatEnd(measure)
            || ((i + 1 < count) && ExpansionMap::IsPreviousRepeatEnd(measures[i + 1].m_measure));
        startsPart = false;
        if (isRepeatEnd) {
            // (in an ending the rule goes by the endings' numbers, not by this)
            part.m_repeatInfo = play::RepeatInfo(2, false);
            startsPart = true;
        }
        if (marks.m_jump == play::JumpInfo::DACAPO) {
            part.m_jumpInfo = play::JumpInfo(play::JumpInfo::DACAPO, std::list<int>{ 1 });
        }
        else if (marks.m_jump == play::JumpInfo::DALSEGNO) {
            part.m_jumpInfo = play::JumpInfo(play::JumpInfo::DALSEGNO, "segno", std::list<int>{ 1 });
        }
        else if (marks.m_jump == play::JumpInfo::TOCODA) {
            part.m_jumpInfo = play::JumpInfo(play::JumpInfo::TOCODA, "coda", std::list<int>{ 2 });
        }
        if (marks.m_fine) part.m_fineInfo = play::FineInfo(true);
        // (the end of an ending needs no word here: what follows it stands in another container)
        if ((marks.m_jump != play::JumpInfo::NONE) || marks.m_fine) startsPart = true;
        startsRepeat = ExpansionMap::IsNextRepeatStart(measure);

        isOrdered = isOrdered || isRepeatEnd || current.m_ending || (marks.m_jump != play::JumpInfo::NONE);
    }
    // Nothing to follow: the score is left as it stands
    if (!isOrdered) return;

    // Each part gets the element the order names: the ending or the section that holds just its measures, where
    // that is a child of the score's section; otherwise a new section there, with its measures and what stands
    // before each of them (a change of key, a break).
    for (auto &[info, partMeasures] : parts) {
        Object *parent = partMeasures.front()->GetParent();
        if ((parent != section) && (parent->GetParent() == section) && HoldsOnly(parent, partMeasures)
            && (parent->Is(ENDING) == (info.m_classId == ENDING))) {
            info.m_target = parent;
            continue;
        }
        Object *top = partMeasures.front();
        while (top->GetParent() != section) top = top->GetParent();
        Section *target = new Section();
        if (top->Is(MEASURE)) {
            // ahead of what stands before the measure, which goes with it
            Object *first = top;
            for (Object *before = section->GetPrevious(first); before && !before->Is({ MEASURE, SECTION, ENDING, EXPANSION });
                before = section->GetPrevious(first)) {
                first = before;
            }
            section->InsertBefore(first, target);
        }
        else {
            section->InsertBefore(top, target);
        }
        for (Measure *measure : partMeasures) {
            Object *from = measure->GetParent();
            std::list<Object *> moved{ measure };
            for (Object *before = from->GetPrevious(measure);
                before && !before->Is({ MEASURE, SECTION, ENDING, EXPANSION }); before = from->GetPrevious(before)) {
                moved.push_front(before);
            }
            for (Object *object : moved) {
                from->DetachChild(from->GetChildIndex(object));
                target->AddChild(object);
            }
        }
        info.m_target = target;
    }

    const xsdAnyURI_List order = WriteOrderOfPlay(parts);
    if (order.empty()) return;
    Expansion *expansion = new Expansion();
    expansion->GetPlistInterface()->SetPlist(order);
    section->InsertChild(expansion, 0);
}

//----------------------------------------------------------------------------
// The order of play
//----------------------------------------------------------------------------

// vibe-coded on Claude AI using prompt:
//
// in C++, write a concise function to parse a string of comma separated ints with optional spaces into a list (array or
// vector) of ints
//
// verified at https://onlinegdb.com/blxCanxpAW
std::list<int> parseInts(const std::string &str)
{
    std::list<int> result;
    std::stringstream ss(str);
    int num;
    while (ss >> num) {
        result.push_back(num);
        if (ss.peek() == ',') ss.ignore();
    }
    return result;
}

xsdAnyURI_List WriteOrderOfPlay(play::Sections &sections)
{
    // The order is gathered here and given to the expansion once, and no more of it is written than can be
    // followed. (Each reference was added to the expansion's own list, which is copied whole for every one, and
    // the cost was counted when all of the order stood there: 60 measures each marked to be played 100 times took
    // two minutes and more to load, 100 of them a quarter of an hour)
    OrderOfPlay order;
    bool isFollowed = true;

    // iterate on sections to create expansion
    // prepopulate the labels map because there are forward jumps (tocoda)
    bool jumpBack = false;
    auto iter = sections.begin();
    auto rptIter = iter;
    auto secIter = iter;
    auto endIter = iter;
    std::map<std::string, decltype(iter)> segnos, codas;
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        if (!it->first.m_segnoLabel.empty()) segnos.insert({ it->first.m_segnoLabel, it });
        if (!it->first.m_codaLabel.empty()) codas.insert({ it->first.m_codaLabel, it });
    }
    // The pass of the repeat being played, and whether what stands at [it] is played in it: an ending is played in
    // the passes its number names, in a repeat as anywhere. (The bars of a repeat were played again whole, an ending
    // among them too: where plain bars stand between a first ending and the bar with its repeat sign, the ending
    // numbered 1 was played on pass 2 as well. 257 of MuseNote's songs are written so.) An ending with no number
    // names no pass and is played as before.
    int pass = 1;
    const auto isPlayed = [&pass, &rptIter](decltype(iter) it) {
        // (an ending the repeat itself starts in is that repeat's own music)
        if ((it->first.m_classId != ENDING) || (it == rptIter)) return true;
        const std::list<int> numbers = parseInts(it->first.m_endingInfo.m_number);
        return numbers.empty() || (std::find(numbers.begin(), numbers.end(), pass) != numbers.end());
    };
    while (isFollowed && (iter != sections.end())) {
        // increment visited count
        iter->first.m_visited++;

        // remember this repeat start
        if (iter->first.m_repeatStart) {
            rptIter = iter;
            pass = 1;
        }

        // handle section
        if (iter->first.m_classId == SECTION) {
            // add the section
            std::string ref = "#" + iter->first.m_target->GetID();
            isFollowed = order.Add(ref);

            // repeat the sections, beginning with the repeat start
            int times = (jumpBack && !iter->first.m_repeatInfo.m_afterJump) ? 1 : iter->first.m_repeatInfo.m_times;
            for (int t = 2; isFollowed && (t <= times); ++t) {
                ++pass;
                for (auto it = rptIter; isFollowed && (it != std::next(iter)); ++it) {
                    if (!isPlayed(it)) continue;
                    std::string ref = "#" + it->first.m_target->GetID();
                    isFollowed = order.Add(ref);
                }
            }

            // remember last section
            secIter = endIter = iter++;
        }
        // ending
        else {
            // gather the endings: each pass, and the endings played in it in score order. An ending is played in
            // the passes its number names that no ending before it has. One that names none goes on from the ending
            // just before it when it has that one's number and no repeat ends that one (one ending written as two
            // brackets); otherwise it is the pass after the last: a blank number (MusicXML: there is an ending, the
            // writer could not tell which), or a pass the repeat before it has left
            std::map<int, std::vector<decltype(iter)>> endings;
            auto begIter = iter;
            std::optional<decltype(iter)> rptNestedIter;
            while (iter != sections.end() && iter->first.m_classId == ENDING) {
                const std::list<int> numbers = parseInts(iter->first.m_endingInfo.m_number);
                bool hasPass = false;
                for (int i : numbers) {
                    if (endings.contains(i)) continue;
                    endings[i].push_back(iter);
                    hasPass = true;
                }
                const bool goesOn = !hasPass && (iter != begIter)
                    && (parseInts(std::prev(iter)->first.m_endingInfo.m_number) == numbers)
                    && (std::prev(iter)->second.back()->GetRight() != BARRENDITION_rptend);
                if (goesOn) {
                    for (auto &pass : endings) {
                        if (pass.second.back() == std::prev(iter)) pass.second.push_back(iter);
                    }
                }
                else if (!hasPass) {
                    const int last = endings.empty() ? 0 : endings.rbegin()->first;
                    endings[(last < INT_MAX) ? last + 1 : last].push_back(iter);
                }
                endIter = iter++;

                // remember a nested repeat start
                if (endIter != begIter && endIter->first.m_repeatStart) rptNestedIter = endIter;

                // increment visited count of subsequent sections
                if (endIter != begIter) endIter->first.m_visited++;
            }

            // when jumping back, keep only last ending: the last of its repeat. That one may stand further on, after
            // plain bars: an ending whose numbers go on from these (a "2" after this "1"). Then none of these is
            // played. One that starts its numbers again belongs to another repeat.
            if (jumpBack && endings.size() > 0) {
                bool isLast = true;
                for (auto it = iter; it != sections.end(); ++it) {
                    if (it->first.m_classId == ENDING) {
                        const std::list<int> numbers = parseInts(it->first.m_endingInfo.m_number);
                        isLast = numbers.empty()
                            || (*std::min_element(numbers.begin(), numbers.end()) <= endings.rbegin()->first);
                        break;
                    }
                    if (it->first.m_repeatStart) break;
                }
                if (isLast) {
                    endings.erase(endings.begin(), std::prev(endings.end()));
                    endIter = endings.begin()->second.back();
                }
                else {
                    endings.clear();
                }
            }

            // whether a repeat sign follows these endings after plain bars, before another ending or repeat begins
            bool signFollows = false;
            for (auto it = iter; (it != sections.end()) && (it->first.m_classId == SECTION) && !it->first.m_repeatStart;
                ++it) {
                const data_BARRENDITION right = it->second.back()->GetRight();
                if ((right == BARRENDITION_rptend) || (right == BARRENDITION_rptboth)) {
                    signFollows = true;
                    break;
                }
            }

            // the map is automatically sorted by key (ending number), so just add them to expansion in the same order
            // skip section first time because it was already added in the SECTION block
            for (auto ending = endings.begin(); isFollowed && (ending != endings.end()); ++ending) {
                if (ending != endings.begin()) {
                    // one ending for several passes comes round by the repeat sign at its end. Where that sign stands
                    // further on, after plain bars, it brings the ending round itself, in the passes it names
                    if ((ending->second == std::prev(ending)->second) && signFollows) continue;
                    ++pass;
                    for (auto it = rptIter; isFollowed && (it != std::next(secIter)); ++it) {
                        if (!isPlayed(it)) continue;
                        std::string ref = "#" + it->first.m_target->GetID();
                        isFollowed = order.Add(ref);
                    }
                }
                for (auto it = ending->second.begin(); isFollowed && (it != ending->second.end()); ++it) {
                    std::string endref = "#" + (*it)->first.m_target->GetID();
                    isFollowed = order.Add(endref);
                }
            }

            // set the repetition to the (latest) nested one
            if (rptNestedIter) {
                rptIter = *rptNestedIter;
                pass = 1;
            }
        }

        // fine
        if (jumpBack && endIter->first.m_fineInfo.m_fine) {
            break;
        }

        // dacapo
        if (endIter->first.m_jumpInfo.m_jump == play::JumpInfo::DACAPO
            && endIter->first.m_jumpInfo.m_times.end()
                != std::find(endIter->first.m_jumpInfo.m_times.begin(), endIter->first.m_jumpInfo.m_times.end(),
                    endIter->first.m_visited)) {
            iter = sections.begin();
            jumpBack = true;
        }

        // dalsegno / tocoda
        if ((endIter->first.m_jumpInfo.m_jump == play::JumpInfo::DALSEGNO
                || endIter->first.m_jumpInfo.m_jump == play::JumpInfo::TOCODA)
            && endIter->first.m_jumpInfo.m_times.end()
                != std::find(endIter->first.m_jumpInfo.m_times.begin(), endIter->first.m_jumpInfo.m_times.end(),
                    endIter->first.m_visited)) {
            const auto &labels = (endIter->first.m_jumpInfo.m_jump == play::JumpInfo::DALSEGNO) ? segnos : codas;
            if (!labels.contains(endIter->first.m_jumpInfo.m_label)) {
                LogWarning(
                    "Order of play: Segno/Coda label '%s' not found", endIter->first.m_jumpInfo.m_label.c_str());
            }
            else {
                iter = labels.at(endIter->first.m_jumpInfo.m_label);
                jumpBack = endIter->first.m_jumpInfo.m_jump == play::JumpInfo::DALSEGNO;
            }
        }
    }
    return order.GetRefs();
}

//----------------------------------------------------------------------------
// Static methods
//----------------------------------------------------------------------------

bool ExpansionMap::IsRepeatStart(Measure *measure)
{
    static const std::vector<data_BARRENDITION> match{ BARRENDITION_rptboth, BARRENDITION_rptstart };

    if (!measure->HasLeft()) return false;

    return (std::find(match.begin(), match.end(), measure->GetLeft()) != match.end());
}

bool ExpansionMap::IsRepeatEnd(Measure *measure)
{
    static const std::vector<data_BARRENDITION> match{ BARRENDITION_rptboth, BARRENDITION_rptend };

    if (!measure->HasRight()) return false;

    return (std::find(match.begin(), match.end(), measure->GetRight()) != match.end());
}

bool ExpansionMap::IsNextRepeatStart(Measure *measure)
{
    static const std::vector<data_BARRENDITION> match{ BARRENDITION_rptboth, BARRENDITION_rptstart };

    if (!measure->HasRight()) return false;

    return (std::find(match.begin(), match.end(), measure->GetRight()) != match.end());
}

bool ExpansionMap::IsPreviousRepeatEnd(Measure *measure)
{
    static const std::vector<data_BARRENDITION> match{ BARRENDITION_rptboth, BARRENDITION_rptend };

    if (!measure->HasLeft()) return false;

    return (std::find(match.begin(), match.end(), measure->GetLeft()) != match.end());
}

} // namespace vrv
