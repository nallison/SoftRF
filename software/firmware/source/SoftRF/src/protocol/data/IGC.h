#ifndef IGC_H
#define IGC_H

#if defined(IGCFILESYS)

void FlightLog_setup();
void openFlightLog();
void logFlightPosition();
void completeFlightLog();
void closeFlightLog();
void FlightLogComment(const char *data);
void MD5_test();

extern bool FlightLogOpen;
extern char FlightLogPath[];

extern char *PSRAMbuf;
extern size_t PSRAMbufUsed;

#endif

#endif
