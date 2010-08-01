/*
	fileio.c - (C) Pete Rittwage
	---
	contains routines used by nibtools to read/write files on the host
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "mnibarch.h"
#include "gcr.h"
#include "nibtools.h"
#include "crc.h"
#include "md5.h"

void parseargs(char *argv[])
{
	int count;

	// parse arguments
	switch ((*argv)[1])
	{
		case 'h':
			track_inc = 1;
			//start_track = 1;  /* my drive knocks on this track - PJR */
			end_track = 83;
			printf("* Using halftracks\n");
			break;

		case 'S':
			if (!(*argv)[2]) usage();
			start_track = (BYTE) (2 * (atoi(&(*argv)[2])));
			printf("* Start track set to %d\n", start_track/2);
			break;

		case 'E':
			if (!(*argv)[2]) usage();
			end_track = (BYTE) (2 * (atoi(&(*argv)[2])));
			printf("* End track set to %d\n", end_track/2);
			break;

		case 'u':
			mode = MODE_UNFORMAT_DISK;
			unformat_passes = atoi(&(*argv)[2]);
			if(!unformat_passes) unformat_passes = 1;
			printf("* Unformat passes = %d\n", unformat_passes);
			break;

		case 'R':
			// hidden secret raw track file writing mode
			printf("* Raw track dump write mode\n");
			mode = MODE_WRITE_RAW;
			break;

		case 'p':
			// custom protection handling
			printf("* Custom copy protection handler: ");
			switch((*argv)[2])
			{
				case 'x':
					printf("V-MAX!\n");
					memset(align_map, ALIGN_VMAX, MAX_TRACKS_1541+1);
					fix_gcr = 0;
					break;

				case 'c':
					printf("V-MAX! (CINEMAWARE)\n");
					memset(align_map, ALIGN_VMAX_CW, MAX_TRACKS_1541+1);
					fix_gcr = 0;
					break;

				case 'g':
				case 'm':
					printf("SecuriSpeed/Early Rainbow Arts\n"); /* turn off reduction for track > 36 */
					for(count = 36; count <= MAX_TRACKS_1541+1; count ++)
					{
						reduce_map[count] = REDUCE_NONE;
						align_map[count] = ALIGN_AUTOGAP;
					}
					fix_gcr = 0;
					break;

				case 'v':
					printf("VORPAL (NEWER)\n");
					memset(align_map, ALIGN_AUTOGAP, MAX_TRACKS_1541+1);
					align_map[18] = ALIGN_NONE;
					break;

				case'r':
					printf("RAPIDLOK\n"); /* don't reduce sync, but everything else */
					for(count = 1; count <= MAX_TRACKS_1541; count ++)
						reduce_map[count] = REDUCE_BAD | REDUCE_GAP;
					memset(align_map, ALIGN_SEC0, MAX_TRACKS_1541+1);
					break;

				default:
					printf("Unknown protection handler\n");
					break;
			}
			break;

		case 'a':
			// custom alignment handling
			printf("* Custom alignment: ");
			if ((*argv)[2] == '0')
			{
				printf("sector 0\n");
				memset(align_map, ALIGN_SEC0, MAX_TRACKS_1541+1);
			}
			else if ((*argv)[2] == 'g')
			{
				printf("gap\n");
				memset(align_map, ALIGN_GAP, MAX_TRACKS_1541+1);
			}
			else if ((*argv)[2] == 'w')
			{
				printf("longest bad GCR run\n");
				memset(align_map, ALIGN_BADGCR, MAX_TRACKS_1541+1);
			}
			else if ((*argv)[2] == 's')
			{
				printf("longest sync\n");
				memset(align_map, ALIGN_LONGSYNC, MAX_TRACKS_1541+1);
			}
			else if ((*argv)[2] == 'a')
			{
				printf("autogap\n");
				memset(align_map, ALIGN_AUTOGAP, MAX_TRACKS_1541+1);
			}
			else if ((*argv)[2] == 'n')
			{
				printf("raw (no alignment, use NIB start)\n");
				memset(align_map, ALIGN_RAW, MAX_TRACKS_1541+1);
			}
			else
				printf("Unknown alignment parameter\n");
			break;

		case 'r':
			reduce_sync = atoi(&(*argv)[2]);
			if(reduce_sync)
			{
				printf("* Reduce sync to %d bytes\n", reduce_sync);
			}
			else
			{
				printf("* Disabled sync reduction\n");
				for(count = 1; count <= MAX_TRACKS_1541; count ++)
				reduce_map[count] &= ~REDUCE_SYNC;
			}
			break;

		case '0':
			printf("* Reduce bad GCR enabled\n");
			for(count = 1; count <= MAX_TRACKS_1541; count ++)
				reduce_map[count] |= REDUCE_BAD;
			break;

		case 'g':
			printf("* Reduce gaps enabled\n");
			for(count = 1; count <= MAX_TRACKS_1541; count ++)
				reduce_map[count] |= REDUCE_GAP;
			break;

		case 'D':
			if (!(*argv)[2]) usage();
			drive = (BYTE) atoi(&(*argv)[2]);
			printf("* Use Device %d\n", drive);
			break;

		case 'G':
			if (!(*argv)[2]) usage();
			gap_match_length = atoi(&(*argv)[2]);
			printf("* Gap match length set to %d\n", gap_match_length);
			break;

		case 'f':
			if (!(*argv)[2])
			{
				fix_gcr = 0;
				printf("* Disabled bad GCR bit reproduction\n");
			}
			else
			{
				fix_gcr = atoi(&(*argv)[2]);
				printf("* Enabled level %d bad GCR reproduction.\n", fix_gcr);
			}
			break;

		case 'v':
			verbose = 1;
			printf("* Verbose mode on\n");
			break;

		case 'm':
			auto_capacity_adjust = 0;
			printf("* Disabled automatic capacity adjustment\n");
			break;

		case 'c':
			printf("* Minimum capacity ignore on\n");
			cap_min_ignore = 1;
			break;

		case 's':
			if (!(*argv)[2]) usage();
			if(!ihs) align_disk = 1;
			skew = atoi(&(*argv)[2]);
			if((skew > 200) || (skew < 0))
			{
				printf("Skew must be between 1 and 200ms\n");
				skew = 0;
			}
			printf("* Skew set to %dms\n",skew);
			break;

		case 't':
			if(!ihs) align_disk = 1;
			printf("* Attempt soft track alignment\n");
			break;

		case 'i':
			printf("* 1571 index hole sensor (use only for side 1)\n");
			align_disk = 0;
			ihs = 1;
			break;

		case 'C':
			rpm_real = atoi(&(*argv)[2]);
			printf("* Simulate track capacity: %dRPM\n",rpm_real);
			break;

		case 'b':
			// custom fillbyte
			printf("* Custom fillbyte: ");
			if ((*argv)[2] == '0')
			{
				printf("$00\n");
				fillbyte = 0x00;
			}
			if ((*argv)[2] == '5')
			{
				printf("$55\n");
				fillbyte = 0x55;
			}
			if ((*argv)[2] == 'f')
			{
				printf("$ff\n");
				fillbyte = 0xff;
			}
			if ((*argv)[2] == '?')
			{
				printf("loop last byte in track\n");
				fillbyte = 0xfe;
			}
			break;

		default:
			usage();
			break;
	}
}


