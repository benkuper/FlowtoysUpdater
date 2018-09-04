/*
  ==============================================================================

    PropConnectScreen.cpp
    Created: 4 Sep 2018 12:44:25pm
    Author:  Ben

  ==============================================================================
*/

#include "PropConnectScreen.h"

PropConnectScreen::PropConnectScreen() :
	AppScreen("Connect Props"),
	flashBT("Upload")
{
	PropManager::getInstance()->addAsyncManagerListener(this);
	addAndMakeVisible(&flashBT);
	flashBT.addListener(this);
	flashBT.setEnabled(false);
}

PropConnectScreen::~PropConnectScreen()
{
	if (PropManager::getInstanceWithoutCreating() != nullptr) PropManager::getInstance()->removeAsyncManagerListener(this);
}

void PropConnectScreen::paint(Graphics & g)
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> fr = r.removeFromBottom(100);

	g.setColour(Colours::lightgrey);
	g.drawFittedText(String(PropManager::getInstance()->props.size()) + " " + typeStrings[(int)(PropManager::getInstance()->selectedType)] + " connected", r, Justification::centred, 3);
}

void PropConnectScreen::resized()
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> fr = r.removeFromBottom(100);

	flashBT.setBounds(fr.withSizeKeepingCentre(100, 40));
}

void PropConnectScreen::reset()
{
	PropManager::getInstance()->checkProps();
}

void PropConnectScreen::newMessage(const PropManager::PropManagerEvent & e)
{
	switch(e.type)
	{
	case PropManager::PropManagerEvent::PROPS_CHANGED:
		flashBT.setEnabled(PropManager::getInstance()->props.size() > 0);
		repaint();
		break;

	default:
		break;
	}
}

void PropConnectScreen::buttonClicked(Button * b)
{
	if (b == &flashBT)
	{
		screenListeners.call(&ScreenListener::screenFinish, this);
	}
}

