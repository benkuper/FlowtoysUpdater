/*
  ==============================================================================

    FirmwareManager.h
    Created: 4 Sep 2018 12:46:38pm
    Author:  Ben

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "PropConstants.h"

class Firmware
{
public:
	Firmware(MemoryBlock data, int totalBytesToSend, var meta,  String infos, String versionString, float version, int pid, int vid) :
		data(data), totalBytesToSend(totalBytesToSend), meta(meta), infos(infos), versionString(versionString), version(version), pid(pid), vid(vid) {}

	MemoryBlock data;
	int totalBytesToSend;

	String infos;
	String versionString;
	var meta;
	float version;
	int pid;
	int vid;
};

class FirmwareComparator
{
public:
	FirmwareComparator() {}
	int compareElements(Firmware * f1, Firmware * f2)
	{
		return f1->version > f2->version ? -1 : (f1->version < f2->version ? 1 : 0); //inverse order, we want the latest first
	}
};

class FirmwareManager :
	public Thread,
	public URL::DownloadTask::Listener
{
public:
	juce_DeclareSingleton(FirmwareManager,true)

	FirmwareManager();
	~FirmwareManager();

	FirmwareComparator comparator;
 
	const String remoteHost = "http://flow-toys.com/fusion/";
	File firmwareFolder;
    OwnedArray<URL::DownloadTask> tasks;

	Firmware * selectedFirmware;

	void loadFirmwares();

	int downloadedFirmwares;
	int onlineFirmwares;

	OwnedArray<Firmware> firmwares;
	Array<Firmware *> getFirmwaresForType(PropType type);

	virtual void run() override;

	// Inherited via Listener
    virtual void progress (URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override;
    virtual void finished(URL::DownloadTask * task, bool success) override;
};