int read_nib(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	int track, nibsize, numtracks, temp_track_inc;
	int header_entry = 0;
	char header[0x100];
	FILE *fpin;

	printf("\nReading NIB file...");

	if ((fpin = fopen(filename, "rb")) == NULL)
	{
		fprintf(stderr, "Couldn't open input file %s!\n", filename);
		return 0;
	}

	if (fread(header, sizeof(header), 1, fpin) != 1) {
		printf("unable to read NIB header\n");
		return 0;
	}

	if (memcmp(header, "MNIB-1541-RAW", 13) != 0)
	{
		printf("input file %s isn't an NIB data file!\n", filename);
		return 0;
	}

	/* Determine number of tracks in image (estimated by filesize) */
	fseek(fpin, 0, SEEK_END);
	nibsize = ftell(fpin);
	numtracks = (nibsize - NIB_HEADER_SIZE) / NIB_TRACK_LENGTH;

	if(numtracks <= 42)
	{
		if(numtracks * 2 < end_track)
			end_track = (numtracks * 2);

		temp_track_inc = 2;
	}
	else
	{
		printf("\nImage contains halftracks!\n");

		if(numtracks < end_track)
			end_track = numtracks;

		temp_track_inc = 1;
	}

	printf("\n%d track image (filesize = %d bytes)\n", numtracks, nibsize);

	rewind(fpin);
	if (fread(header, sizeof(header), 1, fpin) != 1) {
		printf("unable to read NIB header\n");
		return 0;
	}

	for (track = 2; track <= end_track; track += temp_track_inc)
	{
		/* get density from header or use default */
		track_density[track] = (BYTE)(header[0x10 + (header_entry * 2) + 1]);
		track_density[track] %= BM_MATCH;  	 /* discard unused BM_MATCH mark */
		header_entry++;

		/* get track from file */
		fread(track_buffer + (track * NIB_TRACK_LENGTH), NIB_TRACK_LENGTH, 1, fpin);
	}
	fclose(fpin);
	printf("Successfully loaded NIB file\n");
	return 1;
}

