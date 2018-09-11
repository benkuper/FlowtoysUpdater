/*
  ==============================================================================

    Prop.cpp
    Created: 4 Sep 2018 12:45:57pm
    Author:  Ben

  ==============================================================================
*/

#include "Prop.h"

#pragma warning(disable:4189 4244)



Prop::Prop(StringRef productString, StringRef serial, hid_device * device, PropType type, int vid, int pid, const wchar_t * serialNumber) :
    productString(productString),
	type(type),
	serial(serial),
	dVid(vid),
	dPid(pid),
    device(device),
    deviceAckStatus(0),
    dSN(serialNumber),
	deviceStatus(NotSet),
	progression(0),
	infos("[Not set]"),
    queuedNotifier(50)
{
	
    if(!this->productString.toLowerCase().contains("bootloader"))
    {
        DBG("App mode, reset to bootloader");
        sendAppReset(Subject::Bootloader);
        sendGetStatus();
        return;
    }
    
	sendGetVersion(Bootloader);
	sendGetVersion(App);


	infos = /*"Type : " + typeStrings[(int)type] + ", */"firmware : " + fwVersion + ", updated : " + fw_date + ", hardware : " + String(serialNumber);
    
    
}

Prop::~Prop()
{
	hid_close(device);
}


void Prop::sendReset()
{
	MemoryOutputStream s;
	s.writeInt(Command::Reset);
	sendPacket(s);
}

void Prop::sendAppReset(Subject subject)
{
	MemoryOutputStream s;
	s.writeInt(Command::AppReset);
	s.writeInt(subject);
	sendPacket(s);
}

void Prop::sendUpdate(int totalBytesToSend)
{
	//DBG("Send update, vid " << String::toHexString(vid) << ", pid " << String::toHexString(pid));
	MemoryOutputStream s;
	s.writeInt(Command::Update);
	s.writeInt(totalBytesToSend);
	s.writeInt(0); //CRC = 0
	s.writeShort(vid);
	s.writeShort(pid);
	s.writeShort(hw_rev);
	sendPacket(s);
}

void Prop::sendData(const char * data, int length)
{
	MemoryOutputStream s;
	s.writeInt(Command::Data);
	s.write(data, length);
	sendPacket(s);
}

void Prop::sendGetStatus()
{
	MemoryOutputStream s;
	s.writeInt(Command::GetStatus);
	sendPacket(s);
	ScopedPointer<MemoryInputStream> result = readResponse();

	if (result != nullptr)
	{
		uint8_t cmd = result->readByte();
		for (int i = 0; i<3; i++) result->readByte(); //2 useless bytes
		deviceAckStatus = result->readInt();
		deviceStatus = (Status)result->readByte();

		statusRawMessage = result->readString();


		DBG("Status response : " << cmd << " / " << deviceAckStatus << " / " << deviceStatus << " : " << statusRawMessage);
	}
}

void Prop::sendGetVersion(Subject subject)
{
	MemoryOutputStream s;
	s.writeInt(Command::GetVersion);
	s.writeInt(subject);
	sendPacket(s);

	ScopedPointer<MemoryInputStream> result = readResponse();

	if (result != nullptr)
	{
		jassert(result->getDataSize() == 64);

		uint8_t cmd = result->readByte();

		if (subject == App) appActive = result->readByte() == 1;
		else bootloaderActive = result->readByte() == 1;
        
        /*
		String sub = "Subject " + String(subject) + " : ";
		for (int i = 0; i<2; i++) sub += String(result->readByte()) + ", "; //2 useless bytes
        DBG(s);
        */

		int _vid = result->readShort();
		int _pid = result->readShort();
		int _hw_rev = result->readShort();
		int _fw_rev = result->readShort();
		int64 _fw_date = result->readInt();
		char git_rev[8];
		result->read(git_rev, 8);
		String _gitRevString = String(git_rev);

		if (subject == Bootloader)
		{
			vid = _vid;
			pid = _pid;
			hw_rev = _hw_rev;
			gitRevString = _gitRevString;
		} else
		{
			fw_date = Time((int64)(_fw_date * 1000)).toString(true, false);
			fw_rev = _fw_rev;
			fwVersion = String(fw_rev >> 8) + "." + String::formatted("%02d", fw_rev & 0xff);
		}

		DBG("Date bootloader " << (int64)(_fw_date * 1000) << " : " << fw_date);


		char fw_ident[20];
		result->read(fw_ident, 20);
		String fwString = String(fw_ident);

		if (subject == App) fwIdentString = fwString;

		DBG((subject == App ? "App" : "Bootloader") << " version response : cmd = " << (int)cmd << ", appActive = " << (int)appActive << ", bootloader active = " << (int)bootloaderActive << ", vid = " << String::toHexString(vid) << ", pid = " << String::toHexString(pid) << ", hw_rev = " << (int)hw_rev << ", fw_rev = " << fwVersion << ", date = " << fw_date << ", git rev = " << gitRevString << ", fwIdent = " << fwString);
	} else
	{
		DBG("Result is null !");
	}
}


