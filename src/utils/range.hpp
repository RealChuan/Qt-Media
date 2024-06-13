#pragma once

namespace Utils {

template<typename T>
class Range
{
public:
    Range(T min, T max)
        : m_min(min)
        , m_max(max)
    {}

    auto operator==(const Range &other) const -> bool
    {
        return m_min == other.m_min && m_max == other.m_max && m_value == other.m_value;
    }
    auto operator!=(const Range &other) const -> bool { return !(*this == other); }

    auto min() const -> T { return m_min; }
    auto max() const -> T { return m_max; }

    auto clamp(T value) const -> T
    {
        if (value < m_min) {
            return m_min;
        }
        if (value > m_max) {
            return m_max;
        }
        return value;
    }

    auto value() const -> T { return m_value; }
    void setValue(T value) { m_value = clamp(value); }

private:
    T m_min;
    T m_max;
    T m_value;
};

} // namespace Utils
