#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "cpu.h"
#include <unistd.h>

#define MAX_INSTR 0x1000000

typedef struct
{
	unsigned short freq;
	unsigned short pulse;
	unsigned short adsr;
	unsigned char wave;
	int note;
} CHANNEL;

typedef struct
{
	unsigned short cutoff;
	unsigned char ctrl;
	unsigned char type;
} FILTER;

int main(int argc, char **argv);
unsigned char readbyte(FILE *f);
unsigned short readword(FILE *f);

CHANNEL chn[3];
CHANNEL prevchn[3];
CHANNEL prevchn2[3];
FILTER filt;
FILTER prevfilt;

#define FREQLO1 0x0
#define FREQHI1 0x1
#define PWLO1 0x2
#define PWHI1 0x3
#define CR1 0x4
#define AD1 0x5
#define SR1 0x6

#define FREQLO2 0x7
#define FREQHI2 0x8
#define PWLO2 0x9
#define PWHI2 0xa
#define CR2 0xB
#define AD2 0xC
#define SR2 0xD

#define FREQLO3 0xE
#define FREQHI3 0xF
#define PWLO3 0x10
#define PWHI3 0x11
#define CR3 0x12
#define AD3 0x13
#define SR3 0x14

#define FCLO 0x15
#define FCHI 0x16
#define RESFILT 0x17
#define MODEVOL 0x18

