#pragma once
#include <algorithm>
#include <optional>

class BoundedDouble
{
private:
	double m_Value{ 0.0 };
	double m_Min{ 0.0 };
	double m_Max{ 0.0 };

public:
	explicit BoundedDouble(const double InValue = 0.0, const double InMin, const double InMax) :
		m_Value(std::clamp(InValue, InMin, InMax)), m_Min(InMin), m_Max(InMax) {};

	/*
	 * Set the value of the bounded double, including the ability to set new min & max.
	 *
	 */
	void Set(const double InValue = 0.0, const std::optional<double> InMin, const std::optional<double> InMax)
	{
		if (InMin)
		{
			SetMin(InMin.value());
		}

		if (InMax)
		{
			SetMax(InMax.value());
		}

		SetValue(InValue);
	}

	/*
	 *
	 */
	double SetValue(const double InValue) { return m_Value = std::clamp(InValue, m_Min, m_Max); }
	double SetMin(const double InMin)
	{
		m_Min = InMin;
		return m_Value = std::clamp(m_Value, m_Min, m_Max);
	}
	double SetMax(const double InMax)
	{
		m_Max = InMax;
		return m_Value = std::clamp(m_Value, m_Min, m_Max);
	}
	double GetMin() const { return m_Min; }
	double GetMax() const { return m_Max; }
	double GetValue() const { return m_Value; }

	explicit operator double() const { return m_Value; }
};