/* This is a generated file, don't edit */

#define NUM_APPLETS 2
#define KNOWN_APPNAME_OFFSETS 0

const char applet_names[] ALIGN1 = ""
"sh" "\0"
"vi" "\0"
;

#define APPLET_NO_sh 0
#define APPLET_NO_vi 1

#ifndef SKIP_applet_main
int (*const applet_main[])(int argc, char **argv) = {
ash_main,
vi_main,
};
#endif

