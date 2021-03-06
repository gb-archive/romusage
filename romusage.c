// This is free and unencumbered software released into the public domain.
// For more information, please refer to <https://unlicense.org>
// bbbbbr 2020

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "banks.h"
#include "map_file.h"
#include "noi_file.h"
#include "ihx_file.h"

void display_help(void);
int handle_args(int argc, char * argv[]);

char filename_in[MAX_STR_LEN] = {'\0'};
int  show_help_and_exit = false;


void display_help(void) {
    fprintf(stdout,
           "romusage input_file.[map|noi|ihx] [options]\n"
           "\n"
           "Options\n"
           "-h  : Show this help\n"
           "-a  : Show Areas in each Bank\n"
           "-sH : Show HEADER Areas (normally hidden)\n"
           "-g  : Show a small usage graph per bank\n"
           "-G  : Show a large usage graph per bank\n"
           "-m  : Manually specify an Area -m:NAME:HEXADDR:HEXLENGTH\n"
           "-e  : Manually specify an Area that should not overlap -e:NAME:HEXADDR:HEXLENGTH\n"
           "-E  : All areas are exclusive (except HEADERs), warn for any overlaps\n"
           "-q  : Quiet, no output except warnings and errors\n"
           "-R  : Return error code for Area warnings and errors \n"
           "\n"
           "Use: Read a .map, .noi or .ihx file to display area sizes.\n"
           "Example 1: \"romusage build/MyProject.map\"\n"
           "Example 2: \"romusage build/MyProject.noi -a -e:STACK:DEFF:100 -e:SHADOW_OAM:C000:A0\"\n"
           "Example 3: \"romusage build/MyProject.ihx -g\"\n"
           "Example 4: \"romusage build/MyProject.map -q -R\"\n"
           "\n"
           "Notes:\n"
           "  * GBDK / RGBDS map file format detection is automatic.\n"
           "  * Estimates are as close as possible, but may not be complete.\n"
           "    Unless specified with -m/-e they *do not* factor regions lacking\n"
           "    complete ranges in the Map/Noi/Ihx file, for example Shadow OAM and Stack.\n"
           "  * IHX files can only detect overlaps, not detect memory region overflows.\n"
           );
}


int handle_args(int argc, char * argv[]) {

    int i;

    if( argc < 2 ) {
        display_help();
        return false;
    }

    // Copy input filename (if not preceded with option dash)
    if (argv[1][0] != '-')
        snprintf(filename_in, sizeof(filename_in), "%s", argv[1]);

    // Start at first optional argument, argc is zero based
    for (i = 1; i <= (argc -1); i++ ) {

        if (strstr(argv[i], "-h")) {
            display_help();
            show_help_and_exit = true;
            return true;  // Don't parse further input when -h is used
        } else if (strstr(argv[i], "-a")) {
            banks_output_show_areas(true);
        } else if (strstr(argv[i], "-sH")) {
            banks_output_show_headers(true);

        } else if (strstr(argv[i], "-g")) {
            banks_output_show_minigraph(true);
        } else if (strstr(argv[i], "-G")) {
            banks_output_show_largegraph(true);
        } else if (strstr(argv[i], "-E")) {
            set_option_all_areas_exclusive(true);

        } else if (strstr(argv[i], "-q")) {
            set_option_quiet_mode(true);
        } else if (strstr(argv[i], "-R")) {
            set_option_error_on_warning(true);

        } else if (strstr(argv[i], "-m") || strstr(argv[i], "-e")) {
            if (!area_manual_add(argv[i])) {
            fprintf(stdout,"malformed manual area argument: %s\n\n", argv[i]);
            display_help();
            return false;
            }
        } else if (argv[i][0] == '-') {
            fprintf(stdout,"Unknown argument: %s\n\n", argv[i]);
            display_help();
            return false;
        }

    }

    return true;
}


int matches_extension(char * filename, char * extension) {
    return (strcmp(filename + (strlen(filename) - strlen(extension)), extension) == 0);
}


int main( int argc, char *argv[] )  {

    int ret = EXIT_FAILURE; // Default to failure on exit

    if (handle_args(argc, argv)) {

        if (show_help_and_exit) {
            ret = EXIT_SUCCESS;
        }
        else if (strlen(filename_in) >=5) { // Must at least have extension
            // detect file extension
            if (matches_extension(filename_in, (char *)".noi")) {
                if (noi_file_process_areas(filename_in)) {
                    banklist_finalize_and_show();
                    ret = EXIT_SUCCESS; // Exit with success
                }
            } else if (matches_extension(filename_in, (char *)".map")) {
                if (map_file_process_areas(filename_in)) {
                    banklist_finalize_and_show();
                    ret = EXIT_SUCCESS; // Exit with success
                }
            } else if (matches_extension(filename_in, (char *)".ihx")) {
                if (ihx_file_process_areas(filename_in)) {
                    banklist_finalize_and_show();
                    ret = EXIT_SUCCESS; // Exit with success
                }
            }
        }
    }

    if (ret == EXIT_FAILURE)
        printf("Problem with filename or unable to open file! %s\n", filename_in);

    // Override exit code if was set during processing
    if (get_exit_error())
        ret = EXIT_FAILURE;

    return ret;
}