int read_nb2(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	int track, pass_density, pass, nibsize, temp_track_inc, numtracks;
	int header_entry = 0;
	char header[0x100];
	BYTE nibdata[0x2000];
	BYTE tmpdata[0x2000];
	BYTE diskid[2], dummy;
	FILE *fpin;
	size_t errors, best_err, best_pass;
	size_t length, best_len;
	char errorstring[0x1000];

	printf("\nReading NB2 file...");

	temp_track_inc = 1;  /* all nb2 files contain halftracks */

	if ((fpin = fopen(filename, "rb")) == NULL)
	{
		fprintf(stderr, "Couldn't open input file %s!\n", filename);
		return 0;
	}

	if (fread(header, sizeof(header), 1, fpin) != 1)
	{
		printf("unable to read NIB header\n");
		return 0;
	}

	if (memcmp(header, "MNIB-1541-RAW", 13) != 0)
	{
		printf("input file %s isn't an NB2 data file!\n", filename);
		return 0;
	}

	/* Determine number of tracks in image (estimated by filesize) */
	fseek(fpin, 0, SEEK_END);
	nibsize = ftell(fpin);
	numtracks = (nibsize - NIB_HEADER_SIZE) / (NIB_TRACK_LENGTH * 16);

	if(numtracks <= 42)
	{
		if(numtracks * 2 < end_track)
			end_track = (numtracks * 2);

		temp_track_inc = 2;
	}
	else
	{
		printf("\nImage contains halftracks!\n");

		if(numtracks < end_track)
			end_track = numtracks;

		temp_track_inc = 1;
	}
	printf("\n%d track image (filesize = %d bytes)\n", numtracks, nibsize);

	/* get disk id */
	rewind(fpin);
	if(temp_track_inc == 2)
		fseek(fpin, sizeof(header) + (17 * NIB_TRACK_LENGTH * 16) + (8 * NIB_TRACK_LENGTH), SEEK_SET);
	else
		fseek(fpin, sizeof(header) + (17 * 2 * NIB_TRACK_LENGTH * 16) + (8 * NIB_TRACK_LENGTH), SEEK_SET);

	fread(tmpdata, NIB_TRACK_LENGTH, 1, fpin);

	if (!extract_id(tmpdata, diskid))
	{
			fprintf(stderr, "Cannot find directory sector.\n");
			return 0;
	}
	printf("\ndiskid: %c%c\n", diskid[0], diskid[1]);

	rewind(fpin);
	if (fread(header, sizeof(header), 1, fpin) != 1) {
		printf("unable to read NB2 header\n");
		return 0;
	}

	for (track = 2; track <= end_track; track += temp_track_inc)
	{
		/* get density from header or use default */
		track_density[track] = (BYTE)(header[0x10 + (header_entry * 2) + 1]);
		header_entry++;

		best_pass = 0;
		best_err = 0;
		best_len = 0;  /* unused for now */

		printf("\n%4.1f:",(float) track / 2);

		/* contains 16 passes of track, four for each density */
		for(pass_density = 0; pass_density < 4; pass_density ++)
		{
			printf(" (%d)", pass_density);

			for(pass = 0; pass <= 3; pass ++)
			{
				/* get track from file */
				if( (pass_density == track_density[track]) )
				{
					fread(nibdata, NIB_TRACK_LENGTH, 1, fpin);

					length = extract_GCR_track(tmpdata, nibdata,
						&dummy,
						track/2,
						capacity_min[track_density[track]&3],
						capacity_max[track_density[track]&3]);

					errors = check_errors(tmpdata, length, track, diskid, errorstring);

					if( (pass == 0) || (errors < best_err) )
					{
						//track_length[track] = 0x2000;
						memcpy(track_buffer + (track * NIB_TRACK_LENGTH), nibdata, NIB_TRACK_LENGTH);
						best_pass = pass;
						best_err = errors;
					}
				}
				else
					fread(tmpdata, NIB_TRACK_LENGTH, 1, fpin);
			}
		}

		/* output some specs */
		printf(" (");
		if(track_density[track] & BM_NO_SYNC) printf("NOSYNC!");
		if(track_density[track] & BM_FF_TRACK) printf("KILLER!");

		printf("%d:%d) (pass %d, %d errors) %.1f%%", track_density[track]&3, track_length[track], best_pass, best_err,
			((float)track_length[track] / (float)capacity[track_density[track]&3]) * 100);

	}
	fclose(fpin);
	printf("\nSuccessfully loaded NB2 file\n");
	return 1;
}

int read_g64(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	int track, g64maxtrack, temp_track_inc;
	int dens_pointer = 0;
	int g64tracks, g64size, numtracks;
	BYTE header[0x2ac];
	BYTE length_record[2];
	FILE *fpin;

	printf("\nReading G64 file...");

	if ((fpin = fopen(filename, "rb")) == NULL)
	{
		fprintf(stderr, "Couldn't open input file %s!\n", filename);
		return 0;
	}

	if (fread(header, sizeof(header), 1, fpin) != 1) {
		printf("unable to read G64 header\n");
		return 0;
	}

	if (memcmp(header, "GCR-1541", 8) != 0)
	{
		printf("input file %s isn't an G64 data file!\n", filename);
		return 0;
	}

	g64tracks = (char) header[0x9];
	g64maxtrack = (BYTE)header[0xb] << 8 | (BYTE)header[0xa];

	/* Determine number of tracks in image (estimated by filesize) */
	fseek(fpin, 0, SEEK_END);
	g64size = ftell(fpin);
	numtracks = (g64size - sizeof(header)) / (g64maxtrack + 2);

	if(numtracks <= 42)
	{
		if(numtracks * 2 < end_track)
			end_track = (numtracks * 2);

		temp_track_inc = 2;
	}
	else
	{
		printf("\nImage contains halftracks!\n");

		if(numtracks < end_track)
			end_track = numtracks;

		temp_track_inc = 1;
	}

	rewind(fpin);
	if (fread(header, sizeof(header), 1, fpin) != 1) {
		printf("unable to read G64 header\n");
		return 0;
	}

	printf("\nG64: %d total bytes = %d tracks of %d bytes each\n", g64size, numtracks, g64maxtrack);

	for (track = 2; track <= end_track; track += temp_track_inc)
	{
		/* get density from header */
		track_density[track] = header[0x9 + 0x153 + dens_pointer];
		dens_pointer += (4 * temp_track_inc);

		/* get length */
		fread(length_record, 2, 1, fpin);

		track_length[track] = length_record[1] << 8 | length_record[0];

		/* get track from file */
		fread(track_buffer + (track * NIB_TRACK_LENGTH), g64maxtrack, 1, fpin);

		/* output some specs */
		if(verbose)
		{
			printf("%4.1f: (",(float) track / 2);
			if(track_density[track] & BM_NO_SYNC) printf("NOSYNC!");
			if(track_density[track] & BM_FF_TRACK) printf("KILLER!");

			printf("%d:%d) %.1f%%\n", track_density[track]&3, track_length[track],
				((float)track_length[track] / (float)capacity[track_density[track]&3]) * 100);
		}
	}
	fclose(fpin);
	printf("\nSuccessfully loaded G64 file\n");
	return 1;
}

