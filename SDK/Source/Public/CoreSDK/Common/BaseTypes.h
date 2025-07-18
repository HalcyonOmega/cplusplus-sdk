#pragma once
#include <algorithm>
#include <optional>

class BoundedDouble
{

public:
	explicit BoundedDouble(const double InValue = 0.0,
		const double InMin = 0.0,
		const double InMax = 1.0,
		const bool InLocked = true)
		: m_Value(std::clamp(InValue, InMin, InMax)),
		  m_Min(InMin),
		  m_Max(InMax),
		  m_BoundsLocked(InLocked) {};

	/*
	 * Set the value of the bounded double, including the ability to set new min & max.
	 *
	 * @param InValue - Value to set the double
	 * @param InMin - Optional value to set for minimum
	 * @param InMax - Optional value to set for maximum
	 */
	void Set(const double InValue = 0.0,
		const std::optional<double> InMin = std::nullopt,
		const std::optional<double> InMax = std::nullopt)
	{
		if (InMin && !m_BoundsLocked)
		{
			SetMin(InMin.value());
		}

		if (InMax && !m_BoundsLocked)
		{
			SetMax(InMax.value());
		}

		SetValue(InValue);
	}

	/*
	 * Sets the value of the double, clamped between existing min & max
	 */
	double SetValue(const double InValue) { return m_Value = std::clamp(InValue, m_Min, m_Max); }

	/* Set minimum & update value to clamp between new min & max */
	double SetMin(const double InMin)
	{
		if (!m_BoundsLocked)
		{
			m_Min = InMin;
			return SetValue(m_Value);
		}

		return m_Value;
	}

	/* Set maximum & update value to clamp between new max & min */
	double SetMax(const double InMax)
	{
		if (!m_BoundsLocked)
		{
			m_Max = InMax;
			return SetValue(m_Value);
		}

		return m_Value;
	}

	[[nodiscard]] double GetMin() const { return m_Min; }
	[[nodiscard]] double GetMax() const { return m_Max; }
	[[nodiscard]] double GetValue() const { return m_Value; }

	// ReSharper disable once CppNonExplicitConversionOperator
	operator double() const { return m_Value; }

	BoundedDouble& operator=(const double InValue)
	{
		SetValue(InValue);
		return *this;
	}

	[[nodiscard]] static std::optional<BoundedDouble> CreateOptional(const std::optional<double> InValue = std::nullopt,
		const double InMin = 0.0,
		const double InMax = 1.0,
		const bool InLocked = true)
	{
		if (InValue.has_value())
		{
			return BoundedDouble{ InValue.value(), InMin, InMax, InLocked };
		}

		return std::nullopt;
	}

private:
	double m_Value{ 0.0 };
	double m_Min{ 0.0 };
	double m_Max{ 0.0 };
	bool m_BoundsLocked{ true };
};