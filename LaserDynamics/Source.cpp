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
const float tmax = 10E-4f;
const float dt = 50E-5f; // можно вычислять по соотношению из численной схемы
const int tpoints = tmax / dt; // потери данных; для шага по времени, возможно, некритично

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

	std::cout << "inputPlus: " << std::endl;
	for (int i = 0; i < xpoints; i++) std::cout << i << " " << inputPlus[i] << std::endl;

	cl::Buffer inputPlusBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(float), inputPlus, &err);
	checkErr(err, "Buffer::Buffer()");

	float * inputMinus = new float[xpoints]; // double?
	SetToZero(inputMinus);

	std::cout << "inputMinus: " << std::endl;
	for (int i = 0; i < xpoints; i++) std::cout << i << " " << inputMinus[i] << std::endl;

	cl::Buffer inputMinusBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(float), inputMinus, &err);
	checkErr(err, "Buffer::Buffer()");

	float * outputPlus = new float[xpoints]; // double?
	cl::Buffer outputPlusBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(float), outputPlus, &err);
	checkErr(err, "Buffer::Buffer()");

	float * outputMinus = new float[xpoints]; // double?
	cl::Buffer outputMinusBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(float), outputMinus, &err);
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
	err = kernel.setArg(2, inputMinusBuffer);
	checkErr(err, "Kernel::setArg()");
	err = kernel.setArg(3, outputMinusBuffer);
	checkErr(err, "Kernel::setArg()");
	err = kernel.setArg(4, (int)xpoints); // костыль с (int), но вроде нормально
	checkErr(err, "Kernel::setArg()");

	/* создание очереди */
	cl::CommandQueue queue(context, devices[0], 0, &err);
	checkErr(err, "CommandQueue::CommandQueue()");
	cl::Event event;

	/* вычисления */
	for (int i = 0; i < tpoints; i++)
	{
		err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(xpoints), cl::NullRange, NULL, &event);
		checkErr(err, "ComamndQueue::enqueueNDRangeKernel()");

		/* чтение буферов */
		event.wait();
		err = queue.enqueueReadBuffer(outputPlusBuffer, CL_TRUE, 0, xpoints * sizeof(float), outputPlus);
		checkErr(err, "CommandQueue::enqueueReadBuffer()");
		err = queue.enqueueReadBuffer(outputMinusBuffer, CL_TRUE, 0, xpoints * sizeof(float), outputMinus);
		checkErr(err, "CommandQueue::enqueueReadBuffer()");
		
		std::cout << "Result; iteration #" << i << std::endl;
		std::cout << "Plus:" << std::endl;
		for (int j = 0; j < xpoints; j++)
		{
			std::cout << outputPlus[j] << " ";
		}
		std::cout << std::endl << "Minus " << std::endl;
		for (int j = 0; j < xpoints; j++)
		{
			std::cout << outputMinus[j] << " ";
		}
		std::cout << std::endl;

		/* переброс указателей */
		std::swap(inputPlus, outputPlus);
		std::swap(inputMinus, outputMinus);

		/* перезапись входных буферов */
		err = queue.enqueueWriteBuffer(inputPlusBuffer, CL_TRUE, 0, xpoints * sizeof(float), inputPlus);
		checkErr(err, "CommandQueue::enqueueWriteBuffer()");
		err = queue.enqueueWriteBuffer(inputMinusBuffer, CL_TRUE, 0, xpoints * sizeof(float), inputMinus);
		checkErr(err, "CommandQueue::enqueueWriteBuffer()");
	}

	return EXIT_SUCCESS;
}