int
read_d64(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	int track, sector, sector_ref;
	BYTE buffer[256];
	BYTE gcrdata[NIB_TRACK_LENGTH];
	BYTE errorinfo[MAXBLOCKSONDISK];
	BYTE id[3] = { 0, 0, 0 };
	int error, d64size, last_track;
	char errorstring[0x1000], tmpstr[8];
	FILE *fpin;

	printf("\nReading D64 file...");

	if ((fpin = fopen(filename, "rb")) == NULL)
	{
		fprintf(stderr, "Couldn't open input file %s!\n", filename);
		return 0;
	}

	/* here we get to rebuild tracks from scratch */
	memset(errorinfo, SECTOR_OK, sizeof(errorinfo));

	/* determine d64 image size */
	fseek(fpin, 0, SEEK_END);
	d64size = ftell(fpin);

	switch (d64size)
	{
	case (BLOCKSONDISK * 257):		/* 35 track image with errorinfo */
		fseek(fpin, BLOCKSONDISK * 256, SEEK_SET);
		fread(errorinfo, BLOCKSONDISK, 1, fpin); // @@@SRT: check success
		/* FALLTHROUGH */
	case (BLOCKSONDISK * 256):		/* 35 track image w/o errorinfo */
		last_track = 35;
		break;

	case (MAXBLOCKSONDISK * 257):	/* 40 track image with errorinfo */
		fseek(fpin, MAXBLOCKSONDISK * 256, SEEK_SET);
		fread(errorinfo, MAXBLOCKSONDISK, 1, fpin); // @@@SRT: check success
		/* FALLTHROUGH */
	case (MAXBLOCKSONDISK * 256):	/* 40 track image w/o errorinfo */
		last_track = 40;
		break;

	default:
		rewind(fpin);
		fprintf(stderr, "Bad d64 image size.\n");
		return 0;
	}

	// determine disk id from track 18 (offsets $165A2, $165A3)
	fseek(fpin, 0x165a2, SEEK_SET);
	fread(id, 2, 1, fpin); // @@@SRT: check success
	rewind(fpin);

	sector_ref = 0;
	for (track = 1; track <= last_track; track++)
	{
		// clear buffers
		memset(gcrdata, 0x55, sizeof(gcrdata));
		errorstring[0] = '\0';

		for (sector = 0; sector < sector_map[track]; sector++)
		{
			// get error and increase reference pointer in errorinfo
			error = errorinfo[sector_ref++];

			if (error != SECTOR_OK)
			{
				sprintf(tmpstr, " E%dS%d", error, sector);
				strcat(errorstring, tmpstr);
			}

			// read sector from file
			fread(buffer, 256, 1, fpin); // @@@SRT: check success

			// convert to gcr
			convert_sector_to_GCR(buffer,
			  gcrdata + (sector * SECTOR_SIZE), track, sector, id, error);
		}

		// calculate track length
		track_length[track*2] = sector_map[track] * SECTOR_SIZE;

		// use default densities for D64
		track_density[track*2] = speed_map[track];

		// write track
		memcpy(track_buffer + (track * 2 * NIB_TRACK_LENGTH), gcrdata, track_length[track*2]);
		//printf("%s", errorstring);
	}

	// "unformat" last 5 tracks on 35 track disk
	if (last_track == 35)
	{
		for (track = 36 * 2; track <= end_track; track += 2)
		{
			memset(track_buffer + (track * NIB_TRACK_LENGTH), 0, NIB_TRACK_LENGTH);
			track_density[track] = (2 | BM_NO_SYNC);
			track_length[track] = 0;
		}
	}

	fclose(fpin);
	printf("\nSuccessfully loaded D64 file\n");
	return 1;
}

