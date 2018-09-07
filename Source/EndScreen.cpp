/*
  ==============================================================================

    EndScreen.cpp
    Created: 4 Sep 2018 12:44:55pm
    Author:  Ben

  ==============================================================================
*/

#include "EndScreen.h"

EndScreen::EndScreen() :
	AppScreen("Complete"),
	resetBT("Start again")
{
	resetBT.addListener(this);
	addAndMakeVisible(&resetBT);
}

EndScreen::~EndScreen()
{
}

void EndScreen::paint(Graphics & g)
{
	g.setColour(Colours::lightgrey);
	g.drawFittedText("update successful.\ngo forth and find your flow ...", getLocalBounds(), Justification::centred, 3);
}

void EndScreen::resized()
{
	resetBT.setBounds(getLocalBounds().removeFromBottom(100).withSizeKeepingCentre(100, 20));
}

void EndScreen::buttonClicked(Button * b)
{
	if (b == &resetBT)
	{
		screenListeners.call(&ScreenListener::screenFinish, this);
	}
}
