/////////////////////////////////////////////////////////////////////////////
// Name:        ioabc.cpp
// Author:      Klaus Rettinghaus
// Created:     2017
// Copyright (c) Klaus Rettinghaus. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ioabc.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>

//----------------------------------------------------------------------------

#include "accid.h"
#include "annot.h"
#include "arpeg.h"
#include "artic.h"
#include "beam.h"
#include "breath.h"
#include "chord.h"
#include "clef.h"
#include "dir.h"
#include "doc.h"
#include "dynam.h"
#include "ending.h"
#include "expansionmap.h"
#include "fermata.h"
#include "fing.h"
#include "grpsym.h"
#include "hairpin.h"
#include "harm.h"
#include "instrdef.h"
#include "keyaccid.h"
#include "keysig.h"
#include "label.h"
#include "labelabbr.h"
#include "layer.h"
#include "mdiv.h"
#include "measure.h"
#include "metersig.h"
#include "mordent.h"
#include "mrest.h"
#include "mspace.h"
#include "multirest.h"
#include "note.h"
#include "pb.h"
#include "pghead.h"
#include "reh.h"
#include "rend.h"
#include "repeatmark.h"
#include "rest.h"
#include "sb.h"
#include "score.h"
#include "scoredef.h"
#include "section.h"
#include "slur.h"
#include "space.h"
#include "staff.h"
#include "staffdef.h"
#include "staffgrp.h"
#include "syl.h"
#include "tempo.h"
#include "text.h"
#include "tie.h"
#include "trill.h"
#include "tuplet.h"
#include "turn.h"
#include "verse.h"
#include "vrv.h"

namespace vrv {

#ifndef NO_ABC_SUPPORT

// The reader is in two halves. The first reads the text into tunes and knows nothing of the engine's score: the
// fields with their scope (the file, a tune, a voice, a point in the music), the voices, each voice's bars, and in
// each bar its events with what belongs to them. A note's sound is worked out there, once, by ABC's rule. The second
// writes a tune into the document. (The reader before this one wrote each character it met into one layer of one
// staff, and what it did not know it passed over: it had no place to put a voice, an ending or a change of key.)
namespace abc {

    //------------------------------------------------------------------------
    // Characters
    //------------------------------------------------------------------------

    // ABC's own characters are ASCII, and a text is UTF-8: a byte of a letter with an accent is no space, digit or
    // letter of ABC, whatever the C library makes of it under the locale in force. (isspace says yes for the byte
    // 0xA0 under some: a title that ends in "à" lost that byte, and the text was no UTF-8 any more.)
    inline bool IsSpace(char c)
    {
        return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') || (c == '\v') || (c == '\f');
    }
    inline bool IsDigit(char c)
    {
        return (c >= '0') && (c <= '9');
    }
    inline bool IsAlpha(char c)
    {
        return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'));
    }
    inline char ToLower(char c)
    {
        return ((c >= 'A') && (c <= 'Z')) ? static_cast<char>(c - 'A' + 'a') : c;
    }

    /** A text word by word, divided at ABC's white space */
    class Words {
    public:
        explicit Words(const std::string &text) : m_text(text) {}
        bool Next(std::string &word)
        {
            while ((m_at < m_text.size()) && IsSpace(m_text[m_at])) ++m_at;
            if (m_at >= m_text.size()) return false;
            const size_t start = m_at;
            while ((m_at < m_text.size()) && !IsSpace(m_text[m_at])) ++m_at;
            word = m_text.substr(start, m_at - start);
            return true;
        }
        bool Next(int &number)
        {
            const size_t before = m_at;
            std::string word;
            if (!this->Next(word)) return false;
            const bool isNumber = !word.empty() && (word.size() <= 6)
                && (word.find_first_not_of("0123456789", (word[0] == '-') ? 1 : 0) == std::string::npos)
                && (word != "-");
            if (!isNumber) {
                m_at = before;
                return false;
            }
            number = atoi(word.c_str());
            return true;
        }
        std::string Rest()
        {
            const std::string rest = m_text.substr(std::min(m_at, m_text.size()));
            m_at = m_text.size();
            return rest;
        }
        size_t At() const { return m_at; }
        void Back(size_t at) { m_at = at; }

    private:
        std::string m_text;
        size_t m_at = 0;
    };

    //------------------------------------------------------------------------
    // Lengths: fractions of a whole note
    //------------------------------------------------------------------------

    struct Len {
        long long n = 0;
        long long d = 1;

        Len() = default;
        Len(long long num, long long den)
        {
            // (a length ABC cannot mean, a divisor of 0 or numbers past what is kept exactly, is none)
            if ((den <= 0) || (num < 0) || (num > (1LL << 40)) || (den > (1LL << 40))) return;
            const long long g = std::gcd(num, den);
            n = g ? num / g : 0;
            d = g ? den / g : 1;
        }
        Len operator+(const Len &other) const { return Len(n * other.d + other.n * d, d * other.d); }
        Len operator-(const Len &other) const { return Len(n * other.d - other.n * d, d * other.d); }
        Len operator*(const Len &other) const { return Len(n * other.n, d * other.d); }
        bool operator<(const Len &other) const { return n * other.d < other.n * d; }
        bool operator==(const Len &other) const { return (n == other.n) && (d == other.d); }
        bool IsZero() const { return n == 0; }
    };

    //------------------------------------------------------------------------
    // The tune
    //------------------------------------------------------------------------

    constexpr int kNoAccid = 99;

    /** A note's head: what is written, and what sounds */
    struct Head {
        char m_step = 'c'; // a..g
        int m_oct = 4;
        int m_accid = kNoAccid; // the sign written before it: -2..2, or none
        int m_alter = 0; // what sounds, by ABC's rule
        bool m_tie = false;
        bool m_tieDotted = false;
        Len m_length; // as written in a chord, in unit lengths
    };

    struct Clef {
        char m_shape = 0; // G, F, C, P (percussion), N (none); 0: not said
        int m_line = 0;
        int m_dis = 0; // 8 or 15: the octave written at the clef
        bool m_disBelow = false;
    };

    struct Key {
        bool m_none = true; // no signature said ("K:none", or nothing)
        char m_tonic = 0; // a..g
        int m_tonicAccid = 0;
        data_MODE m_mode = MODE_NONE;
        int m_fifths = 0;
        bool m_standard = true; // the signature is the one of its fifths
        std::map<char, int> m_alter; // what the signature does to each step
        std::map<char, int> m_shown; // the signs of a signature that is not standard, by step
    };

    struct Meter {
        std::vector<int> m_counts; // empty: none
        int m_unit = 0;
        char m_sym = 0; // 'C' common, '|' cut
        Len Length() const
        {
            if (m_counts.empty() || (m_unit <= 0)) return Len();
            return Len(std::accumulate(m_counts.begin(), m_counts.end(), 0), m_unit);
        }
    };

    struct Tempo {
        bool m_said = false;
        double m_perBeat = 0; // beats a minute; 0: words only
        Len m_beat; // the beat, of a whole note
        std::string m_words;
    };

    /** A change that takes effect at an event */
    struct Change {
        enum Kind { KEY, METER, CLEF, TEMPO } m_kind = KEY;
        Key m_key;
        Meter m_meter;
        Clef m_clef;
        Tempo m_tempo;
    };

    /** A text at an event: a chord symbol (no place), or an annotation with its place (^ _ < > @) */
    struct TextMark {
        char m_place = 0;
        std::string m_text;
    };

    struct Syllable {
        std::string m_text;
        bool m_dash = false; // the word goes on
        bool m_held = false; // held for the next note
    };

    enum class EventKind { Note, Chord, Rest, Space, BarRest, BarSpace };

    struct Event {
        EventKind m_kind = EventKind::Note;
        std::vector<Head> m_heads;
        Len m_length; // of a whole note, as written (a tuplet's time is the tuplet's)
        bool m_stemless = false; // a length of 0
        bool m_grace = false;
        bool m_graceSlash = false;
        int m_graceGroup = 0;
        int m_tupletNum = 0; // a tuplet starts here: p, q and the notes it holds
        int m_tupletBase = 0;
        int m_tupletCount = 0;
        std::vector<std::string> m_marks; // decorations, by their names
        std::vector<TextMark> m_texts;
        std::vector<bool> m_slurStarts; // each: whether it is dotted
        int m_slurEnds = 0;
        bool m_breaksBeam = false; // white space stands before it
        int m_bars = 1; // of a bar rest
        bool m_barsSaid = false; // by a number ("Z1")
        std::vector<Change> m_changes; // before it
        std::map<int, Syllable> m_syllables; // by verse
        std::string m_partLabel; // a part (P:) starts here: its name
    };

    struct Bar {
        std::vector<Event> m_events;
        std::vector<std::vector<Event>> m_overlays; // voices written over it with "&"
        data_BARRENDITION m_left = BARRENDITION_NONE;
        data_BARRENDITION m_right = BARRENDITION_NONE;
        int m_times = 2; // how often the repeat that ends here is played
        std::string m_ending; // an ending starts here: its numbers ("1, 3")
        bool m_lineEnd = false; // a line of the tune ends after it
        int m_line = 0; // the line of the file it ends on
        std::string m_part; // a part starts here (P:)
        std::vector<Change> m_changes; // said after its last event

        bool IsEmpty() const { return m_events.empty() && m_overlays.empty(); }
    };

    /** Where an event is: for a line of lyrics or symbols */
    struct EventRef {
        int m_bar = 0;
        int m_overlay = -1;
        int m_event = 0;
    };

    struct Voice {
        std::string m_id;
        std::string m_name;
        std::string m_shortName;
        Clef m_clef;
        int m_octave = 0;
        int m_transpose = 0;
        bool m_saidOctave = false; // by the voice itself (else it is the tune's)
        bool m_saidTranspose = false;
        int m_stafflines = 0; // 0: not said
        int m_program = -1;
        int m_channel = -1;
        int m_volume = -1;
        std::vector<Bar> m_bars;

        // While it is read
        Len m_unit;
        Key m_key;
        std::map<int, int> m_barAlter; // the signs written in the bar, by step (and octave)
        std::map<int, int> m_tied; // the notes tied over, by step and octave: what they sound
        std::vector<std::string> m_marks; // waiting for their event
        std::vector<TextMark> m_texts;
        std::vector<bool> m_slurStarts;
        std::vector<Change> m_changes;
        int m_tupletNum = 0;
        int m_tupletBase = 0;
        int m_tupletCount = 0;
        bool m_inGrace = false;
        bool m_graceSlash = false;
        int m_graceGroup = 0;
        int m_brokenNext = 0; // the note after "<" or ">": >0 it is shortened that many times, <0 dotted
        bool m_breaksBeam = true;
        int m_overlay = -1;
        std::string m_ending; // an ending met where its bar had begun: it starts at the next bar
        int m_startTimes = 2; // how often the repeat that has started is played, by its start's colons
        std::string m_partLabel; // the name of a part that starts at the next event
        std::vector<EventRef> m_lineEvents; // the notes of its last line of music
        bool m_lineDone = false; // words were set under that line
        size_t m_lyricAt = 0;
        int m_lyricBar = -1; // the bar the last syllable set stands in
        int m_verse = 0;
        bool m_lastWasLyrics = false;

        Bar &Current() { return m_bars.back(); }
        std::vector<Event> &Events()
        {
            return (m_overlay < 0) ? m_bars.back().m_events : m_bars.back().m_overlays.at(m_overlay);
        }
        Event *Last()
        {
            // the last event that is no grace note, in this bar or the one before
            for (auto bar = m_bars.rbegin(); bar != m_bars.rend(); ++bar) {
                std::vector<Event> &events
                    = ((bar == m_bars.rbegin()) && (m_overlay >= 0)) ? bar->m_overlays.at(m_overlay) : bar->m_events;
                for (auto event = events.rbegin(); event != events.rend(); ++event) {
                    if (!event->m_grace) return &*event;
                }
                if (std::distance(m_bars.rbegin(), bar) >= 1) break;
            }
            return NULL;
        }
    };

    struct Tune {
        std::string m_number;
        std::vector<std::pair<std::string, int>> m_title; // each with its line in the file
        std::vector<std::pair<std::string, int>> m_composer;
        std::vector<std::pair<std::string, int>> m_history;
        std::vector<std::pair<std::string, int>> m_origin;
        std::vector<std::pair<std::pair<std::string, int>, char>> m_info;
        Meter m_meter; // the one in force where the tune is being read
        Meter m_firstMeter; // the header's
        Len m_unit; // zero: not said
        Tempo m_tempo;
        Key m_key;
        Clef m_clef; // said with the key: every voice's that says none
        int m_octave = 0;
        int m_transpose = 0;
        int m_stafflines = 0;
        std::vector<Voice> m_voices;
        std::string m_layout; // of %%score or %%staves
        std::string m_parts; // the order of the parts (P: in the header)
        int m_propagate = 2; // signs hold: 0 for their note, 1 in their octave, 2 in every octave
        bool m_strict = false; // read strictly by the standard: the file or the tune says version 2.1 or later (2.1)
        std::map<char, std::string> m_symbols; // U:
        char m_decoration = '!';
        bool m_lineBreaks = true;
        int m_firstLine = 0; // the line after its header
        int m_current = 0; // the voice being read
        int m_program = -1;
        int m_channel = -1;
        int m_volume = -1;
        std::set<std::string> m_notRead; // what was met and is not read, each kind once
    };

    //------------------------------------------------------------------------
    // Text
    //------------------------------------------------------------------------

    std::string Trimmed(const std::string &text)
    {
        size_t start = 0, end = text.size();
        while ((start < end) && IsSpace(text[start])) ++start;
        while ((end > start) && IsSpace(text[end - 1])) --end;
        return text.substr(start, end - start);
    }