int write_nib(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
    /*	writes contents of buffers into NIB file, with header and density information
    	this is only called by nibread, so it does not extract/compress the track
    */

	int track;
	FILE * fpout;
	char header[0x100];
	int header_entry = 0;

	printf("\nWriting NIB file...\n");

	/* clear header */
	memset(header, 0, sizeof(header));

	/* create output file */
	if ((fpout = fopen(filename, "wb")) == NULL)
	{
		fprintf(stderr, "Couldn't create output file %s!\n", filename);
		return 0;
	}

	/* header now contains whether halftracks were read */
	if(track_inc == 1)
		sprintf(header, "MNIB-1541-RAW%c%c%c", 3, 0, 1);
	else
		sprintf(header, "MNIB-1541-RAW%c%c%c", 3, 0, 0);

	if (fwrite(header, sizeof(header), 1, fpout) != 1) {
		printf("unable to write NIB header\n");
		return 0;
	}

	for (track = start_track; track <= end_track; track += track_inc)
	{
		header[0x10 + (header_entry * 2)] = (BYTE)track;
		header[0x10 + (header_entry * 2) + 1] = track_density[track];
		header_entry++;

		/* process and save track to disk */
		if (fwrite(track_buffer + (NIB_TRACK_LENGTH * track), NIB_TRACK_LENGTH , 1, fpout) != 1)
		{
			printf("unable to rewrite NIB track data\n");
			fclose(fpout);
			return 0;
		}
		fflush(fpout);

		/* output some specs */
		if(verbose)
		{
			printf("%4.1f: (",(float) track / 2);
			if(track_density[track] & BM_NO_SYNC) printf("NOSYNC!");
			if(track_density[track] & BM_FF_TRACK) printf("KILLER!");
			printf("%d:%d)\n", track_density[track]&3, track_length[track]  );
		}
	}

	/* fill NIB-header */
	rewind(fpout);

	if (fwrite(header, sizeof(header), 1, fpout) != 1) {
		printf("unable to rewrite NIB header\n");
		return 0;
	}

	fclose(fpout);
	printf("Successfully saved NIB file\n");
	return 1;
}


int
write_d64(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
    /*	writes contents of buffers into D64 file, with errorblock information (if detected) */

	FILE *fpout;
	int track, sector;
	int save_errorinfo = 0;
	int save_40_errors = 0;
	int save_40_tracks = 0;
	int blockindex = 0;
	BYTE *cycle_start;	/* start position of cycle    */
	BYTE *cycle_stop;	/* stop  position of cycle +1 */
	BYTE id[3];
	BYTE rawdata[260];
	BYTE d64data[MAXBLOCKSONDISK * 256], *d64ptr;
	BYTE errorinfo[MAXBLOCKSONDISK], errorcode;
	int blocks_to_save;

	printf("\nWriting D64 file...\n");

	memset(errorinfo, 0,sizeof(errorinfo));
	memset(rawdata, 0,sizeof(rawdata));
	memset(d64data, 0,sizeof(d64data));

	/* create output file */
	if ((fpout = fopen(filename, "wb")) == NULL)
	{
		fprintf(stderr, "Couldn't create output file %s!\n", filename);
		exit(2);
	}

	/* get disk id */
	if (!extract_id(track_buffer + (18 * 2 * NIB_TRACK_LENGTH), id))
	{
		fprintf(stderr, "Cannot find directory sector.\n");
		return 0;
	}

	d64ptr = d64data;
	for (track = start_track; track <= 40*2; track += 2)
	{
		cycle_start = track_buffer + (track * NIB_TRACK_LENGTH);
		cycle_stop = track_buffer + (track * NIB_TRACK_LENGTH) + track_length[track];

		printf("%.2d (%d):" ,track/2, capacity[speed_map[track/2]]);

		for (sector = 0; sector < sector_map[track/2]; sector++)
		{
			printf("%d", sector);

			memset(rawdata, 0,sizeof(rawdata));
			errorcode = convert_GCR_sector(cycle_start, cycle_stop, rawdata, track/2, sector, id);
			errorinfo[blockindex] = errorcode;	/* OK by default */

			if (errorcode != SECTOR_OK)
			{
				if (track/2 <= 35)
					save_errorinfo = 1;
				else
					save_40_errors = 1;
			}
			else if (track/2 > 35)
			{
				save_40_tracks = 1;
			}

			/* screen information */
			if (errorcode == SECTOR_OK)
				printf(" ");
			else
				printf("%.1x", errorcode);

			/* dump to buffer */
			memcpy(d64ptr, rawdata+1 , 256);
			d64ptr += 256;

			blockindex++;
		}
		printf("\n");
	}

	blocks_to_save = (save_40_tracks) ? MAXBLOCKSONDISK : BLOCKSONDISK;

	if (fwrite(d64data, blocks_to_save * 256, 1, fpout) != 1)
	{
		fprintf(stderr, "Cannot write d64 data.\n");
		return 0;
	}

	if (save_errorinfo == 1)
	{
		assert(sizeof(errorinfo) >= (size_t)blocks_to_save);
		if (fwrite(errorinfo, blocks_to_save, 1, fpout) != 1)
		{
			fprintf(stderr, "Cannot write sector data.\n");
			return 0;
		}
	}

	fclose(fpout);
	printf("Successfully saved D64 file\n");
	return 1;
}


