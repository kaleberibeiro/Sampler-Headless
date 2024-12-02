/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/


/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.
 For details about the syntax and how to create or use a module, see the
 JUCE Module Format.md file.


 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 juce_midi_ci
  vendor:             juce
  version:            7.0.9
  name:               JUCE MIDI CI Classes
  description:        Classes facilitating communication via MIDI Capability Inquiry
  website:            http://www.juce.com/juce
  license:            GPL/Commercial
  minimumCppStandard: 17

  dependencies:       juce_audio_basics

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/


#pragma once
#define JUCE_MIDI_CI_H_INCLUDED

#include "../juce_audio_basics/juce_audio_basics.h"

#include "ci/juce_CIFunctionBlock.h"
#include "ci/juce_CIMuid.h"
#include "ci/juce_CIEncoding.h"
#include "ci/juce_CIEncodings.h"
#include "ci/juce_CIMessages.h"
#include "ci/juce_CIChannelAddress.h"
#include "ci/juce_CIResponderOutput.h"
#include "ci/juce_CIParser.h"
#include "ci/juce_CISupportedAndActive.h"
#include "ci/juce_CIResponderDelegate.h"
#include "ci/juce_CIProfileStates.h"
#include "ci/juce_CIProfileAtAddress.h"
#include "ci/juce_CIProfileDelegate.h"
#include "ci/juce_CIProfileHost.h"
#include "ci/juce_CISubscription.h"
#include "ci/juce_CIPropertyDelegate.h"
#include "ci/juce_CIPropertyExchangeResult.h"
#include "ci/juce_CIPropertyExchangeCache.h"
#include "ci/juce_CIPropertyHost.h"
#include "ci/juce_CIDeviceFeatures.h"
#include "ci/juce_CIDeviceMessageHandler.h"
#include "ci/juce_CIDeviceOptions.h"
#include "ci/juce_CIDeviceListener.h"
#include "ci/juce_CIDevice.h"

namespace juce
{
    namespace ci = midi_ci;
}
