
#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <ctime>

#pragma comment(lib,"OpenCL.lib")

#define SUCCESS 0
#define FAILURE 1

const int numberOfX = 10000;
const int maxActiveTasksNumber = 1024;

using namespace std;

/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if(f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size+1];
		if(!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	cout<<"Error: failed to open file\n:"<<filename<<endl;
	return FAILURE;
}

void setToZero(float* &u){
	for (int i = 0; i < numberOfX; i++)
	{		
		u[i] = 0;
	}
}

int main(int argc, char* argv[])
{	
	const char *filename = "NumerikalMethod.cl";
	int activeTasksNumber = 1;	

	while(activeTasksNumber<=maxActiveTasksNumber){	

	/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms;	
	cl_platform_id platform = NULL;	
	cl_int	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		cout << "Error: Getting platforms!" << endl;
		return FAILURE;
	}

	/*For clarity, choose the first available platform. */
	if(numPlatforms > 0)
	{
		cl_platform_id* platforms = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
		platform = platforms[0];
		free(platforms);
	}

	/*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
	cl_uint	numDevices = 0;
	cl_device_id *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);	
	if (numDevices == 0)	//no GPU available.
	{
		cout << "No GPU device available." << endl;
		cout << "Choose CPU as default device." << endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);	
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
	}
	else
	{
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}
		

	/*Step 3: Create context.*/
	cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);

	/*Step 4: Creating command queue associate with the context.*/
	cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);

	/*Step 5: Create program object */
	string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};	
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);

	/*Step 6: Build program. */
	status=clBuildProgram(program, 1,devices,NULL,NULL,NULL);

	if(status!=CL_SUCCESS){
		size_t log;
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log);
		cout << "logsize = " << log << endl;

		char* buildlog = new char[log] ;
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, log, buildlog, NULL);
		cout << buildlog;
		return FAILURE;

	} else{			

		/*Step 7: Initial input,output for the host and create memory objects for the kernel*/		
		float * inputPLUS = new float[numberOfX];
		setToZero(inputPLUS);		
		float * inputMINUS = new float[numberOfX];
		setToZero(inputMINUS);
		float *outputPLUS = new float[numberOfX];
		float *outputMINUS = new float[numberOfX];			

		cl_mem inputBufferPLUS = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, numberOfX * sizeof(float),inputPLUS, NULL);
		cl_mem outputBufferPLUS = clCreateBuffer(context, CL_MEM_WRITE_ONLY , numberOfX * sizeof(float), NULL, NULL); 
		cl_mem inputBufferMINUS = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, numberOfX * sizeof(float),inputMINUS, NULL);
		cl_mem outputBufferMINUS = clCreateBuffer(context, CL_MEM_WRITE_ONLY , numberOfX * sizeof(float), NULL, NULL); 

		/*Step 8: Create kernel object */			
		cl_kernel kernel = clCreateKernel(program,"teploprovodnost", NULL);

		/*Step 9: Sets Kernel arguments.*/
		status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBufferPLUS);
		status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outputBufferPLUS);
		status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&inputBufferMINUS);
		status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&outputBufferMINUS);

		unsigned int start_time =  clock();

		/*Step 10: Running the kernel.*/
		size_t global_work_size[1] = {activeTasksNumber};
		status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);		

		/*Step 11: Read the cout put back to host memory.*/
		status = clEnqueueReadBuffer(commandQueue, outputBufferPLUS, CL_TRUE, 0, numberOfX * sizeof(cl_float), outputPLUS, 0, NULL, NULL);
		status = clEnqueueReadBuffer(commandQueue, outputBufferMINUS, CL_TRUE, 0, numberOfX * sizeof(cl_float), outputMINUS, 0, NULL, NULL);
	
		unsigned int end_time = clock();
		unsigned int work_time = end_time - start_time;

		printf("Time of work = [ %d ] with activeTasksNumber = %d \n",work_time,activeTasksNumber);		
		
		/*Step 12: Clean the resources.*/
		status = clReleaseKernel(kernel);				    //Release kernel.
		status = clReleaseProgram(program);				    //Release the program object.
		status = clReleaseMemObject(inputBufferPLUS);		//Release mem object.	
		status = clReleaseMemObject(outputBufferPLUS);
		status = clReleaseMemObject(outputBufferMINUS);
		status = clReleaseMemObject(inputBufferMINUS);		//Release mem object.		
		status = clReleaseCommandQueue(commandQueue);	    //Release  Command queue.
		status = clReleaseContext(context);				    //Release context.			

		//output to the console	
		/*for(int i=0;i<numberOfX*10;i++){
			printf("outputPLUS[ %d ] = %f \n",i,outputPLUS[i]);	
		}
		cout<<""<<endl;
		for(int i=0;i<numberOfX*10;i++){
			printf("outputMINUS[ %d ] = %f \n",i,outputMINUS[i]);	
		} */
	

		delete[] outputPLUS;	
		outputPLUS = NULL;
		delete[] outputMINUS;	
		outputMINUS = NULL;
		delete[] inputPLUS;
		inputPLUS = NULL;
		delete[] inputMINUS;	
		inputMINUS = NULL;

		if (devices != NULL)
		{
			free(devices);
			devices = NULL;
		}				
					
	}

	activeTasksNumber*=2;
	}	
	return SUCCESS;
}