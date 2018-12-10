/*
  ==============================================================================

    FirmwareManager.cpp
    Created: 4 Sep 2018 12:46:38pm
    Author:  Ben

  ==============================================================================
*/

#include "FirmwareManager.h"

juce_ImplementSingleton(FirmwareManager)

FirmwareManager::FirmwareManager() :
	Thread("Firmwares"),
    selectedFirmware(nullptr),
    errored(false),
    queuedNotifier(50)

{
	firmwareFolder = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("FlowtoysFirmwares");
	if (!firmwareFolder.exists()) firmwareFolder.createDirectory();

	startThread();
	startTimer(1000*3600); //check every hour
}

FirmwareManager::~FirmwareManager()
{
	signalThreadShouldExit();
	waitForThreadToExit(3000);
}

void FirmwareManager::initLoad()
{
	startThread();
}

void FirmwareManager::clearFirmwares()
{
	DBG("Clear firmwares");
	Array<File> files = firmwareFolder.findChildFiles(File::TypesOfFileToFind::findFiles, false, "*.fwimg");
	for (auto &f : files) f.deleteFile();
}

void FirmwareManager::loadFirmwares()
{
	firmwares.clear();

	DBG("Loading firmwares");
	Array<File> files = firmwareFolder.findChildFiles(File::TypesOfFileToFind::findFiles, false, "*.fwimg");
	DBG("Found " << files.size() << " local files");
	for (auto &f : files)
	{
		Firmware * fw = getFirmwareForFile(f);
		if(fw != nullptr) firmwares.add(fw);
	}

	firmwares.sort(comparator, true);

	DBG(firmwares.size() << " loaded.");
	queuedNotifier.addMessage(new FirmwareManagerEvent(FirmwareManagerEvent::FIRMWARE_LOADED));
}

Firmware * FirmwareManager::getFirmwareForFile(File f)
{
	if (f.getSize() == 0)
	{
		DBG("Wrong file size, removing file");
		f.deleteFile();
		return nullptr;
	}

	ZipFile zip(f);
	const ZipFile::ZipEntry * meta = zip.getEntry("meta");
	const ZipFile::ZipEntry * data = zip.getEntry("data");


	//data
	ScopedPointer<InputStream> dataStream = zip.createStreamForEntry(*data);
	int numBytesToSend = (int)dataStream->getTotalLength();
	float numDataPackets = (float)(ceilf(numBytesToSend*1.0f / DATA_PACKET_MAX_LENGTH));
	int totalBytesToSend = (int)(numDataPackets * DATA_PACKET_MAX_LENGTH);
	DBG("File size : " << numBytesToSend << ", split into " << numDataPackets << " =  " << totalBytesToSend << " total bytes");
	MemoryBlock fwData;
	dataStream->readIntoMemoryBlock(fwData);


	ScopedPointer<InputStream> metaStream = zip.createStreamForEntry(*meta);
	var fwMeta = JSON::fromString(metaStream->readEntireStreamAsString());

	if (!fwMeta.isObject())
	{
		DBG("Problem with meta here");
		return nullptr;
	}

	int targetVID = (int)fwMeta.getProperty("usb_vid", 0);
	int targetPID = (int)fwMeta.getProperty("usb_pid", 0);
	uint16 fwRev = (uint16)(int)fwMeta.getProperty("fw_rev", 0);
	String targetVersion = String(fwRev >> 8) + "." + String::formatted("%02d", fwRev & 0xff);
	String fwDate = Time((int64)((int64)fwMeta.getDynamicObject()->getProperty("fw_date")) * 1000).toString(true, false);
	String gitRev = fwMeta.getProperty("git_rev", "[not set]");
	String fwIdent = fwMeta.getProperty("fw_ident", "[not set]");

	Firmware * fw = new Firmware(fwData, totalBytesToSend, fwMeta, f.getFileNameWithoutExtension() + " - " + fwIdent + ", version " + targetVersion + " (" + fwDate + ")", targetVersion, targetVersion.getFloatValue(), targetPID, targetVID);
	DBG(fwIdentStrings->indexOf(fwIdent) << " <> " << (int)(fwIdent == "capsule"));
	
	for (int i = 0; i < TYPE_MAX; i++)
	{
		if (productIds[i] == targetPID) fw->type = (PropType)i;
	}

	return fw;
}

