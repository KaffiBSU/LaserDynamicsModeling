
	__constant float leng = 1;
	__constant float maxtime = 1;
	__constant float dx = 1.0f/600;	
	__constant float dt = 1.0f/5000;
	__constant float c = 3.0E8f;


	float aPLUS(float x){			
		return 0;	
	}

	float aMINUS(float x){			
		return 0;	
	}

	float rPLUS(float x){			
		return x*(-c);	
	}

	float rMINUS(float x){			
		return x*(-c);	
	}


__kernel void solveLaserDynamics(__global float* inPLUS, __global float* outPLUS,__global float* inMINUS,__global float* outMINUS)
{		
	__private int nk = get_global_size(0);
	__private int numberOfIterations = 1;
	__private int start;
	__private int end;
	const int numberOfX = (int)leng/dx + 1;
	__private int numberOfT = numberOfIterations*(numberOfX-1);	
	__private float middleStepPLUS[602];
	__private float middleStepMINUS[602];
	__private float tmpPlus;
	__private float tmpMinus;
	__local float plus[601];
	__local float minus[601];

	int num = get_global_id(0);	

	start = num*(numberOfX)/nk;
	if(num != nk-1){
		end = (num+1)*(numberOfX)/nk;
	} else {
		end = numberOfX;
	}


	for(int i = start; i < end; i++){
		plus[i] = inPLUS[i];
		minus[i] = inMINUS[i];			
	}	
	barrier(CLK_LOCAL_MEM_FENCE);	

	for(int t = 0;t < numberOfT; t++){
		
		//перенос границ
		plus[0] = minus[numberOfX-1];
		minus[0] = plus[numberOfX-1];
		
		//промежуточный шаг
		for(int i = start; i <= end; i++){
			if(i == 0){
				middleStepPLUS[i] = plus[i]+(aPLUS(i*dx) + aPLUS((i+1)*dx))*(dt/4);
				middleStepMINUS[i] = minus[i]+(aMINUS(i*dx) + aMINUS((i+1)*dx))*(dt/4);
			}else if(i == numberOfX) {
				middleStepPLUS[i] = plus[i-1] + (aPLUS(i*dx) + aPLUS((i-1)*dx))*(dt/4);
				middleStepMINUS[i] = minus[i-1] + (aMINUS(i*dx) + aMINUS((i-1)*dx))*(dt/4);		
			} else{
				middleStepPLUS[i] = (plus[i-1] + plus[i])/2 + (rPLUS(plus[i])-rPLUS(plus[i-1]))/(2*c) + (aPLUS(i*dx)+aPLUS((i-1)*dx))*(dt/4);
				middleStepMINUS[i] = (minus[i-1] + minus[i])/2 + (rMINUS(minus[i])-rMINUS(minus[i-1]))/(2*c) + (aMINUS(i*dx)+aMINUS((i-1)*dx))*(dt/4);
			}			
		} 
		barrier(CLK_LOCAL_MEM_FENCE);		

		//шаг			
		for(int i = start; i < end; i++){	
			tmpPlus = plus[i] + (rPLUS(middleStepPLUS[i+1]) - rPLUS(middleStepPLUS[i]))/c + (aPLUS(i*dx)+aPLUS((i+1)*dx))*(dt/2);
			tmpMinus = minus[i] + (rMINUS(middleStepMINUS[i+1]) - rMINUS(middleStepMINUS[i]))/c + (aMINUS(i*dx)+aMINUS((i+1)*dx))*(dt/2);
			barrier(CLK_LOCAL_MEM_FENCE);
			plus[i] = tmpPlus;
			minus[i] = tmpMinus;
		} 	
		barrier(CLK_LOCAL_MEM_FENCE);	
	}

	//перенос границ	
	plus[0] = minus[numberOfX-1];
	minus[0] = plus[numberOfX-1];


	for(int i = start; i < end; i++){
		outPLUS[i] = plus[i];
		outMINUS[i] = minus[i];
	}
	barrier(CLK_GLOBAL_MEM_FENCE);	

}