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
	AppScreen("Firmware", FW_CHOOSE),
	selectBT("Next step"),
	chooseFileBT("Choose local file")
{
	addChildComponent(selectBT);
	addChildComponent(fwChooser);
	addAndMakeVisible(chooseFileBT);

	selectBT.addListener(this);
	chooseFileBT.addListener(this);
	fwChooser.addListener(this);

	FirmwareManager::getInstance()->addAsyncManagerListener(this);
	
}

FirmwareChooserScreen::~FirmwareChooserScreen()
{
}

void FirmwareChooserScreen::updateVisibility()
{
	bool v = FirmwareManager::getInstance()->firmwaresAreLoaded();
	
	fwChooser.clear();
	fwChooser.setTextWhenNoChoicesAvailable("No firmware available, are you connected to internet ?");
	fwList = FirmwareManager::getInstance()->getFirmwaresForType(PropManager::getInstance()->selectedType);

	//fwChooser.addItem("auto update", -1);
	int indexToSelect = -1;
	for (int i = 0; i < fwList.size(); i++)
	{
		bool isLocal = FirmwareManager::getInstance()->localFirmware == fwList[i];
		String label = fwList[i]->infos;
		if (isLocal) label += " (local)";
		fwChooser.addItem(label, i + 1);
		if (isLocal) indexToSelect = i + 1;
		
	}
    fwChooser.setTextWhenNothingSelected(fwList.size() > 0?fwList[0]->infos:"No firmware");
    
	selectBT.setVisible(v);
	fwChooser.setVisible(v);
	selectBT.setEnabled(fwChooser.getNumItems() > 0);
	if (indexToSelect != -1)
	{
		DBG("Index to select : " << indexToSelect);
		fwChooser.setSelectedId(indexToSelect);
	}
	repaint();
}

void FirmwareChooserScreen::paint(Graphics & g)
{
	bool loaded = FirmwareManager::getInstance()->firmwaresAreLoaded();
	bool errored = FirmwareManager::getInstance()->errored;

	String s = "";
	if (!loaded && !errored) s = "No firmwares available online or locally right now.";
	else if (!loaded && errored) s = "There were errors while downloading online firmwares and no local firmwares availables.";
	else if (loaded && errored) s = "There were errors while downloading oneline firmwares, showing only local firmwares.";

	if (s.isNotEmpty()) g.setColour(Colours::orange);
	else
	{
		g.setColour(Colours::lightgrey);
		//s = "leave as \"auto update\" for the latest version\nor select your preferred firmware.";
		s = "please select the firmware of your choice.\n \
for the most recent version, select the highest version number.\n \
for poi and staff select \"spin(pixel count)\" \n \
for staves, the pixel count is 1 / 2 of the staff. \n \
to ensure the latest versions please restart the updater.";
	}

	g.drawFittedText(s, getLocalBounds().removeFromTop(100).reduced(40, 10), Justification::centred, 5, false);
	
}

void FirmwareChooserScreen::reset()
{
	if (!FirmwareManager::getInstance()->firmwaresAreLoaded() || FirmwareManager::getInstance()->errored) FirmwareManager::getInstance()->initLoad();
	
	updateVisibility();
}

void FirmwareChooserScreen::resized()
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> br = r.removeFromBottom(100);
	selectBT.setBounds(br.withSizeKeepingCentre(100, 40).translated(0, -50));
	chooseFileBT.setBounds(br.withSizeKeepingCentre(120, 40));
	fwChooser.setBounds(r.withSizeKeepingCentre(450, 30).translated(0,30));
}

void FirmwareChooserScreen::newMessage(const FirmwareManager::FirmwareManagerEvent & e)
{
	updateVisibility();
}

void FirmwareChooserScreen::comboBoxChanged(ComboBox *)
{
}

void FirmwareChooserScreen::buttonClicked(Button * b)
{
	if (b == &selectBT)
	{
		int index = fwChooser.getSelectedId();
		FirmwareManager::getInstance()->selectedFirmware = (index <= 0 ? fwList[0] : fwList[index-1]);
		screenListeners.call(&ScreenListener::screenFinish, this);
	} else if (b == &chooseFileBT)
	{
		FileChooser fc("Choose a firmware file",File(),"*.fwimg");
		bool result = fc.browseForFileToOpen();
		if (result)
		{
			File f = fc.getResult();
			bool result =FirmwareManager::getInstance()->setLocalFirmware(f, PropManager::getInstance()->selectedType);
			if (!result)
			{
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Bad file", "The file you chose is either corrupted or not a firmware for " + displayNames[PropManager::getInstance()->selectedType] + ".", "OK");
			}
			updateVisibility();
		}
	}
}
