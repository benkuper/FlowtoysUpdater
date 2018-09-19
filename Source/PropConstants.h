/*
  ==============================================================================

    PropConstants.h
    Created: 4 Sep 2018 4:45:05pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

#define PACKET_SIZE 64
#define DATA_PACKET_MAX_LENGTH (PACKET_SIZE-4) //4 bytes for command then data

enum PropType {NOTSET, CAPSULE, CLUB, TYPE_MAX };
const String typeStrings[TYPE_MAX]{ "NotSet", "Capsule", "Club" };
const String displayNames[TYPE_MAX]{"NotSet", "Capsule 2.0", "Vision Club" };
const String fwIdentStrings[TYPE_MAX]{ "notset", "capsule", "club" };
const int productIds[TYPE_MAX]{0, 0x1000, 0x1001 };
const int flowtoysVID = 0xF107;
