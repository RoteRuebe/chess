#pragma once

#define VERSION_MAJ 1
#define VERSION_MIN 1

enum Verbosity {
    verbose,
    log,
    error
};

void log_msg(char* msg, enum Verbosity type);

