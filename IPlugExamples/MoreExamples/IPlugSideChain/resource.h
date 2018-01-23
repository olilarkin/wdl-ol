#define PLUG_MFR "AcmeInc"
#define PLUG_NAME "IPlugSideChain"

#define PLUG_CLASS_NAME IPlugSideChain

#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_NAME "IPlugSideChain"

#define PLUG_ENTRY IPlugSideChain_Entry
#define PLUG_VIEW_ENTRY IPlugSideChain_ViewEntry

#define PLUG_ENTRY_STR "IPlugSideChain_Entry"
#define PLUG_VIEW_ENTRY_STR "IPlugSideChain_ViewEntry"

#define VIEW_CLASS IPlugSideChain_View
#define VIEW_CLASS_STR "IPlugSideChain_View"

// Format        0xMAJR.MN.BG - in HEX! so version 10.1.5 would be 0x000A0105
#define PLUG_VER 0x00010000
#define VST3_VER_STR "1.0.0"

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'Ipsc'
// make sure this is not the same as BUNDLE_MFR
#define PLUG_MFR_ID 'Acme'

// ProTools stuff
#if (defined(AAX_API) || defined(RTAS_API)) && !defined(_PIDS_)
  #define _PIDS_
  const int PLUG_TYPE_IDS[1] = {'SCN1'};
#endif

#define PLUG_MFR_PT "AcmeInc\nAcmeInc\nAcme"
#define PLUG_NAME_PT "IPlugSideChain\nIPSC"
#define PLUG_TYPE_PT "Effect"
#define PLUG_DOES_AUDIOSUITE 0

/* PLUG_TYPE_PT can be "None", "EQ", "Dynamics", "PitchShift", "Reverb", "Delay", "Modulation", 
"Harmonic" "NoiseReduction" "Dither" "SoundField" "Effect" 
instrument determined by PLUG _IS _INST
*/

#ifdef RTAS_API
// RTAS can only have a mono sc input
// at the moment this is required instead of "2-2 3-2"
#define PLUG_CHANNEL_IO "3-2"
#define PLUG_SC_CHANS 1

#else // AU & VST2
#define PLUG_CHANNEL_IO "2-2 4-2"
#define PLUG_SC_CHANS 2
#endif

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0

// if this is 0 RTAS can't get tempo info
#define PLUG_DOES_MIDI 0

#define PLUG_DOES_STATE_CHUNKS 0

// Unique IDs for each image resource.
#define KNOB_ID 101

// Image resource locations for this plug.
#define KNOB_FN "resources/img/knob.png"

// GUI default dimensions
#define GUI_WIDTH 300
#define GUI_HEIGHT 300

// on MSVC, you must define SA_API in the resource editor preprocessor macros as well as the c++ ones
#if defined(SA_API) && !defined(OS_IOS)
#include "app_wrapper/app_resource.h"
#endif

// vst3 stuff
#define MFR_URL "www.olilarkin.co.uk"
#define MFR_EMAIL "spam@me.com"
#define EFFECT_TYPE_VST3 "Fx"

/* "Fx|Analyzer"", "Fx|Delay", "Fx|Distortion", "Fx|Dynamics", "Fx|EQ", "Fx|Filter",
"Fx", "Fx|Instrument", "Fx|InstrumentExternal", "Fx|Spatial", "Fx|Generator",
"Fx|Mastering", "Fx|Modulation", "Fx|PitchShift", "Fx|Restoration", "Fx|Reverb",
"Fx|Surround", "Fx|Tools", "Instrument", "Instrument|Drum", "Instrument|Sampler",
"Instrument|Synth", "Instrument|Synth|Sampler", "Instrument|External", "Spatial",
"Spatial|Fx", "OnlyRT", "OnlyOfflineProcess", "Mono", "Stereo",
"Surround"
*/
