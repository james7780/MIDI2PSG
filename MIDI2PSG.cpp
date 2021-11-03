// MIDI2PSG.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <malloc.h>
#include <io.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define UINT unsigned int
#include "HP_Midifile.h"					// Reading MIDI file

#include "PSGTracker.h"						// Output format

#define nullptr NULL


/// Compare 2 patterns
/// @return		True if they are identical
bool ComparePattern(TrackerPattern *pattern1, TrackerPattern *pattern2)
{
	for (int j = 0; j < PATTERN_SIZE; j++)
		{
		if (pattern1->notes[j].instrIndex != pattern2->notes[j].instrIndex)
			return false;
		if (pattern1->notes[j].volume != pattern2->notes[j].volume)
			return false;
		if (pattern1->notes[j].noteNumber != pattern2->notes[j].noteNumber)
			return false;
		}

	return true;
}

/// Find a matching pattern in the src patterns
/// @param[in] srcPatterns			The patterns to search
/// @param[in] numPatterns			Number of patterns to search
/// @param[in] pattern					Pattern data to search for
/// @return			Index of found pattern, else -1
int FindMatchingPattern(TrackerPattern *srcPatterns, int numPatterns, TrackerPattern *pattern)
{
	for (int i = 0; i < numPatterns; i++)
		{
		if (ComparePattern(&srcPatterns[i], pattern))
			return i;
		}

	return -1;
}

/// Copy pattern data
void CopyPattern(TrackerPattern *srcPattern, TrackerPattern *destPattern)
{
	for (int j = 0; j < PATTERN_SIZE; j++)
		{
		destPattern->notes[j].instrIndex = srcPattern->notes[j].instrIndex;
		destPattern->notes[j].volume = srcPattern->notes[j].volume;
		destPattern->notes[j].noteNumber = srcPattern->notes[j].noteNumber;
		}
}

/// Reset pattern data
void ResetPattern(TrackerPattern *pattern)
{
	for (int j = 0; j < PATTERN_SIZE; j++)
		{
		pattern->notes[j].instrIndex = 0;
		pattern->notes[j].volume = 0;
		pattern->notes[j].noteNumber = 0;
		}
}