int
write_g64(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	/* writes contents of buffers into G64 file, with header and density information */

	BYTE header[12];
	DWORD gcr_track_p[MAX_HALFTRACKS_1541];
	DWORD gcr_speed_p[MAX_HALFTRACKS_1541];
	BYTE gcr_track[G64_TRACK_MAXLEN + 2];
	size_t track_len;
	int track, index, badgcr;
	FILE * fpout;
	BYTE buffer[NIB_TRACK_LENGTH], tempfillbyte;

	printf("\nWriting G64 file...");

	/* when writing a G64 file, we don't care about the limitations of drive hardware
		However, VICE (as of 2.2) currently ignores G64 header and hardcodes 7928 as the largest
		track size, and also requires it to be 84 tracks no matter if they're used or not.
	*/

	fpout = fopen(filename, "wb");
	if (fpout == NULL)
	{
		printf("Cannot open G64 image %s.\n", filename);
		return 0;
	}

	/* Create G64 header */
	strcpy((char *) header, "GCR-1541");
	header[8] = 0;	/* G64 version */
	header[9] = MAX_HALFTRACKS_1541; // end_track;	/* Number of Halftracks  (VICE can't handle non-84 track images) */
	header[10] = (BYTE) (G64_TRACK_MAXLEN % 256);	/* Size of each stored track */
	header[11] = (BYTE) (G64_TRACK_MAXLEN / 256);

	if (fwrite(header, sizeof(header), 1, fpout) != 1)
	{
		printf("Cannot write G64 header.\n");
		return 0;
	}

	/* Create index and speed tables */
	for (index= 0; index < MAX_HALFTRACKS_1541; index += track_inc)
	{
		/* calculate track positions and speed zone data */
		if(track_inc == 2)
		{
			gcr_track_p[index] = 12 + (MAX_TRACKS_1541 * 16) + ((index/2) * (G64_TRACK_MAXLEN + 2));
			gcr_track_p[index+1] = 0;	/* no halftracks */
			gcr_speed_p[index] = track_density[index+2] & 3;
			gcr_speed_p[index+1] = 0;
		}
		else
		{
			gcr_track_p[index] = 12 + (MAX_TRACKS_1541 * 16) + (index * (G64_TRACK_MAXLEN + 2));
			gcr_speed_p[index] = track_density[index+2] & 3;
		}

	}

	/* write headers */
	if (write_dword(fpout, gcr_track_p, sizeof(gcr_track_p)) < 0)
	{
		printf("Cannot write track header.\n");
		return 0;
	}
	if (write_dword(fpout, gcr_speed_p, sizeof(gcr_speed_p)) < 0)
	{
		printf("Cannot write speed header.\n");
		return 0;
	}

	/* shuffle raw GCR between formats */
	for (track = 2; track <= MAX_HALFTRACKS_1541; track += track_inc)
	{
		size_t raw_track_size[4] = { 6250, 6666, 7142, 7692 };

		/* loop last byte of track data for filler */
		if(fillbyte == 0xfe) /* $fe is special case for loop */
			tempfillbyte = track_buffer[(track * NIB_TRACK_LENGTH) + track_length[track] - 1];
		else
			tempfillbyte = fillbyte;

		memset(&gcr_track[2], tempfillbyte, G64_TRACK_MAXLEN);

		gcr_track[0] = (BYTE) (raw_track_size[speed_map[track/2]] % 256);
		gcr_track[1] = (BYTE) (raw_track_size[speed_map[track/2]] / 256);

		memcpy(buffer, track_buffer + (track * NIB_TRACK_LENGTH), track_length[track]);
		track_len = track_length[track];

		if(track_len)
		{
			/* process/compress GCR data */
			badgcr = check_bad_gcr(buffer, track_length[track]);

			if(rpm_real)
			{
				//capacity[speed_map[track/2]] = raw_track_size[speed_map[track/2]];
				switch (track_density[track])
				{
					case 0:
						capacity[speed_map[track/2]] = DENSITY0/rpm_real;
						break;
					case 1:
						capacity[speed_map[track/2]] = DENSITY1/rpm_real;
						break;
					case 2:
						capacity[speed_map[track/2]] = DENSITY2/rpm_real;
						break;
					case 3:
						capacity[speed_map[track/2]] = DENSITY3/rpm_real;
						break;
				}

				if(capacity[speed_map[track/2]] > G64_TRACK_MAXLEN)
					capacity[speed_map[track/2]] = G64_TRACK_MAXLEN;

				track_len = compress_halftrack(track, buffer, track_density[track], track_length[track]);
			}
			else
			{
				capacity[speed_map[track/2]] = G64_TRACK_MAXLEN;
				track_len = compress_halftrack(track, buffer, track_density[track], track_length[track]);
			}
			printf("(fill:$%.2x) ",tempfillbyte);
			printf("{badgcr:%d}",badgcr);
		}
		else
		{
				/* track doesn't exist: write unformatted track */
				track_len = compress_halftrack(track, buffer, track_density[track], track_length[track]);
				track_len = raw_track_size[speed_map[track/2]];
				memset(buffer, 0, track_len);
		}

		gcr_track[0] = (BYTE) (track_len % 256);
		gcr_track[1] = (BYTE) (track_len / 256);

		// copy back our realigned track
		memcpy(gcr_track+2, buffer, track_len);

		if (fwrite(gcr_track, (G64_TRACK_MAXLEN + 2), 1, fpout) != 1)
		{
			printf("Cannot write track data.\n");
			return 0;
		}
	}
	fclose(fpout);
	printf("\nSuccessfully saved G64 file\n");
	return 1;
}