bool FirmwareManager::setLocalFirmware(File f, PropType expectedType)
{
	Firmware * fw = getFirmwareForFile(f);
	if (fw == nullptr)
	{
		delete fw;
		return false;
	}
	if (fw->type != expectedType)
	{
		delete fw;
		return false;
	}

	localFirmware = fw;
	return fw != nullptr;
}

bool FirmwareManager::firmwaresAreLoaded()
{
	return firmwares.size() > 0;
}

Array<Firmware *> FirmwareManager::getFirmwaresForType(PropType type)
{
	Array<Firmware *> result;
	if (type == NOTSET) return result;

	int targetPID = productIds[type];
	for (auto &f : firmwares)
	{
		if (f->pid == targetPID) result.add(f);
	}

	if (localFirmware != nullptr && localFirmware->type == type) result.add(localFirmware);

	return result;
}

void FirmwareManager::run()
{
	errored = false;

	StringPairArray responseHeaders;
	int statusCode = 0;

	URL updateURL(remoteHost + "firmwares.php");
	ScopedPointer<InputStream> stream(updateURL.createInputStream(false, nullptr, nullptr, String(),
		2000, // timeout in millisecs
		&responseHeaders, &statusCode));
#if JUCE_WINDOWS
	if (statusCode != 200)
	{
		DBG("Failed to connect, status code = " << String(statusCode));
		errored = true;
		loadFirmwares();
		return;
	}
#endif

	DBG("Firmware updater:: Status code " << statusCode);

	if (stream != nullptr)
	{
		String content = stream->readEntireStreamAsString(); 
		var data = JSON::parse(content);

		if (data.isObject())
		{
			bool shouldClear = data.getProperty("clear", false);
			if (shouldClear) clearFirmwares();

			var fileData = data.getProperty("files", var());
			if (fileData.isArray())
			{
				onlineFirmwares = fileData.size();
				DBG("Got " << onlineFirmwares << " online firmwares");
				downloadedFirmwares = 0;
				for (int i = 0; i < onlineFirmwares; i++)
				{
					File f = firmwareFolder.getChildFile(fileData[i].toString());
					if (f.exists())
					{
						DBG("File already downloaded");
						downloadedFirmwares++;
						if (downloadedFirmwares == onlineFirmwares) loadFirmwares();
					} else
					{
                        String fURL = remoteHost + String("firmwares/") + fileData[i].toString().replace(" ","%20"); //add handling for spaces
						DBG("Downloading " << fURL);
						URL downloadURL(fURL);
                        URL::DownloadTask * t = downloadURL.downloadToFile(f, "", this);
                        if(t == nullptr)
                        {
                            DBG("Download errored");
							downloadedFirmwares++;
                        }else
                        {
                            tasks.add(t);
                        }
					}
				}
			}
		} else
		{
			DBG("Error reading online firmware list, loading local firmwares");
			errored = true;
			loadFirmwares();
		}
	} else
	{
		DBG("Couldn't access internet, loading local firmwares");
		errored = true;
		queuedNotifier.addMessage(new FirmwareManagerEvent(FirmwareManagerEvent::FIRMWARE_LOAD_ERROR));
		loadFirmwares();
	}

}

void FirmwareManager::progress (URL::DownloadTask* task, int64 bytesDownloaded, int64 totalLength)
{
    DBG("Progress !");
}

void FirmwareManager::finished(URL::DownloadTask * task, bool success)
{
	downloadedFirmwares++;

	if (success)
	{
		
		if (downloadedFirmwares == onlineFirmwares)
		{
			DBG("ALL firmware downloaded !");
			loadFirmwares();
		}
	}else
    {
        DBG("Finished with errors");

		queuedNotifier.addMessage(new FirmwareManagerEvent(FirmwareManagerEvent::FIRMWARE_LOAD_ERROR));
    }
}

void FirmwareManager::timerCallback()
{
	startThread();
}
