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
#include "QueuedNotifier.h"

class Firmware
{
public:
	Firmware(MemoryBlock data, int totalBytesToSend, var meta,  String infos, String versionString, float version, int hwRev, int pid, int vid) :
		data(data), totalBytesToSend(totalBytesToSend), infos(infos), meta(meta),  versionString(versionString), version(version), hwRev(hwRev), pid(pid), vid(vid)
	{}

	MemoryBlock data;
	int totalBytesToSend;


	String infos;
	String versionString;
	var meta;
	int hwRev;
	float version;
	int pid;
	int vid;
	PropType type;

	static String getHwRevNameforHwRev(int hwRev)
	{
		switch (hwRev)
		{
		case 0: return "notset";
		case 0x300: return "C";
		case 0x400: return "D";
		case 0x500: return "E";
		case 0x600: return "F";
		case 0x700: return "G";
		case 0x800: return "H";
		}

		return "unknown";
	}

	bool isHardwareCompatible(int hardwareRev)
	{
		if (type == CAPSULE)
		{
			if (hardwareRev == 0x400 && hwRev == 0x300) return true;
			if (hardwareRev == 0x300 && hwRev == 0x400) return true;
		}

		return hardwareRev == hwRev;
	}
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
	public URL::DownloadTask::Listener,
	public Timer
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


	std::unique_ptr<Firmware> localFirmware;

	void initLoad();
	void clearFirmwares();
	void loadFirmwares();

	Firmware * getFirmwareForFile(File f);
	bool setLocalFirmware(File f, PropType expectedType);

	int downloadedFirmwares;
	int onlineFirmwares;
	bool errored;

	bool firmwaresAreLoaded();

	OwnedArray<Firmware> firmwares;
	Array<Firmware*> getFirmwaresForType(PropType type, int hardwareRevision);

	virtual void run() override;

	// Inherited via Listener
    virtual void progress (URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength) override;
    virtual void finished(URL::DownloadTask * task, bool success) override;


	void timerCallback() override;

	class FirmwareManagerEvent
	{
	public:
		enum Type { FIRMWARE_LOADED, FIRMWARE_LOAD_ERROR };
		FirmwareManagerEvent(Type type) : type(type) {}

		Type type;
	};

	QueuedNotifier<FirmwareManagerEvent> queuedNotifier;
	typedef QueuedNotifier<FirmwareManagerEvent>::Listener AsyncListener;
	void addAsyncManagerListener(AsyncListener* newListener) { queuedNotifier.addListener(newListener); }
	void addAsyncCoalescedManagerListener(AsyncListener* newListener) { queuedNotifier.addAsyncCoalescedListener(newListener); }
	void removeAsyncManagerListener(AsyncListener* listener) { queuedNotifier.removeListener(listener); }
};
