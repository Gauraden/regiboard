#include "ipc.hpp"
#include <cstdio>

struct Message {
  static int GetSize() {
    const static int kLength = sizeof(Message) - sizeof(long);
    return kLength;
  }

  long mtype;
	char cmd[128];
	char result[32];
};

int     nres;
int     qid;
key_t   msgkey;
Message sent,
        received;

bool InitIPC() {
	msgkey = ftok("/home/regigraf",'a');
	qid = msgget(msgkey, IPC_CREAT | 0660);
  return (qid != -1);
}

void TerminateIPC() {
	msgctl(qid, IPC_RMID, 0);
//	std::cout << "TerminateIPC: QID = " << qid << std::endl;
}

bool SendNewCoords(int x, int y, int what) {
  sent.mtype = 1; // always one
	snprintf(sent.cmd, 128, "got coords; x=%i y=%i what=%i", x, y, what);
	snprintf(sent.result, 5, "%s", "OK");
	nres = msgsnd(qid, &sent, Message::GetSize(), 0);
	if (nres == -1) {
	  return false;
	}
	return true;
}
