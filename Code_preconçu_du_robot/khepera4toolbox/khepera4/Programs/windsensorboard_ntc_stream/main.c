/******************************************************************************
 *
 * Khepera IV Toolbox
 * (c) 2014 EPFL, Lausanne, Switzerland
 * Authors: Jorge Soares et al.
 *
 * Code forked from Khepera III Toolbox
 * (c) 2006 EPFL, Lausanne, Switzerland
 * Authors: Thomas Lochmatter et al.
 *
 * Additional DISAL libraries
 * (c) 2006 EPFL, Lausanne, Switzerland
 * Authors: Thomas Lochmatter, Jim Pugh, Sven Gowal, Pascal Gilbert, and others
 *
 ******************************************************************************/


#include "commandline.h"
#include "windsensorboard.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#define PI 3.14159265358979

// Algorithm parameters

struct sAlgorithm {
    int wait_us; // Waiting time between stream requests
    int count; // Number of samples to report
    int verbosity; // Level of verbosity (0=quiet, 1=normal, 2=verbose, 3=very verbose, ...)
};

// Algorithm instance
struct sAlgorithm algorithm;

// Prints the help text.

void help() {
    printf("Reads the NTC stream.\n");
    printf("Usage: windsensorboard_ntc_stream [OPTIONS]\n");
    printf("  -w --wait-us US       Sets the waiting time in the perception-to-action loop (default: 50000)\n");
    printf("  -c --count C          Sets the minimum number of samples to report before quitting (default: 0 = infinity)\n");
    printf("  -v --verbosity V      Sets the verbosity level (0=quiet, 1=default, 2=verbose, 3=very verbose, ...)\n");
}

// Initializes the algorithm.

void algorithm_init() {
    // Initialize modules
    windsensorboard_init();

    // Read command line arguments
    algorithm.wait_us = commandline_option_value_int("-w", "--wait-us", 50 * 1000);
    algorithm.count = commandline_option_value_int("-c", "--count", 0);
    algorithm.verbosity = commandline_option_value_int("-v", "--verbosity", 1);
}

// Reports NTC sensor values.

void algorithm_run() {
    int i;
    int block_number = 0;

    // Start the stream
    i2c_stream_read_start(&windsensorboard.ntc.stream);

    // Read from the stream
    while (1) {
        // Read the next samples from the stream
        i2c_stream_read_next(&windsensorboard.ntc.stream);

        // Print all (non-empty) blocks
        for (i = 0; i < windsensorboard.ntc.stream.result.blocks_count; i++) {
            int block_status = i2c_stream_readdata_blockstatus(&windsensorboard.ntc.stream, i);
            if (block_status == I2C_STREAM_BLOCKSTATUS_BUFFER_OVERRUN) {
                // Buffer overrun is a non-recoverable error
                printf("$BUFFER_OVERRUN\n");
                return;
            } else if (block_status == I2C_STREAM_BLOCKSTATUS_EMPTY) {
                // Empty block? Do nothing ...
            } else {
                // Print the block
                if (algorithm.verbosity > 0) {
                    printf("$NTC,%d,%d,%d,%d,%d,%d,%d\n", block_number,
                            i2c_stream_readdata_uint16(&windsensorboard.ntc.stream, i, 2),
                            i2c_stream_readdata_uint16(&windsensorboard.ntc.stream, i, 4),
                            i2c_stream_readdata_uint16(&windsensorboard.ntc.stream, i, 6),
                            i2c_stream_readdata_uint16(&windsensorboard.ntc.stream, i, 8),
                            i2c_stream_readdata_uint16(&windsensorboard.ntc.stream, i, 10),
                            i2c_stream_readdata_uint16(&windsensorboard.ntc.stream, i, 12)
                            );
                }
                block_number++;
            }
        }

        // Termination criterion
        if ((algorithm.count > 0) && (block_number >= algorithm.count)) {
            return;
        }

        // Sleep
        usleep(algorithm.wait_us);
    }
}

// Main program.

int main(int argc, char *argv[]) {
    // Command line parsing
    commandline_init();
    commandline_parse(argc, argv);

    // Help
    if (commandline_option_provided("-h", "--help")) {
        help();
        exit(1);
    }

    // Initialization
    algorithm_init();

    // Run
    algorithm_run();
    return 0;
}
