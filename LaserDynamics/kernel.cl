__kernel void solver(__global float * inputPlus, __global float * outputPlus,
					 __global float * inputMinus, __global float * outputMinus, int xpoints)
{

	for (int i = 0; i < xpoints; i++)
	{
		outputPlus[i] = inputPlus[i] + 2;
		outputMinus[i] = inputMinus[i] + 3;
	}

}