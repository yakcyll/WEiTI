#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <windows.h>
#include <queue>

#include "const.h"
//#include "fifo_buffer.h"
//#include "semaphore.h"

using namespace std;

queue<int> integerBuffer[N+1];

long intCount = 6*N;
long limitIn = 0;
long limitOut = N;

/*Mutex GTFOfromIn;
Mutex GTFOfromOut;
Mutex GTFOfromBuffer[N];

Semaphore in(&limitIn);
Semaphore out(&limitOut);
//Semaphore buf[N+1];*/

HANDLE GTFOfromIn;
HANDLE GTFOfromOut;
HANDLE GTFOfromBuffer[N+1];

HANDLE in;
HANDLE out;

HANDLE moverProcesses[MoversNo];
HANDLE giverProcesses[GiversNo];
HANDLE takerProcesses[TakersNo];

DWORD WINAPI givingProcess(LPVOID lpParam) {
	
	cout << "Giver started." << endl;

	while(1) {
		
		WaitForSingleObject(in, INFINITE);

		WaitForSingleObject(GTFOfromIn, INFINITE);
		cout << "Giver: In lock acquired." << endl;

		WaitForSingleObject(GTFOfromBuffer[0], INFINITE);
		integerBuffer[0].push(int(rand()));
		intCount--;
		cout << "Producing a number " << integerBuffer[0].back() << "; count: " << 6*N - intCount << endl;
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

		cout << "Consuming a number " << integerBuffer[N].front() << "; size: " << integerBuffer[N].size() << endl;
		integerBuffer[N].pop();
		intCount++;

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
		//cout << "Mover [" << i << "] : Buffer lock acquired." << endl;

		if(integerBuffer[i].size() > integerBuffer[i+1].size()) {

			integerBuffer[i+1].push(integerBuffer[i].front());
			integerBuffer[i].pop();

		}

		ReleaseMutex(GTFOfromBuffer[i]);

		if (integerBuffer[N].size() >= N)
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

	int moversParam[MoversNo];
	for (int i = 0; i < MoversNo; i++)
		moversParam[i] = (int)(i / (MoversNo / N));

	cout << "Parameters initialized." << endl;

	GTFOfromIn = CreateMutex(NULL, false, NULL);
	GTFOfromOut = CreateMutex(NULL, false, NULL);
	for (int i = 0; i < N+1; i++) {

		GTFOfromBuffer[i] = CreateMutex(NULL, false, NULL);

	}

	cout << "Mutexes created." << endl;

	in = CreateSemaphore(NULL, 6*N, 6*N, NULL);
	out = CreateSemaphore(NULL, 0, 6*N, NULL);

	cout << "Semaphores initialized." << endl;

	for (int i = 0; i < TakersNo; i++) {

		takerProcesses[i] = CreateThread(NULL, 0, takingProcess, NULL, 0, &processID);
		cout << "Taker process no. " << processID << " created." << endl;

	}
	
	for (int i = 0; i < MoversNo; i++) {

		moverProcesses[i] = CreateThread(NULL, 0, movingProcess, &(moversParam[i]), 0, &processID);
		cout << "Mover process no. " << processID << " created." << endl;

	}

	for (int i = 0; i < GiversNo; i++) {

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

	for (int i = 0; i < GiversNo; i++)
		CloseHandle(giverProcesses[i]);

	for (int i = 0; i < MoversNo; i++)
		CloseHandle(moverProcesses[i]);

	for (int i = 0; i < TakersNo; i++)
		CloseHandle(takerProcesses[i]);

	for (int i = 0; i < N+1; i++)
		CloseHandle(GTFOfromBuffer[i]);

	CloseHandle(GTFOfromIn);
	CloseHandle(GTFOfromOut);
	CloseHandle(in);
	CloseHandle(out);

	cout << "Handles closed." << endl;

	return 0;

}