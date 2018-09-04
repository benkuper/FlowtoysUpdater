/*
  ==============================================================================

    Screen.cpp
    Created: 4 Sep 2018 12:38:33pm
    Author:  Ben

  ==============================================================================
*/

#include "Screen.h"

AppScreen::AppScreen(const String &screenName) :
	Component(screenName)
{

}

AppScreen::~AppScreen()
{
}

void AppScreen::paint(Graphics & g)
{
	//g.setColour(Colours::yellow);
	//g.drawText(getName(), getLocalBounds().toFloat(), Justification::centred);
}

void AppScreen::resized()
{
}