int main(int argc, char* argv[])
{
	//FILE *fpMIDI = nullptr;

	// Show info
	printf("MIDI2PSG v1.0 - MIDI to PSG data converter.\n");
	printf("Copyright (C) 2020 by Jum Hig. All rights reserved...\n\n");
	// Validate params
	if (argc != 2)
		{
		printf("Usage: MIDI2PSG <infile>\n");
		printf("   infile: MIDI file (.mid)\n");
		//printf("   outfile: Casfile, e.g. game.cas. Existing file will be overwritten.\n");
		//printf("   d: (Optionable.) When present, output will be raw debug data.\n\n");
		exit(0);
		}

/*
	// Read in the first 4 tracks of the MIDI file
	fpMIDI = fopen(argv[1],"rb");
	if (!fpMIDI)
		{
		printf("Error opening file <%s>.\n",argv[1]);
		exit(-1);
		}
	printf("Reading file <%s>. ",argv[1]);

	// Get the length of the MIDI file
	fseek(fpMIDI, 0, SEEK_END);
	int midiLength = ftell(fpMIDI);
*/

	// Init HP MIDI reading library, and get it to read the MIDI file
	HP_MIDIFILE_MODULE *song = HP_Init();
	if (HP_ERR_NONE != HP_Load(song, argv[1]))
		{
		printf("Error opening file <%s>.\n", argv[1]);
		HP_Free(song);
		exit(-1);
		}

	int ppqn = 0;
	HP_ReadPPQN(song, &ppqn);
	printf("PPQN = %d\n", ppqn);

// TEST - Fix PPQN to 48
	ppqn = 48;
	//ppqn = 24;
	//ppqn = 12;					// Handle 16th notes

	int totalTicks = 0;
	HP_GetLastTime(song, &totalTicks);
	const int numBars = totalTicks / ppqn / 16;
	printf("Song length = %d ticks (%d 1/4 notes, %d bars)\n", totalTicks, totalTicks / ppqn, numBars);

	// Initialise output "bar data"
	TrackerBar *barData = new TrackerBar[numBars]; 		// Song "sequence slots" (bars)
	for (int i = 0; i < numBars; i++)
		{
		barData[i].patternIndex[0] = 0;
		barData[i].patternIndex[1] = 0;
		barData[i].patternIndex[2] = 0;
		barData[i].patternIndex[3] = 0;
		}

	// Initialise output pattern space (enough patterns for 4 tracks)
	const int maxPatterns = numBars * 4;
	TrackerPattern *patternData = new TrackerPattern[maxPatterns];
	for (int i = 0; i < maxPatterns; i++)
		{
		ResetPattern(&patternData[i]);
		}

	// Read all events from track 1 to track 4
	int nextPatternIndex = 1;		// Pattern 0 ALWAYS blank!
	for (int track = 0; track < 4; track++)
		{
		printf ("Reading track %d...\n", track);
		HP_Rewind(song);
		int currentBar = 0;
		TrackerPattern workPatternData;
		ResetPattern(&workPatternData);

		int eventId = 0;
		int eventChannel = 0;
		int eventTime = 0;
		int eventType = 0;
		while (HP_ERR_NONE == HP_ReadEvent(song, &eventId, &eventChannel, &eventTime, &eventType))
			{
			//printf("Event: Id=%d, Channel=%d, Time=%d, Type=0x%X\n", eventId, eventChannel, eventTime, eventType);
			
			// Track/instrument names
			if (HP_INSTRUMENT == eventType)
				{
				int noteTime = 0;
				char *text = NULL;
				if (HP_ERR_NONE == HP_ReadInstrument(song, eventId, &noteTime, &text))
					printf("Instrument: %s (track %d)\n", text, track);
				
				HP_Delete(text);
				}

			// We're only interested in note on/off on the track we're interested in
			if (eventChannel == track && HP_NOTE == eventType)
				{
				int noteValue = 0;
				int noteChannel = 0;
				int noteTime = 0;
				int noteVelocity = 0;
				int noteLength = 0;
				HP_ReadNote(song, eventId, &noteTime, &noteChannel, &noteValue, &noteVelocity, &noteLength);
				printf("   Note: Ticks=%d, Channel=%d, Value=%d, Velocity=%d, Length=%d\n", noteTime, noteChannel, noteValue, noteVelocity, noteLength);
				
				// Assign to the correct pattern
				int bar = noteTime / ppqn / 16;
				// New bar?
				if (bar > currentBar)
					{
					//// Copy work pattern data to relevant output pattern data
					//CopyPattern(&workPatternData, &patternData[nextPattern]);
					//barData[currentBar].patternIndex[track] = nextPattern;
					//nextPattern++;

					// Is this a duplicate of an existing pattern?
					int existingPatternIndex = FindMatchingPattern(patternData, nextPatternIndex, &workPatternData);
					if (existingPatternIndex >= 0)
						{
						// Set up bar data for the existing pattern index
						barData[currentBar].patternIndex[track] = existingPatternIndex;
						}
					else
						{
						// New unique pattern - Copy work pattern data to relevant output pattern data
						CopyPattern(&workPatternData, &patternData[nextPatternIndex]);
						barData[currentBar].patternIndex[track] = nextPatternIndex;
						nextPatternIndex++;
						}

					// Prepare work patterm for new bar
					ResetPattern(&workPatternData);
					currentBar = bar;
					}

				int barStartTick = bar * 16 * ppqn;
				int n = (noteTime - barStartTick) / ppqn;	// 1/4 notes offset into bar
				workPatternData.notes[n].instrIndex = track + 1;
				workPatternData.notes[n].volume = noteVelocity;
				workPatternData.notes[n].noteNumber = noteValue;
				}
			}

		// Copy last work pattern data to relevant output pattern data
		int existingPatternIndex = FindMatchingPattern(patternData, nextPatternIndex, &workPatternData);
		if (existingPatternIndex >= 0)
			{
			// Set up bar data for the existing pattern index
			barData[currentBar].patternIndex[track] = existingPatternIndex;
			}
		else
			{
			// New unique pattern - Copy work pattern data to relevant output pattern data
			CopyPattern(&workPatternData, &patternData[nextPatternIndex]);
			barData[currentBar].patternIndex[track] = nextPatternIndex;
			nextPatternIndex++;
			}
		}	// next track

	HP_Free(song);
	song = nullptr;

	// Write the PSG data output
	printf("Writing PSG data to file...\n");
	char outFileName[256];
	strcpy(outFileName, argv[1]);
	char *periodPos = strchr(outFileName, '.');
	if (periodPos > 0)
		strcpy(periodPos, ".h\0");

	FILE *fpPSG = fopen(outFileName, "wb");
	if (!fpPSG)
		{
		printf("Error opening output file!\n");
		exit(-1);
		}

	fprintf(fpPSG, "// JH PSG tune generated by MIDI2PSG\n");
	fprintf(fpPSG, "// Converted from file: %s\n", argv[1]);
	fprintf(fpPSG, "// NB: Each pattern represents one bar of one track (16 1/4 notes)\n");
	fprintf(fpPSG, "//     Each note in a pattern is 3 bytes - Instr index, volume (0-127), MIDI note number\n");
	fprintf(fpPSG, "static unsigned char tunePatternData[] = {\n");
	//fprintf(fpPSG, "\t// Pattern 0 (always empty)\n");
	//fprintf(fpPSG, "\t0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,\n");
	//fprintf(fpPSG, "\t0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,\n");
	//fprintf(fpPSG, "\t0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,\n");
	//fprintf(fpPSG, "\t0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,\n");
	//fprintf(fpPSG, "\n");

	// Write patterns data
	for (int i = 0; i < nextPatternIndex; i++)
		{
		if (0 == i)
			{
			fprintf(fpPSG, "\t// Pattern 0 (ALWAYS EMPTY!)\n\t");
			}
		else
			{
			int track = i / numBars;
			int bar = i % numBars;
			fprintf(fpPSG, "\t// Pattern %d (Track %d, Bar %d)\n\t", i, track, bar);
			}
		for (int j = 0; j < PATTERN_SIZE; j++)
			{
			int instr = patternData[i].notes[j].instrIndex;
			int vol = patternData[i].notes[j].volume;
			int note = patternData[i].notes[j].noteNumber;
			fprintf(fpPSG, "%d, %d, %d,", instr, vol, note);
			if ((j % 4) == 3)
				fprintf(fpPSG, "\n\t");
			else
				fprintf(fpPSG, "\t");
			}
		fprintf(fpPSG, "\n");
		}

	fprintf(fpPSG, "};\n\n");

	// Write bars data
	fprintf(fpPSG, "// Data for the bars\n");
	fprintf(fpPSG, "static unsigned char tuneBarData[] = {\n");
	for (int i = 0; i < numBars; i++)
		{
		//fprintf(fpPSG, "\t%d, %d, %d, %d,\n", i + 1, numBars + i + 1, 2 * numBars + i + 1, 3 * numBars + i + 1);
		fprintf(fpPSG, "\t%d, %d, %d, %d,\n", barData[i].patternIndex[0], 
																					barData[i].patternIndex[1], 
																					barData[i].patternIndex[2], 
																					barData[i].patternIndex[3]);
		}
	fprintf(fpPSG, "};\n")
		;

	fclose(fpPSG);

	return 0;
}