size_t
compress_halftrack(int halftrack, BYTE *track_buffer, BYTE density, size_t length)
{
	size_t orglen;
	BYTE gcrdata[NIB_TRACK_LENGTH];

	/* copy to spare buffer */
	memcpy(gcrdata, track_buffer, NIB_TRACK_LENGTH);
	memset(track_buffer, 0, NIB_TRACK_LENGTH);

	/* user display */
	printf("\n%4.1f: (", (float) halftrack / 2);
	printf("%d", density & 3);
	if ( (density&3) != speed_map[halftrack/2]) printf("!");
	printf(":%d) ", length);
	if (density & BM_NO_SYNC) printf("NOSYNC ");
	if (density & BM_FF_TRACK) printf("KILLER ");
	//printf("{%x} ", reduce_map[halftrack/2]);
	printf("[");

	/* process and compress track data (if needed) */
	if (length > 0)
	{
		/* If our track contains sync, we reduce to a minimum of 32 bits
		   less is too short for some loaders including CBM, but only 10 bits are technically required */
		orglen = length;
		if ( (length > (capacity[density & 3])) && (!(density & BM_NO_SYNC)) &&
			(reduce_map[halftrack/2] & REDUCE_SYNC) )
		{
			/* reduce sync marks within the track */
			length = reduce_runs(gcrdata, length, capacity[density & 3], reduce_sync, 0xff);

			if (length < orglen)
				printf("rsync:%d ", orglen - length);
		}

		/* reduce bad GCR runs */
		orglen = length;
		if ( (length > (capacity[density & 3])) &&
			(reduce_map[halftrack/2] & REDUCE_BAD) )
		{
			length = reduce_runs(gcrdata, length, capacity[density & 3], 0, 0x00);
			printf("rbadgcr:%d ", orglen - length);
		}

		/* reduce sector gaps -  they occur at the end of every sector and vary from 4-19 bytes, typically  */
		orglen = length;
		if ( (length > (capacity[density & 3])) &&
			(reduce_map[halftrack/2] & REDUCE_GAP) )
		{
			length = reduce_gaps(gcrdata, length, capacity[density & 3]);
			printf("rgaps:%d ", orglen - length);
		}

		/* still not small enough, we have to truncate the end (reduce tail) */
		orglen = length;
		if (length > capacity[density & 3])
		{
			length = capacity[density & 3];
			printf("trunc:%d ", orglen - length);
		}
	}

	/* if track is empty (unformatted) fill with '0' bytes to simulate */
	if ( (!length) && (density & BM_NO_SYNC))
	{
		memset(gcrdata, 0, NIB_TRACK_LENGTH);
		length = NIB_TRACK_LENGTH;
	}

	printf("] (%d) ", length);

	/* write processed track buffer */
	memcpy(track_buffer, gcrdata, length);
	return length;
}

int align_tracks(BYTE *track_buffer, BYTE *track_density, size_t *track_length, BYTE *track_alignment)
{
	int track;
	BYTE nibdata[NIB_TRACK_LENGTH];

	memset(nibdata, 0, sizeof(nibdata));

	printf("\nAligning tracks...\n");

	for (track = start_track; track <= end_track; track += track_inc)
	{
		memcpy(nibdata,  track_buffer + (track * NIB_TRACK_LENGTH), NIB_TRACK_LENGTH);
		memset(track_buffer + (track * NIB_TRACK_LENGTH), 0x00, NIB_TRACK_LENGTH);

		printf("%4.1f: ",(float) track / 2);

		/* process track cycle */
		track_length[track] = extract_GCR_track(
			track_buffer + (track * NIB_TRACK_LENGTH),
			nibdata,
			&track_alignment[track],
			track/2,
			capacity_min[track_density[track]&3],
			capacity_max[track_density[track]&3]
		);

		/* output some specs */
		if(track_density[track] & BM_NO_SYNC) printf("NOSYNC:");
		if(track_density[track] & BM_FF_TRACK) printf("KILLER:");
		printf("(%d:", track_density[track]&3);
		printf("%d) ", track_length[track]);
		printf("[align=%s]\n",alignments[track_alignment[track]]);
	}
	return 1;
}

int
compare_extension(char * filename, char * extension)
{
	char *dot;

	dot = strrchr(filename, '.');
	if (dot == NULL)
		return (0);

	for (++dot; *dot != '\0'; dot++, extension++)
		if (tolower(*dot) != tolower(*extension))
			return (0);

	if (*extension == '\0')
		return (1);
	else
		return (0);
}

int
write_dword(FILE *fd, DWORD * buf, int num)
{
	int i;
	BYTE *tmpbuf;

	tmpbuf = malloc(num);

	for (i = 0; i < (num / 4); i++)
	{
		tmpbuf[i * 4] = buf[i] & 0xff;
		tmpbuf[i * 4 + 1] = (buf[i] >> 8) & 0xff;
		tmpbuf[i * 4 + 2] = (buf[i] >> 16) & 0xff;
		tmpbuf[i * 4 + 3] = (buf[i] >> 24) & 0xff;
	}

	if (fwrite(tmpbuf, num, 1, fd) < 1)
	{
		free(tmpbuf);
		return -1;
	}
	free(tmpbuf);
	return 0;
}

