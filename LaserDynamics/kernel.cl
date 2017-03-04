__kernel void solver(__global float * inputPlus, __global float * outputPlus, int xpoints) {

	for (int i = 0; i < xpoints; i++)
	{
		outputPlus[i] = inputPlus[i] + 12;
	}

}