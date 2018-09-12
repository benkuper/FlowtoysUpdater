#include "AppUpdater.h"
/*
  ==============================================================================

    AppUpdater.cpp
    Created: 8 Apr 2017 4:26:46pm
    Author:  Ben

  ==============================================================================
*/


juce_ImplementSingleton(AppUpdater)

#if JUCE_MAC //for chmod
#include <sys/types.h>
#include <sys/stat.h>
#endif


AppUpdater::AppUpdater() :
	Thread("appUpdater"),
    progression(0),
    queuedNotifier(30)
{
	addAsyncUpdateListener(this);
}

AppUpdater::~AppUpdater()
{
	queuedNotifier.cancelPendingUpdate();
	signalThreadShouldExit();
	waitForThreadToExit(1000);
}

  void AppUpdater::setURLs(URL _updateURL, String _downloadURLBase, String _filePrefix)
{
	updateURL = _updateURL;  
	filePrefix = _filePrefix;
	downloadURLBase = _downloadURLBase;  
	if (!downloadURLBase.endsWithChar('/')) downloadURLBase += "/";
}

  String AppUpdater::getDownloadFileName(String version, String extension)
  {
	  String fileURL = filePrefix + "-";
#if JUCE_WINDOWS
	  fileURL += "win-x64";
#elif JUCE_MAC
	  fileURL += "osx";
#elif JUCE_LINUX
	  fileURL += "linux";
#endif

	  fileURL += "-" + version + "." + extension;

	  return fileURL;
  }

  void AppUpdater::checkForUpdates()
{
	if (updateURL.isEmpty() || downloadURLBase.isEmpty()) return;
	startThread();
}

void AppUpdater::showDialog(String title, String msg, String changelog)
{
	
	progression = 0;

	updateWindow = new UpdateDialogWindow(msg, changelog);
	DialogWindow::LaunchOptions dw;
	dw.content.set(updateWindow, false);
	dw.dialogTitle = title;
	dw.useNativeTitleBar = false;
	dw.escapeKeyTriggersCloseButton = true;
	dw.dialogBackgroundColour = Colour(0x555555);
	dw.launchAsync();
	
}

void AppUpdater::downloadUpdate()
{

	DBG("Download file name " << downloadingFileName);

	targetDir.createDirectory();

#if JUCE_WINDOWS
	File targetFile = File::getSpecialLocation(File::tempDirectory).getChildFile(downloadingFileName);
#else	
	File targetFile = targetDir.getChildFile(downloadingFileName);
	if (targetFile.existsAsFile()) targetFile.deleteFile();
#endif

	downloadingFileName = targetFile.getFileName();

	URL downloadURL = URL(downloadURLBase + downloadingFileName);

	DBG("Downloading " + downloadURL.toString(false) + "...");
	downloadTask = downloadURL.downloadToFile(targetFile, "", this);

	if (downloadTask == nullptr)
	{
		DBG("Error while downloading " + downloadingFileName + ",\ntry downloading it directly from the website.");
		queuedNotifier.addMessage(new AppUpdateEvent(AppUpdateEvent::DOWNLOAD_ERROR));
	}
	queuedNotifier.addMessage(new AppUpdateEvent(AppUpdateEvent::DOWNLOAD_STARTED));
}

