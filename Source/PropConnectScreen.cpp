/*
  ==============================================================================

    PropConnectScreen.cpp
    Created: 4 Sep 2018 12:44:25pm
    Author:  Ben

  ==============================================================================
*/

#include "PropConnectScreen.h"

PropConnectScreen::PropConnectScreen() :
	AppScreen("Connect Props", CONNECT),
	flashBT("Upload"),
	helpBT("helpBT")
{
	PropManager::getInstance()->addAsyncManagerListener(this);
	addAndMakeVisible(&flashBT);
	flashBT.addListener(this);
	flashBT.setEnabled(false);

	helpBT.setColour(helpBT.textColourId, Colours::lightblue);
	helpBT.setText("Having troubles connecting your prop? Click here to find help or give feedback. Thank you!", dontSendNotification);
	helpBT.setJustificationType(Justification::centred);
	addAndMakeVisible(&helpBT);
	helpBT.setMouseCursor(MouseCursor::PointingHandCursor);
	helpBT.addMouseListener(this, false);
}

PropConnectScreen::~PropConnectScreen()
{
	if (PropManager::getInstanceWithoutCreating() != nullptr) PropManager::getInstance()->removeAsyncManagerListener(this);
}

void PropConnectScreen::paint(Graphics & g)
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> fr = r.removeFromBottom(100);

	int numProps = PropManager::getInstance()->props.size();
	String s = numProps == 0?"connect all the props to be updated via USB.\nyou can use a USB hub to update multiple compatible props at once.":String(numProps) + " " + displayNames[(int)(PropManager::getInstance()->selectedType)].toLowerCase() + " connected:\n";
	int index = 0;
	for (auto &p : PropManager::getInstance()->props)
	{
		if (index > 0) s += "\n";
		s += "["+ String(index) + "] "+ p->infos;
		index++;
	}


	g.setColour(Colours::lightgrey);
	g.drawFittedText(s, r, Justification::centred, numProps+1);
}

void PropConnectScreen::resized()
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> fr = r.removeFromBottom(100);

	helpBT.setBounds(fr.removeFromTop(30));
	flashBT.setBounds(fr.withSizeKeepingCentre(100, 40));
}

void PropConnectScreen::reset()
{
    PropManager::getInstance()->clear();
    
    
	PropManager::getInstance()->checkProps();
}

void PropConnectScreen::mouseDown(const MouseEvent & e)
{
	AppScreen::mouseDown(e);

	if (e.eventComponent == &helpBT)
	{
		URL url("https://flowtoys2.freshdesk.com/support/solutions/articles/6000213534-how-to-update-your-capsule-2-0-firmware");
		url.launchInDefaultBrowser();
	}
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

