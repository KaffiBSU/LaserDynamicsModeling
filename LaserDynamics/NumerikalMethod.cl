
	__constant float maxtime = 3.31E-12f;
	__constant int numberXpoints = 10000;		
	__constant int numberTpoints = 8000;
	__constant float c = 3.0E8f;
	__constant float h = 6.63E-34;
	__constant float internalLoses = 0.00127f;
	__constant float AM_L = 0.00124f;
	__constant float SA_L = 0;
	__constant float SA_absorbsion = 2.7E-22f;
	__constant float SA_emission = 8.4E-22f;
	__constant float refrIndex = 1.828f;
	__constant float SA_lifeTime = 4.0E-6f;
	__constant float AM_lifeTime = 100.0E-6f;	
	__constant float absorption = 7.13E-24f;
	__constant float emission = 2.3E-23f;	
	__constant float AM_ionsConc = 4.3E26f;
	__constant float SA_ionsConc = 1;		//?
	__constant float r1 = 1.0f;
	__constant float r2 = 0.7f;
	__constant float R_lum = 0.001f;	
	__constant float pump_power = 160;
	__constant float pump_waveLength = 0.808E-6f;
	__constant float pump_crossSection = 0.62E-6f;

	float I(){
		return (pump_power*pump_waveLength)/(pump_crossSection*h*c);
	}

	float alpha(){
		return ((SA_absorbsion+SA_emission)/(emission*AM_lifeTime))*SA_lifeTime*(1+absorption*AM_lifeTime*I());
	}

	float koefficient_A(){
		//return (AM_L*AM_ionsConc*I()*absorption*emission*AM_lifeTime)/(internalLoses*(1+absorption*AM_lifeTime*I()));		
		return 4130.46;
	}

	float aPLUS(float x, float u){	
		
		return ((internalLoses*c)/(refrIndex*(SA_L+AM_L)))*((koefficient_A()*x - 1)+ (1-x)*(exp(-u)*R_lum));		
		
	}

	float aMINUS(float x, float u){
		
		return ((internalLoses*c)/(refrIndex*(SA_L+AM_L)))*((koefficient_A()*x - 1)+ (1-x)*(exp(-u)*R_lum));	
		
	}

	float rPLUS(float x){	

		//return (-1)*x*((c)*internalLoses)/(refrIndex*(1));
		
		//return (-1)*x*(internalLoses)/(refrIndex*(SA_L+AM_L));	
		return (-1)*x*0.560281;
		
	}

	float rMINUS(float x){

		//return (-1)*x*(internalLoses)/(refrIndex*(SA_L+AM_L));
		return (-1)*x*0.560281;
	}

	float d_AM_func(float d_AM, float uPLUS, float uMINUS, float x){
		return ((1/AM_lifeTime)+(absorption*((pump_power*pump_waveLength)/(pump_crossSection*h*c))*exp((-1)*absorption*AM_ionsConc*x)))*(1-(1+(exp(uPLUS) + exp(uMINUS)))*d_AM);
		
	}

