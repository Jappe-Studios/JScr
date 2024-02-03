#pragma once

#include "Vector.h"

namespace JScr::Utils
{
    struct Range
    {
    public:
        Range(const Vector2i& begin, const Vector2i& end) : m_begin(begin), m_end(end) {}

        const Vector2i& Begin() const { return m_begin; }
        const Vector2i& End() const { return m_end; }
    private:
        const Vector2i& m_begin;
        const Vector2i& m_end;
    };
}