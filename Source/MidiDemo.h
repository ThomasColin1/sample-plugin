/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2022 - Raw Material Software Limited

  ==============================================================================
*/

#pragma once
#include "DemoUtilities.h"
#include "DSPDemos_Common.h"
#include <iostream>
#include <string>
using namespace std;
using namespace dsp;


//==============================================================================
struct GainDemoDSP
{
    void prepare (const ProcessSpec&)
    {
        gain.setGainDecibels (-6.0f);
    }

    void process (const ProcessContextReplacing<float>& context)
    {
        gain.process (context);
    }

    void reset()
    {
        gain.reset();
    }

    void updateParameters()
    {
        gain.setGainDecibels (static_cast<float> (gainParam.getCurrentValue()));
    }

    //==============================================================================
    Gain<float> gain;
    SliderParameter gainParam { { -100.0, 20.0 }, 3.0, -6.0, "Gain", "dB" };

    std::vector<DSPDemoParameterBase*> parameters { &gainParam };
};
//==============================================================================
struct MidiDeviceListEntry : ReferenceCountedObject
{
    MidiDeviceListEntry (MidiDeviceInfo info) : deviceInfo (info) {}

    MidiDeviceInfo deviceInfo;
    std::unique_ptr<MidiInput> inDevice;
    std::unique_ptr<MidiOutput> outDevice;

    using Ptr = ReferenceCountedObjectPtr<MidiDeviceListEntry>;
};