void Prop::sendPacket(MemoryOutputStream & stream, bool prependReportByte)
{
	if (device == nullptr)
	{
		DBG("Device null !");
		return;
	}

	MemoryOutputStream data;
	if (prependReportByte) data.writeByte(0);
	data.write(stream.getData(), stream.getDataSize());
	while (data.getDataSize() < PACKET_SIZE + (prependReportByte ? 1 : 0)) data.writeByte(0); //prepend report id


																							  //const unsigned char * d = (const unsigned char *)data.getData();
																							  //for (int i = 0; i < PACKET_SIZE; i++) DBG("Sending " << i << " : " << (int)d[i]);

	hid_write(device, (const unsigned char *)data.getData(), data.getDataSize());
	//DBG("Send packet " << bytesWritten << " bytes sent");
}

MemoryInputStream * Prop::readResponse()
{
	if (device == nullptr)
	{
		DBG("Device null !");
		return nullptr;
	}

	unsigned char response[PACKET_SIZE];
	int numRead = hid_read(device, response, PACKET_SIZE);

	//DBG("Read " << numRead << " bytes read");

	//for (int i = 0; i < PACKET_SIZE; i++) DBG(i << " : " << (int)response[i]);

	if (numRead > 0)
	{
		MemoryInputStream * result = new MemoryInputStream(response, numRead, true);
		return result;
	} else
	{
		return nullptr;
	}
}

void Prop::flash(MemoryBlock * dataBlock, int totalBytesToSend)
{
	jassert(!MessageManager::getInstance()->isThisTheMessageThread());

	Thread * thread = Thread::getCurrentThread();


	const int bufferSize = 64; //fixed for bootloader

	setProgression(0);

	//sendReset();
	//sleep(100);

	sendGetStatus();

	DBG("Device status : " << deviceStatus);
	sendGetStatus();

	if (deviceStatus != Idle)
	{
		sendReset();

		while (deviceStatus != Idle)
		{
			thread->sleep(10);
			sendGetStatus();
		}
	}

	sendUpdate(totalBytesToSend);
	thread->sleep(10);


	DBG("Erasing...");
	int index = 0;
	while (!thread->threadShouldExit())
	{
		sendGetStatus();
		thread->sleep(1);

		if (deviceStatus == Error)
		{
			DBG("ERROR ! " << statusRawMessage);
			//flashState->setValueWithData(ERROR);
			return;
		} else if (deviceStatus == EraseBusy)
		{
			//Erasing..
			DBG("Erasing...");
		} else if (deviceStatus == ProgramIdle)
		{
			DBG("Erase complete.");
			break;
		}

		DBG(deviceAckStatus);
		if (deviceAckStatus > 0)
		{
			MessageManagerLock mmLock;
			if (mmLock.lockWasGained())
			{
				setProgression(deviceAckStatus*.5f / 51200);
				//progression->queuedNotifier.triggerAsyncUpdate();
				thread->sleep(1);
			}
		}
		index++;
	}

	DBG("Flashing...");

	int currentPacket = 0;
	int currentDataOffset = 0;


	while (!thread->threadShouldExit())
	{
		sendGetStatus();
		if (deviceStatus == Error)
		{
			DBG("ERROR ! " << statusRawMessage);
			//flashState->setValueWithData(ERROR);
			return;
		} else if (deviceStatus == ProgramIdle && deviceAckStatus < totalBytesToSend)
		{
			//DBG("Sending with packet #" << currentPacket<<", offset : " << currentDataOffset << " (ackStatus : " << deviceAckStatus << ")");
			char data[DATA_PACKET_MAX_LENGTH];
			memset(data, 0, sizeof(char) * DATA_PACKET_MAX_LENGTH);
			dataBlock->copyTo(data, deviceAckStatus, DATA_PACKET_MAX_LENGTH);
			sendData(data, DATA_PACKET_MAX_LENGTH);

			currentDataOffset = deviceAckStatus;
			currentPacket++;

		} else if (deviceStatus == ProgramDone)
		{
			//Out of the loop
			break;
		}

		if (currentPacket % 10 == 0)
		{
			MessageManagerLock mmLock;
			if (mmLock.lockWasGained())
			{
				DBG("Progression : " << progression);
				setProgression(.5f + deviceAckStatus * .5f / totalBytesToSend);
				//progression->queuedNotifier.triggerAsyncUpdate();
				thread->sleep(1);
			}
		}
	}

	DBG("Flashing done !");
    sendAppReset(Subject::App);
	//flashState->setValueWithData(SUCCESS);
	setProgression(1);
}

void Prop::setProgression(float value)
{
	progression = value;
	queuedNotifier.addMessage(new PropEvent(this, PropEvent::FLASHING_PROGRESS, progression));
}

