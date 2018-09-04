/*
  ==============================================================================

    PropConnectScreen.h
    Created: 4 Sep 2018 12:44:25pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

#include "Screen.h"
#include "PropUI.h"
#include "PropManager.h"

class PropConnectScreen : 
	public AppScreen,
	public PropManager::AsyncListener,
	public Button::Listener
{
public:
	PropConnectScreen();
	~PropConnectScreen();

	TextButton flashBT;

	void paint(Graphics &g) override;
	void resized() override;
	void reset() override;

	void newMessage(const PropManager::PropManagerEvent &e) override;

	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropConnectScreen)

		// Inherited via Listener
		virtual void buttonClicked(Button *) override;
};