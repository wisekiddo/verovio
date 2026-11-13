/////////////////////////////////////////////////////////////////////////////
// Name:        ioabc.h
// Author:      Klaus Rettinghaus
// Created:     2017
// Copyright (c) Klaus Rettinghaus. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_IOABC_H__
#define __VRV_IOABC_H__

#include <string>
#include <vector>

//----------------------------------------------------------------------------

#include "iobase.h"
#include "pugixml.hpp"
#include "vrvdef.h"

namespace vrv {

//----------------------------------------------------------------------------
// ABCInput
//----------------------------------------------------------------------------

/**
 * Reads ABC (the standard, version 2.1) in two steps: the text is read into tunes (ioabc.cpp, namespace abc: the
 * fields with their scope, the voices, each voice's bars and each bar's events with what belongs to them), and
 * each tune is then written into the document.
 */
class ABCInput : public Input {
public:
    // constructors and destructors
    ABCInput(Doc *doc);
    virtual ~ABCInput();

    bool Import(const std::string &abc) override;

    /** Line breaks of the tune are written (they are, unless the tune says "I:linebreak <none>") */
    void SetLayoutEncoded(bool encoded) { m_layoutInformation = encoded ? LAYOUT_ENCODED : LAYOUT_NONE; }
};

} // namespace vrv

#endif