void AppUpdater::run()
{
    //First cleanup update_temp directory
    targetDir = File::getSpecialLocation(File::currentApplicationFile).getParentDirectory().getChildFile("update_temp");
    if (targetDir.exists()) targetDir.deleteRecursively();

    
	StringPairArray responseHeaders;
	int statusCode = 0;
	ScopedPointer<InputStream> stream(updateURL.createInputStream(false, nullptr, nullptr, String(),
		2000, // timeout in millisecs
		&responseHeaders, &statusCode));
#if JUCE_WINDOWS
	if (statusCode != 200)
	{
		DBG("Failed to connect, status code = " + String(statusCode));
		return;
	}
#endif

	DBG("AppUpdater:: Status code " << statusCode);

	if (stream != nullptr)
	{
		String content = stream->readEntireStreamAsString();
		var data = JSON::parse(content);

		if (data.isObject())
		{

#if !JUCE_DEBUG
			if (data.getProperty("testing", false)) return;
#endif


			DBG(JSON::toString(data));

			String version = data.getProperty("version", "");

			if (versionIsNewerThan(version, ProjectInfo::versionString))
			{
				String msg = "A new version of "+ String(ProjectInfo::projectName) +" is available : " + version + ", do you want to update the app ?\nYou can also deactivate updates in the preferences.";

				Array<var> * changelog = data.getProperty("changelog", var()).getArray();
				String changelogString = "Changes since your version :\n\n";
				changelogString += "Version " + version + ":\n"; 
				for (auto &c : *changelog) changelogString += c.toString() + "\n";
				changelogString += "\n\n";

				Array<var> * oldChangelogs = data.getProperty("archives", var()).getArray();
				for (int i = oldChangelogs->size() - 1; i >= 0; i--)
				{
					var ch = oldChangelogs->getUnchecked(i);
					String chVersion = ch.getProperty("version", "1.0.0");
					if (versionIsNewerThan(ProjectInfo::versionString, chVersion)) break;

					changelogString += "Version " + chVersion + ":\n";
					Array<var> * versionChangelog = ch.getProperty("changelog", var()).getArray();
					for (auto &c : *versionChangelog) changelogString += c.toString() + "\n";
					changelogString += "\n\n"; 
				}


				String title = "New version available";

				String extension = "zip";
				#if JUCE_WINDOWS
								extension = data.getProperty("winExtension", "zip");
				#elif JUCE_MAC
								extension = data.getProperty("osxExtension", "zip");
				#elif JUCE_LINUX
								extension = data.getProperty("linuxExtension", "zip");
				#endif

				downloadingFileName = getDownloadFileName(version, extension);

				queuedNotifier.addMessage(new AppUpdateEvent(AppUpdateEvent::UPDATE_AVAILABLE, title, msg, changelogString));

			} else
			{
				DBG("App is up to date :) (Latest version online : " << version << ")");
			}
		} else
		{
			DBG("Error while checking updates, update file is not valid");
		}

	} else
	{
		DBG("Error while trying to access to the update file");
	}
}

void AppUpdater::finished(URL::DownloadTask * task, bool success)
{
	if (!success)
	{ 
		DBG("Error while downloading " + downloadingFileName + ",\ntry downloading it directly from the website.\nError code : " + String(task->statusCode()));
		queuedNotifier.addMessage(new AppUpdateEvent(AppUpdateEvent::DOWNLOAD_ERROR)); return;
	}


#if JUCE_WINDOWS
	File f = File::getSpecialLocation(File::tempDirectory).getChildFile(downloadingFileName);
#else
    File appFile = File::getSpecialLocation(File::currentApplicationFile);
    File appDir = appFile.getParentDirectory();
    
	File f = appDir.getChildFile("update_temp/" + downloadingFileName);
#endif
    
    
	if (!f.exists())
	{
		DBG("File doesn't exist");
		return;
	}

	if (f.getSize() < 1000000) //if file is less than 1Mo, got a problem
	{
		DBG("Wrong file size, try downloading it directly from the website");
		return;
	}

#if JUCE_WINDOWS
	File appFile = File::getSpecialLocation(File::tempDirectory).getChildFile(ProjectInfo::projectName + String("_install.exe"));
	f.copyFileTo(appFile);

#else
	File td = f.getParentDirectory();
	{
		ZipFile zip(f);
		zip.uncompressTo(td);
		Array<File> filesToCopy;

		appFile.moveFileTo(td.getNonexistentChildFile("oldApp", appFile.getFileExtension()));

        
		DBG("Move to " << appDir.getFullPathName());
		for (int i = 0; i < zip.getNumEntries(); i++)
		{
			File zf = td.getChildFile(zip.getEntry(i)->filename);
			DBG("File exists : " << (int)f.exists());
			zf.copyFileTo(appDir.getChildFile(zip.getEntry(i)->filename));
			//DBG("Move result for " << zf.getFileName() << " = " << (int)result);
		}
        
	}
	File curAppFile = File::getSpecialLocation(File::currentApplicationFile);
	File curAppDir = curAppFile.getParentDirectory();
	File tempDir = curAppDir.getChildFile("temp");
	tempDir.deleteRecursively();

#endif
    
#if JUCE_MAC
	chmod(File::getSpecialLocation(File::currentExecutableFile).getFullPathName().toUTF8(), S_IRWXO | S_IRWXU | S_IRWXG);
#endif

	queuedNotifier.addMessage(new AppUpdateEvent(AppUpdateEvent::UPDATE_FINISHED, f));

	MessageManagerLock mmLock;

	if (mmLock.lockWasGained())
	{
        
        DBG("Here");
		if (updateWindow != nullptr) updateWindow->removeFromDesktop();
		JUCEApplication::getInstance()->systemRequestedQuit();
        DBG("Start process");
		appFile.startAsProcess();
	}
	

}

