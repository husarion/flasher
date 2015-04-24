#include "console.h"

#include <stdio.h>
#include "myFTDI.h"

#include <termios.h>
#include <pthread.h>

int mygetch()
{
	struct termios oldt, newt;
	int ch;
	tcgetattr(fileno(stdin), &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(fileno(stdin), TCSANOW, &newt);
	ch = getchar();
	tcsetattr(fileno(stdin), TCSANOW, &oldt);
	return ch;
}

void* thread(void*)
{
	for (;;)
	{
		int c = mygetch();
		char cc = (char)(c & 0xff);
		int res = uart_tx(&cc, 1);
	}
}

void openConsole(int speed)
{
	bool res = uart_open(speed, true);
	
	pthread_t t;
	pthread_create(&t, 0, thread, 0);
	
	for (;;)
	{
		int res;
		char data[1024];
		res = uart_rx_any(data, 1024);
		if (res)
		{
			for (int i = 0; i < res; i++)
				putchar(data[i]);
		}
	}
}