unsigned int crc_dir_track(BYTE *track_buffer, size_t *track_length)
{
	/* this calculates a CRC32 for the BAM and first directory sector, which is sufficient to differentiate most disks */

	unsigned char data[512];
	unsigned int result;
	BYTE id[3];
	BYTE rawdata[260];
	BYTE errorcode;

	crcInit();

	/* get disk id */
	if (!extract_id(track_buffer + (18 * 2 * NIB_TRACK_LENGTH), id))
	{
		fprintf(stderr, "Cannot find directory sector.\n");
		return 0;
	}

	memset(data, 0, sizeof(data));

	/* t18s0 */
	memset(rawdata, 0, sizeof(rawdata));
	errorcode = convert_GCR_sector(
		track_buffer + ((18*2) * NIB_TRACK_LENGTH),
		track_buffer + ((18*2) * NIB_TRACK_LENGTH) + track_length[18*2],
		rawdata, 18, 0, id);
	memcpy(data, rawdata+1 , 256);

	/* t18s1 */
	memset(rawdata, 0, sizeof(rawdata));
	errorcode = convert_GCR_sector(
		track_buffer + ((18*2) * NIB_TRACK_LENGTH),
		track_buffer + ((18*2) * NIB_TRACK_LENGTH) + track_length[18*2],
		rawdata, 18, 1, id);
	memcpy(data+256, rawdata+1, 256);

	result = crcFast(data, sizeof(data));
	return result;
}

unsigned int crc_all_tracks(BYTE *track_buffer, size_t *track_length)
{
	/* this calculates a CRC32 for all sectors on the disk */

	unsigned char data[BLOCKSONDISK * 256];
	unsigned int result;
	int track, sector, index, valid;
	BYTE id[3];
	BYTE rawdata[260];
	BYTE errorcode;

	memset(data, 0, sizeof(data));
	crcInit();

	/* get disk id */
	if (!extract_id(track_buffer + (18*2 * NIB_TRACK_LENGTH), id))
	{
		fprintf(stderr, "Cannot find directory sector.\n");
		return 0;
	}

	index = valid = 0;
	for (track = start_track; track <= 35*2; track += 2)
	{
		for (sector = 0; sector < sector_map[track/2]; sector++)
		{
			memset(rawdata, 0, sizeof(rawdata));

			errorcode = convert_GCR_sector(
				track_buffer + (track * NIB_TRACK_LENGTH),
				track_buffer + (track * NIB_TRACK_LENGTH) + track_length[track],
				rawdata, track/2, sector, id);

			memcpy(data+(index*256), rawdata+1, 256);
			index++;

			if(errorcode == SECTOR_OK)
				valid++;
		}
	}

	if(index != valid)
		printf("[%d/%d sectors] ", valid, index);

	result = crcFast(data, sizeof(data));
	return result;
}

unsigned int md5_dir_track(BYTE *track_buffer, size_t *track_length, unsigned char *result)
{
	/* this calculates a MD5 hash of the BAM and first directory sector, which is sufficient to differentiate most disks */

	unsigned char data[512];
	BYTE id[3];
	BYTE rawdata[260];
	BYTE errorcode;

	crcInit();
	memset(data, 0, sizeof(data));

	/* get disk id */
	if (!extract_id(track_buffer + (18*2 * NIB_TRACK_LENGTH), id))
	{
		fprintf(stderr, "Cannot find directory sector.\n");
		return 0;
	}

	/* t18s0 */
	memset(rawdata, 0, sizeof(rawdata));
	errorcode = convert_GCR_sector(
		track_buffer + ((18*2) * NIB_TRACK_LENGTH),
		track_buffer + ((18*2) * NIB_TRACK_LENGTH) + track_length[18*2],
		rawdata, 18, 0, id);
	memcpy(data, rawdata+1 , 256);

	/* t18s1 */
	memset(rawdata, 0, sizeof(rawdata));
	errorcode = convert_GCR_sector(
		track_buffer + ((18*2) * NIB_TRACK_LENGTH),
		track_buffer + ((18*2) * NIB_TRACK_LENGTH) + track_length[18*2],
		rawdata, 18, 1, id);
	memcpy(data+256, rawdata+1, 256);

	md5(data, sizeof(data), result);
	return 1;
}

unsigned int md5_all_tracks(BYTE *track_buffer, size_t *track_length, unsigned char *result)
{
	/* this calculates an MD5 hash for all sectors on the disk */

	unsigned char data[BLOCKSONDISK * 256];
	int track, sector, index, valid;
	BYTE id[3];
	BYTE rawdata[260];
	BYTE errorcode;

	crcInit();
	memset(data, 0, sizeof(data));

	/* get disk id */
	if (!extract_id(track_buffer + (18*2 * NIB_TRACK_LENGTH), id))
	{
		fprintf(stderr, "Cannot find directory sector.\n");
		return 0;
	}

	index = valid = 0;
	for (track = start_track; track <= 35*2; track += 2)
	{
		for (sector = 0; sector < sector_map[track/2]; sector++)
		{
			memset(rawdata, 0, sizeof(rawdata));

			errorcode = convert_GCR_sector(
				track_buffer + (track * NIB_TRACK_LENGTH),
				track_buffer + (track * NIB_TRACK_LENGTH) + track_length[track],
				rawdata, track/2, sector, id);

			memcpy(data+(index*256), rawdata+1, 256);
			index++;

			if(errorcode == SECTOR_OK)
				valid++;
		}
	}

	if(index != valid)
		printf("[%d/%d sectors] ", valid, index);

	md5(data, sizeof(data), result);
	return 1;
}




