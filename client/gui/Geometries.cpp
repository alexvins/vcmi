#include "StdInc.h"
#include "Geometries.h"
#include "../CMT.h"

Rect Rect::createCentered( int w, int h )
{
	return Rect(mainScreen->getWidth()/2 - w/2, mainScreen->getHeight()/2 - h/2, w, h);
}

Rect Rect::around(const Rect &r, int width /*= 1*/) /*creates rect around another */
{
	return Rect(r.x - width, r.y - width, r.w + width * 2, r.h + width * 2);
}

Rect Rect::centerIn(const Rect &r)
{
	return Rect(r.x + (r.w - w) / 2, r.y + (r.h - h) / 2, w, h);
}
