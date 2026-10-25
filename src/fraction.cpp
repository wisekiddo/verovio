/////////////////////////////////////////////////////////////////////////////
// Name:        fraction.cpp
// Author:      Laurent Pugin
// Created:     2024
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "fraction.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <math.h>
#include <numeric>

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// Fraction
//----------------------------------------------------------------------------

Fraction::Fraction(int num, int denom)
{
    if (denom == 0) {
        LogDebug("Denominator cannot be zero.");
        denom = 1;
    }
    *this = Fit(num, denom);
}

Fraction::Fraction(data_DURATION duration)
{
    duration = vrv::DurationMin(duration, DURATION_2048);
    duration = vrv::DurationMax(duration, DURATION_maxima);
    int den = pow(2, (duration + 1));
    *this = Fit(8, den);
}

// Each term of a result is a product of two ints, or the sum of two such products: below 2^63, so it is taken in 64
// bits, and Fit reduces it before it is stored. (They were taken in an int, and wrapped before they were reduced.)

Fraction Fraction::operator+(const Fraction &other) const
{
    return Fit(int64_t(m_numerator) * other.m_denominator + int64_t(other.m_numerator) * m_denominator,
        int64_t(m_denominator) * other.m_denominator);
}

Fraction Fraction::operator-(const Fraction &other) const
{
    return Fit(int64_t(m_numerator) * other.m_denominator - int64_t(other.m_numerator) * m_denominator,
        int64_t(m_denominator) * other.m_denominator);
}

Fraction Fraction::operator*(const Fraction &other) const
{
    return Fit(int64_t(m_numerator) * other.m_numerator, int64_t(m_denominator) * other.m_denominator);
}

Fraction Fraction::operator/(const Fraction &other) const
{
    if (other.m_numerator == 0) {
        LogDebug("Cannot divide by zero.");
        return *this;
    }
    return Fit(int64_t(m_numerator) * other.m_denominator, int64_t(m_denominator) * other.m_numerator);
}

Fraction Fraction::operator%(const Fraction &other) const
{
    if (other.m_numerator == 0) {
        LogDebug("Cannot divide by zero.");
        return *this;
    }

    // Convert both fractions to common denominator
    const int64_t commonDenominator = int64_t(m_denominator) * other.m_denominator;
    const int64_t leftNumerator = int64_t(m_numerator) * other.m_denominator;
    const int64_t rightNumerator = int64_t(other.m_numerator) * m_denominator;

    // Integer quotient
    const int64_t quotient = leftNumerator / rightNumerator;

    // Remainder as a fraction
    return Fit(leftNumerator - quotient * rightNumerator, commonDenominator);
}

bool Fraction::operator==(const Fraction &other) const
{
    return int64_t(m_numerator) * other.m_denominator == int64_t(other.m_numerator) * m_denominator;
}

std::strong_ordering Fraction::operator<=>(const Fraction &other) const
{
    return int64_t(m_numerator) * other.m_denominator <=> int64_t(other.m_numerator) * m_denominator;
}

double Fraction::ToDouble() const
{
    return static_cast<double>(m_numerator) / m_denominator;
}

std::string Fraction::ToString() const
{
    return StringFormat("%d/%d", m_numerator, m_denominator);
}

Fraction Fraction::Fit(int64_t num, int64_t denom)
{
    if (denom < 0) { // Keep the denominator positive
        num = -num;
        denom = -denom;
    }
    const int64_t gcdVal = std::gcd(num, denom);
    if (gcdVal > 1) {
        num /= gcdVal;
        denom /= gcdVal;
    }
    Fraction fraction;
    int64_t a = std::abs(num), b = denom;
    if ((a <= INT_MAX) && (b <= INT_MAX)) {
        fraction.m_numerator = int(num);
        fraction.m_denominator = int(b);
        return fraction;
    }
    // A result too large or too fine for an int is the fraction nearest to it toward zero whose terms fit. So nothing
    // wraps: the result keeps its sign, is no larger than the exact one, keeps the order of exact results (a sum of
    // durations never goes back), and every later step works on it exactly. For its magnitude a/b that is the last
    // convergent of a/b whose terms fit when it is below a/b, or else the semiconvergent below a/b next to it: the
    // two neighbours of a/b among fractions whose terms fit (Stern-Brocot). Past INT_MAX whole notes it is INT_MAX
    int64_t p0 = 0, q0 = 1, p1 = 1, q1 = 0; // the convergents before the first: 0/1 and 1/0
    bool below = false; // whether p1/q1 is below a/b
    while (b != 0) {
        const int64_t t = a / b;
        const int64_t k = std::min({ t, p1 ? (INT_MAX - p0) / p1 : t, q1 ? (INT_MAX - q0) / q1 : t });
        if (k < t) { // the next convergent does not fit
            if (!below) {
                p1 = p0 + k * p1;
                q1 = q0 + k * q1;
            }
            break;
        }
        const int64_t p2 = p0 + t * p1, q2 = q0 + t * q1;
        p0 = p1;
        q0 = q1;
        p1 = p2;
        q1 = q2;
        below = !below;
        const int64_t r = a - t * b;
        a = b;
        b = r;
    }
    fraction.m_numerator = int((num < 0) ? -p1 : p1);
    fraction.m_denominator = int(q1);
    return fraction;
}

std::pair<data_DURATION, Fraction> Fraction::ToDur() const
{
    if (m_numerator == 0) return { DURATION_NONE, 0 };

    int value = ceil(log2((double)m_denominator / (double)m_numerator * 8)) - 1;
    data_DURATION dur = static_cast<data_DURATION>(value);
    dur = vrv::DurationMax(DURATION_maxima, dur);
    dur = vrv::DurationMin(DURATION_2048, dur);

    Fraction remainder = *this - Fraction(dur);
    // Making sure we would not be triggering an infinite loop when looping over the remainder
    if ((remainder >= *this) || (remainder < 0)) remainder = 0;
    return { dur, remainder };
}

void Fraction::Reduce(int &numerator, int &denominator)
{
    Fraction fraction(numerator, denominator);
    numerator = fraction.GetNumerator();
    denominator = fraction.GetDenominator();
}

} // namespace vrv
