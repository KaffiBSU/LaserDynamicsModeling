// Volna.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#define M_PI 3.1415926535897932384626433832795
using namespace std;

double Rand(double ampl) {
	return (ampl*rand()) / RAND_MAX;
}

int main()
{
	// insert code here...
	FILE* gnuplotPipe = _popen("C:/gnuplot/bin/gnuplot.exe", "w");
	fprintf(gnuplotPipe, "set grid\n");
	constexpr const int N = 400;

	constexpr const double length = 2;
	constexpr const double dx = length / N;  //
	constexpr const double tmax = 4;
	double dt = dx;
	double U[N + 1];
	double U1[N + 1];
	double U2[N + 1];
	double ULeft[N + 1];
	double U3[N + 1];
	double U4[N + 1];
	int c = 1.;
	int T = round(tmax/dt);
	int k = 0;
	ofstream u1;

	//u1.open("D:/1.txt");
	for (int i = 1; i <= N; i++)
	{
		U[i] = 1 / (0.1*sqrt(2 * M_PI)*exp(pow(i*dx - 0.5, 2) / (2 * 0.01)));
		U1[i] = Rand(1e-12);
		U2[i] = Rand(1e-12);
		U3[i] = Rand(1e-12);
		U4[i] = Rand(1e-12);
		ULeft[i] = Rand(1e-12);
	}


	if (gnuplotPipe) {
		fprintf(gnuplotPipe, "set object 1 rectangle from screen 0,0 to screen 1,1 fillcolor rgb\"white\" behind\n"); // заливка цветом(белым)
																													  //         fprintf(gnuplotPipe, "set xrange [0:"+to_string(x_furthest)+"]\n");
		fprintf(gnuplotPipe, "set xrange [0:2]\n"); // интервал по x
		fprintf(gnuplotPipe, "set yrange [-5:5]\n"); // интервал по y

		fprintf(gnuplotPipe, "plot [][0:1] 2\n"); //отображение окна
		printf("Press enter when window will appear");
		//         write(1, mes, 36);
		fflush(gnuplotPipe);  //очистка
		getchar();
		for (int t = 1; t <= T * 15; t++)
		{
			//fprintf(gnuplotPipe, "plot '-' using 1:2 with points lc rgb 'black' pt 7\n");
			fprintf(gnuplotPipe, "plot '-' using 1:2 with points lc rgb 'black' pt 7, '-' using 1:3 with points lc rgb 'red' pt 7\n");

			if (k % 2 == 0)    // если k-четное, идет рассчет для новой волны вправо
			{
				if (k == 0)    // используется только один раз, для начала движения вправо
				{
					if (t >= N*k && t <= N*(k + 1))
					{
						U1[1] = U[N*(k + 1) + 1 - t];
					}
				}
				else {
					if (t >= N*k && t <= N*(k + 1))
					{
						U1[1] = U[t - N*k];
					}
				}
			}
			if (t % N == 0)  // счетчик
			{
				k = k + 1;
			}
			if (k % 2 == 1)  // если k-нечетное, заполняем начальный массив для левой волны
			{
				if (t == N*k)
				{
					for (int i = 1; i <= N; i++)
					{
						U4[i] = U1[i];
					}
				}
				if (t >= N*k && t < N*(k + 1))
				{
					U3[N] = U4[N*(k + 1) - t];
				}
			}
			if (k % 2 == 0)
			{

				if (t == N*k)
				{
					for (int i = 1; i <= N; i++)
					{
						U[i] = U3[i];
					}
				}
			}
			/*if (t == 199)
			{
				for (int i = 1; i <= N; i++)
				{
					u1 << i << "   " << U1[i] << endl;
				}
			}*/
			U2[1] = U1[1];
			U2[N] = U1[N - 1];
			ULeft[1] = ULeft[2];
			ULeft[N] = U3[N - 1];
			for (int i = 2; i <= N - 1; i++)
			{
				U2[i] = (U1[i] - c*dt / dx*(U1[i] - U1[i - 1]) - c*dt / dx*(1 - c*dt)*((U1[i] - U1[i - 1])*(U1[i + 1] - U1[i]) / (U1[i + 1] - U1[i] * 0.9999999) - (U1[i] - U1[i - 1])));
				ULeft[i] = U3[i] - c*dt / dx*(U3[i] - U3[i + 1]) - c*dt / dx*(1 - c*dt)*((U3[i] - U3[i - 1])*(U3[i + 1] - U3[i]) / (U3[i + 1] - U3[i] * 0.9999999) - (U3[i] - U3[i - 1]));
			}
			if (t % 10 == 0)
			{
				for (int i = 1; i <= N; i++)
				{
					//fprintf(gnuplotPipe, "%lf, %lf\n", dx * i, U2[i]);
					fprintf(gnuplotPipe, "%20.16lf, %20.16lf, %20.16lf\n", dx * i, U2[i], ULeft[i]);
					//u1 << t << "   " << i*dx << "   " << U2[i] << endl;
					//u1 << t << "   " << i*dx << "   " << ULeft[i] << endl;
				}
				fprintf(gnuplotPipe, "e\n");
				fflush(gnuplotPipe);
			}
			for (int i = 1; i <= N; i++)
			{

				U1[i] = U2[i];
				U3[i] = ULeft[i];
			}
			}
		
		printf("press enter to continue...");
		getchar();

		fprintf(gnuplotPipe, "exit\n");
	}
	else printf("gnuplot not found...");
	return 0;
}

