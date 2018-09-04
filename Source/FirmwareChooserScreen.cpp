/*
  ==============================================================================

    FirmwareChooserScreen.cpp
    Created: 4 Sep 2018 12:39:25pm
    Author:  Ben

  ==============================================================================
*/

#include "FirmwareChooserScreen.h"
#include "PropManager.h"

FirmwareChooserScreen::FirmwareChooserScreen() :
	AppScreen("Firmware"),
	selectBT("Select")
{
	addAndMakeVisible(selectBT);
	addAndMakeVisible(fwChooser);
	selectBT.addListener(this);
	fwChooser.addListener(this);
}

FirmwareChooserScreen::~FirmwareChooserScreen()
{
}

void FirmwareChooserScreen::reset()
{
	fwChooser.clear();
	fwChooser.setTextWhenNoChoicesAvailable("No firmware available, are you connected to internet ?");
	fwChooser.setTextWhenNothingSelected("Auto-select latest version");

	fwList = FirmwareManager::getInstance()->getFirmwaresForType(PropManager::getInstance()->selectedType);
	for (int i = 0; i < fwList.size(); i++)
	{
		fwChooser.addItem(fwList[i]->infos, i + 1);
	}
}

void FirmwareChooserScreen::resized()
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> br = r.removeFromBottom(100);
	selectBT.setBounds(br.withSizeKeepingCentre(100, 40));
	fwChooser.setBounds(r.withSizeKeepingCentre(300, 30));
	
}

void FirmwareChooserScreen::comboBoxChanged(ComboBox *)
{
}

void FirmwareChooserScreen::buttonClicked(Button * b)
{
	if (b == &selectBT)
	{
		int index = fwChooser.getSelectedId();
		FirmwareManager::getInstance()->selectedFirmware = (index == 0 ? fwList[0] : fwList[index-1]);
		screenListeners.call(&ScreenListener::screenFinish, this);
	}
}