//==============================================================================
class MidiDemo  : public Component,
                  private Timer,
                  private MidiKeyboardState::Listener,
                  private MidiInputCallback,
                  private AsyncUpdater
{
public:
    //==============================================================================
    MidiDemo()
        : midiKeyboard       (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
          midiInputSelector  (new MidiDeviceListBox ("Midi Input Selector",  *this, true)),
          midiOutputSelector (new MidiDeviceListBox ("Midi Output Selector", *this, false))
    {
        noteTimings = new double[128];
        for(int i=0;i<128;i++) {
            noteTimings[i] = i/128.0;
        }
        
        addAndMakeVisible (fileReaderComponent);

        
        // addLabelAndSetStyle (midiInputLabel);
        // addLabelAndSetStyle (midiOutputLabel);
        // addLabelAndSetStyle (incomingMidiLabel);
        // addLabelAndSetStyle (outgoingMidiLabel);

        midiKeyboard.setName ("MIDI Keyboard");
        addAndMakeVisible (midiKeyboard);
        addAndMakeVisible (button);
        addAndMakeVisible (noteEditor);
        addAndMakeVisible (title);
        addAndMakeVisible (randomizerButton);

        midiMonitor.setMultiLine (true);
        midiMonitor.setReturnKeyStartsNewLine (false);
        midiMonitor.setReadOnly (true);
        midiMonitor.setScrollbarsShown (true);
        midiMonitor.setCaretVisible (false);
        midiMonitor.setPopupMenuEnabled (false);
        midiMonitor.setText ({});
        // addAndMakeVisible (midiMonitor);

        if (! BluetoothMidiDevicePairingDialogue::isAvailable())
            pairButton.setEnabled (false);

        // addAndMakeVisible (pairButton);
        pairButton.onClick = []
        {
            RuntimePermissions::request (RuntimePermissions::bluetoothMidi,
                                         [] (bool wasGranted)
                                         {
                                             if (wasGranted)
                                                 BluetoothMidiDevicePairingDialogue::open();
                                         });
        };
        button.onClick = [this] { 
            // cout << this->fileReaderComponent.getRelativePosition() << endl; 
            cout << this->noteSelected << endl;
            cout << this->noteTimings[this->noteSelected] << endl;
            // this->noteTimings[this->noteEditor.getText().getIntValue()] = this->timeEditor.getText().getDoubleValue();
            this->noteTimings[this->noteSelected] = this->fileReaderComponent.getRelativePosition();            
            cout << this->noteTimings[this->noteSelected] << endl;
            };
        randomizerButton.onClick = [this] {
            for (int i = 0; i < 127; i++){
                this->noteTimings[i] = (double)(rand() % 100) / 100;
            }
        };
        keyboardState.addListener (this);

        // addAndMakeVisible (midiInputSelector .get());
        // addAndMakeVisible (midiOutputSelector.get());

        title.setText("SAMPLER", dontSendNotification);
        title.setFont (Font (35.0f, Font::bold));
        title.setColour(Label::textColourId, Colour(255,255,255));
        

        setSize (732, 500);
        fileReaderComponent.setBounds(0,520, 732,200);

        this->resized();
    }

    ~MidiDemo() override
    {
        stopTimer();
        midiInputs .clear();
        midiOutputs.clear();
        keyboardState.removeListener (this);

        midiInputSelector .reset();
        midiOutputSelector.reset();
    }

    //==============================================================================
    void timerCallback() override
    {
        updateDeviceList (true);
        updateDeviceList (false);
    }

    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
        fileReaderComponent.goToRelativePosition(noteTimings[midiNoteNumber]);
        fileReaderComponent.play();
        cout << midiNoteNumber << endl;
        this->noteEditor.setText("Note pressed : "+String(midiNoteNumber), juce::dontSendNotification);
        this->noteSelected = midiNoteNumber;
        //this->timeEditor.setText(String(this->fileReaderComponent.getTime()));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }

    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override
    {
        MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber, velocity));
        fileReaderComponent.stop();
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        sendToOutputs (m);
    }

    void paint (Graphics& g) override {
        g.fillAll (juce::Colour(18,18,22));
    }

    void resized() override
    {
        title.setBounds(0,0,getWidth(),80);
        title.setJustificationType(Justification(Justification::horizontallyCentred));
        auto margin = 10;
        fileReaderComponent.setBounds(0,getHeight()-200, getWidth(),200);
        auto midiHeight=getHeight()-200;
        midiInputLabel.setBounds (margin, margin,
                                  (getWidth() / 2) - (2 * margin), 24);

        midiOutputLabel.setBounds ((getWidth() / 2) + margin, margin,
                                   (getWidth() / 2) - (2 * margin), 24);

        midiInputSelector->setBounds (margin, (2 * margin) + 24,
                                      (getWidth() / 2) - (2 * margin),
                                      (midiHeight / 3) - ((4 * margin) + 24 + 24));

        midiOutputSelector->setBounds ((getWidth() / 2) + margin, (2 * margin) + 24,
                                       (getWidth() / 2) - (2 * margin),
                                       (midiHeight / 3) - ((4 * margin) + 24 + 24));

        pairButton.setBounds (margin, (midiHeight / 3) - (margin + 24),
                              getWidth() - (2 * margin), 24);

        outgoingMidiLabel.setBounds (margin, midiHeight / 3, getWidth() - (2 * margin), 24);
        midiKeyboard.setBounds (margin, (midiHeight / 3) + (24 + margin), getWidth() - (2 * margin), 64);

        incomingMidiLabel.setBounds (margin, (midiHeight / 3) + (24 + (2 * margin) + 64),
                                     getWidth() - (2 * margin), 24);

        auto y1 = (midiHeight / 3) + ((2 * 24) + (3 * margin) + 64);
        auto y2 = (midiHeight / 3) + ((2 * 24) + (3 * margin) + 64);
        midiMonitor.setBounds (margin, y1,
                               getWidth() - (2 * margin), midiHeight - y2 - margin-50);
    // button.setBounds (margin, y1,
    //                            200, midiHeight - y2 - margin);
    button.setBounds (margin, y1,
                              getWidth()/2 - (2 * margin), 24);
    randomizerButton.setBounds (margin+getWidth()/2, y1,
                              getWidth()/2 - (2 * margin), 24);

        
    noteEditor.setBounds (margin, (midiHeight / 3) + (margin - 15),
                               200, 50);
    }

    void openDevice (bool isInput, int index)
    {
        if (isInput)
        {
            jassert (midiInputs[index]->inDevice.get() == nullptr);
            midiInputs[index]->inDevice = MidiInput::openDevice (midiInputs[index]->deviceInfo.identifier, this);

            if (midiInputs[index]->inDevice.get() == nullptr)
            {
                DBG ("MidiDemo::openDevice: open input device for index = " << index << " failed!");
                return;
            }

            midiInputs[index]->inDevice->start();
        }
        else
        {
            jassert (midiOutputs[index]->outDevice.get() == nullptr);
            midiOutputs[index]->outDevice = MidiOutput::openDevice (midiOutputs[index]->deviceInfo.identifier);

            if (midiOutputs[index]->outDevice.get() == nullptr)
            {
                DBG ("MidiDemo::openDevice: open output device for index = " << index << " failed!");
            }
        }
    }

    void closeDevice (bool isInput, int index)
    {
        if (isInput)
        {
            jassert (midiInputs[index]->inDevice.get() != nullptr);
            midiInputs[index]->inDevice->stop();
            midiInputs[index]->inDevice.reset();
        }
        else
        {
            jassert (midiOutputs[index]->outDevice.get() != nullptr);
            midiOutputs[index]->outDevice.reset();
        }
    }

    int getNumMidiInputs() const noexcept
    {
        return midiInputs.size();
    }

    int getNumMidiOutputs() const noexcept
    {
        return midiOutputs.size();
    }

    ReferenceCountedObjectPtr<MidiDeviceListEntry> getMidiDevice (int index, bool isInput) const noexcept
    {
        return isInput ? midiInputs[index] : midiOutputs[index];
    }

