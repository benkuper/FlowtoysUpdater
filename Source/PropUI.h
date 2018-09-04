/*
  ==============================================================================

    PropUI.h
    Created: 4 Sep 2018 12:46:02pm
    Author:  Ben

  ==============================================================================
*/

#pragma once
#include "Prop.h"

class PropUI :
	public Component
{
public:
	PropUI(Prop * prop);
	~PropUI();

	Prop * prop;
};