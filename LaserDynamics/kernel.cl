#define xpoints 200
#define local_xpoints 50
#define size 4

__constant double A = 1;
__constant double length_res = 1; // длина резонатора
__constant double dt = 0.0005;
__constant double dx = 0.01;

/* граничные условия, зависящие от времени */
double x0()
{
	return 0;
}

double xmax()
{
	return 0;
}

void next_step(double* in_plus, double* in_minus, double* out_plus, double* out_minus,
			   __local double* lefts_plus, __local double* rights_plus,	__local double* lefts_minus, 
			   __local double* rights_minus, int my_id)
{
	/* вычисление внешних граничных точек */
	if (my_id == 0)
	{
		out_plus[0] = x0();
		out_minus[0] = x0();
	}
	if (my_id == (size - 1))
	{
		out_plus[local_xpoints - 1] = xmax();
		out_minus[local_xpoints - 1] = xmax();
	}
	/* вычисление внутренних граничных точек */
	//левой:
	if (my_id != 0)
	{
		out_plus[0] = in_plus[0] - dt / dx * (in_plus[0] - rights_plus[my_id - 1]);
		out_minus[0] = in_minus[0] - dt / dx * (in_minus[0] - rights_minus[my_id - 1]);
	}
	//правой:
	/*if (my_id != (size - 1))
	{
		out_plus[local_xpoints - 1] = in_plus[local_xpoints - 1] - dt / dx * (in_plus[local_xpoints - 1] - lefts_plus[my_id + 1]);
		out_minus[local_xpoints - 1] = in_minus[local_xpoints - 1] - dt / dx * (in_minus[local_xpoints - 1] - lefts_minus[my_id + 1]);
	}*/
	/* вычисление внутренних точек */
	for (int i = 1; i < local_xpoints/* - 1*/; i++)
	{
		out_plus[i] = in_plus[i] - dt / dx * (in_plus[i] - in_plus[i - 1]);
		out_minus[i] = in_minus[i] - dt / dx * (in_minus[i] - in_minus[i - 1]);
	}
}

__kernel void solver(__global double * inputPlus, __global double * outputPlus,
					 __global double * inputMinus, __global double * outputMinus)
{
	int my_id = get_global_id(0);

	/* вычисления будут проводиться в приватной (быстрой) памяти */
	__private double in_plus[local_xpoints];
	__private double in_minus[local_xpoints];
	__private double out_plus[local_xpoints];
	__private double out_minus[local_xpoints];

	/* общая память */
	__local double lefts_plus[size - 1];
	__local double rights_plus[size - 1];
	__local double lefts_minus[size - 1];
	__local double rights_minus[size - 1];

	/* worker'ы разбирают массивы */
	for (int i = 0; i < local_xpoints; i++)
	{
		in_plus[i] = inputPlus[my_id * local_xpoints + i];
		in_minus[i] = inputMinus[my_id * local_xpoints + i];
	}

	/* складываем левые и правые точки в локальную память */
	/* для этой численной схемы lefts не нужны */
	/*if (my_id != 0)
	{
		lefts_plus[my_id - 1] = in_plus[0];
		lefts_minus[my_id - 1] = in_minus[0];
	}*/
	if (my_id != (size - 1))
	{
		rights_plus[my_id] = in_plus[local_xpoints - 1];
		rights_minus[my_id] = in_minus[local_xpoints - 1]; 
	}

	/* вычисления *////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	next_step(in_plus, in_minus, out_plus, out_minus,
				lefts_plus, rights_plus, lefts_minus, rights_minus, my_id);
	/* конец вычислений *//////////////////////////////////////////////////////////////////////////////////////////////////////////

	for (int i = 0; i < local_xpoints; i++)
	{
		outputPlus[my_id * local_xpoints + i] = out_plus[i];
		outputMinus[my_id * local_xpoints + i] = out_minus[i];
	}
}