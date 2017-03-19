#include <utility>
#include <CL/cl.hpp>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>

const size_t xpoints = 200; // число разбиений по пространству
const double tmax = 10E-4f;
const double dt = 10E-5f; // можно вычислять по соотношению из численной схемы
const int tpoints = tmax / dt; // потери данных; для шага по времени, возможно, некритично

void SetInitialCondition(double *u)
{
	for (int i = 0; i < xpoints; i++)
	{
		if (i < 1 || i > 2) u[i] = 0;
		else u[i] = 1;
	}
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
	double * inputPlus = new double[xpoints];
	SetInitialCondition(inputPlus);

	std::cout << "inputPlus: " << std::endl;
	for (int i = 0; i < xpoints; i++) std::cout << i << " " << inputPlus[i] << std::endl;

	cl::Buffer inputPlusBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(double), inputPlus, &err);
	checkErr(err, "Buffer::Buffer()");

	double * inputMinus = new double[xpoints];
	SetInitialCondition(inputMinus);

	std::cout << "inputMinus: " << std::endl;
	for (int i = 0; i < xpoints; i++) std::cout << i << " " << inputMinus[i] << std::endl;

	cl::Buffer inputMinusBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(double), inputMinus, &err);
	checkErr(err, "Buffer::Buffer()");

	double * outputPlus = new double[xpoints];
	cl::Buffer outputPlusBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(double), outputPlus, &err);
	checkErr(err, "Buffer::Buffer()");

	double * outputMinus = new double[xpoints];
	cl::Buffer outputMinusBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, xpoints * sizeof(double), outputMinus, &err);
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

	/* создание очереди */
	cl::CommandQueue queue(context, devices[0], 0, &err);
	checkErr(err, "CommandQueue::CommandQueue()");
	cl::Event event;

	FILE* gnuplotPipe = _popen("D:/gnuplot/bin/gnuplot.exe", "w");
	fprintf(gnuplotPipe, "set grid\n");
	if (gnuplotPipe)
	{
		fprintf(gnuplotPipe, "set object 1 rectangle from screen 0,0 to screen 1,1 fillcolor rgb\"white\" behind\n"); // заливка цветом(белым)
		fprintf(gnuplotPipe, "set xrange [0:200]\n"); // интервал по x
		fprintf(gnuplotPipe, "set yrange [0:1]\n"); // интервал по y

		fprintf(gnuplotPipe, "plot [][0:1] 2\n"); //отображение окна
		printf("Press enter when window will appear");
		fflush(gnuplotPipe);  //очистка

		/* вычисления */
		for (int i = 0; i < 1000/*tpoints*/; i++)
		{
			err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(4), cl::NDRange(4), NULL, &event);
			checkErr(err, "ComamndQueue::enqueueNDRangeKernel()");

			/* чтение буферов */
			event.wait();
			err = queue.enqueueReadBuffer(outputPlusBuffer, CL_TRUE, 0, xpoints * sizeof(double), outputPlus);
			checkErr(err, "CommandQueue::enqueueReadBuffer()");
			err = queue.enqueueReadBuffer(outputMinusBuffer, CL_TRUE, 0, xpoints * sizeof(double), outputMinus);
			checkErr(err, "CommandQueue::enqueueReadBuffer()");
			fprintf(gnuplotPipe, "plot '-' using 1:2 with points lc rgb 'black' pt 7\n");
			for (int i = 0; i < xpoints; i++)
			{
				fprintf(gnuplotPipe, "%d, %lf\n", i, outputPlus[i]);
			}
			fprintf(gnuplotPipe, "e\n");
			fflush(gnuplotPipe);
			/*std::cout << "Result; iteration #" << i << std::endl;
			std::cout << "Plus:" << std::endl;
			for (int j = 0; j < xpoints; j++)
			{
			std::cout << outputPlus[j] << " ";
			}*/
			/*std::cout << std::endl << "Minus " << std::endl;
			for (int j = 0; j < xpoints; j++)
			{
			std::cout << outputMinus[j] << " ";
			}*/
			std::cout << std::endl;

			/* переброс указателей */
			std::swap(inputPlus, outputPlus);
			std::swap(inputMinus, outputMinus);

			/* перезапись входных буферов */
			err = queue.enqueueWriteBuffer(inputPlusBuffer, CL_TRUE, 0, xpoints * sizeof(double), inputPlus);
			checkErr(err, "CommandQueue::enqueueWriteBuffer()");
			err = queue.enqueueWriteBuffer(inputMinusBuffer, CL_TRUE, 0, xpoints * sizeof(double), inputMinus);
			checkErr(err, "CommandQueue::enqueueWriteBuffer()");
		}
		printf("press enter to continue...");
		getchar();

		fprintf(gnuplotPipe, "exit\n");
	}
	return EXIT_SUCCESS;
}