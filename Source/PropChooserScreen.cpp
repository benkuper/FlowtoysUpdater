/*
  ==============================================================================

    PropChooserScreen.cpp
    Created: 4 Sep 2018 12:38:47pm
    Author:  Ben

  ==============================================================================
*/

#include "PropChooserScreen.h"
#include "PropManager.h"

PropChooserScreen::PropChooserScreen() :
	AppScreen("Prop Type")
{
	for (int i = 0; i < TYPE_MAX; i++)
	{
		int dataSize = 0;
		const char * imageData = BinaryData::getNamedResource((typeStrings[i] + "_png").getCharPointer(), dataSize);
		Image img = ImageCache::getFromMemory(imageData, dataSize);
		DBG(typeStrings[i] + "_png" << " : " << img.getWidth());

		ImageButton * b = new ImageButton();
		b->setImages(true, false, true,
			img, .8f, Colours::transparentWhite,
			img, 1, Colours::transparentWhite,
			img, 1, Colours::yellow.withAlpha(.1f));
		addAndMakeVisible(b);
		buttons.add(b);
		b->addListener(this);
	}
}

PropChooserScreen::~PropChooserScreen()
{
}

void PropChooserScreen::resized()
{
	Rectangle<int> r = getLocalBounds();
	Point<int> cp = r.getCentre();
	const int gap = 200;

	for (int i = 0; i < buttons.size(); i++)
	{
		Point<int> p = cp + Point<int>((i - (buttons.size() / 2.0f) + .5f)*gap, 0);
		buttons[i]->setCentrePosition(p);
	}
}

void PropChooserScreen::buttonClicked(Button * b)
{
	int index = buttons.indexOf(dynamic_cast<ImageButton *>(b));
	if (index == -1) return;

	PropManager::getInstance()->setSelectedType((PropType)index);

	screenListeners.call(&ScreenListener::screenFinish, this);
}