__kernel void laserDynamics(__global float* inPLUS, __global float* outPLUS,__global float* inMINUS,__global float* outMINUS)
{	
	__private float leng = AM_L + SA_L;
	__private float dx = leng/(numberXpoints-1);
	__private int numberAMpoints = (int)numberXpoints*AM_L/leng;
	//__private int numberSApoints = (int)numberXpoints*SA_L/leng;
	__private int numberSApoints = numberXpoints - numberAMpoints;
	__private int nk = get_global_size(0);	
	__private int startX;
	__private int endX;
	const int numberOfX = numberXpoints;	
	__private float dt = maxtime/numberTpoints;	
	__private float middleStepPLUS[10001];
	__private float middleStepMINUS[10001];
	__private float tmpPlus;
	__private float tmpMinus;
	__local float plus[10000];
	__local float minus[10000];
	__local float D_AM[10000];	

	float k1,k2,k3,k4,k5,k6,k7,k8,tmpD_AM;

	int num = get_global_id(0);	

	startX = num*(numberOfX)/nk;
	if(num != nk-1){
		endX = (num+1)*(numberOfX)/nk;
	} else {
		endX = numberOfX;
	}


	for(int i = startX; i < endX; i++){
		plus[i] = inPLUS[i];
		minus[i] = inMINUS[i];
		D_AM[i] = 0.1;
	}	
	barrier(CLK_LOCAL_MEM_FENCE);	

	for(int t = 0;t <= numberTpoints; t++){		
		
		//промежуточный шаг
		for(int i = startX; i <= endX; i++){
			if(i == 0){
				middleStepPLUS[i] = plus[i]+(aPLUS(D_AM[i],plus[i]) + aPLUS(D_AM[i],plus[i]))*(dt/4);
				middleStepMINUS[i] = minus[i]+(aMINUS(D_AM[i],minus[i]) + aMINUS(D_AM[i],minus[i]))*(dt/4);
			}else if(i == numberOfX) {
				middleStepPLUS[i] = plus[i-1] + (aPLUS(D_AM[i-1],plus[i-1]) + aPLUS(D_AM[i-1],plus[i-1]))*(dt/4);
				middleStepMINUS[i] = minus[i-1] + (aMINUS(D_AM[i-1],minus[i-1]) + aMINUS(D_AM[i-1],minus[i-1]))*(dt/4);		
			} else{
				middleStepPLUS[i] = (plus[i-1] + plus[i])/2 + (rPLUS(plus[i])-rPLUS(plus[i-1]))/(2) + (aPLUS(D_AM[i-1],plus[i-1]) + aPLUS(D_AM[i],plus[i]))*(dt/4);
				middleStepMINUS[i] = (minus[i-1] + minus[i])/2 + (rMINUS(minus[i])-rMINUS(minus[i-1]))/(2) + (aMINUS(D_AM[i-1],minus[i-1]) + aMINUS(D_AM[i],minus[i]))*(dt/4);
			}			
		} 
		barrier(CLK_LOCAL_MEM_FENCE);		

		//шаг			
		for(int i = startX; i < endX; i++){	

			tmpPlus = plus[i] + (rPLUS(middleStepPLUS[i+1]) - rPLUS(middleStepPLUS[i]))/c + (aPLUS(D_AM[i],plus[i]) + aPLUS(D_AM[i],plus[i]))*(dt/2);
			tmpMinus = minus[i] + (rMINUS(middleStepMINUS[i+1]) - rMINUS(middleStepMINUS[i]))/c + (aMINUS(D_AM[i],minus[i]) + aMINUS(D_AM[i],minus[i]))*(dt/2);
			barrier(CLK_LOCAL_MEM_FENCE);
			plus[i] = tmpPlus;
			minus[i] = tmpMinus;

			k1= d_AM_func(D_AM[i],plus[i],minus[i],i*dx)*dt;
			k2= d_AM_func(D_AM[i]+k1/5.,plus[i],minus[i],i*dx)*dt;
			k3= d_AM_func(D_AM[i]+k1*3/40.+k2*9/40.,plus[i],minus[i],i*dx)*dt;
			k4= d_AM_func(D_AM[i]+k1*44/45.-k2*56/15.+k3*32/9.,plus[i],minus[i],i*dx)*dt;
			k5= d_AM_func(D_AM[i]+k1*19372/6561.-k2*25360/2187.+k3*64448/6561.-k4*212/729.,plus[i],minus[i],i*dx)*dt;
			k6= d_AM_func(D_AM[i]+k1*9017/3168.-k2*355/33.+k3*46732/5247.+k4*49/176.-k5*5103/18656,plus[i],minus[i],i*dx)*dt;
			k7= d_AM_func(D_AM[i]+k1*35/384.+k3*500/1113.+k4*125/192.-k5*2187/6784.+k6*11/84.,plus[i],minus[i],i*dx)*dt;
			k8= k1*35/384.+k3*500/1113.+k4*125/192.-k5*2187/6784.+k6*11/84.;
			tmpD_AM = D_AM[i] + k8;
			D_AM[i] = tmpD_AM;			
		} 	
		barrier(CLK_LOCAL_MEM_FENCE);

		//перенос границ
		plus[0] = minus[numberOfX-1] + log(r1);
		minus[0] = plus[numberOfX-1] + log(r2);				
	}	

	/*for(int i = startX; i < endX; i++){
		outPLUS[i] = plus[i];
		outMINUS[i] = D_AM[i];		
	}
	barrier(CLK_GLOBAL_MEM_FENCE);*/	

}