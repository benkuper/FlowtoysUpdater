/*
  ==============================================================================

    PropManager.h
    Created: 4 Sep 2018 1:36:56pm
    Author:  Ben

  ==============================================================================
*/

#pragma once

#include "Prop.h"

class PropManager :
	public Thread,
	public Timer,
	public Prop::AsyncListener
{
public:
	juce_DeclareSingleton(PropManager, true)

	PropManager();
	~PropManager();
	
	void checkProps();

	OwnedArray<Prop> props;

	//Settings from user choices
	//bool shouldCheck;
	PropType selectedType;
	
	bool isFlashing();

	void setSelectedType(PropType t);

    void clear();
	void reset();

	Prop * getItemWithSerial(StringRef serial);
	Prop * getItemWithHidDevice(hid_device * device);

	Prop * openDevice(hid_device_info * deviceInfo);

	PropType getTypeForProductID(int productID);

	void newMessage(const Prop::PropEvent &e) override;

	void flash();

	// Inherited via Thread
	virtual void run() override;

	// Inherited via Timer
	virtual void timerCallback() override;

	class PropManagerEvent
	{
	public:
		enum Type { PROPS_CHANGED, FLASHING_PROGRESS, FLASHING_ERROR };
		PropManagerEvent(Type type) : type(type) {}
		PropManagerEvent(Type type, float progress) : type(type), progress(progress) {}

		Type type;
		float progress;
	};

	QueuedNotifier<PropManagerEvent> queuedNotifier;
	typedef QueuedNotifier<PropManagerEvent>::Listener AsyncListener;
	void addAsyncManagerListener(AsyncListener* newListener) { queuedNotifier.addListener(newListener); }
	void addAsyncCoalescedManagerListener(AsyncListener* newListener) { queuedNotifier.addAsyncCoalescedListener(newListener); }
	void removeAsyncManagerListener(AsyncListener* listener) { queuedNotifier.removeListener(listener); }
};
