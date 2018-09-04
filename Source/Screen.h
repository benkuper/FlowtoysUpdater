/*
  ==============================================================================

    Screen.h
    Created: 4 Sep 2018 12:38:33pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class AppScreen : public Component
{
public:
	AppScreen(const String &screenName);
	~AppScreen();

	virtual void paint(Graphics &g) override;
	virtual void resized() override;

	virtual void reset() {}

	class ScreenListener
	{
	public:
		virtual ~ScreenListener() {}

		virtual void screenFinish(AppScreen *) {}
	};

	ListenerList<ScreenListener> screenListeners;
	void addScreenListener(ScreenListener* newListener) { screenListeners.add(newListener); }
	void removeScreenListener(ScreenListener* listener) { screenListeners.remove(listener); }

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppScreen)
};