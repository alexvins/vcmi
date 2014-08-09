/*
 * CBaseRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IRenderer.h"

class CBaseRenderer : public IRenderer
{
public:
	CBaseRenderer();
	virtual ~CBaseRenderer();
protected:
private:
};


template <typename _WindowImpl>
class CBaseRendererT : public CBaseRenderer
{
public:
	CBaseRendererT(){};
	virtual ~CBaseRendererT(){};
protected:
	void setCurrentWindow(_WindowImpl * newCurrentWindow)
	{
		currentWindow.reset(newCurrentWindow);
	};
	_WindowImpl * getCurrentWindow()
	{
		return currentWindow.get();
	};
private:
	boost::thread_specific_ptr<_WindowImpl> currentWindow;
};
