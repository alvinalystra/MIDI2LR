/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/
/*
  ==============================================================================

  Main.cpp

This file is part of MIDI2LR. Copyright 2015-2016 by Rory Jaffe.

MIDI2LR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

MIDI2LR is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MIDI2LR.  If not, see <http://www.gnu.org/licenses/>.
  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainComponent.h"
#include "LR_IPC_OUT.h"
#include "LR_IPC_IN.h"
#include "VersionChecker.h"
#include "MainWindow.h"
#include "CommandMap.h"
#include "SettingsManager.h"
#include "MIDISender.h"

#ifndef _WIN32
//missing make_unique (C++14) in XCode
namespace std
{
    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args)
    {
        return unique_ptr<T>{new T{ args... }};
    }
}
#endif

//constexpr doesn't work in XCode for String; auto type deduction also fails
const juce::String ShutDownString{ "--LRSHUTDOWN" };

/**********************************************************************************************//**
 * @class   MIDI2LRApplication
 *
 * @brief   A MIDI 2 lr application.
 *
 *
 *
 **************************************************************************************************/

class MIDI2LRApplication: public JUCEApplication
{
public:
    MIDI2LRApplication()
    {
        m_commandMap = std::make_shared<CommandMap>();
        m_profileManager = std::make_shared<ProfileManager>();
        m_settingsManager = std::make_shared<SettingsManager>();
        m_midiProcessor = std::make_shared<MIDIProcessor>();
        m_midiSender = std::make_shared<MIDISender>();
        m_lr_IPC_OUT = std::shared_ptr<LR_IPC_OUT>(new LR_IPC_OUT, [](LR_IPC_OUT* me)
        {
            me->shutdown();
        });
        m_lr_IPC_IN = std::shared_ptr<LR_IPC_IN>(new LR_IPC_IN, [](LR_IPC_IN* me)
        {
            me->shutdown();
        });
    }

    const String getApplicationName() override
    {
        return ProjectInfo::projectName;
    }
    const String getApplicationVersion() override
    {
        return ProjectInfo::versionString;
    }
    bool moreThanOneInstanceAllowed() override
    {
        return false;
    }

//==============================================================================
    void initialise(const String& commandLine) override
    {
        if (commandLine != ShutDownString)
        {
            m_midiProcessor->Init();
            m_midiSender->Init();
            m_lr_IPC_OUT->Init(m_commandMap, m_midiProcessor);
            //set the reference to the command map
            m_profileManager->Init(m_lr_IPC_OUT, m_commandMap, m_midiProcessor);
            //init the IPC_In
            m_lr_IPC_IN->Init(m_commandMap, m_profileManager, m_midiSender);
            // init the settings manager
            m_settingsManager->Init(m_lr_IPC_OUT, m_profileManager);
            mainWindow = std::make_unique<MainWindow>(getApplicationName());
            mainWindow->Init(m_commandMap, m_lr_IPC_IN, m_lr_IPC_OUT, m_midiProcessor, m_profileManager, m_settingsManager, m_midiSender);
            // Check for latest version
            _versionChecker.Init(m_settingsManager);
            _versionChecker.startThread();
        }
        else
        {
            // apparently the appication is already terminated
            mainWindow = nullptr; // (deletes our window)
            quit();
        }

    }

    void shutdown() override
    {
        std::call_once(ShutdownOnce,
            [&]()
        {
            // Save the current profile as default.xml
            auto defaultProfile = File::getSpecialLocation(File::currentExecutableFile).getSiblingFile("default.xml");
            m_commandMap->toXMLDocument(defaultProfile);
            m_lr_IPC_OUT.reset();
            m_lr_IPC_IN.reset();
            //below resets added
            m_commandMap.reset();
            m_profileManager.reset();
            m_settingsManager.reset();
            m_midiProcessor.reset();
            m_midiSender.reset();
            mainWindow = nullptr; // (deletes our window)
            quit();
        });
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        this->shutdown();
    }

    void anotherInstanceStarted(const String& commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
        if (commandLine == ShutDownString)
        {
            //shutting down
            this->shutdown();
        }
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
    VersionChecker _versionChecker;
    std::shared_ptr<CommandMap> m_commandMap;
    std::shared_ptr<LR_IPC_IN> m_lr_IPC_IN;
    std::shared_ptr<LR_IPC_OUT> m_lr_IPC_OUT;
    std::shared_ptr<ProfileManager> m_profileManager;
    std::shared_ptr<SettingsManager> m_settingsManager;
    std::shared_ptr<MIDIProcessor> m_midiProcessor;
    std::shared_ptr<MIDISender> m_midiSender;
    std::once_flag ShutdownOnce;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(MIDI2LRApplication)