    void AppendUtf8(std::string &out, unsigned long code)
    {
        if (code < 0x80) {
            out += static_cast<char>(code);
        }
        else if (code < 0x800) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
        else if (code < 0x10000) {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
        else if (code < 0x110000) {
            out += static_cast<char>(0xF0 | (code >> 18));
            out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    /**
     * A text string with its accents and symbols (ABC 2.1, 8.2): a mnemonic (\'e), a named entity (&eacute;) or a
     * code point (é) is the character it names. What names none stays as it is written.
     */
    std::string Decoded(const std::string &text)
    {
        static const std::map<std::string, unsigned long> mnemonics = {
            { "`A", 0xC0 }, { "`a", 0xE0 }, { "`E", 0xC8 }, { "`e", 0xE8 }, { "`I", 0xCC }, { "`i", 0xEC },
            { "`O", 0xD2 }, { "`o", 0xF2 }, { "`U", 0xD9 }, { "`u", 0xF9 }, { "'A", 0xC1 }, { "'a", 0xE1 },
            { "'E", 0xC9 }, { "'e", 0xE9 }, { "'I", 0xCD }, { "'i", 0xED }, { "'O", 0xD3 }, { "'o", 0xF3 },
            { "'U", 0xDA }, { "'u", 0xFA }, { "'Y", 0xDD }, { "'y", 0xFD }, { "^A", 0xC2 }, { "^a", 0xE2 },
            { "^E", 0xCA }, { "^e", 0xEA }, { "^I", 0xCE }, { "^i", 0xEE }, { "^O", 0xD4 }, { "^o", 0xF4 },
            { "^U", 0xDB }, { "^u", 0xFB }, { "^Y", 0x176 }, { "^y", 0x177 }, { "~A", 0xC3 }, { "~a", 0xE3 },
            { "~N", 0xD1 }, { "~n", 0xF1 }, { "~O", 0xD5 }, { "~o", 0xF5 }, { "\"A", 0xC4 }, { "\"a", 0xE4 },
            { "\"E", 0xCB }, { "\"e", 0xEB }, { "\"I", 0xCF }, { "\"i", 0xEF }, { "\"O", 0xD6 }, { "\"o", 0xF6 },
            { "\"U", 0xDC }, { "\"u", 0xFC }, { "\"Y", 0x178 }, { "\"y", 0xFF }, { "cC", 0xC7 }, { "cc", 0xE7 },
            { "AA", 0xC5 }, { "aa", 0xE5 }, { "/O", 0xD8 }, { "/o", 0xF8 }, { "uA", 0x102 }, { "ua", 0x103 },
            { "uE", 0x114 }, { "ue", 0x115 }, { "vS", 0x160 }, { "vs", 0x161 }, { "vZ", 0x17D }, { "vz", 0x17E },
            { "HO", 0x150 }, { "Ho", 0x151 }, { "HU", 0x170 }, { "Hu", 0x171 }, { "AE", 0xC6 }, { "ae", 0xE6 },
            { "OE", 0x152 }, { "oe", 0x153 }, { "ss", 0xDF }, { "DH", 0xD0 }, { "dh", 0xF0 }, { "TH", 0xDE },
            { "th", 0xFE },
        };
        static const std::map<std::string, unsigned long> entities = {
            { "Agrave", 0xC0 }, { "agrave", 0xE0 }, { "Egrave", 0xC8 }, { "egrave", 0xE8 }, { "Igrave", 0xCC },
            { "igrave", 0xEC }, { "Ograve", 0xD2 }, { "ograve", 0xF2 }, { "Ugrave", 0xD9 }, { "ugrave", 0xF9 },
            { "Aacute", 0xC1 }, { "aacute", 0xE1 }, { "Eacute", 0xC9 }, { "eacute", 0xE9 }, { "Iacute", 0xCD },
            { "iacute", 0xED }, { "Oacute", 0xD3 }, { "oacute", 0xF3 }, { "Uacute", 0xDA }, { "uacute", 0xFA },
            { "Yacute", 0xDD }, { "yacute", 0xFD }, { "Acirc", 0xC2 }, { "acirc", 0xE2 }, { "Ecirc", 0xCA },
            { "ecirc", 0xEA }, { "Icirc", 0xCE }, { "icirc", 0xEE }, { "Ocirc", 0xD4 }, { "ocirc", 0xF4 },
            { "Ucirc", 0xDB }, { "ucirc", 0xFB }, { "Atilde", 0xC3 }, { "atilde", 0xE3 }, { "Ntilde", 0xD1 },
            { "ntilde", 0xF1 }, { "Otilde", 0xD5 }, { "otilde", 0xF5 }, { "Auml", 0xC4 }, { "auml", 0xE4 },
            { "Euml", 0xCB }, { "euml", 0xEB }, { "Iuml", 0xCF }, { "iuml", 0xEF }, { "Ouml", 0xD6 },
            { "ouml", 0xF6 }, { "Uuml", 0xDC }, { "uuml", 0xFC }, { "Yuml", 0x178 }, { "yuml", 0xFF },
            { "Ccedil", 0xC7 }, { "ccedil", 0xE7 }, { "Aring", 0xC5 }, { "aring", 0xE5 }, { "Oslash", 0xD8 },
            { "oslash", 0xF8 }, { "AElig", 0xC6 }, { "aelig", 0xE6 }, { "OElig", 0x152 }, { "oelig", 0x153 },
            { "szlig", 0xDF }, { "ETH", 0xD0 }, { "eth", 0xF0 }, { "THORN", 0xDE }, { "thorn", 0xFE },
            { "Scaron", 0x160 }, { "scaron", 0x161 }, { "copy", 0xA9 }, { "amp", 0x26 }, { "quot", 0x22 },
            { "lt", 0x3C }, { "gt", 0x3E }, { "nbsp", 0xA0 }, { "sharp", 0x266F }, { "flat", 0x266D },
            { "natural", 0x266E },
        };
        std::string out;
        for (size_t i = 0; i < text.size(); ++i) {
            const char c = text[i];
            if ((c == '\\') && (i + 5 < text.size()) && (text[i + 1] == 'u')) {
                const std::string digits = text.substr(i + 2, 4);
                if (digits.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
                    AppendUtf8(out, std::strtoul(digits.c_str(), NULL, 16));
                    i += 5;
                    continue;
                }
            }
            if ((c == '\\') && (text.size() - i >= 3)) {
                const auto found = mnemonics.find(text.substr(i + 1, 2));
                if (found != mnemonics.end()) {
                    AppendUtf8(out, found->second);
                    i += 2;
                    continue;
                }
            }
            if (c == '&') {
                const size_t end = text.find(';', i);
                if ((end != std::string::npos) && (end - i <= 8)) {
                    const auto found = entities.find(text.substr(i + 1, end - i - 1));
                    if (found != entities.end()) {
                        AppendUtf8(out, found->second);
                        i = end;
                        continue;
                    }
                }
            }
            out += c;
        }
        return out;
    }

    //------------------------------------------------------------------------
    // Reading: the fields
    //------------------------------------------------------------------------

    const std::string kFifths = "fcgdaeb";

    /** What a signature of [fifths] does to each step (more than seven alters all seven) */
    std::map<char, int> AlterOf(int fifths)
    {
        std::map<char, int> alter;
        const int count = std::min(std::abs(fifths), 7);
        for (int i = 0; i < count; ++i) {
            if (fifths > 0) alter[kFifths[i]] = 1;
            if (fifths < 0) alter[kFifths[6 - i]] = -1;
        }
        return alter;
    }

    /** A number at [i] of [text], which is moved past it; -1 where none stands, or one too long to mean anything */
    int ReadNumber(const std::string &text, size_t &i)
    {
        if ((i >= text.size()) || !IsDigit(text[i])) return -1;
        long long number = 0;
        bool tooLong = false;
        while ((i < text.size()) && IsDigit(text[i])) {
            number = number * 10 + (text[i] - '0');
            if (number > 1000000) tooLong = true;
            if (tooLong) number = 0;
            ++i;
        }
        return tooLong ? -1 : static_cast<int>(number);
    }

    /** The value of [name]= in [text] ("" where it is not said); a value in quotes is all of what they hold */
    std::string Property(const std::string &text, const std::string &name)
    {
        size_t at = 0;
        while ((at = text.find(name + "=", at)) != std::string::npos) {
            if ((at == 0) || IsSpace(text[at - 1])) break;
            ++at;
        }
        if (at == std::string::npos) return "";
        at += name.size() + 1;
        if ((at < text.size()) && (text[at] == '"')) {
            const size_t end = text.find('"', at + 1);
            return text.substr(at + 1, (end == std::string::npos) ? std::string::npos : end - at - 1);
        }
        size_t end = at;
        while ((end < text.size()) && !IsSpace(text[end])) ++end;
        return text.substr(at, end - at);
    }

    /** A clef by its name: "treble", "bass3", "alto", "treble-8", "bass+8", "perc", "none" (false: no clef's name) */
    bool ReadClef(std::string name, Clef &clef)
    {
        Clef read;
        const size_t octave = name.find_first_of("+-");
        if (octave != std::string::npos) {
            const int dis = atoi(name.c_str() + octave + 1);
            if ((dis == 8) || (dis == 15)) {
                read.m_dis = dis;
                read.m_disBelow = (name[octave] == '-');
            }
            name.resize(octave);
        }
        int line = 0;
        if (!name.empty() && IsDigit(name.back())) {
            line = name.back() - '0';
            name.pop_back();
        }
        if ((name == "treble") || (name == "G")) {
            read.m_shape = 'G';
            read.m_line = 2;
        }
        else if ((name == "bass") || (name == "F")) {
            read.m_shape = 'F';
            read.m_line = 4;
        }
        else if ((name == "alto") || (name == "C")) {
            read.m_shape = 'C';
            read.m_line = 3;
        }
        else if (name == "tenor") {
            read.m_shape = 'C';
            read.m_line = 4;
        }
        else if (name == "perc") {
            read.m_shape = 'P';
        }
        else if (name == "none") {
            read.m_shape = 'N';
        }
        else {
            return false;
        }
        if ((line >= 1) && (line <= 5) && (read.m_line != 0)) read.m_line = line;
        clef = read;
        return true;
    }

    /**
     * What a K: or V: field says of the staff, after the key: the clef (by "clef=" or by its name alone), and
     * octave=, transpose=, stafflines=. True when a clef was said.
     */
    bool ReadStaffProperties(const std::string &text, Clef &clef, int &octave, int &transpose, int &stafflines)
    {
        bool hasClef = false;
        const std::string named = Property(text, "clef");
        if (!named.empty()) hasClef = ReadClef(named, clef);
        Words words(text);
        std::string word;
        while (!hasClef && (words.Next(word))) {
            if (word.find('=') != std::string::npos) continue;
            // (the tonic and mode of a key are no clef: "C", "F" and "G" name one only after "clef=")
            if ((word == "C") || (word == "F") || (word == "G") || (word == "none")) continue;
            hasClef = ReadClef(word, clef);
        }
        const std::string oct = Property(text, "octave");
        if (!oct.empty()) octave = std::clamp(atoi(oct.c_str()), -4, 4);
        const std::string trans = Property(text, "transpose");
        if (!trans.empty()) transpose = std::clamp(atoi(trans.c_str()), -60, 60);
        const std::string lines = Property(text, "stafflines");
        if (!lines.empty()) {
            // (a count past what any staff has is none: five lines, as where nothing is said)
            const long count = std::strtol(lines.c_str(), NULL, 10);
            stafflines = ((count >= 1) && (count <= 32)) ? static_cast<int>(count) : 0;
        }
        return hasClef;
    }

    /** K: the key. False where the field says nothing of the key (a clef alone): [key] is then left as it is */
    bool ReadKey(const std::string &value, Key &key)
    {
        Words words(value);
        std::string word;
        if (!(words.Next(word))) return false;
        Key read;
        std::string lower = word;
        for (char &c : lower) c = (ToLower(c));
        if (word == "HP") {
            // the Highland pipes with no signature drawn: F and C sharp, G natural
            read.m_none = false;
            read.m_standard = false;
            read.m_alter = { { 'f', 1 }, { 'c', 1 } };
        }
        else if (word == "Hp") {
            read.m_none = false;
            read.m_standard = false;
            read.m_alter = { { 'f', 1 }, { 'c', 1 } };
            read.m_shown = { { 'f', 1 }, { 'c', 1 }, { 'g', 0 } };
        }
        else if (lower == "none") {
            read.m_none = true;
        }
        else if ((word[0] >= 'A') && (word[0] <= 'G')) {
            read.m_none = false;
            read.m_tonic = ToLower(word[0]);
            int fifths = static_cast<int>(kFifths.find(read.m_tonic)) - 1;
            size_t at = 1;
            if ((at < word.size()) && (word[at] == '#')) {
                read.m_tonicAccid = 1;
                fifths += 7;
                ++at;
            }
            else if ((at < word.size()) && (word[at] == 'b')) {
                read.m_tonicAccid = -1;
                fifths -= 7;
                ++at;
            }
            // the mode: after the tonic, or the next word. Capitals do not matter, and three letters are enough
            std::string mode = word.substr(at);
            if (mode.empty()) {
                const size_t before = words.At();
                std::string next;
                if (words.Next(next)) {
                    std::string start = next.substr(0, 3);
                    for (char &c : start) c = (ToLower(c));
                    static const std::set<std::string> modes
                        = { "maj", "min", "ion", "dor", "phr", "lyd", "mix", "aeo", "loc" };
                    if (modes.contains(start) || (next == "m")) {
                        mode = next;
                    }
                    else {
                        words.Back(before);
                    }
                }
            }
            for (char &c : mode) c = (ToLower(c));
            mode.resize(std::min<size_t>(mode.size(), 3));
            read.m_mode = MODE_major;
            if ((mode == "min") || (mode == "m")) {
                read.m_mode = MODE_minor;
                fifths -= 3;
            }
            else if (mode == "ion") {
                read.m_mode = MODE_ionian;
            }
            else if (mode == "dor") {
                read.m_mode = MODE_dorian;
                fifths -= 2;
            }
            else if (mode == "phr") {
                read.m_mode = MODE_phrygian;
                fifths -= 4;
            }
            else if (mode == "lyd") {
                read.m_mode = MODE_lydian;
                fifths += 1;
            }
            else if (mode == "mix") {
                read.m_mode = MODE_mixolydian;
                fifths -= 1;
            }
            else if (mode == "aeo") {
                read.m_mode = MODE_aeolian;
                fifths -= 3;
            }
            else if (mode == "loc") {
                read.m_mode = MODE_locrian;
                fifths -= 5;
            }
            read.m_fifths = fifths;
            read.m_alter = AlterOf(fifths);
        }
        else {
            return false;
        }
        // "exp" and the signs after the key: the signature is those alone, or the key's with those changed
        bool isExplicit = false;
        bool hasSigns = false;
        while (words.Next(word)) {
            if (word == "exp") {
                isExplicit = true;
                read.m_alter.clear();
                continue;
            }
            if (word.find('=') != std::string::npos && (word[0] != '=')) continue;
            size_t at = 0;
            int alter = 0;
            if (word[0] == '^') {
                alter = ((word.size() > 1) && (word[1] == '^')) ? 2 : 1;
            }
            else if (word[0] == '_') {
                alter = ((word.size() > 1) && (word[1] == '_')) ? -2 : -1;
            }
            else if (word[0] != '=') {
                continue;
            }
            at = (std::abs(alter) == 2) ? 2 : 1;
            if ((at + 1 != word.size()) || !IsAlpha(word[at])) continue;
            const char step = (ToLower(word[at]));
            if ((step < 'a') || (step > 'g')) continue;
            if (alter == 0) {
                read.m_alter.erase(step);
            }
            else {
                read.m_alter[step] = alter;
            }
            hasSigns = true;
        }
        if (isExplicit || hasSigns) {
            read.m_none = false;
            // (a signature that is one of the fifteen all the same is written as that one)
            read.m_standard = false;
            for (int fifths = -7; fifths <= 7; ++fifths) {
                if (AlterOf(fifths) == read.m_alter) {
                    read.m_standard = true;
                    read.m_fifths = fifths;
                }
            }
            if (!read.m_standard) read.m_shown = read.m_alter;
        }
        key = read;
        return true;
    }

    /** M: the meter ("none" and what cannot be read are no meter) */
    Meter ReadMeter(const std::string &value)
    {
        Meter meter;
        const std::string text = Trimmed(value);
        if (text == "C") {
            meter.m_counts = { 4 };
            meter.m_unit = 4;
            meter.m_sym = 'C';
        }
        else if (text == "C|") {
            meter.m_counts = { 2 };
            meter.m_unit = 2;
            meter.m_sym = '|';
        }
        else {
            const size_t slash = text.find('/');
            if (slash == std::string::npos) return meter;
            size_t at = 0;
            while (at < slash) {
                if (IsDigit(text[at])) {
                    const int count = ReadNumber(text, at);
                    if ((count < 1) || (count > 1000)) return Meter();
                    meter.m_counts.push_back(count);
                }
                else {
                    ++at;
                }
            }
            at = slash + 1;
            const int unit = ReadNumber(text, at);
            // (a unit that is no power of two, 0 or a number too long, is no meter)
            if ((unit < 1) || (unit > 256) || ((unit & (unit - 1)) != 0) || meter.m_counts.empty()) return Meter();
            meter.m_unit = unit;
        }
        return meter;
    }

    /** L: the unit length (zero where it cannot be read) */
    Len ReadUnit(const std::string &value)
    {
        const std::string text = Trimmed(value);
        size_t at = 0;
        const int num = ReadNumber(text, at);
        if (num < 1) return Len();
        if ((at >= text.size()) || (text[at] != '/')) return (num == 1) ? Len(1, 1) : Len();
        ++at;
        const int den = ReadNumber(text, at);
        if ((den < 1) || (den > 512)) return Len();
        return Len(num, den);
    }

    /**
     * Q: the tempo. "1/4=120", with words before or after in quotes, several beats that make one ("1/4 3/8=70"),
     * and the forms of before 2.0: a number alone is that many unit lengths a minute, "C=100" and "C3=40" too.
     */
    Tempo ReadTempo(const std::string &value, const Len &unit)
    {
        Tempo tempo;
        tempo.m_said = true;
        std::string rest;
        // the words
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '"') {
                const size_t end = value.find('"', i + 1);
                const std::string words = value.substr(i + 1, (end == std::string::npos) ? std::string::npos : end - i - 1);
                if (!words.empty()) tempo.m_words = Decoded(words);
                if (end == std::string::npos) break;
                i = end;
            }
            else {
                rest += value[i];
            }
        }
        const size_t equals = rest.find('=');
        if (equals == std::string::npos) {
            // a number alone
            const std::string number = Trimmed(rest);
            if (!number.empty() && IsDigit(number[0])) {
                tempo.m_perBeat = std::strtod(number.c_str(), NULL);
                tempo.m_beat = unit;
            }
            if (!std::isfinite(tempo.m_perBeat) || (tempo.m_perBeat < 0) || (tempo.m_perBeat > 100000)) tempo.m_perBeat = 0;
            return tempo;
        }
        const double perBeat = std::strtod(rest.c_str() + equals + 1, NULL);
        tempo.m_perBeat = (std::isfinite(perBeat) && (perBeat > 0) && (perBeat <= 100000)) ? perBeat : 0;
        const std::string beats = rest.substr(0, equals);
        Len beat;
        size_t at = 0;
        while (at < beats.size()) {
            if (IsDigit(beats[at])) {
                const int num = ReadNumber(beats, at);
                int den = 1;
                if ((at < beats.size()) && (beats[at] == '/')) {
                    ++at;
                    den = ReadNumber(beats, at);
                }
                if ((num >= 0) && (den >= 1)) beat = beat + Len(num, den);
            }
            else if (beats[at] == 'C') {
                // "C" is the unit length, "C3" three of them
                ++at;
                const int count = IsDigit((at < beats.size() ? beats[at] : ' ')) ? ReadNumber(beats, at) : 1;
                beat = beat + unit * Len(std::max(count, 1), 1);
            }
            else {
                ++at;
            }
        }
        tempo.m_beat = beat.IsZero() ? Len(1, 4) : beat;
        return tempo;
    }

    //------------------------------------------------------------------------
    // Reading: a file into its tunes
    //------------------------------------------------------------------------

    class Reader {
    public:
        std::vector<Tune> Read(const std::string &text);

    private:
        void HeaderField(Tune &tune, char name, const std::string &value, int line);
        void BodyField(Tune &tune, char name, const std::string &value, int line);
        void Directive(Tune &tune, const std::string &text, bool inBody);
        void StartBody(Tune &tune, int line);
        Voice &UseVoice(Tune &tune, const std::string &field);
        Voice &Current(Tune &tune);
        void Music(Tune &tune, const std::string &code, int line);
        void BarLine(Tune &tune, const std::string &code, size_t &i);
        void Ending(Voice &voice, const std::string &code, size_t &i);
        bool ReadHead(Tune &tune, Voice &voice, const std::string &code, size_t &i, Head &head);
        Len ReadLength(const std::string &code, size_t &i, bool &isZero);
        Event &Add(Tune &tune, Voice &voice, Event event);
        void Broken(Voice &voice, const std::string &code, size_t &i);
        void Chord(Tune &tune, Voice &voice, const std::string &code, size_t &i, char closing);
        void Mark(Tune &tune, Voice &voice, const std::string &name);
        void MarksAtBarEnd(Voice &voice);
        void CloseBar(Voice &voice, data_BARRENDITION sign);
        void Symbol(Tune &tune, Voice &voice, const std::string &definition);
        void Lyrics(Tune &tune, const std::string &text, bool isContinued);
        void SymbolLine(Tune &tune, const std::string &text);
        Event *EventAt(Voice &voice, const EventRef &ref);
        void NotRead(Tune &tune, const std::string &what);
    };

    void Reader::NotRead(Tune &tune, const std::string &what)
    {
        if (tune.m_notRead.insert(what).second) LogWarning("ABC import: not read: %s", what.c_str());
    }

    /** Whether [version] ("2.1", "2.0") is 2.1 or later: the standard is then read strictly (2.1, 12) */
    static bool IsStrict(const std::string &version)
    {
        size_t at = 0;
        const int major = ReadNumber(version, at);
        int minor = 0;
        if ((at < version.size()) && (version[at] == '.')) {
            ++at;
            minor = std::max(ReadNumber(version, at), 0);
        }
        return (major > 2) || ((major == 2) && (minor >= 1));
    }

    std::vector<Tune> Reader::Read(const std::string &text)
    {
        std::vector<Tune> tunes;
        // The version the file says on its first line ("%abc-2.1"), after a byte order mark. None: loose
        bool isStrictFile = false;
        {
            std::string first = text.substr(0, text.find('\n'));
            if (first.rfind("\xEF\xBB\xBF", 0) == 0) first.erase(0, 3);
            if ((first.rfind("%abc-", 0) == 0)) isStrictFile = IsStrict(Trimmed(first.substr(5)));
        }
        bool inHeader = false;
        bool inBody = false;
        bool inText = false;
        char lastField = 0;
        std::istringstream lines(text);
        std::string line;
        int lineNum = 1;
        // The file header: the fields and directives a file begins with, up to its first empty line. They set the
        // defaults of every tune of the file, and a tune's own fields override them (2.2.2)
        std::vector<std::pair<std::string, int>> fileHeader;
        bool inFileHeader = true;
        while (std::getline(lines, line)) {
            ++lineNum;
            if (!line.empty() && (line.back() == '\r')) line.pop_back();
            if (inFileHeader && tunes.empty()) {
                const bool isEmpty = (line.find_first_not_of(" \t") == std::string::npos);
                const bool isField = (line.size() >= 2) && (line[1] == ':') && IsAlpha(line[0]) && (line[0] != 'X');
                if (isEmpty) {
                    if (!fileHeader.empty()) inFileHeader = false;
                    continue;
                }
                if (isField || (line.rfind("%%", 0) == 0)) {
                    fileHeader.push_back({ line, lineNum });
                    continue;
                }
                if (line.rfind("%", 0) != 0) inFileHeader = false;
            }
            // A tune begins at its X, with a number after it or without, and its header runs to its key
            if (!line.empty() && (line[0] == 'X') && ((line.size() == 1) || (line[1] == ':')) && !inHeader) {
                tunes.emplace_back();
                tunes.back().m_strict = isStrictFile;
                for (const auto &[headerLine, headerLineNum] : fileHeader) {
                    if (headerLine.rfind("%%", 0) == 0) {
                        this->Directive(tunes.back(), headerLine.substr(2), false);
                    }
                    else {
                        this->HeaderField(tunes.back(), headerLine[0], headerLine.substr(2), headerLineNum);
                    }
                }
                if (line.size() > 2) tunes.back().m_number = Trimmed(line.substr(2).substr(0, line.substr(2).find('%')));
                tunes.back().m_symbols = { { '~', "!roll!" }, { 'H', "!fermata!" }, { 'L', "!accent!" },
                    { 'M', "!lowermordent!" }, { 'O', "!coda!" }, { 'P', "!uppermordent!" }, { 'S', "!segno!" },
                    { 'T', "!trill!" }, { 'u', "!upbow!" }, { 'v', "!downbow!" } };
                inHeader = true;
                inBody = false;
                continue;
            }
            if (!inHeader && !inBody) continue;
            Tune &tune = tunes.back();
            // text between %%begintext and %%endtext is no music
            if (line.rfind("%%begintext", 0) == 0) inText = true;
            if (inText) {
                if (line.rfind("%%endtext", 0) == 0) inText = false;
                continue;
            }
            if (line.find_first_not_of(" \t") == std::string::npos) {
                // an empty line ends the tune; in a header it is passed over
                if (inBody) inBody = false;
                continue;
            }
            if (line.rfind("%%", 0) == 0) {
                this->Directive(tune, line.substr(2), inBody);
                continue;
            }
            if (line[0] == '%') continue;
            const bool hasColon = (line.size() >= 2) && (line[1] == ':');
            if (inHeader) {
                if (!hasColon) continue;
                const std::string value = line.substr(2);
                if (line[0] == '+') {
                    // (a field carried on: read for lyrics only, which a header has none of)
                    continue;
                }
                this->HeaderField(tune, line[0], value, lineNum);
                if (line[0] == 'K') {
                    inHeader = false;
                    inBody = true;
                    this->StartBody(tune, lineNum);
                }
                continue;
            }
            // In the tune: a field on its own line, or music. A line that begins with a bar line is music ("|:",
            // "::"), and so is a note before one ("A:|"); the fields a tune's body can hold are known
            static const std::string bodyFields = "IKLMmNPQRrsTUVWw+";
            const bool isField = hasColon
                && ((bodyFields.find(line[0]) != std::string::npos)
                    || (IsAlpha(line[0]) && !((line.size() > 2) && ((line[2] == '|') || (line[2] == ':')))
                        && (std::string("ABCDEFGabcdefgzxZXy").find(line[0]) == std::string::npos)));
            if (isField) {
                const char name = line[0];
                if (name == '+') {
                    if (lastField == 'w') {
                        this->Lyrics(tune, line.substr(2), true);
                    }
                    else {
                        this->NotRead(tune, "a field carried on by \"+:\" that is no line of lyrics");
                    }
                    continue;
                }
                lastField = name;
                this->BodyField(tune, name, line.substr(2), lineNum);
            }
            else {
                lastField = 0;
                this->Music(tune, line, lineNum);
            }
        }
        // (a tune with no key has no music: its header never ended)
        if (inHeader) tunes.pop_back();
        // a jump written after a tune's last note, with no bar line after it
        for (Tune &tune : tunes) {
            for (Voice &voice : tune.m_voices) {
                voice.m_overlay = -1;
                if (!voice.m_bars.empty()) this->MarksAtBarEnd(voice);
            }
        }
        return tunes;
    }

    /** A field's value without its comment and the white space around it */
    static std::string FieldValue(const std::string &value)
    {
        std::string text;
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '%') {
                if ((i > 0) && (value[i - 1] == '\\')) {
                    text.back() = '%';
                    continue;
                }
                break;
            }
            text += value[i];
        }
        return Trimmed(text);
    }

