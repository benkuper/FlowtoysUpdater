/*
  ==============================================================================

    FirmwareChooserScreen.h
    Created: 4 Sep 2018 12:39:25pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

#include "Screen.h"
#include "FirmwareManager.h"

class FirmwareChooserScreen :
	public AppScreen,
	public Button::Listener,
	public ComboBox::Listener
{
public:
	FirmwareChooserScreen();
	~FirmwareChooserScreen();

	Array<Firmware *> fwList;
	ComboBox fwChooser;

	TextButton selectBT;

	void reset() override;

	void resized() override;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FirmwareChooserScreen)

		// Inherited via Listener
		virtual void comboBoxChanged(ComboBox * comboBoxThatHasChanged) override;

	// Inherited via Listener
	virtual void buttonClicked(Button *) override;
};