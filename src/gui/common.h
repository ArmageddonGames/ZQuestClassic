#ifndef ZC_GUI_COMMON_H
#define ZC_GUI_COMMON_H

#include <string_view>

extern bool is_large;

namespace GUI
{

/* Finds the first character preceded by & and returns it. && is ignored.
 * Returns 0 if no character was found.
 */
int getAccelKey(const std::string_view text);

/* Returns a in small mode, b in large mode. */
template<typename T>
inline T sized(T a, T b)
{
	return is_large ? b : a;
}

/* A simple wrapper to force sizes into units. */
class Size
{
public:
	static Size em(size_t size);
	static Size em(double size);
	static Size largePixels(int size);
	static inline constexpr Size pixels(int size)
	{
		return Size(size);
	}

	inline constexpr operator int() const
	{
		return value;
	}

	inline constexpr Size operator+(const Size& other) const
	{
		return Size(value+other.value);
	}

	inline constexpr bool operator<(const Size& other) const
	{
		return value<other.value;
	}

	inline constexpr bool operator>(const Size& other) const
	{
		return value>other.value;
	}

	inline constexpr bool operator==(const Size& other) const
	{
		return value==other.value;
	}

	inline constexpr bool operator!=(const Size& other) const
	{
		return value!=other.value;
	}

private:
	int value;

	inline constexpr Size(int raw): value(raw) {}
};

inline Size operator ""_em(unsigned long long size)
{
	return Size::em((size_t)size);
}

inline Size operator ""_em(long double size)
{
	return Size::em((double)size);
}

inline constexpr Size operator ""_px(unsigned long long size)
{
	return Size::pixels(size);
}

inline Size operator ""_lpx(unsigned long long size)
{
	return Size::largePixels(size);
}

}

#endif