    void Reader::HeaderField(Tune &tune, char name, const std::string &raw, int line)
    {
        const std::string value = FieldValue(raw);
        if (value.empty()) return;
        switch (name) {
            case 'T': tune.m_title.push_back({ Decoded(value), line }); break;
            case 'C': tune.m_composer.push_back({ Decoded(value), line }); break;
            case 'H': tune.m_history.push_back({ Decoded(value), line }); break;
            case 'O': tune.m_origin.push_back({ Decoded(value), line }); break;
            case 'B':
            case 'D':
            case 'F':
            case 'G':
            case 'N':
            case 'R':
            case 'S':
            case 'Z': tune.m_info.push_back({ { Decoded(value), line }, name }); break;
            case 'I': this->Directive(tune, value, false); break;
            case 'K': {
                ReadKey(value, tune.m_key);
                ReadStaffProperties(value, tune.m_clef, tune.m_octave, tune.m_transpose, tune.m_stafflines);
                break;
            }
            case 'L': {
                const Len unit = ReadUnit(value);
                if (!unit.IsZero()) tune.m_unit = unit;
                break;
            }
            case 'M': tune.m_meter = ReadMeter(value); break;
            case 'P': {
                // The order of the parts: letters, a number after a letter or a group in round brackets for how
                // often, and dots anywhere (3.1.9). A field that says anything else ("Play AABA last time") is no
                // order this reader can follow: the tune is played as written, and that is said
                const bool isOrder = !value.empty()
                    && (value.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789(). ") == std::string::npos);
                if (isOrder) {
                    tune.m_parts = value;
                }
                else {
                    this->NotRead(tune, "an order of parts (P:) in other words than letters, numbers and brackets");
                }
                break;
            }
            case 'Q': {
                // (the unit a number alone counts is the tune's, which may be said after it: read at the body's start)
                tune.m_tempo.m_said = true;
                tune.m_tempo.m_words = value;
                break;
            }
            case 'U': {
                const size_t equals = value.find('=');
                if ((equals != std::string::npos) && !Trimmed(value.substr(0, equals)).empty()) {
                    tune.m_symbols[Trimmed(value.substr(0, equals))[0]] = Trimmed(value.substr(equals + 1));
                }
                break;
            }
            case 'V': this->UseVoice(tune, value); break;
            case 'm': this->NotRead(tune, "a macro (m:)"); break;
            case 'X': break;
            default: break;
        }
    }

    void Reader::StartBody(Tune &tune, int line)
    {
        tune.m_firstLine = line;
        tune.m_firstMeter = tune.m_meter;
        // The unit length when none is said: a sixteenth for a meter shorter than 3/4, an eighth otherwise
        if (tune.m_unit.IsZero()) {
            const Len meter = tune.m_meter.Length();
            tune.m_unit = (!meter.IsZero() && (meter < Len(3, 4))) ? Len(1, 16) : Len(1, 8);
        }
        if (tune.m_tempo.m_said) tune.m_tempo = ReadTempo(tune.m_tempo.m_words, tune.m_unit);
        // The voices the header named have the tune's unit, key and clef, where they said none of their own
        for (Voice &voice : tune.m_voices) {
            voice.m_unit = tune.m_unit;
            voice.m_key = tune.m_key;
            if (!voice.m_clef.m_shape) voice.m_clef = tune.m_clef;
            if (!voice.m_stafflines) voice.m_stafflines = tune.m_stafflines;
            if (!voice.m_saidOctave) voice.m_octave = tune.m_octave;
            if (!voice.m_saidTranspose) voice.m_transpose = tune.m_transpose;
            if (voice.m_program < 0) voice.m_program = tune.m_program;
            if (voice.m_channel < 0) voice.m_channel = tune.m_channel;
            if (voice.m_volume < 0) voice.m_volume = tune.m_volume;
        }
        tune.m_current = 0;
    }

    Voice &Reader::Current(Tune &tune)
    {
        if (tune.m_voices.empty()) this->UseVoice(tune, "1");
        return tune.m_voices.at(std::clamp(tune.m_current, 0, static_cast<int>(tune.m_voices.size()) - 1));
    }

    /** V: the voice named first in [field], made if it is new, with what the field says of it; it is read from now on */
    Voice &Reader::UseVoice(Tune &tune, const std::string &field)
    {
        Words words(field);
        std::string id;
        words.Next(id);
        int index = -1;
        for (size_t i = 0; i < tune.m_voices.size(); ++i) {
            if (tune.m_voices[i].m_id == id) index = static_cast<int>(i);
        }
        if (index < 0) {
            // (a tune may name voices past any sense: they are read into the last one)
            if (tune.m_voices.size() >= 64) {
                index = static_cast<int>(tune.m_voices.size()) - 1;
            }
            else {
                Voice voice;
                voice.m_id = id;
                voice.m_unit = tune.m_unit;
                voice.m_key = tune.m_key;
                voice.m_clef = tune.m_clef;
                voice.m_octave = tune.m_octave;
                voice.m_transpose = tune.m_transpose;
                voice.m_stafflines = tune.m_stafflines;
                voice.m_program = tune.m_program;
                voice.m_channel = tune.m_channel;
                voice.m_volume = tune.m_volume;
                voice.m_bars.emplace_back();
                tune.m_voices.push_back(voice);
                index = static_cast<int>(tune.m_voices.size()) - 1;
            }
        }
        Voice &voice = tune.m_voices[index];
        std::string rest;
        rest = words.Rest();
        for (const char *name : { "name", "nm" }) {
            const std::string value = Property(rest, name);
            if (!value.empty()) voice.m_name = Decoded(value);
        }
        for (const char *name : { "snm", "sname", "subname" }) {
            const std::string value = Property(rest, name);
            if (!value.empty()) voice.m_shortName = Decoded(value);
        }
        Clef clef = voice.m_clef;
        if (!Property(rest, "octave").empty()) voice.m_saidOctave = true;
        if (!Property(rest, "transpose").empty()) voice.m_saidTranspose = true;
        if (ReadStaffProperties(rest, clef, voice.m_octave, voice.m_transpose, voice.m_stafflines)) {
            // in the tune, a clef said for a voice that has music is a change of clef there
            const bool hasMusic = (voice.m_bars.size() > 1) || !voice.m_bars.back().IsEmpty();
            if (hasMusic) {
                Change change;
                change.m_kind = Change::CLEF;
                change.m_clef = clef;
                voice.m_changes.push_back(change);
            }
            else {
                voice.m_clef = clef;
            }
        }
        tune.m_current = index;
        return voice;
    }

    void Reader::Directive(Tune &tune, const std::string &text, bool inBody)
    {
        Words words(text);
        std::string word;
        if (!(words.Next(word))) return;
        if (word == "abc-version") {
            std::string version;
            if (words.Next(version)) tune.m_strict = IsStrict(version);
        }
        else if ((word == "score") || (word == "staves")) {
            std::string layout;
            layout = words.Rest();
            tune.m_layout = FieldValue(layout);
        }
        else if (word == "MIDI") {
            std::string what;
            words.Next(what);
            std::vector<int> numbers;
            int number;
            while (words.Next(number)) numbers.push_back(number);
            int *program = &tune.m_program, *channel = &tune.m_channel, *volume = &tune.m_volume;
            if (inBody || !tune.m_voices.empty()) {
                Voice &voice = this->Current(tune);
                program = &voice.m_program;
                channel = &voice.m_channel;
                volume = &voice.m_volume;
            }
            if ((what == "program") && !numbers.empty()) {
                // "program n", or "program channel n"
                if (numbers.size() >= 2) *channel = std::clamp(numbers[0], 1, 16);
                *program = std::clamp(numbers.back(), 0, 127);
            }
            else if ((what == "channel") && !numbers.empty()) {
                *channel = std::clamp(numbers[0], 1, 16);
            }
            else if ((what == "control") && (numbers.size() >= 2) && (numbers[0] == 7)) {
                *volume = std::clamp(numbers[1], 0, 127);
            }
        }
        else if (word == "propagate-accidentals") {
            std::string mode;
            words.Next(mode);
            tune.m_propagate = (mode == "not") ? 0 : (mode == "octave") ? 1 : 2;
        }
        else if (word == "linebreak") {
            std::string how;
            how = words.Rest();
            tune.m_lineBreaks = (how.find("<none>") == std::string::npos);
        }
        else if (word == "decoration") {
            std::string character;
            if (words.Next(character)) tune.m_decoration = character[0];
        }
    }

    void Reader::BodyField(Tune &tune, char name, const std::string &raw, int line)
    {
        const std::string value = FieldValue(raw);
        switch (name) {
            case 'K': {
                Voice &voice = this->Current(tune);
                Key key = voice.m_key;
                if (ReadKey(value, key)) {
                    voice.m_key = key;
                    Change change;
                    change.m_kind = Change::KEY;
                    change.m_key = key;
                    voice.m_changes.push_back(change);
                }
                Clef clef = voice.m_clef;
                int stafflines = 0;
                if (ReadStaffProperties(value, clef, voice.m_octave, voice.m_transpose, stafflines)) {
                    Change change;
                    change.m_kind = Change::CLEF;
                    change.m_clef = clef;
                    voice.m_changes.push_back(change);
                }
                break;
            }
            case 'L': {
                const Len unit = ReadUnit(value);
                if (!unit.IsZero()) this->Current(tune).m_unit = unit;
                break;
            }
            case 'M': {
                tune.m_meter = ReadMeter(value);
                Change change;
                change.m_kind = Change::METER;
                change.m_meter = tune.m_meter;
                this->Current(tune).m_changes.push_back(change);
                break;
            }
            case 'Q': {
                Change change;
                change.m_kind = Change::TEMPO;
                change.m_tempo = ReadTempo(value, this->Current(tune).m_unit);
                this->Current(tune).m_changes.push_back(change);
                break;
            }
            case 'V': this->UseVoice(tune, value); break;
            case 'P': {
                // A part starts where its field stands, and its name is printed above its start (3.1.9). Where the
                // header gives the order of the parts, the part is played by it, and a part can only begin at a
                // measure: said where a bar has begun (after a pickup, with no bar line written before the part's
                // first note) it closes that bar, with no bar line drawn, since the file writes none. With no order
                // in the header the field is the part's name only, and the bar stays as written
                // A part of the order is named by a letter, as the header's order names them ("P:ABAC"); a field
                // that says anything else ("P:D.S.") names no part, and is a text to print only
                Voice &voice = this->Current(tune);
                if (value.empty()) break;
                const bool isPart = (value.size() == 1) && (value[0] >= 'A') && (value[0] <= 'Z');
                if (isPart && !tune.m_parts.empty() && !voice.Current().IsEmpty()) {
                    this->CloseBar(voice, BARRENDITION_invis);
                }
                if (isPart && voice.Current().IsEmpty()) voice.Current().m_part = value;
                voice.m_partLabel = value;
                break;
            }
            case 'I': this->Directive(tune, value, true); break;
            case 'U': {
                const size_t equals = value.find('=');
                if ((equals != std::string::npos) && !Trimmed(value.substr(0, equals)).empty()) {
                    tune.m_symbols[Trimmed(value.substr(0, equals))[0]] = Trimmed(value.substr(equals + 1));
                }
                break;
            }
            case 'w': this->Lyrics(tune, raw, false); break;
            case 's': this->SymbolLine(tune, raw); break;
            case 'W': this->NotRead(tune, "words printed after the tune (W:)"); break;
            case 'm': this->NotRead(tune, "a macro (m:)"); break;
            case 'T':
            case 'N':
            case 'R':
            case 'r': break;
            default: break;
        }
    }

    //------------------------------------------------------------------------
    // Reading: a line of music
    //------------------------------------------------------------------------

    /** A length after a note: "2", "/", "//", "/4", "3/2", "3/". One of 0 ("C0") is said by [isZero] */
    Len Reader::ReadLength(const std::string &code, size_t &i, bool &isZero)
    {
        isZero = false;
        int num = 1;
        int den = 1;
        if ((i < code.size()) && IsDigit(code[i])) {
            num = ReadNumber(code, i);
            // (a length too long to mean anything is the unit)
            if (num < 0) num = 1;
            if (num == 0) {
                isZero = true;
                num = 1;
            }
        }
        // "/" and digits, or slashes alone ("/" is a half, "//" a quarter): a slash after the digits of a divisor
        // ("a/4/") is no part of the length (4.3). It was taken for one more halving
        if ((i < code.size()) && (code[i] == '/')) {
            ++i;
            if ((i < code.size()) && IsDigit(code[i])) {
                const int divisor = ReadNumber(code, i);
                if (divisor >= 1) den = std::min(divisor, 1024);
            }
            else {
                den = 2;
                while ((i < code.size()) && (code[i] == '/') && (den < 1024)) {
                    den *= 2;
                    ++i;
                }
            }
        }
        return Len(num, den);
    }

    /** A note's head at [i]: its sign, its letter, its octave marks. False, and [i] where it was, when none is there */
    bool Reader::ReadHead(Tune &tune, Voice &voice, const std::string &code, size_t &i, Head &head)
    {
        size_t at = i;
        int accid = kNoAccid;
        if (at < code.size()) {
            if (code[at] == '^') {
                accid = 1;
                ++at;
                if ((at < code.size()) && (code[at] == '^')) {
                    accid = 2;
                    ++at;
                }
            }
            else if (code[at] == '_') {
                accid = -1;
                ++at;
                if ((at < code.size()) && (code[at] == '_')) {
                    accid = -2;
                    ++at;
                }
            }
            else if (code[at] == '=') {
                accid = 0;
                ++at;
            }
        }
        if (at >= code.size()) return false;
        const char letter = code[at];
        const bool isUpper = (letter >= 'A') && (letter <= 'G');
        const bool isLower = (letter >= 'a') && (letter <= 'g');
        if (!isUpper && !isLower) return false;
        ++at;
        head.m_step = ToLower(letter);
        head.m_oct = isUpper ? 4 : 5;
        while ((at < code.size()) && ((code[at] == '\'') || (code[at] == ','))) {
            head.m_oct += (code[at] == '\'') ? 1 : -1;
            ++at;
        }
        head.m_oct = std::clamp(head.m_oct + voice.m_octave, 0, 9);
        head.m_accid = accid;
        i = at;
        return true;
    }

    /**
     * Adds [event] to the voice with what waits for it (decorations, texts, slurs, changes, a tuplet, the second
     * half of a broken rhythm), and works out what its notes sound: a sign holds to the bar line (for its note, its
     * octave or every octave, as the tune says), a note tied to keeps the sound of the one it is tied from, and
     * the key says the rest.
     */
    Event &Reader::Add(Tune &tune, Voice &voice, Event event)
    {
        event.m_marks = voice.m_marks;
        voice.m_marks.clear();
        event.m_texts = voice.m_texts;
        voice.m_texts.clear();
        event.m_slurStarts = voice.m_slurStarts;
        voice.m_slurStarts.clear();
        event.m_changes = voice.m_changes;
        voice.m_changes.clear();
        event.m_breaksBeam = voice.m_breaksBeam;
        voice.m_breaksBeam = false;
        if (!event.m_grace) {
            event.m_partLabel = voice.m_partLabel;
            voice.m_partLabel.clear();
        }
        if (voice.m_inGrace) {
            event.m_grace = true;
            event.m_graceSlash = voice.m_graceSlash;
            event.m_graceGroup = voice.m_graceGroup;
        }
        const bool isTimed = !event.m_grace && (event.m_kind != EventKind::BarRest) && (event.m_kind != EventKind::BarSpace);
        if (isTimed && voice.m_tupletNum) {
            event.m_tupletNum = voice.m_tupletNum;
            event.m_tupletBase = voice.m_tupletBase;
            event.m_tupletCount = voice.m_tupletCount;
            voice.m_tupletNum = 0;
        }
        if (isTimed && voice.m_brokenNext) {
            const int times = std::min(std::abs(voice.m_brokenNext), 3);
            event.m_length = event.m_length
                * ((voice.m_brokenNext > 0) ? Len(1, 1LL << times) : Len((2LL << times) - 1, 1LL << times));
            voice.m_brokenNext = 0;
        }
        const bool keepsTies = event.m_grace;
        for (Head &head : event.m_heads) {
            const int inBar = (head.m_step - 'a') + ((tune.m_propagate == 1) ? 8 * (head.m_oct + 1) : 0);
            const int tiedKey = (head.m_step - 'a') + 8 * (head.m_oct + 1);
            if (head.m_accid != kNoAccid) {
                head.m_alter = head.m_accid;
                if (tune.m_propagate > 0) voice.m_barAlter[inBar] = head.m_alter;
            }
            else if (!event.m_grace && voice.m_tied.contains(tiedKey)) {
                head.m_alter = voice.m_tied.at(tiedKey);
            }
            else if (voice.m_barAlter.contains(inBar)) {
                head.m_alter = voice.m_barAlter.at(inBar);
            }
            else {
                const auto found = voice.m_key.m_alter.find(head.m_step);
                head.m_alter = (found == voice.m_key.m_alter.end()) ? 0 : found->second;
            }
        }
        if (!keepsTies) {
            voice.m_tied.clear();
            for (const Head &head : event.m_heads) {
                if (head.m_tie) voice.m_tied[(head.m_step - 'a') + 8 * (head.m_oct + 1)] = head.m_alter;
            }
        }
        std::vector<Event> &events = voice.Events();
        events.push_back(event);
        if (!event.m_grace && ((event.m_kind == EventKind::Note) || (event.m_kind == EventKind::Chord))) {
            voice.m_lineEvents.push_back(
                { static_cast<int>(voice.m_bars.size()) - 1, voice.m_overlay, static_cast<int>(events.size()) - 1 });
        }
        return events.back();
    }

    /** A chord at [i], after its opening: its notes up to [closing], each with its own length; the chord's is its
     * first note's, times the length after the closing */
    void Reader::Chord(Tune &tune, Voice &voice, const std::string &code, size_t &i, char closing)
    {
        Event event;
        event.m_kind = EventKind::Chord;
        // A chord holds notes, and decorations and texts set on it; it cannot hold a bar line. One that no closing
        // character closes before a bar line ends there, and that is said. (It ran to the end of the line, and the bar
        // lines it met were lost: two bars became one)
        bool isClosed = false;
        while (i < code.size()) {
            if (code[i] == closing) {
                isClosed = true;
                break;
            }
            if (code[i] == '|') break;
            Head head;
            if (this->ReadHead(tune, voice, code, i, head)) {
                bool isZero = false;
                head.m_length = this->ReadLength(code, i, isZero);
                if ((i < code.size()) && (code[i] == '-')) {
                    head.m_tie = true;
                    ++i;
                }
                // (a chord of more notes than a hand has fingers for many times over is cut)
                if (event.m_heads.size() < 64) event.m_heads.push_back(head);
            }
            else if ((code[i] == tune.m_decoration) || (code[i] == '"')) {
                // a decoration or a text inside the chord is the chord's
                const size_t end = code.find(code[i], i + 1);
                if (end == std::string::npos) {
                    i = code.size();
                    break;
                }
                if (code[i] == '"') {
                    this->Symbol(tune, voice, code.substr(i, end - i + 1));
                }
                else {
                    this->Mark(tune, voice, code.substr(i + 1, end - i - 1));
                }
                i = end + 1;
            }
            else {
                if (!IsSpace(code[i])) {
                    this->NotRead(tune, StringFormat("the character '%c' in a chord", code[i]));
                }
                ++i;
            }
        }
        if (!isClosed) this->NotRead(tune, "a chord that nothing closes before its bar line ends: it ends there");
        if (isClosed) ++i;
        bool isZero = false;
        const Len outer = isClosed ? this->ReadLength(code, i, isZero) : Len(1, 1);
        if (event.m_heads.empty()) return;
        event.m_length = voice.m_unit * event.m_heads.front().m_length * outer;
        event.m_stemless = isZero;
        this->Add(tune, voice, event);
        this->Broken(voice, code, i);
    }

    /** ">" and "<" after the event just added: it is dotted and the next shortened, or the other way round */
    void Reader::Broken(Voice &voice, const std::string &code, size_t &i)
    {
        int times = 0;
        const char sign = (i < code.size()) ? code[i] : 0;
        if ((sign != '>') && (sign != '<')) return;
        while ((i < code.size()) && (code[i] == sign)) {
            ++times;
            ++i;
        }
        std::vector<Event> &events = voice.Events();
        if (events.empty() || events.back().m_grace) return;
        times = std::min(times, 3);
        Event &event = events.back();
        const Len dotted((2LL << times) - 1, 1LL << times);
        const Len shortened(1, 1LL << times);
        event.m_length = event.m_length * ((sign == '>') ? dotted : shortened);
        voice.m_brokenNext = (sign == '>') ? times : -times;
    }

    /**
     * A jump written after a bar's last note, before its bar line ("... !D.S.! |"), is said at the bar's end: it is
     * the last event's. (A decoration waits for the note after it, and that one stands in the next bar, or nowhere.)
     */
    void Reader::MarksAtBarEnd(Voice &voice)
    {
        static const std::set<std::string> jumps = { "D.S.", "D.C.", "dacapo", "dacoda", "fine" };
        std::vector<Event> &events = voice.Events();
        if (events.empty() || voice.m_marks.empty()) return;
        std::vector<std::string> waiting;
        for (const std::string &name : voice.m_marks) {
            if (jumps.contains(name)) {
                events.back().m_marks.push_back(name);
            }
            else {
                waiting.push_back(name);
            }
        }
        voice.m_marks = waiting;
    }

    /** Ends the voice's bar with [sign] on its right, and begins the next one */
    void Reader::CloseBar(Voice &voice, data_BARRENDITION sign)
    {
        voice.Current().m_right = sign;
        this->MarksAtBarEnd(voice);
        voice.m_bars.emplace_back();
        voice.m_overlay = -1;
        voice.m_barAlter.clear();
        voice.m_tupletNum = 0;
        if (!voice.m_ending.empty()) {
            voice.Current().m_ending = voice.m_ending;
            voice.m_ending.clear();
        }
    }

    /** The decorations the standard names (4.14), and the dynamics: what "+...+" holds in the dialect of 2.0 */
    static bool IsDecorationName(const std::string &name)
    {
        static const std::set<std::string> names = { "trill", "trill(", "trill)", "lowermordent", "uppermordent",
            "mordent", "pralltriller", "roll", "turn", "turnx", "invertedturn", "invertedturnx", "arpeggio", ">",
            "accent", "emphasis", "fermata", "invertedfermata", "tenuto", "0", "1", "2", "3", "4", "5", "+", "plus",
            "snap", "slide", "wedge", "upbow", "downbow", "open", "thumb", "breath", "pppp", "ppp", "pp", "p", "mp",
            "mf", "f", "ff", "fff", "ffff", "sfz", "crescendo(", "<(", "crescendo)", "<)", "diminuendo(", ">(",
            "diminuendo)", ">)", "segno", "coda", "D.S.", "D.C.", "dacoda", "dacapo", "fine", "shortphrase",
            "mediumphrase", "longphrase", "staccato", "marcato", "^", "wedge" };
        return names.contains(name);
    }

    /** A decoration by its name: it waits for its note. The line break of older ABC ("!" alone) is none */
    void Reader::Mark(Tune &tune, Voice &voice, const std::string &name)
    {
        if (name.empty()) return;
        voice.m_marks.push_back(name);
    }

    /** What a letter stands for (U:, or the standard's own): a decoration between "!", or a text in quotes */
    void Reader::Symbol(Tune &tune, Voice &voice, const std::string &definition)
    {
        if (definition.size() < 2) return;
        const char first = definition.front();
        if (((first == '!') || (first == '+')) && (definition.back() == first)) {
            this->Mark(tune, voice, definition.substr(1, definition.size() - 2));
        }
        else if ((first == '"') && (definition.back() == '"')) {
            TextMark text;
            std::string inner = definition.substr(1, definition.size() - 2);
            if (!inner.empty() && (std::string("^_<>@").find(inner[0]) != std::string::npos)) {
                text.m_place = inner[0];
                inner.erase(0, 1);
            }
            text.m_text = Decoded(inner);
            voice.m_texts.push_back(text);
        }
    }

    /** The numbers of an ending at [i] ("1", "1,3", "1-3"), which is moved past them: as "1, 2, 3" */
    void Reader::Ending(Voice &voice, const std::string &code, size_t &i)
    {
        std::vector<int> numbers;
        while (i < code.size()) {
            if (!IsDigit(code[i])) break;
            const int first = ReadNumber(code, i);
            int last = first;
            if ((i + 1 < code.size()) && (code[i] == '-') && IsDigit(code[i + 1])) {
                ++i;
                last = ReadNumber(code, i);
            }
            // (a range past any sense is its first number)
            if ((first < 1) || (last < first) || (last - first > 32)) last = first;
            for (int n = first; (n >= 1) && (n <= last); ++n) numbers.push_back(n);
            if ((i + 1 < code.size()) && (code[i] == ',') && IsDigit(code[i + 1])) {
                ++i;
                continue;
            }
            break;
        }
        std::string text;
        for (int n : numbers) text += (text.empty() ? "" : ", ") + std::to_string(n);
        if (text.empty()) return;
        // An ending starts at a bar's beginning. Met after the bar has begun, it starts at the next one
        if (voice.Current().IsEmpty()) {
            voice.Current().m_ending = text;
        }
        else {
            voice.m_ending = text;
        }
    }

    /**
     * A bar line at [i], which is moved past it: any of | || |] [| |: :| :: :|: :||: |:: ::| [|] .| with the
     * colons of a repeat counted (":: ... ::" is played three times). After the bar's first event it ends the bar;
     * before, it stands on the bar's left, or is the end of the bar before.
     */
    void Reader::BarLine(Tune &tune, const std::string &code, size_t &i)
    {
        Voice &voice = this->Current(tune);
        int before = 0;
        int after = 0;
        bool isDotted = false;
        if (code[i] == '.') {
            isDotted = true;
            ++i;
        }
        while ((i < code.size()) && (code[i] == ':')) {
            ++before;
            ++i;
        }
        std::string bars;
        while (i < code.size()) {
            const char c = code[i];
            if (c == '|') {
                bars += c;
            }
            else if ((c == '[') && (i + 1 < code.size()) && (code[i + 1] == '|')) {
                bars += c;
            }
            else if ((c == ']') && !bars.empty() && (bars.back() == '|')) {
                bars += c;
            }
            else {
                break;
            }
            ++i;
        }
        // (colons after a bar line start a repeat; with none before, two colons alone are an end and a start)
        if (!bars.empty()) {
            while ((i < code.size()) && (code[i] == ':')) {
                ++after;
                ++i;
            }
        }
        else if (before >= 2) {
            after = before / 2;
            before -= after;
        }
        // The sign: an end where colons stand before it, a start where they stand after, else what its strokes say
        data_BARRENDITION sign = BARRENDITION_single;
        if (before > 0) {
            sign = BARRENDITION_rptend;
        }
        else if (after > 0) {
            sign = BARRENDITION_rptstart;
        }
        else if (bars == "[|]") {
            sign = BARRENDITION_invis;
        }
        else if (bars.find(']') != std::string::npos) {
            sign = BARRENDITION_end;
        }
        else if ((bars.size() >= 2) || (bars.find('[') != std::string::npos)) {
            sign = BARRENDITION_dbl;
        }
        else if (isDotted) {
            sign = BARRENDITION_dotted;
        }
        const bool isBoth = (before > 0) && (after > 0);
        // (how often a repeat is played is said by the colons of its end, or of its start)
        int times = std::min(before + 1, 100);
        if (before > 0) {
            times = std::max(times, voice.m_startTimes);
            voice.m_startTimes = 2;
        }
        if (after > 0) voice.m_startTimes = std::min(after + 1, 100);

        Bar &bar = voice.Current();
        if (bar.IsEmpty()) {
            // Before the bar's first event it stands on the bar's left: an end there is the end of the repeat before
            bar.m_left = isBoth ? BARRENDITION_rptboth : sign;
            if ((before > 0) && (voice.m_bars.size() >= 2)) {
                Bar &previous = voice.m_bars[voice.m_bars.size() - 2];
                previous.m_times = std::max(previous.m_times, times);
            }
        }
        else {
            if (before > 0) bar.m_times = std::max(bar.m_times, times);
            this->CloseBar(voice, sign);
            // an end and a start in one: the start is the next bar's
            if (isBoth) voice.Current().m_left = BARRENDITION_rptstart;
        }
        voice.m_breaksBeam = true;
        // an ending's number directly after the bar line: "|1", ":|2"
        if ((i < code.size()) && IsDigit(code[i])) this->Ending(voice, code, i);
    }

    void Reader::Music(Tune &tune, const std::string &code, int line)
    {
        bool endsLine = true;
        {
            // a voice's notes of this line are what a line of words after it is set under
            Voice &voice = this->Current(tune);
            voice.m_lineEvents.clear();
            voice.m_verse = 0;
            voice.m_breaksBeam = true;
        }
        size_t i = 0;
        while (i < code.size()) {
            Voice &voice = this->Current(tune);
            const char c = code[i];
            const char next = (i + 1 < code.size()) ? code[i + 1] : 0;
            if (IsSpace(c)) {
                voice.m_breaksBeam = true;
                ++i;
            }
            else if (c == '%') {
                break;
            }
            else if (c == '\\') {
                // the line goes on: no line break in the score
                endsLine = false;
                ++i;
            }
            else if (c == '`') {
                ++i;
            }
            else if ((c == '|') || ((c == ':') && ((next == '|') || (next == ':')))
                || ((c == '[') && (next == '|')) || ((c == '.') && (next == '|'))) {
                this->BarLine(tune, code, i);
            }
            else if ((c == '[') && IsDigit(next)) {
                ++i;
                this->Ending(voice, code, i);
            }
            else if ((c == '[') && (next == '"')) {
                // an ending named by a text: it names no pass
                const size_t end = code.find('"', i + 2);
                i = (end == std::string::npos) ? code.size() : end + 1;
                this->NotRead(tune, "an ending named by a text");
            }
            else if ((c == '[') && IsAlpha(next) && (i + 2 < code.size()) && (code[i + 2] == ':')) {
                // a field in the music: to its "]", or to the end of the line where it has none
                const size_t end = code.find(']', i);
                const std::string value = code.substr(i + 3, (end == std::string::npos) ? std::string::npos : end - i - 3);
                this->BodyField(tune, next, value, line);
                i = (end == std::string::npos) ? code.size() : end + 1;
            }
            else if (c == '[') {
                // a chord: its notes to the "]", each with its own length; the chord's is its first note's
                ++i;
                this->Chord(tune, voice, code, i, ']');
            }
            else if ((c == '+') && (tune.m_decoration != '+')) {
                // "+" before 2.1: a decoration of the dialect of 2.0 ("+trill+"), or a chord of the dialect before 1.6
                // ("+CEG+"). A file read strictly (version 2.1 or later) has neither (12.1.2, 12.1.3)
                const size_t end = code.find('+', i + 1);
                const std::string inner = (end == std::string::npos) ? "" : code.substr(i + 1, end - i - 1);
                if (end == std::string::npos) {
                    this->NotRead(tune, "a \"+\" that no other closes on its line");
                    ++i;
                }
                else if (tune.m_strict) {
                    this->NotRead(tune, "\"+...+\" in a file read strictly (no decoration and no chord in 2.1)");
                    i = end + 1;
                }
                else if (IsDecorationName(inner)) {
                    this->Mark(tune, voice, inner);
                    i = end + 1;
                }
                else {
                    ++i;
                    this->Chord(tune, voice, code, i, '+');
                }
            }
            else if (c == '(') {
                if (IsDigit(next)) {
                    // a tuplet: (p, (p:q, (p:q:r, (p::r
                    ++i;
                    // (any number an int holds: -1 for one that is longer)
                    const auto number = [&]() {
                        long long value = 0;
                        while ((i < code.size()) && IsDigit(code[i])) {
                            if (value <= INT_MAX) value = value * 10 + (code[i] - '0');
                            ++i;
                        }
                        return (value > INT_MAX) ? -1 : static_cast<int>(value);
                    };
                    const int p = number();
                    int q = 0;
                    int r = 0;
                    if ((i < code.size()) && (code[i] == ':')) {
                        ++i;
                        if ((i < code.size()) && IsDigit(code[i])) q = number();
                        if ((i < code.size()) && (code[i] == ':')) {
                            ++i;
                            if ((i < code.size()) && IsDigit(code[i])) r = number();
                        }
                    }
                    // A tuplet ABC cannot mean is none, and its notes are read as they are written: one of no notes,
                    // and one with a number too long for an int
                    if ((p < 1) || (q < 0) || (r < 0)) {
                        LogWarning("ABC import: a tuplet that cannot be one is read as its notes");
                    }
                    else {
                        if (!q) {
                            // The time when none is written: 2, 4 and 8 in 3, 3 and 6 in 2, any other in 3 in a
                            // compound meter (6/8, 9/8, 12/8) and in 2 otherwise
                            const int count = tune.m_meter.m_counts.empty()
                                ? 0
                                : std::accumulate(tune.m_meter.m_counts.begin(), tune.m_meter.m_counts.end(), 0);
                            const bool isCompound = (count > 3) && ((count % 3) == 0);
                            switch (p) {
                                case 2:
                                case 4:
                                case 8: q = 3; break;
                                case 3:
                                case 6: q = 2; break;
                                default: q = isCompound ? 3 : 2; break;
                            }
                        }
                        voice.m_tupletNum = p;
                        voice.m_tupletBase = q;
                        voice.m_tupletCount = (r > 0) ? r : p;
                    }
                }
                else {
                    voice.m_slurStarts.push_back(false);
                    ++i;
                }
            }
            else if (c == ')') {
                Event *last = voice.Last();
                if (last) ++last->m_slurEnds;
                ++i;
            }
            else if (c == '-') {
                Event *last = voice.Last();
                if (last && ((last->m_kind == EventKind::Note) || (last->m_kind == EventKind::Chord))) {
                    for (Head &head : last->m_heads) {
                        head.m_tie = true;
                        voice.m_tied[(head.m_step - 'a') + 8 * (head.m_oct + 1)] = head.m_alter;
                    }
                }
                ++i;
            }
            else if (c == '.') {
                if (next == '(') {
                    voice.m_slurStarts.push_back(true);
                    i += 2;
                }
                else if (next == '-') {
                    Event *last = voice.Last();
                    if (last && ((last->m_kind == EventKind::Note) || (last->m_kind == EventKind::Chord))) {
                        for (Head &head : last->m_heads) {
                            head.m_tie = true;
                            head.m_tieDotted = true;
                            voice.m_tied[(head.m_step - 'a') + 8 * (head.m_oct + 1)] = head.m_alter;
                        }
                    }
                    i += 2;
                }
                else {
                    this->Mark(tune, voice, "staccato");
                    ++i;
                }
            }
            else if (c == tune.m_decoration) {
                // A "!" that no other closes on its line, or one before white space, is the line break of older
                // ABC, and is passed over
                const size_t end = code.find(tune.m_decoration, i + 1);
                if ((end == std::string::npos) || IsSpace(next) || (next == 0)) {
                    ++i;
                }
                else {
                    this->Mark(tune, voice, code.substr(i + 1, end - i - 1));
                    i = end + 1;
                }
            }
            else if (c == '"') {
                // a text: to its closing quote, or to the end of the line
                const size_t end = code.find('"', i + 1);
                const std::string inner = code.substr(i + 1, (end == std::string::npos) ? std::string::npos : end - i - 1);
                this->Symbol(tune, voice, "\"" + inner + "\"");
                i = (end == std::string::npos) ? code.size() : end + 1;
            }
            else if (c == '{') {
                voice.m_inGrace = true;
                voice.m_graceSlash = (next == '/');
                ++voice.m_graceGroup;
                i += voice.m_graceSlash ? 2 : 1;
            }
            else if (c == '}') {
                voice.m_inGrace = false;
                ++i;
            }
            else if ((c == 'z') || (c == 'x')) {
                ++i;
                Event event;
                event.m_kind = (c == 'z') ? EventKind::Rest : EventKind::Space;
                bool isZero = false;
                event.m_length = voice.m_unit * this->ReadLength(code, i, isZero);
                this->Add(tune, voice, event);
                this->Broken(voice, code, i);
            }
            else if ((c == 'Z') || (c == 'X')) {
                // whole bars of rest, seen or unseen: "Z" alone is one bar
                ++i;
                Event event;
                event.m_kind = (c == 'Z') ? EventKind::BarRest : EventKind::BarSpace;
                // ("Z" with no number is one bar, and so is a number that is none: 0, or one too long)
                const bool hasNumber = IsDigit((i < code.size()) ? code[i] : ' ');
                const int bars = hasNumber ? ReadNumber(code, i) : 1;
                event.m_bars = std::clamp(bars, 1, 1000);
                event.m_barsSaid = hasNumber && (bars >= 1);
                this->Add(tune, voice, event);
            }
            else if (c == 'y') {
                // space on the page only
                ++i;
                while ((i < code.size()) && IsDigit(code[i])) ++i;
            }
            else if (c == '&') {
                // another voice over this bar, from the bar's start
                voice.Current().m_overlays.emplace_back();
                voice.m_overlay = static_cast<int>(voice.Current().m_overlays.size()) - 1;
                voice.m_barAlter.clear();
                voice.m_breaksBeam = true;
                ++i;
            }
            else if (c == '$') {
                // a line break in the score
                if (!voice.Current().IsEmpty()) {
                    voice.Current().m_lineEnd = true;
                    voice.Current().m_line = line;
                }
                else if (voice.m_bars.size() >= 2) {
                    voice.m_bars[voice.m_bars.size() - 2].m_lineEnd = true;
                    voice.m_bars[voice.m_bars.size() - 2].m_line = line;
                }
                ++i;
            }
            else if ((c == '>') || (c == '<')) {
                this->Broken(voice, code, i);
            }
            else {
                Head head;
                if (this->ReadHead(tune, voice, code, i, head)) {
                    Event event;
                    event.m_kind = EventKind::Note;
                    bool isZero = false;
                    event.m_length = voice.m_unit * this->ReadLength(code, i, isZero);
                    event.m_stemless = isZero;
                    event.m_heads.push_back(head);
                    this->Add(tune, voice, event);
                    this->Broken(voice, code, i);
                }
                else if (tune.m_symbols.contains(c)) {
                    this->Symbol(tune, voice, tune.m_symbols.at(c));
                    ++i;
                }
                else {
                    // The reserved characters are ignored, as the standard asks (8.1); anything else that is no part
                    // of the music is said
                    if (std::string("#*;?@").find(c) != std::string::npos) {
                        this->NotRead(tune, "a reserved character (# * ; ? @), ignored as the standard asks");
                    }
                    else {
                        this->NotRead(tune, StringFormat("the character '%c' in the music", c));
                    }
                    ++i;
                }
            }
        }
        Voice &voice = this->Current(tune);
        if (endsLine) {
            if (!voice.Current().IsEmpty()) {
                voice.Current().m_lineEnd = true;
                voice.Current().m_line = line;
            }
            else if (voice.m_bars.size() >= 2) {
                voice.m_bars[voice.m_bars.size() - 2].m_lineEnd = true;
                voice.m_bars[voice.m_bars.size() - 2].m_line = line;
            }
        }
    }

    //------------------------------------------------------------------------
    // Reading: lines of words and of symbols under the music
    //------------------------------------------------------------------------

    Event *Reader::EventAt(Voice &voice, const EventRef &ref)
    {
        if ((ref.m_bar < 0) || (ref.m_bar >= static_cast<int>(voice.m_bars.size()))) return NULL;
        Bar &bar = voice.m_bars[ref.m_bar];
        std::vector<Event> *events = &bar.m_events;
        if (ref.m_overlay >= 0) {
            if (ref.m_overlay >= static_cast<int>(bar.m_overlays.size())) return NULL;
            events = &bar.m_overlays[ref.m_overlay];
        }
        if ((ref.m_event < 0) || (ref.m_event >= static_cast<int>(events->size()))) return NULL;
        return &events->at(ref.m_event);
    }

    /**
     * w: the words under the voice's last line of music, a syllable a note: "-" divides a word, "_" holds the
     * syllable before for one more note, "*" passes a note over, "~" is a space inside a syllable, "\-" a hyphen in
     * one, and "|" goes on to the next bar. A second line of words under the same music is a second verse; one
     * carried on by "+:" goes on where the line before stopped.
     */
    void Reader::Lyrics(Tune &tune, const std::string &raw, bool isContinued)
    {
        Voice &voice = this->Current(tune);
        if (!isContinued) {
            ++voice.m_verse;
            voice.m_lyricAt = 0;
            voice.m_lyricBar = -1;
        }
        std::string text = raw;
        const size_t comment = text.find('%');
        if (comment != std::string::npos) text.resize(comment);
        std::string syllable;
        Event *previous = NULL;
        const auto place = [&](bool hasDash) {
            if (voice.m_lyricAt >= voice.m_lineEvents.size()) {
                syllable.clear();
                return;
            }
            Event *event = this->EventAt(voice, voice.m_lineEvents[voice.m_lyricAt]);
            voice.m_lyricBar = voice.m_lineEvents[voice.m_lyricAt].m_bar;
            ++voice.m_lyricAt;
            if (event && !syllable.empty()) {
                Syllable said;
                said.m_text = Decoded(syllable);
                said.m_dash = hasDash;
                event->m_syllables[voice.m_verse] = said;
                previous = event;
            }
            syllable.clear();
        };
        for (size_t i = 0; i < text.size(); ++i) {
            const char c = text[i];
            if ((c == '\\') && (i + 1 < text.size()) && (text[i + 1] == '-')) {
                syllable += '-';
                ++i;
            }
            else if (c == '~') {
                syllable += ' ';
            }
            else if (IsSpace(c)) {
                if (!syllable.empty()) place(false);
            }
            else if (c == '-') {
                // (a hyphen with no syllable before it stands for a note of its own)
                if (syllable.empty()) {
                    if (voice.m_lyricAt < voice.m_lineEvents.size()) ++voice.m_lyricAt;
                }
                else {
                    place(true);
                }
            }
            else if (c == '_') {
                if (!syllable.empty()) place(false);
                if (previous && previous->m_syllables.contains(voice.m_verse)) {
                    previous->m_syllables[voice.m_verse].m_held = true;
                }
                if (voice.m_lyricAt < voice.m_lineEvents.size()) ++voice.m_lyricAt;
            }
            else if (c == '*') {
                if (!syllable.empty()) place(false);
                if (voice.m_lyricAt < voice.m_lineEvents.size()) ++voice.m_lyricAt;
            }
            else if (c == '|') {
                if (!syllable.empty()) place(false);
                // On to the first note of the bar after the one the last syllable stands in (a syllable held past
                // its bar's end does not carry the words into the next bar but one)
                if (!voice.m_lineEvents.empty()) {
                    const int bar = (voice.m_lyricBar >= 0) ? voice.m_lyricBar : voice.m_lineEvents.front().m_bar;
                    voice.m_lyricAt = 0;
                    while ((voice.m_lyricAt < voice.m_lineEvents.size())
                        && (voice.m_lineEvents[voice.m_lyricAt].m_bar <= bar)) {
                        ++voice.m_lyricAt;
                    }
                    voice.m_lyricBar = bar + 1;
                }
            }
            else {
                syllable += c;
            }
        }
        if (!syllable.empty()) place(false);
    }

    /** s: a line of symbols under the voice's last line of music: decorations and texts, a note each, "*" for none */
    void Reader::SymbolLine(Tune &tune, const std::string &raw)
    {
        Voice &voice = this->Current(tune);
        std::string text = raw;
        size_t at = 0;
        size_t i = 0;
        const auto target = [&]() -> Event * {
            if (at >= voice.m_lineEvents.size()) return NULL;
            return this->EventAt(voice, voice.m_lineEvents[at]);
        };
        while (i < text.size()) {
            const char c = text[i];
            if (c == '%') break;
            if (c == '*') {
                ++at;
                ++i;
            }
            else if (c == '|') {
                if ((at > 0) && (at <= voice.m_lineEvents.size())) {
                    const int bar = voice.m_lineEvents[at - 1].m_bar;
                    while ((at < voice.m_lineEvents.size()) && (voice.m_lineEvents[at].m_bar <= bar)) ++at;
                }
                ++i;
            }
            else if ((c == '"') || (c == tune.m_decoration)) {
                const size_t end = text.find(c, i + 1);
                if (end == std::string::npos) break;
                Event *event = target();
                if (event) {
                    if (c == '"') {
                        TextMark mark;
                        std::string inner = text.substr(i + 1, end - i - 1);
                        if (!inner.empty() && (std::string("^_<>@").find(inner[0]) != std::string::npos)) {
                            mark.m_place = inner[0];
                            inner.erase(0, 1);
                        }
                        mark.m_text = Decoded(inner);
                        event->m_texts.push_back(mark);
                    }
                    else {
                        event->m_marks.push_back(text.substr(i + 1, end - i - 1));
                    }
                }
                ++at;
                i = end + 1;
            }
            else {
                ++i;
            }
        }
    }

} // namespace abc

//----------------------------------------------------------------------------
// Writing a tune into the document
//----------------------------------------------------------------------------

namespace {