char *notename[] =
{"C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
	"C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
	"C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
	"C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
	"C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
	"C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
	"C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
	"C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "A-7", "A#7", "B-7"};

char *filtername[] =
{"Off", "Low", "Bnd", "L+B", "Hi ", "L+H", "B+H", "LBH"};

unsigned char freqtbllo[] = {
	0x17,0x27,0x39,0x4b,0x5f,0x74,0x8a,0xa1,0xba,0xd4,0xf0,0x0e,
	0x2d,0x4e,0x71,0x96,0xbe,0xe8,0x14,0x43,0x74,0xa9,0xe1,0x1c,
	0x5a,0x9c,0xe2,0x2d,0x7c,0xcf,0x28,0x85,0xe8,0x52,0xc1,0x37,
	0xb4,0x39,0xc5,0x5a,0xf7,0x9e,0x4f,0x0a,0xd1,0xa3,0x82,0x6e,
	0x68,0x71,0x8a,0xb3,0xee,0x3c,0x9e,0x15,0xa2,0x46,0x04,0xdc,
	0xd0,0xe2,0x14,0x67,0xdd,0x79,0x3c,0x29,0x44,0x8d,0x08,0xb8,
	0xa1,0xc5,0x28,0xcd,0xba,0xf1,0x78,0x53,0x87,0x1a,0x10,0x71,
	0x42,0x89,0x4f,0x9b,0x74,0xe2,0xf0,0xa6,0x0e,0x33,0x20,0xff};

unsigned char freqtblhi[] = {
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
	0x02,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x04,
	0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x08,
	0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
	0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1f,0x20,
	0x22,0x24,0x27,0x29,0x2b,0x2e,0x31,0x34,0x37,0x3a,0x3e,0x41,
	0x45,0x49,0x4e,0x52,0x57,0x5c,0x62,0x68,0x6e,0x75,0x7c,0x83,
	0x8b,0x93,0x9c,0xa5,0xaf,0xb9,0xc4,0xd0,0xdd,0xea,0xf8,0xff};

int main(int argc, char **argv)
{
	int subtune = 0;
	int seconds = 60;
	int instr = 0;
	int frames = 0;
	int spacing = 0;
	int pattspacing = 0;
	int firstframe = 0;
	int counter = 0;
	int basefreq = 0;
	int basenote = 0xb0;
	int lowres = 0;
	int rows = 0;
	int oldnotefactor = 1;
	int timeseconds = 0;
	int usage = 0;
	unsigned loadend;
	unsigned loadpos;
	unsigned loadsize;
	unsigned loadaddress;
	unsigned initaddress;
	unsigned playaddress;
	unsigned dataoffset;
	FILE *in;
	char *sidname = 0;
	int c;

	// Scan arguments
	for (c = 1; c < argc; c++)
	{
		if (argv[c][0] == '-')
		{
			switch(toupper(argv[c][1]))
			{
				case '?':
					usage = 1;
					break;

				case 'A':
					sscanf(&argv[c][2], "%u", &subtune);
					break;

				case 'C':
					sscanf(&argv[c][2], "%x", &basefreq);
					break;

				case 'D':
					sscanf(&argv[c][2], "%x", &basenote);
					break;

				case 'F':
					sscanf(&argv[c][2], "%u", &firstframe);
					break;

				case 'L':
					lowres = 1;
					break;

				case 'N':
					sscanf(&argv[c][2], "%u", &spacing);
					break;

				case 'O':
					sscanf(&argv[c][2], "%u", &oldnotefactor);
					if (oldnotefactor < 1) oldnotefactor = 1;
					break;

				case 'P':
					sscanf(&argv[c][2], "%u", &pattspacing);
					break;

				case 'S':
					timeseconds = 1;
					break;

				case 'T':
					sscanf(&argv[c][2], "%u", &seconds);
					break;
			}
		}
		else
		{
			if (!sidname) sidname = argv[c];
		}
	}

	// Usage display
	if ((argc < 2) || (usage))
	{
		printf("Usage: SIDDUMP <sidfile> [options]\n"
				"Warning: CPU emulation may be buggy/inaccurate, illegals support very limited\n\n"
				"Options:\n"
				"-a<value> Accumulator value on init (subtune number) default = 0\n"
				"-c<value> Frequency recalibration. Give note frequency in hex\n"
				"-d<value> Select calibration note (abs.notation 80-DF). Default middle-C (B0)\n"
				"-f<value> First frame to display, default 0\n"
				"-l        Low-resolution mode (only display 1 row per note)\n"
				"-n<value> Note spacing, default 0 (none)\n"
				"-o<value> ""Oldnote-sticky"" factor. Default 1, increase for better vibrato display\n"
				"          (when increased, requires well calibrated frequencies)\n"
				"-p<value> Pattern spacing, default 0 (none)\n"
				"-s        Display time in minutes:seconds:frame format\n"
				"-t<value> Playback time in seconds, default 60\n");
		return 1;
	}

	// Recalibrate frequencytable
	if (basefreq)
	{
		basenote &= 0x7f;
		if ((basenote < 0) || (basenote > 96))
		{
			printf("Warning: Calibration note out of range. Aborting recalibration.\n");
		}
		else
		{
			for (c = 0; c < 96; c++)
			{
				double note = c - basenote;
				double freq = (double)basefreq * pow(2.0, note/12.0);
				int f = freq;
				if (freq > 0xffff) freq = 0xffff;
				freqtbllo[c] = f & 0xff;
				freqtblhi[c] = f >> 8;
			}
		}
	}

	// Check other parameters for correctness
	if ((lowres) && (!spacing)) lowres = 0;

	// Open SID file
	if (!sidname)
	{
		printf("Error: no SID file specified.\n");
		return 1;
	}

	in = fopen(sidname, "rb");
	if (!in)
	{
		printf("Error: couldn't open SID file.\n");
		return 1;
	}

	// Read interesting parts of the SID header
	fseek(in, 6, SEEK_SET);
	dataoffset = readword(in);
	loadaddress = readword(in);
	initaddress = readword(in);
	playaddress = readword(in);
	fseek(in, dataoffset, SEEK_SET);
	if (loadaddress == 0)
		loadaddress = readbyte(in) | (readbyte(in) << 8);

	// Load the C64 data
	loadpos = ftell(in);
	fseek(in, 0, SEEK_END);
	loadend = ftell(in);
	fseek(in, loadpos, SEEK_SET);
	loadsize = loadend - loadpos;
	if (loadsize + loadaddress >= 0x10000)
	{
		printf("Error: SID data continues past end of C64 memory.\n");
		fclose(in);
		return 1;
	}
	fread(&mem[loadaddress], loadsize, 1, in);
	fclose(in);

	// Print info & run initroutine
	/*
	printf("Load address: $%04X Init address: $%04X Play address: $%04X\n", loadaddress, initaddress, playaddress);
	printf("Calling initroutine with subtune %d\n", subtune);
	*/
	initcpu(initaddress, subtune, 0, 0);
	instr = 0;
	while (runcpu())
	{
		instr++;
		if (instr > MAX_INSTR)
		{
			printf("Error: CPU executed abnormally high amount of instructions.\n");
			return 1;
		}
	}

	// Clear channelstructures in preparation & print first time info
	memset(&chn, 0, sizeof chn);
	memset(&filt, 0, sizeof filt);
	memset(&prevchn, 0, sizeof prevchn);
	memset(&prevchn2, 0, sizeof prevchn2);
	memset(&prevfilt, 0, sizeof prevfilt);

	// Data collection & display loop
	while(1)
	{
		int c;

		// Run the playroutine
		instr = 0;
		initcpu(playaddress, 0, 0, 0);
		while (runcpu())
		{
			instr++;
			if (instr > MAX_INSTR)
			{
				printf("Error: CPU executed abnormally high amount of instructions.\n");
				return 1;
			}
		}

		// Get SID parameters from each channel and the filter
		for (c = 0; c < 3; c++)
		{
			chn[c].freq = mem[0xd400 + 7*c] | (mem[0xd401 + 7*c] << 8);
			chn[c].pulse = (mem[0xd402 + 7*c] | (mem[0xd403 + 7*c] << 8)) & 0xfff;
			chn[c].wave = mem[0xd404 + 7*c];
			chn[c].adsr = mem[0xd406 + 7*c] | (mem[0xd405 + 7*c] << 8);
		}
		filt.cutoff = (mem[0xd415] << 5) | (mem[0xd416] << 8);
		filt.ctrl = mem[0xd417];
		filt.type = mem[0xd418];

		// Frame display
		if (frames >= firstframe)
		{
			size_t pos=0;
			unsigned char output[512];
			int time = frames - firstframe;
			output[0] = 0;

			/*
			if (!timeseconds)
				sprintf(&output[strlen(output)], "| %5d | ", time);
			else
				sprintf(&output[strlen(output)], "|%01d:%02d.%02d| ", time/3000, (time/50)%60, time%50);
			*/

			// Loop for each channel
			for (c = 0; c < 3; c++)
			{
				int newnote = 0;

				// Keyoff-keyon sequence detection
				if (chn[c].wave >= 0x10)
				{
					if ((chn[c].wave & 1) && ((!(prevchn2[c].wave & 1)) || (prevchn2[c].wave < 0x10)))
						prevchn[c].note = -1;
				}

				// Frequency
				if ((frames == firstframe) || (prevchn[c].note == -1) || (chn[c].freq != prevchn[c].freq))
				{
					int d;
					int dist = 0x7fffffff;
					int delta = ((int)chn[c].freq) - ((int)prevchn2[c].freq);

					output[pos++] = FREQLO1 + (c * 7);
					output[pos++] = chn[c].freq & 0xFF;
					output[pos++] = FREQHI1 + (c * 7);
					output[pos++] = chn[c].freq >> 8;

					if (chn[c].wave >= 0x10)
					{
						// Get new note number
						for (d = 0; d < 96; d++)
						{
							int cmpfreq = freqtbllo[d] | (freqtblhi[d] << 8);
							int freq = chn[c].freq;

							if (abs(freq - cmpfreq) < dist)
							{
								dist = abs(freq - cmpfreq);
								// Favor the old note
								if (d == prevchn[c].note) dist /= oldnotefactor;
								chn[c].note = d;
							}
						}

						// Print new note
						if (chn[c].note != prevchn[c].note)
						{
							if (prevchn[c].note == -1)
							{
								if (lowres) newnote = 1;
							}
						}
					}
					//else sprintf(&output[strlen(output)], " ... ..  ");
				}
				//else sprintf(&output[strlen(output)], "....  ... ..  ");

				// Waveform
				if ((frames == firstframe) || (newnote) || (chn[c].wave != prevchn[c].wave))
				{
					output[pos++] = CR1 + (c * 7);
					output[pos++] = chn[c].wave;
				}
				//else sprintf(&output[strlen(output)], ".. ");

				// ADSR
				if ((frames == firstframe) || (newnote) || (chn[c].adsr != prevchn[c].adsr))
				{
					output[pos++] = AD1 + (c * 7);
					output[pos++] = chn[c].adsr >> 8;
					output[pos++] = SR1 + (c * 7);
					output[pos++] = chn[c].adsr & 0xFF;
				}
				//else sprintf(&output[strlen(output)], ".... ");

				// Pulse
				if ((frames == firstframe) || (newnote) || (chn[c].pulse != prevchn[c].pulse))
				{
					output[pos++] = PWLO1 + (c * 7);
					output[pos++] = chn[c].pulse & 0xFF;
					output[pos++] = PWHI1 + (c * 7);
					output[pos++] = chn[c].pulse >> 8;
				}
				//else sprintf(&output[strlen(output)], "... ");

				//sprintf(&output[strlen(output)], "| ");
			}

			// Filter cutoff
			if ((frames == firstframe) || (filt.cutoff != prevfilt.cutoff))
			{
				output[pos++] = FCLO;
				output[pos++] = (filt.cutoff & 0x7);
				output[pos++] = FCHI;
				output[pos++] = ((filt.cutoff >> 8) & 0xFF);
			}
			//else sprintf(&output[strlen(output)], ".... ");

			// Filter control
			if ((frames == firstframe) || (filt.ctrl != prevfilt.ctrl))
			{
				output[pos++] = RESFILT;
				output[pos++] = filt.ctrl;
			}
			//else sprintf(&output[strlen(output)], ".. ");

			// Filter passband
			/*
			if ((frames == firstframe) || ((filt.type & 0x70) != (prevfilt.type & 0x70)))
			{
				output[pos++] = MODEVOL;
				output[pos++] = (filt.type & 0xF0) | (prevfilt.type & 0xF);
			}
			//else sprintf(&output[strlen(output)], "... ");

			// Mastervolume
			if ((frames == firstframe) || ((filt.type & 0xf) != (prevfilt.type & 0xf)))
			{
				output[pos++] = MODEVOL;
				output[pos++] = (prevfilt.type & 0xF0) | (filt.type & 0xF);
			}
			//else sprintf(&output[strlen(output)], ". ");
			*/
			if((frames == firstframe) || (filt.type != prevfilt.type))
			{
				output[pos++] = MODEVOL;
				output[pos++] = filt.type;
			}
			output[pos++]=0xFF; // End of frame

			prevfilt = filt;
			for (c = 0; c < 3; c++) prevchn2[c] = chn[c];
			write(STDOUT_FILENO, output, pos);
		}

		// Advance to next frame
		frames++;
	}
	return 0;
}

unsigned char readbyte(FILE *f)
{
	unsigned char res;

	fread(&res, 1, 1, f);
	return res;
}

unsigned short readword(FILE *f)
{
	unsigned char res[2];

	fread(&res, 2, 1, f);
	return (res[0] << 8) | res[1];
}




