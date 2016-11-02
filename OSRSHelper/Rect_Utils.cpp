#include "Rect_Utils.h"

bool rect_equals(RECT a, RECT b)
{
	return a.left == b.left &&
		a.right == b.right &&
		a.top == b.top &&
		a.bottom == b.bottom;
}