    /**
     * A length as notes can say it: its value and its dots, the longest first (5 eighths are a half and an eighth),
     * down to a 1024th, the shortest value the engine has a written form for. What is left beyond that is said in
     * the log: [isExact] is false then. (Down to a 256th only, the two shortest spaces of a gap written to the 1024th
     * were each taken for a 256th, and the next bar came in late.)
     */
    std::vector<std::pair<data_DURATION, int>> Pieces(abc::Len length, bool *isExact = NULL)
    {
        static const std::vector<std::pair<data_DURATION, abc::Len>> values = { { DURATION_long, abc::Len(4, 1) },
            { DURATION_breve, abc::Len(2, 1) }, { DURATION_1, abc::Len(1, 1) }, { DURATION_2, abc::Len(1, 2) },
            { DURATION_4, abc::Len(1, 4) }, { DURATION_8, abc::Len(1, 8) }, { DURATION_16, abc::Len(1, 16) },
            { DURATION_32, abc::Len(1, 32) }, { DURATION_64, abc::Len(1, 64) }, { DURATION_128, abc::Len(1, 128) },
            { DURATION_256, abc::Len(1, 256) }, { DURATION_512, abc::Len(1, 512) },
            { DURATION_1024, abc::Len(1, 1024) } };
        std::vector<std::pair<data_DURATION, int>> pieces;
        while (!length.IsZero() && (pieces.size() < 16)) {
            bool isFound = false;
            for (const auto &[duration, value] : values) {
                for (int dots = 3; (dots >= 0) && !isFound; --dots) {
                    const abc::Len dotted = value * abc::Len((2LL << dots) - 1, 1LL << dots);
                    if (length < dotted) continue;
                    pieces.push_back({ duration, dots });
                    length = length - dotted;
                    isFound = true;
                }
                if (isFound) break;
            }
            // (what is left is shorter than any note)
            if (!isFound) break;
        }
        if (isExact) *isExact = length.IsZero();
        if (pieces.empty()) pieces.push_back({ DURATION_1024, 0 });
        return pieces;
    }

