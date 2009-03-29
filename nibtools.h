/*
 * NIBTOOLS
 * Copyright (c) Pete Rittwage <peter(at)rittwage(dot)com>
 * based on MNIB by Markus Brenner <markus(at)brenner(dot)de>
 */


#define FL_STEPTO      0x00
#define FL_MOTOR       0x01
#define FL_RESET       0x02
#define FL_READWOSYNC  0x03
#define FL_READNORMAL  0x04
#define FL_READIHS 0x05
#define FL_DENSITY     0x06
#define FL_SCANKILLER  0x07
#define FL_SCANDENSITY 0x08
#define FL_READMOTOR   0x09
#define FL_TEST        0x0a
#define FL_WRITENOSYNC   0x0b
#define FL_WRITEIHS 0x0c
#define FL_CAPACITY    0x0d
#define FL_INITTRACK 0x0e
#define FL_VERIFY_CODE 0x0f
#define FL_ZEROTRACK 0x10

/*
#define FL_READIHS 0x03
#define FL_WRITEIHS 0x04
#define FL_STEPTO      0x00
#define FL_MOTOR       0x01
#define FL_RESET       0x02
#define FL_READNORMAL  0x03
#define FL_WRITESYNC   0x04
#define FL_DENSITY     0x05
#define FL_SCANKILLER  0x06
#define FL_SCANDENSITY 0x07
#define FL_READWOSYNC  0x08
#define FL_READMOTOR   0x09
#define FL_TEST        0x0a
#define FL_WRITENOSYNC 0x0b
#define FL_CAPACITY    0x0c
#define FL_INITTRACK   0x0d
#define FL_FINDSYNC    0x0e
#define FL_READMARKER  0x0f
#define FL_VERIFY_CODE 0x10
#define FL_ZEROTRACK 0x11
*/

#define DISK_NORMAL    0

#define IMAGE_NIB      0	/* destination image format */
#define IMAGE_D64      1
#define IMAGE_G64      2
#define IMAGE_NB2	3

#define BM_MATCH       0x10 /* not used but exists in very old images */
#define BM_NO_CYCLE 0x20
#define BM_NO_SYNC     0x40
#define BM_FF_TRACK    0x80

#define DENSITY_SAMPLES 1

/* custom density maps for reading */
#define DENSITY_STANDARD 0
#define DENSITY_RAPIDLOK	1

// tested extensively with Pete's Newtronics mech-based 1541.
//#define CAPACITY_MARGIN 11	// works with FL_WRITENOSYNC
//#define CAPACITY_MARGIN 13	// works with both FL_WRITESYNC and FL_WRITENOSYNC
//#define CAPACITY_MARGIN 16		// safe value
#define CAPACITY_MARGIN 10 // better measurement allows better margins

#define MODE_READ_DISK     	0
#define MODE_WRITE_DISK    	1
#define MODE_UNFORMAT_DISK 	2
#define MODE_WRITE_RAW	   	3
#define MODE_TEST_ALIGNMENT 4


#ifndef DJGPP
#include <opencbm.h>
#endif

/* global variables */
extern char bitrate_range[4];
extern char bitrate_value[4];
extern char density_branch[4];
extern BYTE density_map_rapidlok[];
extern int skew_map[];
extern int density_map;
extern int mode;
extern FILE * fplog;
extern int read_killer;
extern int error_retries;
extern int force_align;
extern int align_disk;
extern int force_density;
extern int track_match;
extern int interactive_mode;
extern int gap_match_length;
extern int cap_min_ignore;
extern int verbose;
extern int fillbyte;
extern float motor_speed;
extern int skew;
extern int ihs;
extern int density_map;
extern int start_track, end_track, track_inc;
extern int fix_gcr, reduce_sync, reduce_gap, reduce_badgcr;
extern int imagetype, auto_capacity_adjust;
extern int extended_parallel_test;
extern int force_nosync;

/* common */
void usage(void);

/* nibread.c */
int disk2file(CBM_FILE fd, char * filename);

/* nibwrite.c */
int loadimage(char * filename);
int writeimage(CBM_FILE fd);

/* fileio.c */
int read_nib(char *filename, BYTE *track_buffer, BYTE *track_density, int *track_length, BYTE *track_alignment);
int read_nb2(char *filename, BYTE *track_buffer, BYTE *track_density, int *track_length, BYTE *track_alignment);
int read_g64(char *filename, BYTE *track_buffer, BYTE *track_density, int *track_length);
int read_d64(char *filename, BYTE *track_buffer, BYTE *track_density, int *track_length);
int write_nib(char *filename, BYTE *track_buffer, BYTE *track_density, int *track_length);
int write_g64(char *filename, BYTE *track_buffer, BYTE *track_density, int *track_length);
int write_d64(char *filename, BYTE *track_buffer, BYTE *track_density, int *track_length);
int compress_halftrack(int halftrack, BYTE *track_buffer, BYTE track_density, int track_length);
int align_tracks(BYTE *track_buffer, BYTE *track_density, int *track_length, BYTE *track_alignment);
int write_dword(FILE * fd, DWORD * buf, int num);
unsigned int crc_dir_track(BYTE *track_buffer, int *track_length);
unsigned int crc_all_tracks(BYTE *track_buffer, int *track_length);
unsigned int md5_dir_track(BYTE *track_buffer, int *track_length, unsigned char *result);
unsigned int md5_all_tracks(BYTE *track_buffer, int *track_length, unsigned char *result);

/* read.c */
BYTE read_halftrack(CBM_FILE fd, int halftrack, BYTE * buffer);
BYTE paranoia_read_halftrack(CBM_FILE fd, int halftrack, BYTE * buffer, int * errors);
int read_floppy(CBM_FILE fd, BYTE *track_buffer, BYTE *track_density, int *track_length);
void write_nb2(CBM_FILE fd, char * filename);
void get_disk_id(CBM_FILE fd);
BYTE scan_density(CBM_FILE fd);

/* write.c */
void master_disk(CBM_FILE fd, BYTE *track_buffer, BYTE *track_density, int *track_length);
void write_raw(CBM_FILE fd, BYTE *track_buffer, BYTE *track_density, int *track_length);
void unformat_disk(CBM_FILE fd);
void unformat_track(CBM_FILE fd, int track);
unsigned int track_capacity(CBM_FILE fd);
void init_aligned_disk(CBM_FILE fd);
void adjust_target(CBM_FILE fd);
void kill_track(CBM_FILE fd, int track);

/* drive.c  */
int compare_extension(char * filename, char * extension);
void ARCH_SIGNALDECL handle_signals(int sig);
void ARCH_SIGNALDECL handle_exit(void);
int upload_code(CBM_FILE fd, BYTE drive);
int reset_floppy(CBM_FILE fd, BYTE drive);
int init_floppy(CBM_FILE fd, BYTE drive, int bump);
int set_density(CBM_FILE fd, int density);
int set_bitrate(CBM_FILE fd, int density);
BYTE set_default_bitrate(CBM_FILE fd, int track);
BYTE scan_track(CBM_FILE fd, int track);
void perform_bump(CBM_FILE fd, BYTE drive);
int test_par_port(CBM_FILE fd);
void send_mnib_cmd(CBM_FILE fd, unsigned char cmd);
void set_full_track(CBM_FILE fd);
void motor_on(CBM_FILE fd);
void motor_off(CBM_FILE fd);
void step_to_halftrack(CBM_FILE fd, int halftrack);
int verify_floppy(CBM_FILE fd);
#ifdef DJGPP
#include <unistd.h>
int find_par_port(CBM_FILE fd);
#endif
