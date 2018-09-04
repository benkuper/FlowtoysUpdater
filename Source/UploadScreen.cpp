/*
  ==============================================================================

    UploadScreen.cpp
    Created: 4 Sep 2018 12:44:51pm
    Author:  Ben

  ==============================================================================
*/

#include "UploadScreen.h"

UploadScreen::UploadScreen() :
	AppScreen("Upload"),
	progression(0)
{
	PropManager::getInstance()->addAsyncManagerListener(this);
}

UploadScreen::~UploadScreen()
{
}

void UploadScreen::reset()
{
	progression = 0;
	PropManager::getInstance()->flash();
}

void UploadScreen::paint(Graphics & g)
{
	g.setColour(Colours::lightgrey);
	Rectangle<int> r = getLocalBounds().withSizeKeepingCentre(300, 40).translated(0, -60);
	g.drawFittedText(String("Flashing ") + String(PropManager::getInstance()->props.size()) + " with " + FirmwareManager::getInstance()->selectedFirmware->infos, r , Justification::centred, 5);

	r.translate(0, 60);
	r.setSize(300, 20);
	g.fillRoundedRectangle(r.toFloat(), 2);
	g.setColour(Colours::limegreen);
	r.setWidth(r.getWidth()*progression);
	g.fillRoundedRectangle(r.toFloat(), 2);
}

void UploadScreen::newMessage(const PropManager::PropManagerEvent & e)
{
	if (e.type == PropManager::PropManagerEvent::FLASHING_PROGRESS)
	{
		if (e.progress == 1)
		{
			screenListeners.call(&ScreenListener::screenFinish, this);
		} else
		{
			progression = e.progress;
			repaint();
		}
	}
}