    data_ACCIDENTAL_WRITTEN WrittenAccid(int alter)
    {
        switch (alter) {
            case -2: return ACCIDENTAL_WRITTEN_ff;
            case -1: return ACCIDENTAL_WRITTEN_f;
            case 1: return ACCIDENTAL_WRITTEN_s;
            case 2: return ACCIDENTAL_WRITTEN_x;
            default: return ACCIDENTAL_WRITTEN_n;
        }
    }

    data_ACCIDENTAL_GESTURAL GesturalAccid(int alter)
    {
        switch (alter) {
            case -2: return ACCIDENTAL_GESTURAL_ff;
            case -1: return ACCIDENTAL_GESTURAL_f;
            case 1: return ACCIDENTAL_GESTURAL_s;
            case 2: return ACCIDENTAL_GESTURAL_ss;
            default: return ACCIDENTAL_GESTURAL_n;
        }
    }

    /** One element of a layer, with what the beams and tuplets go by */
    struct Atom {
        LayerElement *m_element = NULL;
        int m_event = -1; // the event it is a piece of
        bool m_isBeamable = false;
        bool m_breaks = false; // white space stands before it
        int m_grace = 0; // its group of grace notes
        bool m_isTimed = false; // the first piece of an event a tuplet counts
        int m_tupletNum = 0;
        int m_tupletBase = 0;
        int m_tupletCount = 0;
    };

    /** An event where it stands in the score: for what hangs on it */
    struct Placed {
        const abc::Event *m_event = NULL;
        Measure *m_measure = NULL;
        LayerElement *m_first = NULL; // its first piece: a note, a chord, a rest
        LayerElement *m_last = NULL;
        std::vector<Note *> m_firstNotes; // by head
        std::vector<Note *> m_lastNotes;
    };

    struct StaffPlan {
        std::vector<int> m_voices;
        int m_group = -1;
    };

    struct GroupPlan {
        char m_symbol = 0; // '{' or '['
        bool m_barThru = false;
    };

    class Writer {
    public:
        Writer(Doc *doc, ABCInput *input) : m_doc(doc), m_input(input) {}
        void Header();
        void Write(abc::Tune &tune);

    private:
        void Normalize(abc::Tune &tune);
        void Plan(const abc::Tune &tune);
        void WorkEntry(const abc::Tune &tune, Mdiv *mdiv);
        void PageHead(const abc::Tune &tune, Score *score);
        void ScoreDefinition(const abc::Tune &tune, Score *score);
        void ImplyRepeatStart(Measure *measure, bool isInEnding);
        void FillLayer(abc::Tune &tune, int voiceIndex, const std::vector<abc::Event> &events, Layer *layer,
            Measure *measure, bool isFirstOfStaff, std::vector<Placed> &placed);
        void Nest(Layer *layer, std::vector<Atom> &atoms);
        void Hang(abc::Tune &tune, std::vector<Placed> &placed);
        void Decorate(abc::Tune &tune, const Placed &at, const std::string &name);
        KeySig *KeySigOf(const abc::Key &key);
        MeterSig *MeterSigOf(const abc::Meter &meter);
        vrv::Clef *ClefOf(const abc::Clef &clef);
        Tempo *TempoOf(const abc::Tempo &tempo);

