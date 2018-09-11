/*
  ==============================================================================

    PropManager.cpp
    Created: 4 Sep 2018 1:36:56pm
    Author:  Ben

  ==============================================================================
*/

#include "PropManager.h"
#include "hidapi.h"
#include "FirmwareManager.h"

juce_ImplementSingleton(PropManager)

PropManager::PropManager() :
	Thread("Props"),
	shouldCheck(false),
	selectedType(NOTSET),
    queuedNotifier(100)
{
	startTimerHz(2);
}

PropManager::~PropManager()
{
	signalThreadShouldExit();
	waitForThreadToExit(1000);
}

void PropManager::clear()
{
    props.clear();
}

void PropManager::checkProps()
{
	if (isFlashing()) return;
	if (!shouldCheck) return;

	bool changed = false;
	Array<String> foundSerials;

	//for (int i = 0; i < TYPE_MAX; i++)
	//{
	hid_device_info * deviceInfos = hid_enumerate(flowtoysVID, productIds[(int)selectedType]);
	hid_device_info * dInfo = deviceInfos;


	while (dInfo != nullptr)
	{
		Prop * p = getItemWithSerial(String(dInfo->serial_number));
        
        
		if (p != nullptr)
		{
			//already opened, do nothing;
        }else if(!String(dInfo->product_string).toLowerCase().contains("bootloader"))
        {
            DBG("App mode, not connecting");
            //sendAppReset(Subject::Bootloader);
            //sendGetStatus();
            //return;
        }else
		{
			openDevice(dInfo);
			changed = true;
		}

		foundSerials.add(dInfo->serial_number);
		dInfo = dInfo->next;
	}

	hid_free_enumeration(deviceInfos);
	//}

	Array<Prop *> devicesToRemove;
	for (auto &d : props)
	{
		if (!foundSerials.contains(d->serial))
		{
			//if (d->flashState == Device::FLASHING) continue;
			DBG("Device removed : " << d->serial);
			devicesToRemove.add(d);
		}
	}

	for (auto &d : devicesToRemove)
	{
		props.removeObject(d);
		changed = true;
	}

	if (changed)
	{
		DBG("Prop Changed");
		queuedNotifier.addMessage(new PropManagerEvent(PropManagerEvent::PROPS_CHANGED));
	}
}


Prop * PropManager::getItemWithSerial(StringRef serial)
{
	for (auto &d : props) if (d->serial == serial) return d;
	return nullptr;
}

Prop * PropManager::getItemWithHidDevice(hid_device * device)
{

	for (auto &d : props) if (d->device == device) return d;
	return nullptr;
}

Prop * PropManager::openDevice(hid_device_info * deviceInfo)
{
	if (deviceInfo->vendor_id == 0 || deviceInfo->product_id == 0 || deviceInfo->serial_number == 0) return nullptr;

	hid_device * d = hid_open(deviceInfo->vendor_id, deviceInfo->product_id, deviceInfo->serial_number);
	if (d == NULL)
	{
		DBG("Device could not be opened " << deviceInfo->serial_number);
		return nullptr;
	}

	DBG("Device opened : " << deviceInfo->product_string << " (" << deviceInfo->manufacturer_string << ") " << deviceInfo->serial_number << " : " << String::toHexString(deviceInfo->vendor_id) << ", " << String::toHexString(deviceInfo->product_id) << ", " << deviceInfo->product_string);

	Prop * cd = new Prop(String(deviceInfo->product_string), String(deviceInfo->serial_number), d, getTypeForProductID(deviceInfo->product_id), (int)deviceInfo->vendor_id, (int)deviceInfo->product_id, deviceInfo->serial_number);
	props.add(cd);
	cd->addAsyncCapsuleListener(this);
	return cd;
}

bool PropManager::isFlashing()
{
	return isThreadRunning();
}

void PropManager::setSelectedType(PropType t)
{
	props.clear(); 
	selectedType = t;
	shouldCheck = true;
}

void PropManager::reset()
{
	props.clear();
	shouldCheck = false;
}

void PropManager::run()
{
	//flash
	//Run the update thread here

	if (props.isEmpty()) {
		DBG("Nothing to flash");
		return;
	}

	int i = 0;
	for (auto &d : props)
	{
		if (threadShouldExit()) return;
		Firmware * f = FirmwareManager::getInstance()->selectedFirmware;

		DBG("Flashing device " << d->infos);
		d->flash(&f->data, f->totalBytesToSend);
		i++;
		sleep(50);
	}

	queuedNotifier.addMessage(new PropManagerEvent(PropManagerEvent::FLASHING_PROGRESS, 1));
	sleep(500);
}

void PropManager::timerCallback()
{
	checkProps();
}



//Constants



PropType PropManager::getTypeForProductID(int productID) {
	switch (productID)
	{
	case 0x1000: return CAPSULE;
	case 0x1001: return CLUB;
	}

	return CAPSULE;
}

void PropManager::newMessage(const Prop::PropEvent & e)
{
	if (e.type == Prop::PropEvent::FLASHING_PROGRESS)
	{
		float totalProgression = 0;
		for (auto &d : props) totalProgression += d->progression;
		totalProgression /= props.size();

		DBG("Total progression " << totalProgression);
		queuedNotifier.addMessage(new PropManagerEvent(PropManagerEvent::FLASHING_PROGRESS,  totalProgression));
	}
}

void PropManager::flash()
{
	startThread();
}