void AppUpdater::progress(URL::DownloadTask *, int64 bytesDownloaded, int64 totalLength)
{
	progression = bytesDownloaded * 1.0f / totalLength;
	queuedNotifier.addMessage(new AppUpdateEvent(AppUpdateEvent::DOWNLOAD_PROGRESS, progression));
}

void AppUpdater::newMessage(const AppUpdateEvent & e)
{
	switch(e.type)
	{
	case AppUpdateEvent::UPDATE_AVAILABLE:
		showDialog(e.title, e.msg,e.changelog);
		break;

	case AppUpdateEvent::DOWNLOAD_ERROR:
	case AppUpdateEvent::UPDATE_FINISHED:
		updateWindow->getTopLevelComponent()->exitModalState(0);
		break;
            
        default:
            break;
	}
}

UpdateDialogWindow::UpdateDialogWindow(const String & msg, const String & changelog) :
	okButton("OK"),
	cancelButton("Cancel")
{
	addAndMakeVisible(&msgLabel);
	addAndMakeVisible(&changelogLabel);

	msgLabel.setColour(msgLabel.textColourId, Colours::lightgrey);
	msgLabel.setText(msg, dontSendNotification);

	changelogLabel.setMultiLine(true);
	changelogLabel.setColour(changelogLabel.backgroundColourId, Colours::darkgrey.darker());
	changelogLabel.setColour(changelogLabel.textColourId, Colours::lightgrey);
	changelogLabel.setScrollBarThickness(8);
	changelogLabel.setReadOnly(true);
	changelogLabel.setText(changelog);

	addAndMakeVisible(&okButton);
	addAndMakeVisible(&cancelButton);

	okButton.addListener(this);
	cancelButton.addListener(this);

	setSize(400,300);
}

void UpdateDialogWindow::paint(Graphics & g)
{
	g.fillAll(Colours::darkgrey);

	g.setColour(Colour(0xff111111));
	g.fillRoundedRectangle(progressionRect.toFloat(),2);
	g.setColour(Colours::limegreen);
	g.fillRoundedRectangle(progressionRect.toFloat().withWidth(progressionRect.getWidth()*AppUpdater::getInstance()->progression), 2);
}

void UpdateDialogWindow::resized()
{
	Rectangle<int> r = getLocalBounds().reduced(10);
	Rectangle<int> br = r.removeFromBottom(20);
	r.removeFromBottom(10);

	progressionRect = r.removeFromBottom(8);
	r.removeFromBottom(10);


	msgLabel.setBounds(r.removeFromTop(100));
	r.removeFromTop(10);
	changelogLabel.setBounds(r);

	cancelButton.setBounds(br.removeFromRight(100));
	br.removeFromRight(10);
	okButton.setBounds(br.removeFromRight(100));
}

void UpdateDialogWindow::newMessage(const AppUpdateEvent & e)
{
	if (e.type == AppUpdateEvent::DOWNLOAD_PROGRESS) repaint();
}

void UpdateDialogWindow::buttonClicked(Button * b)
{
	if (b == &okButton) AppUpdater::getInstance()->downloadUpdate();
	else if (b == &cancelButton)
	{
		getTopLevelComponent()->exitModalState(0);
	}
}


bool AppUpdater::versionIsNewerThan(String versionToCheck, String referenceVersion)
{
	StringArray fileVersionSplit;
	fileVersionSplit.addTokens(versionToCheck, juce::StringRef("."), juce::StringRef("\""));

	StringArray minVersionSplit;
	minVersionSplit.addTokens(referenceVersion, juce::StringRef("."), juce::StringRef("\""));

	int maxVersionNumbers = jmax<int>(fileVersionSplit.size(), minVersionSplit.size());
	while (fileVersionSplit.size() < maxVersionNumbers) fileVersionSplit.add("0");
	while (minVersionSplit.size() < maxVersionNumbers) minVersionSplit.add("0");

	for (int i = 0; i < maxVersionNumbers; i++)
	{
		int fV = fileVersionSplit[i].getIntValue();
		int minV = minVersionSplit[i].getIntValue();
		if (fV > minV) return true;
		else if (fV < minV) return false;
	}

	//if equals return false
	return false;
}