        Doc *m_doc;
        ABCInput *m_input;
        pugi::xml_node m_workList;
        std::vector<StaffPlan> m_staves;
        std::vector<GroupPlan> m_groups;
        std::vector<std::pair<int, int>> m_voicePlace; // each voice's staff and layer, from 1
        std::vector<abc::Clef> m_clefs; // each voice's clef where it is being written
        abc::Meter m_meter; // the meter where the tune is being written
        // what waits for its end
        std::map<std::string, ControlElement *> m_open; // a trill's line, a hairpin: by what closes it
        // where a repeat end with no start goes back to (ABC 2.1, 4.8)
        Measure *m_repeatsFrom = NULL;
        bool m_repeatsFromNext = false;
        bool m_hasRepeatStart = false;
        bool m_repeatsFromInEnding = false;
    };

    void Writer::Header()
    {
        pugi::xml_node meiHead = m_doc->m_header.append_child("meiHead");
        pugi::xml_node fileDesc = meiHead.append_child("fileDesc");
        pugi::xml_node fileTitleStmt = fileDesc.append_child("titleStmt");
        pugi::xml_node fileTitle = fileTitleStmt.append_child("title");
        fileTitle.text().set("");
        pugi::xml_node pubStmt = fileDesc.append_child("pubStmt");
        pubStmt.append_child(pugi::node_pcdata);

        pugi::xml_node encodingDesc = meiHead.append_child("encodingDesc");
        pugi::xml_node appInfo = encodingDesc.append_child("appInfo");
        pugi::xml_node app = appInfo.append_child("application");
        pugi::xml_node appName = app.append_child("name");
        appName.text().set("Verovio");
        pugi::xml_node appText = app.append_child("p");
        appText.text().set("Transcoded from abc music");

        const time_t t = time(0);
        struct tm *now = localtime(&t);
        std::string dateStr = StringFormat("%d-%02d-%02dT%02d:%02d:%02d", now->tm_year + 1900, now->tm_mon + 1,
            now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
        app.append_attribute("isodate").set_value(dateStr.c_str());
        app.append_attribute("version").set_value(GetVersion().c_str());

        m_workList = meiHead.append_child("workList");
    }

    void Writer::WorkEntry(const abc::Tune &tune, Mdiv *mdiv)
    {
        pugi::xml_node work = m_workList.append_child("work");
        work.append_attribute("n").set_value(mdiv->GetN().c_str());
        work.append_attribute("data").set_value(StringFormat("#%s", mdiv->GetID().c_str()).c_str());
        bool isFirst = true;
        for (const auto &it : tune.m_title) {
            pugi::xml_node title = work.append_child("title");
            title.text().set((it.first).c_str());
            if (it.second != 0) title.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
            title.append_attribute("analog").set_value("abc:T");
            title.append_attribute("type").set_value(isFirst ? "main" : "alternative");
            isFirst = false;
        }
        for (const auto &it : tune.m_composer) {
            pugi::xml_node composer = work.append_child("composer");
            composer.text().set((it.first).c_str());
            composer.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
            composer.append_attribute("analog").set_value("abc:C");
        }
        if (!tune.m_history.empty()) {
            pugi::xml_node history = work.append_child("history");
            history.append_attribute("analog").set_value("abc:H");
            for (const auto &it : tune.m_history) {
                pugi::xml_node histLine = history.append_child("p");
                histLine.text().set((it.first).c_str());
                histLine.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.second).c_str());
            }
        }
        if (!tune.m_info.empty()) {
            pugi::xml_node notes = work.append_child("notesStmt");
            for (const auto &it : tune.m_info) {
                pugi::xml_node annot = notes.append_child("annot");
                annot.text().set((it.first).first.c_str());
                annot.append_attribute("xml:id").set_value(StringFormat("abcLine%02d", it.first.second).c_str());
                annot.append_attribute("analog").set_value(StringFormat("abc:%c", it.second).c_str());
            }
        }
    }

    void Writer::PageHead(const abc::Tune &tune, Score *score)
    {
        PgHead *pgHead = new PgHead();
        pgHead->SetFunc(PGFUNC_first);
        bool isFirst = true;
        for (const auto &it : tune.m_title) {
            Rend *titleRend = new Rend();
            titleRend->SetHalign(HORIZONTALALIGNMENT_center);
            titleRend->SetValign(VERTICALALIGNMENT_middle);
            if (!isFirst) {
                data_FONTSIZE fontsize;
                fontsize.SetTerm(FONTSIZETERM_small);
                titleRend->SetFontsize(fontsize);
            }
            isFirst = false;
            Text *text = new Text();
            text->SetText(UTF8to32(it.first));
            titleRend->AddChild(text);
            pgHead->AddChild(titleRend);
        }
        for (const auto &it : tune.m_composer) {
            Rend *compRend = new Rend();
            compRend->SetHalign(HORIZONTALALIGNMENT_right);
            compRend->SetValign(VERTICALALIGNMENT_bottom);
            Text *composer = new Text();
            composer->SetText(UTF8to32(it.first));
            compRend->AddChild(composer);
            if (!tune.m_origin.empty()) {
                Text *origin = new Text();
                origin->SetText(UTF8to32(" (" + tune.m_origin.front().first + ")"));
                compRend->AddChild(origin);
            }
            pgHead->AddChild(compRend);
        }
        if (tune.m_composer.empty() && !tune.m_origin.empty()) {
            Rend *originRend = new Rend();
            originRend->SetHalign(HORIZONTALALIGNMENT_right);
            originRend->SetValign(VERTICALALIGNMENT_bottom);
            Text *origin = new Text();
            origin->SetText(UTF8to32("(" + tune.m_origin.front().first + ")"));
            originRend->AddChild(origin);
            pgHead->AddChild(originRend);
        }
        assert(score->GetScoreDef());
        score->GetScoreDef()->AddChild(pgHead);
    }

    KeySig *Writer::KeySigOf(const abc::Key &key)
    {
        KeySig *keySig = new KeySig();
        if (key.m_tonic) {
            keySig->SetPname(keySig->AttPitch::StrToPitchname(std::string(1, key.m_tonic)));
            if (key.m_tonicAccid > 0) keySig->SetAccid(ACCIDENTAL_GESTURAL_basic_s);
            if (key.m_tonicAccid < 0) keySig->SetAccid(ACCIDENTAL_GESTURAL_basic_f);
        }
        keySig->SetMode(key.m_mode);
        if (key.m_standard) {
            if (key.m_fifths != 0) {
                const int count = std::min(std::abs(key.m_fifths), 7);
                keySig->SetSig(
                    keySig->AttKeySigLog::StrToKeysignature(StringFormat("%d%s", count, (key.m_fifths < 0) ? "f" : "s")));
            }
        }
        else {
            // a signature that is none of the fifteen: its signs, each said
            for (const char step : std::string("fcgdaeb")) {
                const auto shown = key.m_shown.find(step);
                if (shown == key.m_shown.end()) continue;
                KeyAccid *keyAccid = new KeyAccid();
                keyAccid->SetPname(keyAccid->AttPitch::StrToPitchname(std::string(1, step)));
                keyAccid->SetAccid(WrittenAccid(shown->second));
                keySig->AddChild(keyAccid);
            }
        }
        return keySig;
    }

    MeterSig *Writer::MeterSigOf(const abc::Meter &meter)
    {
        MeterSig *meterSig = new MeterSig();
        if (meter.m_counts.empty()) return meterSig;
        if (meter.m_sym == 'C') meterSig->SetSym(METERSIGN_common);
        if (meter.m_sym == '|') meterSig->SetSym(METERSIGN_cut);
        meterSig->SetCount({ meter.m_counts, (meter.m_counts.size() > 1) ? MeterCountSign::Plus : MeterCountSign::None });
        meterSig->SetUnit(meter.m_unit);
        return meterSig;
    }

    vrv::Clef *Writer::ClefOf(const abc::Clef &clef)
    {
        vrv::Clef *written = new vrv::Clef();
        switch (clef.m_shape) {
            case 'F':
                written->SetShape(CLEFSHAPE_F);
                written->SetLine(clef.m_line);
                break;
            case 'C':
                written->SetShape(CLEFSHAPE_C);
                written->SetLine(clef.m_line);
                break;
            case 'P':
                written->SetShape(CLEFSHAPE_perc);
                written->SetLine(3);
                break;
            case 'N': written->SetShape(CLEFSHAPE_NONE); break;
            default:
                written->SetShape(CLEFSHAPE_G);
                written->SetLine(clef.m_line ? clef.m_line : 2);
                break;
        }
        if (clef.m_dis) {
            written->SetDis((clef.m_dis == 15) ? OCTAVE_DIS_15 : OCTAVE_DIS_8);
            written->SetDisPlace(clef.m_disBelow ? STAFFREL_basic_below : STAFFREL_basic_above);
        }
        return written;
    }

    Tempo *Writer::TempoOf(const abc::Tempo &said)
    {
        Tempo *tempo = new Tempo();
        if (said.m_perBeat > 0) {
            tempo->SetMm(said.m_perBeat);
            const std::vector<std::pair<data_DURATION, int>> beat = Pieces(said.m_beat);
            if (beat.size() == 1) {
                tempo->SetMmUnit(beat.front().first);
                if (beat.front().second) tempo->SetMmDots(beat.front().second);
            }
            // what is played: quarter notes a minute
            tempo->SetMidiBpm(said.m_perBeat * 4.0 * static_cast<double>(said.m_beat.n) / static_cast<double>(said.m_beat.d));
        }
        if (!said.m_words.empty()) {
            Text *text = new Text();
            text->SetText(UTF8to32(said.m_words));
            tempo->AddChild(text);
        }
        return tempo;
    }

    /**
     * What is read is put in the form the score has: a voice's last bar that holds nothing is none; bars of rest
     * written as one ("Z4") are that many bars where voices must stand bar against bar, and unseen ones ("X4") always.
     */
    void Writer::Normalize(abc::Tune &tune)
    {
        const bool isAlone = (tune.m_voices.size() == 1);
        for (abc::Voice &voice : tune.m_voices) {
            while (!voice.m_bars.empty() && voice.m_bars.back().IsEmpty()) {
                // (a line break after the last bar line stays with the bar before)
                voice.m_bars.pop_back();
            }
            std::vector<abc::Bar> bars;
            for (abc::Bar &bar : voice.m_bars) {
                const bool isBarRest = (bar.m_events.size() == 1) && bar.m_overlays.empty() && (bar.m_events[0].m_bars > 1)
                    && ((bar.m_events[0].m_kind == abc::EventKind::BarSpace)
                        || ((bar.m_events[0].m_kind == abc::EventKind::BarRest) && !isAlone));
                if (!isBarRest) {
                    bars.push_back(bar);
                    continue;
                }
                const int count = std::min(bar.m_events[0].m_bars, 1000);
                for (int i = 0; i < count; ++i) {
                    abc::Bar one;
                    one.m_events.push_back(bar.m_events[0]);
                    one.m_events[0].m_bars = 1;
                    if (i == 0) {
                        one.m_left = bar.m_left;
                        one.m_ending = bar.m_ending;
                        one.m_part = bar.m_part;
                    }
                    else {
                        one.m_events[0].m_changes.clear();
                        one.m_events[0].m_marks.clear();
                        one.m_events[0].m_texts.clear();
                    }
                    one.m_right = (i == count - 1) ? bar.m_right : BARRENDITION_single;
                    if (i == count - 1) {
                        one.m_times = bar.m_times;
                        one.m_lineEnd = bar.m_lineEnd;
                        one.m_line = bar.m_line;
                    }
                    bars.push_back(one);
                }
            }
            voice.m_bars = bars;
        }
    }

    /**
     * The staves: as %%score or %%staves says (voices in round brackets share a staff; curly and square brackets
     * group staves with a brace or a bracket; "|" carries the bar lines through), each voice on its own otherwise,
     * and so is a voice the directive does not name.
     */
    void Writer::Plan(const abc::Tune &tune)
    {
        m_staves.clear();
        m_groups.clear();
        m_voicePlace.assign(tune.m_voices.size(), { 0, 0 });
        std::vector<bool> isPlaced(tune.m_voices.size(), false);
        const auto voiceOf = [&](const std::string &id) {
            for (size_t i = 0; i < tune.m_voices.size(); ++i) {
                if (tune.m_voices[i].m_id == id) return static_cast<int>(i);
            }
            return -1;
        };
        int group = -1;
        bool inStaff = false;
        std::string id;
        bool opensStaff = false;
        for (const char c : tune.m_layout + " ") {
            if (abc::IsSpace(c) || (std::string("(){}[]|").find(c) != std::string::npos)) {
                if (!id.empty()) {
                    const int voice = voiceOf(id);
                    id.clear();
                    if ((voice >= 0) && !isPlaced[voice]) {
                        isPlaced[voice] = true;
                        if (!inStaff || opensStaff) {
                            m_staves.push_back({ {}, group });
                            opensStaff = false;
                        }
                        m_staves.back().m_voices.push_back(voice);
                    }
                }
                if (c == '(') {
                    inStaff = true;
                    opensStaff = true;
                }
                else if (c == ')') {
                    inStaff = false;
                }
                else if ((c == '{') || (c == '[')) {
                    m_groups.push_back({ c, false });
                    group = static_cast<int>(m_groups.size()) - 1;
                }
                else if ((c == '}') || (c == ']')) {
                    group = -1;
                }
                else if ((c == '|') && (group >= 0)) {
                    m_groups[group].m_barThru = true;
                }
            }
            else {
                id += c;
            }
        }
        for (size_t i = 0; i < tune.m_voices.size(); ++i) {
            if (!isPlaced[i]) m_staves.push_back({ { static_cast<int>(i) }, -1 });
        }
        for (size_t staff = 0; staff < m_staves.size(); ++staff) {
            for (size_t layer = 0; layer < m_staves[staff].m_voices.size(); ++layer) {
                m_voicePlace[m_staves[staff].m_voices[layer]] = { static_cast<int>(staff) + 1, static_cast<int>(layer) + 1 };
            }
        }
    }

    void Writer::ScoreDefinition(const abc::Tune &tune, Score *score)
    {
        StaffGrp *top = new StaffGrp();
        StaffGrp *grouped = NULL;
        int group = -1;
        for (size_t i = 0; i < m_staves.size(); ++i) {
            const StaffPlan &plan = m_staves[i];
            const abc::Voice &voice = tune.m_voices.at(plan.m_voices.front());
            StaffDef *staffDef = new StaffDef();
            staffDef->SetN(static_cast<int>(i) + 1);
            staffDef->SetLines(voice.m_stafflines ? voice.m_stafflines : 5);
            staffDef->SetTransSemi(voice.m_transpose);
            if (!voice.m_name.empty()) {
                Label *label = new Label();
                Text *text = new Text();
                text->SetText(UTF8to32(voice.m_name));
                label->AddChild(text);
                staffDef->AddChild(label);
            }
            if (!voice.m_shortName.empty()) {
                LabelAbbr *label = new LabelAbbr();
                Text *text = new Text();
                text->SetText(UTF8to32(voice.m_shortName));
                label->AddChild(text);
                staffDef->AddChild(label);
            }
            if ((voice.m_program >= 0) || (voice.m_channel >= 0) || (voice.m_volume >= 0)) {
                InstrDef *instrDef = new InstrDef();
                if (voice.m_program >= 0) instrDef->SetMidiInstrnum(voice.m_program);
                if (voice.m_channel >= 0) instrDef->SetMidiChannel(voice.m_channel - 1);
                if (voice.m_volume >= 0) instrDef->SetMidiVolume(100.0 * voice.m_volume / 127.0);
                staffDef->AddChild(instrDef);
            }
            staffDef->AddChild(this->ClefOf(voice.m_clef));
            if (!tune.m_firstMeter.m_counts.empty()) staffDef->AddChild(this->MeterSigOf(tune.m_firstMeter));
            if (plan.m_group != group) {
                group = plan.m_group;
                grouped = NULL;
                if (group >= 0) {
                    grouped = new StaffGrp();
                    GrpSym *grpSym = new GrpSym();
                    grpSym->SetSymbol(
                        (m_groups[group].m_symbol == '{') ? staffGroupingSym_SYMBOL_brace : staffGroupingSym_SYMBOL_bracket);
                    grouped->AddChild(grpSym);
                    if (m_groups[group].m_barThru) grouped->SetBarThru(BOOLEAN_true);
                    top->AddChild(grouped);
                }
            }
            (grouped ? grouped : top)->AddChild(staffDef);
        }
        this->PageHead(tune, score);
        assert(score->GetScoreDef());
        score->GetScoreDef()->AddChild(top);
        KeySig *keySig = this->KeySigOf(tune.m_key);
        if (tune.m_key.m_standard) keySig->IsAttribute(true);
        score->GetScoreDef()->AddChild(keySig);
        const std::vector<std::pair<data_DURATION, int>> unit = Pieces(tune.m_unit);
        if ((unit.size() == 1) && !unit.front().second) score->GetScoreDef()->SetDurDefault(unit.front().first);
    }

    // "A repeat end with no start goes back to the beginning of the tune, or to the latest double bar line or end of
    // repeated section" (ABC 2.1, 4.8, in other words): tunes are written "A :| B :|" and mean A A B B. The start
    // that is meant is written on the measure it stands before, so that the tune plays so whoever reads it next.
    // No start is implied inside an ending: the end that closes a later ending ("|1 ... :|2 ... :|") is the endings'
    // own sign, and they bring their repeat round by their numbers.
    void Writer::ImplyRepeatStart(Measure *measure, bool isInEnding)
    {
        const auto isEnd = [](data_BARRENDITION sign) {
            return (sign == BARRENDITION_rptend) || (sign == BARRENDITION_rptboth);
        };
        const auto isStart = [](data_BARRENDITION sign) {
            return (sign == BARRENDITION_rptstart) || (sign == BARRENDITION_rptboth);
        };
        const auto isDouble = [](data_BARRENDITION sign) {
            return (sign == BARRENDITION_dbl) || (sign == BARRENDITION_end);
        };
        const auto endRepeat = [&]() {
            if (!m_hasRepeatStart && m_repeatsFrom && !m_repeatsFromInEnding) {
                m_repeatsFrom->SetLeft(isEnd(m_repeatsFrom->GetLeft()) ? BARRENDITION_rptboth : BARRENDITION_rptstart);
            }
            m_hasRepeatStart = false;
        };
        const data_BARRENDITION left = measure->GetLeft();
        const data_BARRENDITION right = measure->GetRight();
        if (isEnd(left)) endRepeat();
        if (m_repeatsFromNext || isEnd(left) || isDouble(left)) {
            m_repeatsFrom = measure;
            m_repeatsFromInEnding = isInEnding;
            m_repeatsFromNext = false;
        }
        if (isStart(left)) m_hasRepeatStart = true;
        if (isEnd(right)) endRepeat();
        if (isStart(right)) m_hasRepeatStart = true;
        if (isEnd(right) || isDouble(right)) m_repeatsFromNext = true;
    }

