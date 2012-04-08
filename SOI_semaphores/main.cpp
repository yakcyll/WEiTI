#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <windows.h>
#include <queue>
#include <cassert>

#include "const.h"
//#include "fifo_buffer.h"
//#include "semaphore.h"

using namespace std;

queue<int> integerBuffer[THENUMBERN+1];

long intCount = ITEMLIMIT;
long limitIn = 0;
long limitOut = THENUMBERN;

/*Mutex GTFOfromIn;
Mutex GTFOfromOut;
Mutex GTFOfromBuffer[N];

Semaphore in(&limitIn);
Semaphore out(&limitOut);
//Semaphore buf[N+1];*/

HANDLE GTFOfromIn;
HANDLE GTFOfromOut;
HANDLE GTFOfromBuffer[THENUMBERN+1];

HANDLE in;
HANDLE out;

HANDLE moverProcesses[MOVERSNO];
HANDLE giverProcesses[GIVERSNO];
HANDLE takerProcesses[TAKERSNO];

DWORD WINAPI givingProcess(LPVOID lpParam) {
	
	cout << "Giver started." << endl;

	while(1) {
		
		WaitForSingleObject(in, INFINITE);

		WaitForSingleObject(GTFOfromIn, INFINITE);
		cout << "Giver: In lock acquired." << endl;

		WaitForSingleObject(GTFOfromBuffer[0], INFINITE);
		integerBuffer[0].push(int(rand()));
		intCount--;
		cout << "Producing a number " << integerBuffer[0].back() << "; count: " << ITEMLIMIT - intCount << endl;
		ReleaseMutex(GTFOfromBuffer[0]);
		
		ReleaseMutex(GTFOfromIn);
		cout << "Giver: In lock released." << endl;
	}

}

DWORD WINAPI takingProcess(LPVOID lpParam) {

	cout << "Taker started." << endl;

	while(1) {
		WaitForSingleObject(out, INFINITE);

		WaitForSingleObject(GTFOfromOut, INFINITE);
		cout << "Taker: Out lock acquired." << endl;

		WaitForSingleObject(GTFOfromBuffer[THENUMBERN], INFINITE);

		if (integerBuffer[THENUMBERN].empty()) {
			cout << "FATAL ERROR: Mutexes acquired before the conditions were met." << endl;
			assert(0);
		}
		cout << "Consuming a number " << integerBuffer[THENUMBERN].front() << "; size: " << integerBuffer[THENUMBERN].size() << endl;
		integerBuffer[THENUMBERN].pop();
		intCount++;

		ReleaseMutex(GTFOfromBuffer[THENUMBERN]);

		WaitForSingleObject(GTFOfromIn, INFINITE);
		cout << "Taker: In lock acquired." << endl;

		ReleaseSemaphore(in, 1, NULL);

		ReleaseMutex(GTFOfromIn);
		ReleaseMutex(GTFOfromOut);
		cout << "Taker: In and Out locks released." << endl;
	}

}

DWORD WINAPI movingProcess(LPVOID lpParam) {

	int i = *((int *)lpParam);

	cout << "Mover [" << i << "] started." << endl;

	while(1) {
		WaitForSingleObject(GTFOfromBuffer[i], INFINITE);
		//WaitForSingleObject(GTFOfromBuffer[i+1], INFINITE);
		//cout << "Mover [" << i << "] : Buffer lock acquired." << endl;

		if(integerBuffer[i].size() > integerBuffer[i+1].size()) {

			integerBuffer[i+1].push(integerBuffer[i].front());
			integerBuffer[i].pop();

		}

		//ReleaseMutex(GTFOfromBuffer[i+1]);
		ReleaseMutex(GTFOfromBuffer[i]);

		if ( i == THENUMBERN-1 && integerBuffer[THENUMBERN].size() >= THENUMBERN )
			ReleaseSemaphore(out, 1, NULL);
		//cout << "Mover [" << i << "]: Buffer lock released." << endl;
	}

}

int main (int argc, char* argv[]) {
	
	/*for (int i = 0; i < N+1; i++) {

		buf[i].limit = &(integerBuffer[i].count);

	}*/

	char x;
	DWORD WINAPI processID;

	srand(time(NULL));

	int moversParam[MOVERSNO];
	for (int i = 0; i < MOVERSNO; i++)
		moversParam[i] = i % THENUMBERN;

	cout << "Parameters initialized." << endl;

	GTFOfromIn = CreateMutex(NULL, false, NULL);
	GTFOfromOut = CreateMutex(NULL, false, NULL);
	for (int i = 0; i < THENUMBERN+1; i++) {

		GTFOfromBuffer[i] = CreateMutex(NULL, false, NULL);

	}

	cout << "Mutexes created." << endl;

	in = CreateSemaphore(NULL, ITEMLIMIT, ITEMLIMIT, NULL);
	out = CreateSemaphore(NULL, 0, ITEMLIMIT, NULL);

	cout << "Semaphores initialized." << endl;

	for (int i = 0; i < TAKERSNO; i++) {

		takerProcesses[i] = CreateThread(NULL, 0, takingProcess, NULL, 0, &processID);
		cout << "Taker process no. " << processID << " created." << endl;

	}
	
	for (int i = 0; i < MOVERSNO; i++) {

		moverProcesses[i] = CreateThread(NULL, 0, movingProcess, &(moversParam[i]), 0, &processID);
		cout << "Mover process no. " << processID << " created." << endl;

	}

	for (int i = 0; i < GIVERSNO; i++) {

		giverProcesses[i] = CreateThread(NULL, 0, givingProcess, NULL, 0,  &processID);
		cout << "Giver process no. " << processID << " created." << endl;

	}

	while(x = getchar()) {
		if (x == 'x') break;
		/*if (x == 's') {

			cout << "Flushing buffers:" << endl;
			for (int i = 0; i < N+1; i++) {
				cout << i << ": ";
				integerBuffer[i].flush();
			}

		}
		if (x == 'm') {

			cout << "Mutexes (0 - unlocked, 1 - locked): " << endl;
			cout << "GTFOfromIn       : " << GTFOfromIn.Status()     << endl;
			cout << "in.GTFO          : " << in.GTFO.Status()		 << endl;
			cout << "GTFOfromOut      : " << GTFOfromOut.Status()    << endl;
			cout << "out.GTFO         : " << out.GTFO.Status()		 << endl;
			for (int i = 0; i < N; i++)
				cout << "GTFOfromBuffer[" << i << "]: " << GTFOfromBuffer[i].Status() << endl;

		}*/

	}

	for (int i = 0; i < GIVERSNO; i++)
		CloseHandle(giverProcesses[i]);

	for (int i = 0; i < MOVERSNO; i++)
		CloseHandle(moverProcesses[i]);

	for (int i = 0; i < TAKERSNO; i++)
		CloseHandle(takerProcesses[i]);

	for (int i = 0; i < THENUMBERN+1; i++)
		CloseHandle(GTFOfromBuffer[i]);

	CloseHandle(GTFOfromIn);
	CloseHandle(GTFOfromOut);
	CloseHandle(in);
	CloseHandle(out);

	cout << "Handles closed." << endl;

	return 0;

}