#include <utility>
#include <CL/cl.hpp>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>

const size_t xpoints = 10; // число разбиений по пространству

void SetToZero(float *u)
{
	for (int i = 0; i < xpoints; i++) u[i] = 0;
}

/* проверка ошибок */
inline void checkErr(cl_int err, const char * name) {
	if (err != CL_SUCCESS) {
		std::cerr << "ERROR: " << name << " (" << err << ")" << std::endl;
		exit(EXIT_FAILURE);
	}
}

int main(void){
	cl_int err; // переменная для кодов ошибок

	std::vector< cl::Platform > platformList;
	cl::Platform::get(&platformList);
	checkErr(platformList.size() != 0 ? CL_SUCCESS : -1, "cl::Platform::get");
	std::cerr << "Platform number is: " << platformList.size() << std::endl;

	std::string platformVendor;
	platformList[0].getInfo((cl_platform_info)CL_PLATFORM_VENDOR, &platformVendor); // выбор платформы
	std::cerr << "Current platform is by: " << platformVendor << std::endl;

	std::vector< cl::Device > deviceList;
	std::string deviceVendor;
	platformList[0].getDevices(CL_DEVICE_TYPE_ALL, &deviceList); // выбор устройства
	std::cerr << "Device number is: " << deviceList.size() << std::endl;
	deviceList[0].getInfo(CL_DEVICE_NAME, &deviceVendor);
	std::cerr << "Current device name is: " << deviceVendor << std::endl;

	/* здесь указываем платформу и тип устройства для контекста */
	cl_context_properties cprops[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platformList[0])(), 0 };
	cl::Context context(CL_DEVICE_TYPE_CPU, cprops, NULL, NULL, &err);
	checkErr(err, "Context::Context()");

	/* ассоциирование устройств с контекстом */
	std::vector<cl::Device> devices;
	devices = context.getInfo<CL_CONTEXT_DEVICES>();
	checkErr(devices.size() > 0 ? CL_SUCCESS : -1, "devices.size() > 0");

	/* создание буферов */
	float * inputPlus = new float[xpoints]; // double?
	SetToZero(inputPlus);
	for (int i = 0; i < xpoints; i++) std::cout << i << " " << inputPlus[i] << std::endl;
	cl::Buffer inputPlusBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(float), inputPlus, &err);
	checkErr(err, "Buffer::Buffer()");

	float * outputPlus = new float[xpoints]; // double?
	cl::Buffer outputPlusBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(float), outputPlus, &err);
	checkErr(err, "Buffer::Buffer()");

	/* создание объекта программы */
	std::ifstream file("kernel.cl");
	checkErr(file.is_open() ? CL_SUCCESS : -1, "kernel.cl");
	std::string prog(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
	cl::Program::Sources source(1, std::make_pair(prog.c_str(), prog.length() + 1));
	cl::Program program(context, source);
	err = program.build(devices, "");
	std::cout << program.getBuildInfo <CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
	checkErr(err, "Program::build()");
	
	/* создание ядра и задание аргументов */
	cl::Kernel kernel(program, "solver", &err);
	checkErr(err, "Kernel::Kernel()");
	err = kernel.setArg(0, inputPlusBuffer);
	checkErr(err, "Kernel::setArg()");
	err = kernel.setArg(1, outputPlusBuffer);
	checkErr(err, "Kernel::setArg()");
	err = kernel.setArg(2, (int) xpoints); // костыль с (int), но вроде нормально
	checkErr(err, "Kernel::setArg()");

	/* создание очереди и отправка ядра */
	cl::CommandQueue queue(context, devices[0], 0, &err);
	checkErr(err, "CommandQueue::CommandQueue()");
	cl::Event event;
	err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(xpoints), cl::NullRange, NULL, &event);
	checkErr(err, "ComamndQueue::enqueueNDRangeKernel()");

	/* чтение буфера */
	event.wait();
	err = queue.enqueueReadBuffer(outputPlusBuffer, CL_TRUE, 0, xpoints * sizeof(float), outputPlus);
	checkErr(err, "ComamndQueue::enqueueReadBuffer()");

	std::cout << "Result:" << std::endl;
	for (int i = 0; i < xpoints; i++)
	{
		std::cout << i << " " << outputPlus[i] << std::endl;
	}
	
	return EXIT_SUCCESS;
}