    /**
     * Writes the events of one voice in one bar into [layer]: each event as the notes its length takes (one, or
     * several tied), with its signs, its words and what changes before it; then the beams and the tuplets.
     */
    void Writer::FillLayer(abc::Tune &tune, int voiceIndex, const std::vector<abc::Event> &events, Layer *layer,
        Measure *measure, bool isFirstOfStaff, std::vector<Placed> &placed)
    {
        std::vector<Atom> atoms;
        abc::Clef &clef = m_clefs.at(voiceIndex);
        for (size_t index = 0; index < events.size(); ++index) {
            const abc::Event &event = events[index];
            Placed at;
            at.m_event = &event;
            at.m_measure = measure;
            std::vector<abc::Tempo> tempos;
            for (const abc::Change &change : event.m_changes) {
                if (change.m_kind == abc::Change::CLEF) {
                    clef = change.m_clef;
                    Atom atom;
                    atom.m_element = this->ClefOf(change.m_clef);
                    atom.m_event = static_cast<int>(index);
                    atoms.push_back(atom);
                }
                else if ((change.m_kind == abc::Change::KEY) && ((index > 0) || (m_staves.size() > 1))) {
                    // (at a bar's start, in a tune of one staff, it is written before the measure)
                    if (!isFirstOfStaff) continue;
                    Atom atom;
                    atom.m_element = this->KeySigOf(change.m_key);
                    atom.m_event = static_cast<int>(index);
                    atoms.push_back(atom);
                }
                else if (change.m_kind == abc::Change::TEMPO) {
                    tempos.push_back(change.m_tempo);
                }
            }
            const int octaveShift = clef.m_dis ? ((clef.m_dis == 15) ? 2 : 1) * (clef.m_disBelow ? -1 : 1) : 0;

            if ((event.m_kind == abc::EventKind::BarRest) || (event.m_kind == abc::EventKind::BarSpace)) {
                Atom atom;
                if (event.m_kind == abc::EventKind::BarSpace) {
                    // An unseen bar takes the meter's time: said by spaces of that length, where a meter is known
                    // (an empty bar that says nothing takes none)
                    const abc::Len length = m_meter.Length();
                    if (!length.IsZero()) {
                        for (const auto &[duration, dots] : Pieces(length)) {
                            Space *space = new Space();
                            space->SetDur(duration);
                            if (dots) space->SetDots(dots);
                            Atom piece;
                            piece.m_element = space;
                            piece.m_event = static_cast<int>(index);
                            atoms.push_back(piece);
                            if (!at.m_first) at.m_first = space;
                            at.m_last = space;
                        }
                        placed.push_back(at);
                        continue;
                    }
                    atom.m_element = new MSpace();
                }
                else if ((event.m_bars > 1) || event.m_barsSaid) {
                    MultiRest *multiRest = new MultiRest();
                    multiRest->SetNum(event.m_bars);
                    atom.m_element = multiRest;
                }
                else {
                    atom.m_element = new MRest();
                }
                atom.m_event = static_cast<int>(index);
                atoms.push_back(atom);
                at.m_first = at.m_last = atom.m_element;
                placed.push_back(at);
                continue;
            }

            // The pieces: a grace note is an eighth, a length of 0 a quarter with no stem
            std::vector<std::pair<data_DURATION, int>> pieces;
            if (event.m_grace) {
                pieces.push_back({ DURATION_8, 0 });
            }
            else if (event.m_stemless) {
                pieces.push_back({ DURATION_4, 0 });
            }
            else {
                bool isExact = true;
                pieces = Pieces(event.m_length, &isExact);
                if (!isExact && tune.m_notRead.insert("a length shorter than a 1024th").second) {
                    LogWarning("ABC import: not read: a length shorter than a 1024th (the note is given the nearest)");
                }
            }
            for (size_t piece = 0; piece < pieces.size(); ++piece) {
                const auto [duration, dots] = pieces[piece];
                Atom atom;
                atom.m_event = static_cast<int>(index);
                atom.m_grace = event.m_grace ? event.m_graceGroup : 0;
                atom.m_breaks = (piece == 0) && event.m_breaksBeam;
                atom.m_isTimed = (piece == 0) && !event.m_grace;
                if (piece == 0) {
                    atom.m_tupletNum = event.m_tupletNum;
                    atom.m_tupletBase = event.m_tupletBase;
                    atom.m_tupletCount = event.m_tupletCount;
                }
                std::vector<Note *> notes;
                if (event.m_kind == abc::EventKind::Rest) {
                    Rest *rest = new Rest();
                    rest->SetDur(duration);
                    if (dots) rest->SetDots(dots);
                    atom.m_element = rest;
                }
                else if (event.m_kind == abc::EventKind::Space) {
                    Space *space = new Space();
                    space->SetDur(duration);
                    if (dots) space->SetDots(dots);
                    atom.m_element = space;
                }
                else {
                    Chord *chord = (event.m_kind == abc::EventKind::Chord) ? new Chord() : NULL;
                    for (const abc::Head &head : event.m_heads) {
                        Note *note = new Note();
                        note->SetPname(note->AttPitch::StrToPitchname(std::string(1, head.m_step)));
                        note->SetOct(head.m_oct);
                        if (octaveShift) note->SetOctGes(std::clamp(head.m_oct + octaveShift, 0, 9));
                        if ((head.m_accid != abc::kNoAccid) && (piece == 0)) {
                            Accid *accid = new Accid();
                            accid->SetAccid(WrittenAccid(head.m_accid));
                            note->AddChild(accid);
                        }
                        else {
                            // what sounds, where no sign says it at the note: the key's, a sign earlier in the bar,
                            // or the note it is tied from
                            const auto inKey = tune.m_voices[voiceIndex].m_key.m_alter.find(head.m_step);
                            const bool keyAlters = (inKey != tune.m_voices[voiceIndex].m_key.m_alter.end());
                            if (head.m_alter || keyAlters || (head.m_accid != abc::kNoAccid)) {
                                Accid *accid = new Accid();
                                accid->IsAttribute(true);
                                accid->SetAccidGes(GesturalAccid(head.m_alter));
                                note->AddChild(accid);
                            }
                        }
                        if (event.m_grace) {
                            note->SetGrace(event.m_graceSlash ? GRACE_unacc : GRACE_acc);
                            if (event.m_graceSlash) note->SetStemMod(STEMMODIFIER_1slash);
                        }
                        if (chord) {
                            chord->AddChild(note);
                        }
                        else {
                            note->SetDur(duration);
                            if (dots) note->SetDots(dots);
                            if (event.m_stemless) note->SetStemVisible(BOOLEAN_false);
                        }
                        notes.push_back(note);
                    }
                    if (chord) {
                        chord->SetDur(duration);
                        if (dots) chord->SetDots(dots);
                        if (event.m_stemless) chord->SetStemVisible(BOOLEAN_false);
                        if (event.m_grace) chord->SetGrace(event.m_graceSlash ? GRACE_unacc : GRACE_acc);
                        atom.m_element = chord;
                    }
                    else {
                        atom.m_element = notes.front();
                    }
                    atom.m_isBeamable = !event.m_grace && (duration >= DURATION_8) && (duration <= DURATION_256);
                    // the pieces of one length are tied, note by note
                    if (piece > 0) {
                        for (size_t head = 0; head < notes.size(); ++head) {
                            Tie *tie = new Tie();
                            tie->SetStartid("#" + at.m_lastNotes.at(head)->GetID());
                            tie->SetEndid("#" + notes[head]->GetID());
                            measure->AddChild(tie);
                        }
                    }
                    if (piece == 0) at.m_firstNotes = notes;
                    at.m_lastNotes = notes;
                }
                if (piece == 0) at.m_first = atom.m_element;
                at.m_last = atom.m_element;
                atoms.push_back(atom);
            }
            // its words
            for (const auto &[number, syllable] : event.m_syllables) {
                if (!at.m_first || !at.m_first->Is({ NOTE, CHORD })) break;
                Verse *verse = new Verse();
                verse->SetN(number);
                Syl *syl = new Syl();
                Text *text = new Text();
                text->SetText(UTF8to32(syllable.m_text));
                syl->AddChild(text);
                if (syllable.m_dash) {
                    syl->SetCon(sylLog_CON_d);
                    syl->SetWordpos(sylLog_WORDPOS_m);
                }
                else if (syllable.m_held) {
                    syl->SetCon(sylLog_CON_u);
                }
                verse->AddChild(syl);
                at.m_first->AddChild(verse);
            }
            for (const abc::Tempo &said : tempos) {
                Tempo *tempo = this->TempoOf(said);
                tempo->SetStartid("#" + at.m_first->GetID());
                measure->AddChild(tempo);
            }
            placed.push_back(at);
        }
        this->Nest(layer, atoms);
    }

    /**
     * Puts the elements of a layer into it with their beams and tuplets. A tuplet holds the next notes ABC gives it
     * (p, or r of "(p:q:r") in its bar and before the next tuplet starts. A beam joins notes shorter than a quarter
     * that no white space, rest or grace note divides; grace notes of one group are beamed among themselves. Where a
     * beam and a tuplet overlap in part, the beam is divided at the tuplet's edge.
     */
    void Writer::Nest(Layer *layer, std::vector<Atom> &atoms)
    {
        struct Range {
            int m_start = 0;
            int m_end = 0;
            bool m_isTuplet = false;
            int m_num = 0;
            int m_base = 0;
        };
        std::vector<Range> tuplets;
        std::vector<Range> beams;
        const int count = static_cast<int>(atoms.size());

        // the tuplets
        int open = -1;
        int remaining = 0;
        int closingEvent = -1;
        for (int i = 0; i < count; ++i) {
            if ((open >= 0) && (closingEvent >= 0) && (atoms[i].m_event != closingEvent)) {
                tuplets[open].m_end = i - 1;
                open = -1;
                closingEvent = -1;
            }
            if (atoms[i].m_tupletNum) {
                if (open >= 0) tuplets[open].m_end = i - 1;
                tuplets.push_back({ i, count - 1, true, atoms[i].m_tupletNum, atoms[i].m_tupletBase });
                open = static_cast<int>(tuplets.size()) - 1;
                remaining = atoms[i].m_tupletCount;
                closingEvent = -1;
            }
            if ((open >= 0) && atoms[i].m_isTimed) {
                if (--remaining <= 0) closingEvent = atoms[i].m_event;
            }
        }

        // the beams of the notes, and of each group of grace notes
        const auto addBeam = [&](int start, int end) {
            int notes = 0;
            for (int i = start; i <= end; ++i) notes += (atoms[i].m_isBeamable || atoms[i].m_grace) ? 1 : 0;
            if (notes >= 2) beams.push_back({ start, end, false, 0, 0 });
        };
        int runStart = -1;
        int lastNote = -1;
        const auto closeRun = [&]() {
            if (runStart >= 0) addBeam(runStart, lastNote);
            runStart = -1;
        };
        for (int i = 0; i < count; ++i) {
            const Atom &atom = atoms[i];
            if (atom.m_grace) {
                closeRun();
            }
            else if (atom.m_isBeamable) {
                if (atom.m_breaks) closeRun();
                if (runStart < 0) runStart = i;
                lastNote = i;
            }
            else if (atom.m_element->Is({ NOTE, CHORD, REST, SPACE, MREST, MSPACE, MULTIREST })) {
                closeRun();
            }
            // (a clef or a key between two notes stays in their beam)
        }
        closeRun();
        for (int i = 0; i < count;) {
            if (!atoms[i].m_grace) {
                ++i;
                continue;
            }
            int end = i;
            while ((end + 1 < count) && (atoms[end + 1].m_grace == atoms[i].m_grace)) ++end;
            addBeam(i, end);
            i = end + 1;
        }

        // a beam that a tuplet's edge crosses is divided there
        std::vector<Range> divided;
        for (const Range &beam : beams) {
            std::set<int> cuts{ beam.m_start, beam.m_end + 1 };
            for (const Range &tuplet : tuplets) {
                const bool isInside = (tuplet.m_start >= beam.m_start) && (tuplet.m_end <= beam.m_end);
                const bool holdsIt = (tuplet.m_start <= beam.m_start) && (tuplet.m_end >= beam.m_end);
                if (isInside || holdsIt) continue;
                if ((tuplet.m_start > beam.m_start) && (tuplet.m_start <= beam.m_end)) cuts.insert(tuplet.m_start);
                if ((tuplet.m_end >= beam.m_start) && (tuplet.m_end < beam.m_end)) cuts.insert(tuplet.m_end + 1);
            }
            for (auto cut = cuts.begin(); std::next(cut) != cuts.end(); ++cut) {
                const int start = *cut;
                const int end = *std::next(cut) - 1;
                int notes = 0;
                for (int i = start; i <= end; ++i) notes += (atoms[i].m_isBeamable || atoms[i].m_grace) ? 1 : 0;
                if (notes >= 2) divided.push_back({ start, end, false, 0, 0 });
            }
        }

        // from the outside in: where a tuplet and a beam hold the same notes, the tuplet holds the beam
        std::vector<Range> ranges = tuplets;
        ranges.insert(ranges.end(), divided.begin(), divided.end());
        std::stable_sort(ranges.begin(), ranges.end(), [](const Range &a, const Range &b) {
            if (a.m_start != b.m_start) return a.m_start < b.m_start;
            if (a.m_end != b.m_end) return a.m_end > b.m_end;
            return a.m_isTuplet && !b.m_isTuplet;
        });
        std::vector<std::pair<Object *, int>> stack; // each open one, and where it ends
        size_t next = 0;
        for (int i = 0; i < count; ++i) {
            while ((next < ranges.size()) && (ranges[next].m_start == i)) {
                const Range &range = ranges[next++];
                if (range.m_end < range.m_start) continue;
                Object *container = NULL;
                if (range.m_isTuplet) {
                    Tuplet *tuplet = new Tuplet();
                    tuplet->SetNum(range.m_num);
                    tuplet->SetNumbase(range.m_base);
                    container = tuplet;
                }
                else {
                    container = new Beam();
                }
                stack.push_back({ container, range.m_end });
            }
            (stack.empty() ? static_cast<Object *>(layer) : stack.back().first)->AddChild(atoms[i].m_element);
            while (!stack.empty() && (stack.back().second <= i)) {
                Object *done = stack.back().first;
                stack.pop_back();
                (stack.empty() ? static_cast<Object *>(layer) : stack.back().first)->AddChild(done);
            }
        }
    }

    /** A decoration at an event, by its name (ABC 2.1, 4.14) */
    void Writer::Decorate(abc::Tune &tune, const Placed &at, const std::string &name)
    {
        if (!at.m_first) return;
        const std::string ref = "#" + at.m_first->GetID();
        const bool isNote = at.m_first->Is({ NOTE, CHORD });
        const auto artic = [&](data_ARTICULATION articulation) {
            if (!isNote) return;
            Artic *element = vrv_cast<Artic *>(at.m_first->GetChild(0, ARTIC));
            if (!element) {
                element = new Artic();
                at.m_first->AddChild(element);
            }
            data_ARTICULATION_List list = element->GetArtic();
            list.push_back(articulation);
            element->SetArtic(list);
        };
        const auto control = [&](ControlElement *element) { at.m_measure->AddChild(element); };
        static const std::set<std::string> dynamics
            = { "pppp", "ppp", "pp", "p", "mp", "mf", "f", "ff", "fff", "ffff", "sfz" };

        if ((name == "staccato") || (name == ".")) {
            artic(ARTICULATION_stacc);
        }
        else if ((name == ">") || (name == "accent") || (name == "emphasis")) {
            artic(ARTICULATION_acc);
        }
        else if ((name == "^") || (name == "marcato")) {
            artic(ARTICULATION_marc);
        }
        else if (name == "tenuto") {
            artic(ARTICULATION_ten);
        }
        else if ((name == "+") || (name == "plus")) {
            artic(ARTICULATION_stop);
        }
        else if (name == "snap") {
            artic(ARTICULATION_snap);
        }
        else if (name == "slide") {
            artic(ARTICULATION_scoop);
        }
        else if (name == "wedge") {
            artic(ARTICULATION_stacciss);
        }
        else if (name == "upbow") {
            artic(ARTICULATION_upbow);
        }
        else if (name == "downbow") {
            artic(ARTICULATION_dnbow);
        }
        else if (name == "open") {
            artic(ARTICULATION_open);
        }
        else if ((name == "trill") || (name == "trill(")) {
            Trill *trill = new Trill();
            trill->SetStartid(ref);
            if (name == "trill(") {
                trill->SetExtender(BOOLEAN_true);
                m_open["trill)"] = trill;
            }
            control(trill);
        }
        else if (name == "trill)") {
            if (m_open.contains(name)) {
                vrv_cast<Trill *>(m_open[name])->SetEndid("#" + at.m_last->GetID());
                m_open.erase(name);
            }
        }
        else if ((name == "mordent") || (name == "lowermordent")) {
            Mordent *mordent = new Mordent();
            mordent->SetStartid(ref);
            mordent->SetForm(mordentLog_FORM_lower);
            control(mordent);
        }
        else if ((name == "pralltriller") || (name == "uppermordent")) {
            Mordent *mordent = new Mordent();
            mordent->SetStartid(ref);
            mordent->SetForm(mordentLog_FORM_upper);
            control(mordent);
        }
        else if ((name == "roll") || (name == "turn")) {
            Turn *turn = new Turn();
            turn->SetStartid(ref);
            turn->SetForm(turnLog_FORM_upper);
            control(turn);
        }
        else if (name == "invertedturn") {
            Turn *turn = new Turn();
            turn->SetStartid(ref);
            turn->SetForm(turnLog_FORM_lower);
            control(turn);
        }
        else if (name == "arpeggio") {
            Arpeg *arpeg = new Arpeg();
            arpeg->GetPlistInterface()->AddRef(ref);
            control(arpeg);
        }
        else if ((name == "fermata") || (name == "invertedfermata")) {
            Fermata *fermata = new Fermata();
            fermata->SetStartid(ref);
            if (name == "invertedfermata") {
                fermata->SetForm(fermataVis_FORM_inv);
                fermata->SetPlace(STAFFREL_below);
            }
            else {
                fermata->SetPlace(STAFFREL_above);
            }
            control(fermata);
        }
        else if ((name.size() == 1) && (name[0] >= '0') && (name[0] <= '5')) {
            Fing *fing = new Fing();
            fing->SetStartid(ref);
            Text *text = new Text();
            text->SetText(UTF8to32(name));
            fing->AddChild(text);
            control(fing);
        }
        else if (name == "breath") {
            Breath *breath = new Breath();
            breath->SetStartid(ref);
            control(breath);
        }
        else if (dynamics.contains(name)) {
            Dynam *dynam = new Dynam();
            dynam->SetStartid(ref);
            Text *text = new Text();
            text->SetText(UTF8to32(name));
            dynam->AddChild(text);
            control(dynam);
        }
        else if ((name == "<(") || (name == "crescendo(") || (name == ">(") || (name == "diminuendo(")) {
            const bool isCres = (name == "<(") || (name == "crescendo(");
            Hairpin *hairpin = new Hairpin();
            hairpin->SetStartid(ref);
            hairpin->SetForm(isCres ? hairpinLog_FORM_cres : hairpinLog_FORM_dim);
            m_open[isCres ? "cres" : "dim"] = hairpin;
            control(hairpin);
        }
        else if ((name == "<)") || (name == "crescendo)") || (name == ">)") || (name == "diminuendo)")) {
            const std::string which = ((name == "<)") || (name == "crescendo)")) ? "cres" : "dim";
            if (m_open.contains(which)) {
                vrv_cast<Hairpin *>(m_open[which])->SetEndid("#" + at.m_last->GetID());
                m_open.erase(which);
            }
        }
        else if ((name == "segno") || (name == "coda") || (name == "D.S.") || (name == "D.C.") || (name == "dacapo")
            || (name == "fine")) {
            RepeatMark *mark = new RepeatMark();
            mark->SetStartid(ref);
            mark->SetFunc((name == "segno")
                    ? repeatMarkLog_FUNC_segno
                    : (name == "coda") ? repeatMarkLog_FUNC_coda
                                       : (name == "D.S.") ? repeatMarkLog_FUNC_dalSegno
                                                          : (name == "fine") ? repeatMarkLog_FUNC_fine
                                                                             : repeatMarkLog_FUNC_daCapo);
            control(mark);
        }
        else if (name == "dacoda") {
            // "To Coda": a direction typed as the readers type it, which the order of play goes by
            Dir *dir = new Dir();
            dir->SetStartid(ref);
            dir->SetType("tocoda");
            Text *text = new Text();
            text->SetText(UTF8to32("To Coda"));
            dir->AddChild(text);
            control(dir);
        }
        else {
            if (tune.m_notRead.insert("the decoration \"" + name + "\"").second) {
                LogWarning("ABC import: not read: the decoration \"%s\"", name.c_str());
            }
        }
    }

