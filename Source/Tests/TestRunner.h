#pragma once

// TestRunner.h — aggregates every *Tests.h file so their static instances
// register themselves with juce::UnitTest before runAllTests() executes.
// Add each new test header here as DSP blocks are added.

#include "InputConditionerTests.h"
#include "EnvelopeFollowerTests.h"
#include "WaveshaperChainTests.h"
#include "SubOctaveDividerTests.h"
