#include <sys/ipc.h>
#include <sys/msg.h>

void InitIPC();
void TerminateIPC();
bool SendNewCoords(int x, int y, int what);