    /** What hangs on the events of one voice, in the order they are played: ties, slurs, decorations and texts */
    void Writer::Hang(abc::Tune &tune, std::vector<Placed> &placed)
    {
        std::vector<Slur *> slurs;
        for (size_t i = 0; i < placed.size(); ++i) {
            const Placed &at = placed[i];
            const abc::Event &event = *at.m_event;
            if (!at.m_first) continue;
            // ties: to the next note of that pitch, when the next event that is no grace note holds one. The same pitch
            // is the same letter, octave and sound (4.11: "two notes of the same pitch"): D flat tied to D natural is no
            // tie, and both are played. (By letter and octave alone, the second note was swallowed by the first)
            for (size_t head = 0; head < event.m_heads.size(); ++head) {
                if (!event.m_heads[head].m_tie || event.m_grace || (head >= at.m_lastNotes.size())) continue;
                for (size_t j = i + 1; j < placed.size(); ++j) {
                    const abc::Event &after = *placed[j].m_event;
                    if (after.m_grace) continue;
                    for (size_t other = 0; other < after.m_heads.size(); ++other) {
                        if ((after.m_heads[other].m_step != event.m_heads[head].m_step)
                            || (after.m_heads[other].m_oct != event.m_heads[head].m_oct)
                            || (after.m_heads[other].m_alter != event.m_heads[head].m_alter)
                            || (other >= placed[j].m_firstNotes.size())) {
                            continue;
                        }
                        Tie *tie = new Tie();
                        tie->SetStartid("#" + at.m_lastNotes[head]->GetID());
                        tie->SetEndid("#" + placed[j].m_firstNotes[other]->GetID());
                        if (event.m_heads[head].m_tieDotted) tie->SetLform(LINEFORM_dotted);
                        at.m_measure->AddChild(tie);
                        break;
                    }
                    break;
                }
            }
            // slurs: one that ends here is the last that started on another event
            for (int end = 0; end < event.m_slurEnds; ++end) {
                for (auto slur = slurs.rbegin(); slur != slurs.rend(); ++slur) {
                    if ((*slur)->GetStartid() == "#" + at.m_first->GetID()) continue;
                    (*slur)->SetEndid("#" + at.m_last->GetID());
                    slurs.erase(std::next(slur).base());
                    break;
                }
            }
            for (const bool isDotted : event.m_slurStarts) {
                Slur *slur = new Slur();
                slur->SetStartid("#" + at.m_first->GetID());
                if (isDotted) slur->SetLform(LINEFORM_dotted);
                at.m_measure->AddChild(slur);
                slurs.push_back(slur);
            }
            for (const std::string &name : event.m_marks) this->Decorate(tune, at, name);
            // the name of a part, above its start
            if (!event.m_partLabel.empty()) {
                Reh *reh = new Reh();
                reh->SetStartid("#" + at.m_first->GetID());
                reh->SetPlace(STAFFREL_above);
                Text *text = new Text();
                text->SetText(UTF8to32(event.m_partLabel));
                reh->AddChild(text);
                at.m_measure->AddChild(reh);
            }
            for (const abc::TextMark &said : event.m_texts) {
                Text *text = new Text();
                text->SetText(UTF8to32(said.m_text));
                if (!said.m_place) {
                    Harm *harm = new Harm();
                    harm->SetStartid("#" + at.m_first->GetID());
                    harm->AddChild(text);
                    at.m_measure->AddChild(harm);
                }
                else {
                    Dir *dir = new Dir();
                    dir->SetStartid("#" + at.m_first->GetID());
                    dir->SetPlace((said.m_place == '_') ? STAFFREL_below : STAFFREL_above);
                    dir->AddChild(text);
                    at.m_measure->AddChild(dir);
                }
            }
        }
        // a slur that never ends is none
        for (Slur *slur : slurs) {
            Object *parent = slur->GetParent();
            if (parent) parent->DeleteChild(slur);
        }
    }

    void Writer::Write(abc::Tune &tune)
    {
        this->Normalize(tune);
        if (tune.m_voices.empty()) {
            abc::Voice voice;
            voice.m_id = "1";
            voice.m_clef = tune.m_clef;
            voice.m_key = tune.m_key;
            voice.m_stafflines = tune.m_stafflines;
            voice.m_transpose = tune.m_transpose;
            tune.m_voices.push_back(voice);
        }
        this->Plan(tune);
        m_clefs.clear();
        for (const abc::Voice &voice : tune.m_voices) m_clefs.push_back(voice.m_clef);
        m_open.clear();
        m_meter = tune.m_firstMeter;
        m_repeatsFrom = NULL;
        m_repeatsFromNext = false;
        m_hasRepeatStart = false;
        m_repeatsFromInEnding = false;
        // (a key that changes in the tune is the voices' last one by now: each note was given its sound as it was
        // read, and what is asked here is only whether the tune's key alters a step)
        for (abc::Voice &voice : tune.m_voices) voice.m_key = tune.m_key;

        Mdiv *mdiv = new Mdiv();
        mdiv->SetVisibility(Visible);
        if (!tune.m_number.empty()) {
            const int number = atoi(tune.m_number.c_str());
            if (number < 1) LogError("ABC import: reference number should be a positive integer");
            mdiv->SetN(std::to_string(number));
        }
        m_doc->AddChild(mdiv);
        if (tune.m_title.empty()) {
            LogWarning("ABC import: Title field missing, creating empty title");
            tune.m_title.push_back({ "", 0 });
        }
        this->WorkEntry(tune, mdiv);

        Score *score = new Score();
        mdiv->AddChild(score);
        this->ScoreDefinition(tune, score);
        Section *section = new Section();
        score->AddChild(section);
        m_input->SetLayoutEncoded(tune.m_lineBreaks);
        if (tune.m_lineBreaks) {
            Pb *pb = new Pb();
            pb->SetID(StringFormat("abcLine%02d", tune.m_firstLine + 1));
            section->AddChild(pb);
        }

        size_t measures = 0;
        for (const abc::Voice &voice : tune.m_voices) measures = std::max(measures, voice.m_bars.size());
        const auto rank = [](data_BARRENDITION sign) {
            switch (sign) {
                case BARRENDITION_NONE: return 0;
                case BARRENDITION_single: return 1;
                case BARRENDITION_dotted:
                case BARRENDITION_invis: return 2;
                case BARRENDITION_dbl: return 3;
                case BARRENDITION_end: return 4;
                default: return 5;
            }
        };
        std::vector<std::vector<Placed>> placed(tune.m_voices.size());
        std::map<const Measure *, int> times;
        Object *holder = section; // the section, or the ending that is open
        bool hasTempo = false;
        std::vector<std::pair<std::string, Measure *>> partStarts; // each part of the tune (P:), where it starts
        std::vector<Measure *> beforePart; // and the measure before it
        Measure *lastMeasure = NULL;
        for (size_t m = 0; m < measures; ++m) {
            // The measure's bar lines, its ending and its line break: the first voice's that says them, a repeat
            // sign before any other
            data_BARRENDITION left = BARRENDITION_NONE;
            data_BARRENDITION right = BARRENDITION_NONE;
            int repeated = 2;
            std::string ending;
            bool lineEnd = false;
            int line = 0;
            std::string part;
            bool isFirstSaid = true;
            for (const abc::Voice &voice : tune.m_voices) {
                if (m >= voice.m_bars.size()) continue;
                const abc::Bar &bar = voice.m_bars[m];
                if (rank(bar.m_left) > rank(left)) left = bar.m_left;
                if (rank(bar.m_right) > rank(right)) right = bar.m_right;
                repeated = std::max(repeated, bar.m_times);
                if (ending.empty()) ending = bar.m_ending;
                if (part.empty()) part = bar.m_part;
                if (isFirstSaid) {
                    lineEnd = bar.m_lineEnd;
                    line = bar.m_line;
                    isFirstSaid = false;
                }
            }
            // what changes at the bar's start, for every staff: the meter, and in a tune of one staff the key
            ScoreDef *scoreDef = NULL;
            for (size_t v = 0; v < tune.m_voices.size(); ++v) {
                const abc::Voice &voice = tune.m_voices[v];
                if ((m >= voice.m_bars.size()) || voice.m_bars[m].m_events.empty()) continue;
                for (const abc::Change &change : voice.m_bars[m].m_events.front().m_changes) {
                    if (change.m_kind == abc::Change::METER) {
                        if (!scoreDef) scoreDef = new ScoreDef();
                        if (scoreDef->FindDescendantByType(METERSIG)) continue;
                        m_meter = change.m_meter;
                        MeterSig *meterSig = this->MeterSigOf(change.m_meter);
                        meterSig->IsAttribute(true);
                        scoreDef->AddChild(meterSig);
                    }
                    else if ((change.m_kind == abc::Change::KEY) && (m_staves.size() == 1)) {
                        if (!scoreDef) scoreDef = new ScoreDef();
                        if (scoreDef->FindDescendantByType(KEYSIG)) continue;
                        KeySig *keySig = this->KeySigOf(change.m_key);
                        if (change.m_key.m_standard) keySig->IsAttribute(true);
                        scoreDef->AddChild(keySig);
                    }
                }
            }
            // An ending ends at a bar line that is more than a plain one: on the right of its last bar, or on the
            // left of the bar after it (a repeat start or a double bar that begins the next line of music)
            if ((holder != section) && ending.empty() && (rank(left) >= 3)) holder = section;
            if (!ending.empty()) {
                Ending *element = new Ending();
                element->SetN(ending);
                section->AddChild(element);
                holder = element;
            }
            if (scoreDef) holder->AddChild(scoreDef);

            // (No number: the file gives none, and a stored score's marks know a bar of an ABC tune by its notes,
            // as the scores read before were written. A number made up here would lose every one of them)
            Measure *measure = new Measure();
            if (left != BARRENDITION_NONE) measure->SetLeft(left);
            if (right != BARRENDITION_NONE) measure->SetRight(right);
            this->ImplyRepeatStart(measure, holder != section);
            if (repeated > 2) times[measure] = repeated;
            // (a part starts outside any ending)
            if (!part.empty() && ending.empty()) holder = section;
            holder->AddChild(measure);
            if (partStarts.empty() && part.empty()) part = " "; // what stands before the first part
            if (!part.empty()) {
                partStarts.push_back({ part, measure });
                beforePart.push_back(lastMeasure);
            }
            lastMeasure = measure;

            for (size_t s = 0; s < m_staves.size(); ++s) {
                Staff *staff = new Staff();
                staff->SetN(static_cast<int>(s) + 1);
                measure->AddChild(staff);
                int layerN = 0;
                std::vector<const std::vector<abc::Event> *> overlays;
                for (size_t k = 0; k < m_staves[s].m_voices.size(); ++k) {
                    const int v = m_staves[s].m_voices[k];
                    Layer *layer = new Layer();
                    layer->SetN(++layerN);
                    staff->AddChild(layer);
                    if (m >= tune.m_voices[v].m_bars.size()) continue;
                    const abc::Bar &bar = tune.m_voices[v].m_bars[m];
                    this->FillLayer(tune, v, bar.m_events, layer, measure, k == 0, placed[v]);
                    for (const std::vector<abc::Event> &overlay : bar.m_overlays) overlays.push_back(&overlay);
                }
                // the voices written over the bar with "&": a layer each, after the staff's own
                for (const std::vector<abc::Event> *overlay : overlays) {
                    Layer *layer = new Layer();
                    layer->SetN(++layerN);
                    staff->AddChild(layer);
                    std::vector<Placed> over;
                    this->FillLayer(tune, m_staves[s].m_voices.front(), *overlay, layer, measure, false, over);
                    this->Hang(tune, over);
                }
            }
            if (!hasTempo && tune.m_tempo.m_said) {
                Tempo *tempo = this->TempoOf(tune.m_tempo);
                tempo->SetTstamp(1);
                measure->AddChild(tempo);
                hasTempo = true;
            }
            // An ending ends at a bar line that is more than a plain one, or where the next one starts
            if ((holder != section) && (rank(right) >= 3)) holder = section;
            if (lineEnd && tune.m_lineBreaks && (m + 1 < measures)) {
                Sb *sb = new Sb();
                sb->SetID(StringFormat("abcLine%02d", line + 1));
                holder->AddChild(sb);
            }
        }
        for (std::vector<Placed> &voice : placed) this->Hang(tune, voice);

        // The parts in the order the header gives them (P:AB2 is A B B; a group in round brackets is repeated whole),
        // where the tune names parts: what stands before the first part is played first, once
        std::vector<std::pair<const Measure *, const Measure *>> playlist;
        if (!tune.m_parts.empty() && (partStarts.size() > 1 || (!partStarts.empty() && (partStarts.front().first != " ")))) {
            std::map<char, std::pair<const Measure *, const Measure *>> stretches;
            for (size_t i = 0; i < partStarts.size(); ++i) {
                const Measure *last = lastMeasure;
                if (i + 1 < partStarts.size()) last = beforePart[i + 1];
                if (!last) continue;
                // (a part is named by its letter; what stands before the first part is " ")
                const char label = partStarts[i].first[0];
                if (!stretches.contains(label)) stretches[label] = { partStarts[i].second, last };
            }
            if (stretches.contains(' ')) playlist.push_back(stretches.at(' '));
            // the order as letters: a number repeats the letter or the group before it
            std::vector<std::string> groups{ "" };
            const std::string &said = tune.m_parts;
            for (size_t i = 0; (i < said.size()) && (groups.front().size() < 2000); ++i) {
                const char c = said[i];
                std::string unit;
                if (c == '(') {
                    groups.emplace_back();
                    continue;
                }
                else if ((c == ')') && (groups.size() > 1)) {
                    unit = groups.back();
                    groups.pop_back();
                }
                else if ((c >= 'A') && (c <= 'Z')) {
                    unit = std::string(1, c);
                }
                else {
                    continue;
                }
                size_t after = i + 1;
                int repeat = abc::IsDigit((after < said.size()) ? said[after] : ' ') ? abc::ReadNumber(said, after) : 1;
                if (repeat != 1) i = after - 1;
                repeat = std::clamp(repeat, 1, 100);
                for (int r = 0; (r < repeat) && (groups.back().size() < 2000); ++r) groups.back() += unit;
            }
            for (const char label : groups.front()) {
                if (stretches.contains(label)) playlist.push_back(stretches.at(label));
            }
        }

        // The order of play, by the engine's one rule: repeats (as often as their colons say), endings, jumps
        WriteOrderOfPlayFor(score, times, playlist);
    }

} // namespace

//----------------------------------------------------------------------------
// ABCInput
//----------------------------------------------------------------------------

ABCInput::ABCInput(Doc *doc) : Input(doc)
{
    m_layoutInformation = LAYOUT_ENCODED;
}

ABCInput::~ABCInput() {}

bool ABCInput::Import(const std::string &abc)
{
    m_doc->Reset();
    m_doc->SetType(Raw);

    abc::Reader reader;
    std::vector<abc::Tune> tunes = reader.Read(abc);
    Writer writer(m_doc, this);
    writer.Header();
    // A file with no tune in it (no X, or one with no key after it) is refused
    if (tunes.empty()) return false;
    for (abc::Tune &tune : tunes) writer.Write(tune);

    m_doc->ConvertToPageBasedDoc();
    return true;
}

#else

ABCInput::ABCInput(Doc *doc) : Input(doc) {}

ABCInput::~ABCInput() {}

bool ABCInput::Import(const std::string &abc)
{
    return false;
}

#endif /* NO_ABC_SUPPORT */

} // namespace vrv