private:
    //==============================================================================
    struct MidiDeviceListBox : private ListBoxModel,
                               public ListBox
    {
        MidiDeviceListBox (const String& name,
                           MidiDemo& contentComponent,
                           bool isInputDeviceList)
            : ListBox (name),
              parent (contentComponent),
              isInput (isInputDeviceList)
        {
            setModel (this);
            setOutlineThickness (1);
            setMultipleSelectionEnabled (true);
            setClickingTogglesRowSelection (true);
        }

        //==============================================================================
        int getNumRows() override
        {
            return isInput ? parent.getNumMidiInputs()
                           : parent.getNumMidiOutputs();
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            auto textColour = getLookAndFeel().findColour (ListBox::textColourId);

            if (rowIsSelected)
                g.fillAll (textColour.interpolatedWith (getLookAndFeel().findColour (ListBox::backgroundColourId), 0.5));


            g.setColour (textColour);
            g.setFont ((float) height * 0.7f);

            if (isInput)
            {
                if (rowNumber < parent.getNumMidiInputs())
                    g.drawText (parent.getMidiDevice (rowNumber, true)->deviceInfo.name,
                                5, 0, width, height,
                                Justification::centredLeft, true);
            }
            else
            {
                if (rowNumber < parent.getNumMidiOutputs())
                    g.drawText (parent.getMidiDevice (rowNumber, false)->deviceInfo.name,
                                5, 0, width, height,
                                Justification::centredLeft, true);
            }
        }

        //==============================================================================
        void selectedRowsChanged (int) override
        {
            auto newSelectedItems = getSelectedRows();
            if (newSelectedItems != lastSelectedItems)
            {
                for (auto i = 0; i < lastSelectedItems.size(); ++i)
                {
                    if (! newSelectedItems.contains (lastSelectedItems[i]))
                        parent.closeDevice (isInput, lastSelectedItems[i]);
                }

                for (auto i = 0; i < newSelectedItems.size(); ++i)
                {
                    if (! lastSelectedItems.contains (newSelectedItems[i]))
                        parent.openDevice (isInput, newSelectedItems[i]);
                }

                lastSelectedItems = newSelectedItems;
            }
        }

        //==============================================================================
        void syncSelectedItemsWithDeviceList (const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices)
        {
            SparseSet<int> selectedRows;
            for (auto i = 0; i < midiDevices.size(); ++i)
                if (midiDevices[i]->inDevice.get() != nullptr || midiDevices[i]->outDevice.get() != nullptr)
                    selectedRows.addRange (Range<int> (i, i + 1));

            lastSelectedItems = selectedRows;
            updateContent();
            setSelectedRows (selectedRows, dontSendNotification);
        }

    private:
        //==============================================================================
        MidiDemo& parent;
        bool isInput;
        SparseSet<int> lastSelectedItems;
    };

    //==============================================================================
    void handleIncomingMidiMessage (MidiInput* /*source*/, const MidiMessage& message) override
    {
        // This is called on the MIDI thread
        const ScopedLock sl (midiMonitorLock);
        incomingMessages.add (message);
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        // This is called on the message loop
        Array<MidiMessage> messages;

        {
            const ScopedLock sl (midiMonitorLock);
            messages.swapWith (incomingMessages);
        }

        String messageText;

        for (auto& m : messages)
            messageText << m.getDescription() << "\n";

        midiMonitor.insertTextAtCaret (messageText);
    }

    void sendToOutputs (const MidiMessage& msg)
    {
        for (auto midiOutput : midiOutputs)
            if (midiOutput->outDevice.get() != nullptr)
                midiOutput->outDevice->sendMessageNow (msg);
    }

    //==============================================================================
    bool hasDeviceListChanged (const Array<MidiDeviceInfo>& availableDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                                : midiOutputs;

        if (availableDevices.size() != midiDevices.size())
            return true;

        for (auto i = 0; i < availableDevices.size(); ++i)
            if (availableDevices[i] != midiDevices[i]->deviceInfo)
                return true;

        return false;
    }

    ReferenceCountedObjectPtr<MidiDeviceListEntry> findDevice (MidiDeviceInfo device, bool isInputDevice) const
    {
        const ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                                      : midiOutputs;

        for (auto& d : midiDevices)
            if (d->deviceInfo == device)
                return d;

        return nullptr;
    }

    void closeUnpluggedDevices (const Array<MidiDeviceInfo>& currentlyPluggedInDevices, bool isInputDevice)
    {
        ReferenceCountedArray<MidiDeviceListEntry>& midiDevices = isInputDevice ? midiInputs
                                                                                : midiOutputs;

        for (auto i = midiDevices.size(); --i >= 0;)
        {
            auto& d = *midiDevices[i];

            if (! currentlyPluggedInDevices.contains (d.deviceInfo))
            {
                if (isInputDevice ? d.inDevice .get() != nullptr
                                  : d.outDevice.get() != nullptr)
                    closeDevice (isInputDevice, i);

                midiDevices.remove (i);
            }
        }
    }

    void updateDeviceList (bool isInputDeviceList)
    {
        auto availableDevices = isInputDeviceList ? MidiInput::getAvailableDevices()
                                                  : MidiOutput::getAvailableDevices();

        if (hasDeviceListChanged (availableDevices, isInputDeviceList))
        {

            ReferenceCountedArray<MidiDeviceListEntry>& midiDevices
                = isInputDeviceList ? midiInputs : midiOutputs;

            closeUnpluggedDevices (availableDevices, isInputDeviceList);

            ReferenceCountedArray<MidiDeviceListEntry> newDeviceList;

            // add all currently plugged-in devices to the device list
            for (auto& newDevice : availableDevices)
            {
                MidiDeviceListEntry::Ptr entry = findDevice (newDevice, isInputDeviceList);

                if (entry == nullptr)
                    entry = new MidiDeviceListEntry (newDevice);

                newDeviceList.add (entry);
            }

            // actually update the device list
            midiDevices = newDeviceList;

            // update the selection status of the combo-box
            if (auto* midiSelector = isInputDeviceList ? midiInputSelector.get() : midiOutputSelector.get())
                midiSelector->syncSelectedItemsWithDeviceList (midiDevices);
        }
    }

    //==============================================================================
    void addLabelAndSetStyle (Label& label)
    {
        label.setFont (Font (15.00f, Font::plain));
        label.setJustificationType (Justification::centredLeft);
        label.setEditable (false, false, false);
        label.setColour (TextEditor::textColourId, Colours::black);
        label.setColour (TextEditor::backgroundColourId, Colour (0x00000000));

        addAndMakeVisible (label);
    }

    //==============================================================================
    Label midiInputLabel    { "Midi Input Label",  "MIDI Input:" };
    Label midiOutputLabel   { "Midi Output Label", "MIDI Output:" };
    Label incomingMidiLabel { "Incoming Midi Label", "Received MIDI messages:" };
    Label outgoingMidiLabel { "Outgoing Midi Label", "Play the keyboard to send MIDI messages..." };
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent midiKeyboard;
    TextEditor midiMonitor  { "MIDI Monitor" };
    TextButton pairButton   { "MIDI Bluetooth devices..." };
    TextButton button { "Modify the pairing" };
    TextButton randomizerButton { "Randomize" };
    Label noteEditor;
    Label title;
    int noteSelected;

    double* noteTimings;
    

    AudioFileReaderComponent<GainDemoDSP> fileReaderComponent;
    std::unique_ptr<MidiDeviceListBox> midiInputSelector, midiOutputSelector;
    ReferenceCountedArray<MidiDeviceListEntry> midiInputs, midiOutputs;

    CriticalSection midiMonitorLock;
    Array<MidiMessage> incomingMessages;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiDemo)
};
