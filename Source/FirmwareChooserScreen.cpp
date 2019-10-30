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
	selectBT("upload"),
	chooseFileBT("Choose local file")
{
	addChildComponent(selectBT);
	addChildComponent(fwChooser);
	addAndMakeVisible(chooseFileBT);

	selectBT.addListener(this);
	chooseFileBT.addListener(this);
	fwChooser.addListener(this);

	helpBT.setColour(helpBT.textColourId, Colours::lightblue);
	helpBT.setText("need help? click here.", dontSendNotification);
	helpBT.setJustificationType(Justification::centred);
	addAndMakeVisible(&helpBT);
	helpBT.setMouseCursor(MouseCursor::PointingHandCursor);
	helpBT.addMouseListener(this, false);

	FirmwareManager::getInstance()->addAsyncManagerListener(this);

}

FirmwareChooserScreen::~FirmwareChooserScreen()
{
}

void FirmwareChooserScreen::updateVisibility()
{
	bool v = FirmwareManager::getInstance()->firmwaresAreLoaded();

	String noFWText = "No firmware available, are you connected to internet?";
	fwChooser.clear();
	fwChooser.setTextWhenNoChoicesAvailable(noFWText);
	
	fwList = FirmwareManager::getInstance()->getFirmwaresForType(PropManager::getInstance()->selectedType, -1);

	int targetHW = -1;
	if (PropManager::getInstance()->props.size() > 0)
	{
		targetHW = PropManager::getInstance()->props[0]->hw_rev;
	}

	int indexToSelect = -1;
	for (int i = 0; i < fwList.size(); i++)
	{
		bool isLocal = FirmwareManager::getInstance()->localFirmware.get() == fwList[i];
		String label = fwList[i]->infos;
		if (isLocal) label += " (local)";
/*
		else if (targetHW == -1) {} //keep for testing
*/
		else if (!fwList[i]->isHardwareCompatible(targetHW))
		{
			DBG("Not compatible hardware !");
			continue;
		}

		fwChooser.addItem(label, i + 1);
		if (isLocal) indexToSelect = i + 1;

	}

	fwChooser.setTextWhenNothingSelected(fwChooser.getNumItems() > 0 ? "select a firmware" : noFWText);


	selectBT.setVisible(v);
	fwChooser.setVisible(v);
	selectBT.setEnabled(fwChooser.getNumItems() > 0);
	if (indexToSelect != -1)
	{
		DBG("Index to select : " << indexToSelect);
		fwChooser.setSelectedId(indexToSelect);
	}
	else if (fwChooser.getNumItems() > 0)
	{
		fwChooser.setSelectedItemIndex(0);
	}
	repaint();
}

void FirmwareChooserScreen::paint(Graphics& g)
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

		s += "SELECT YOUR FIRMWARE \
			\nto ensure the latest version, restart the updater, then choose the version marked \"CURRENT\"";

		if (PropManager::getInstance()->selectedType == PropType::CLUB)
		{
			s += "\nfor clubs, select your club type* for all other props, select spinXY(XY=pixel count) \
\n\n\nTO FIND THE CORRECT PIXEL COUNT FOR YOUR PROP \
\nturn on your prop + count the number of pixels down the length \
\nfor staves it is 1/2 the total pixels \
\nonly count 1 column of leds down the length \
\ndon't count every LED around the prop - there are 2-4 LEDs per pixel";
		}
	}

	g.drawFittedText(s, getLocalBounds().removeFromTop(200).reduced(40, 10), Justification::centred, 5, false);

}

void FirmwareChooserScreen::reset()
{
	if (!FirmwareManager::getInstance()->firmwaresAreLoaded() || FirmwareManager::getInstance()->errored) FirmwareManager::getInstance()->initLoad();

	updateVisibility();
}

void FirmwareChooserScreen::mouseDown(const MouseEvent& e)
{
	AppScreen::mouseDown(e);

	if (e.eventComponent == &helpBT)
	{
		URL url("https://flowtoys2.freshdesk.com/support/solutions/articles/6000213534-how-to-update-your-capsule-2-0-firmware");
		url.launchInDefaultBrowser();
	}
}

void FirmwareChooserScreen::resized()
{
	Rectangle<int> r = getLocalBounds();
	Rectangle<int> br = r.removeFromBottom(60);
	selectBT.setBounds(br.withSizeKeepingCentre(100, 40).translated(0, -50));
	chooseFileBT.setBounds(br.withSizeKeepingCentre(120, 40));
	helpBT.setBounds(r.withSizeKeepingCentre(450, 30).translated(0,40));
	fwChooser.setBounds(r.withSizeKeepingCentre(450, 30).translated(0, 80));
}

void FirmwareChooserScreen::newMessage(const FirmwareManager::FirmwareManagerEvent&)
{
	updateVisibility();
}

void FirmwareChooserScreen::comboBoxChanged(ComboBox*)
{
}

void FirmwareChooserScreen::buttonClicked(Button* b)
{
	if (b == &selectBT)
	{
		int index = fwChooser.getSelectedId();
		FirmwareManager::getInstance()->selectedFirmware = (index <= 0 ? fwList[0] : fwList[index - 1]);
		screenListeners.call(&ScreenListener::screenFinish, this);
	}
	else if (b == &chooseFileBT)
	{
		FileChooser fc("Choose a firmware file", File(), "*.fwimg");
		bool result = fc.browseForFileToOpen();
		if (result)
		{
			File f = fc.getResult();
			bool fResult = FirmwareManager::getInstance()->setLocalFirmware(f, PropManager::getInstance()->selectedType);
			if (!fResult)
			{
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Bad file", "The file you chose is either corrupted or not a firmware for " + displayNames[PropManager::getInstance()->selectedType] + ".", "OK");
			}
			updateVisibility();
		}
	}
}
