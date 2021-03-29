// audio.h - Defines for the audio part of Sky Raider
// JH 2018

// Maths:
// - At BPM = 100 (bps = 1.666), new beat every 36 frames, new quarter-note every 9 frames.
// - However playing "instruments" envelopes need to be updated every frame

#define NULL 0
typedef unsigned char uchar;
typedef unsigned int uint;

#define NUM_CHANNELS	4
#define SPEED_75BPM		11
#define SPEED_83BPM		10
#define SPEED_90BPM   9
#define SPEED_100BPM	8		// Default song speed (100 BPM)
#define SPEED_115BPM	7		// 
#define SPEED_130BPM	6		// 
#define NUM_TRACKS		4		// Number of tracks in a song
#define PATTERN_SIZE	16		// Number of quarter-notes in a pattern
#define MAX_INSTR		8		// Max num of instr used in a song

//Instrument = struct with reg settings, and vol/pitch envelope:
//	+ Settings (feedback, shifter, counter, clock)
//	+ Envelope length
//	+ Loop flag
//	+ Vol envelope data
//	+ Pitch envelope data
#define ENVELOPE_SIZE	64
typedef struct INSTRUMENT {
	uint shifter;					// shift bits
	uint feedback;					// feedback bits
	uchar integrate;				// 0 or 1
	uchar octave;					// clock select 6 = lowest octave, 0 = highest
	uchar loopLength;				// Envelope loop length (0 for no loop)
	uchar envPos;					// Current position in envelope
	uchar volData[ENVELOPE_SIZE];	// Volume envelope
	uchar pitchData[ENVELOPE_SIZE];	// Pitch envelope
};

///////////////////////////////////////////////////////////////////////
// Sound system API (functions)
///////////////////////////////////////////////////////////////////////
extern void InitSound(void);
//extern void StartSound(uchar channel, struct INSTRUMENT *instrument, char vol, uchar freq);
extern void StartSound(uchar channel, struct INSTRUMENT *instrument, uchar freq);
extern void EndSound(uchar channel);
extern void EndAllSound(void);
extern void UpdateSound(void);
//extern void PlaySample(uchar *sampleData);


///////////////////////////////////////////////////////////////////////
// Tracker API
///////////////////////////////////////////////////////////////////////

/// Represents data for a note in a song
typedef struct TrackerNote
{
	uchar instrIndex;				// which instrument to trigger (0 = no intrument)
	uchar volume;					// volume of this note - 0..127
	uchar noteNumber;				// MIDI note number   /// TODO - Need freq table!
};

/// Represents data for 1 track of a pattern in the song
typedef struct TrackerPattern
{
	struct TrackerNote notes[PATTERN_SIZE];
};

/// Represents data for a "sequence slot" (bar) in the song
typedef struct TrackerBar
{
	uchar patternIndex[NUM_TRACKS]; // Which patterns are on the 4 tracks of this slot
};

/// Music player struct
typedef struct TrackerSong
{
	uchar speed;					// How many frames between quarter notes (speed 9 = 100 BPM)
	uchar tick;						// current speed countdown tick (0..speed)
	
	uchar numPatterns;
	struct TrackerPattern *patternData;
	
	uchar numBars;					// Number of sequence slots in the song
	struct TrackerBar *barData;		// Song "sequence slots" (bars)
	
	uchar numInstruments;						// Number of instruments used
	struct INSTRUMENT *instruments[MAX_INSTR];	// Pointer to the instruments
	
	uchar currentBar;					// Current pattern slot we are playing
	uchar currentPatternPos;			// Current position in the current pattern slot
	uchar currentPattern[NUM_TRACKS];	// Current patterns per 4 tracks in the current slot
	uchar currentInstr[NUM_TRACKS];		// Current intrument playing on each track
};

/// Initialise a song (to zero values)
void InitTrackerSong(struct TrackerSong *song);
/// Called every frame to update tracker state, trigger notes
void UpdateTrackerSong(struct TrackerSong *song);
