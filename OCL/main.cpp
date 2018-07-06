#include<CL/cl.h>

#include<iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <inttypes.h>
#define SUCCESS 0
#define FAILURE 1
#define XMAX 1.5
#define XMIN -2.5
#define YMAX 2.0
#define YMIN -2.0
#define MAXITERATIONS 20000
#define BMP_NAME "./MandleBrot"
#define WIDTH 8192
#define HEIGHT 8192


cl_double PWIDTH = ((XMAX - XMIN) / WIDTH);
cl_double PHEIGHT = ((YMAX - YMIN) / HEIGHT);

void getTime(int(*pFunc)(char* type), char* type)
{
	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	pFunc(type);
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<long double> time_span = std::chrono::duration_cast<std::chrono::seconds>(t2 - t1);
	std::cout << time_span.count() << " seconds " << type << std::endl;

}


int convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if (f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size + 1];
		if (!str)
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
	std::cout << "Error: failed to open file\n:" << filename << std::endl;
	return FAILURE;
}

void writeBMP(cl_char* dataR, cl_char* dataG, cl_char* dataB, char* type)
{
	std::cout << "Start Creating File" << std::endl;
	uint32_t headers[13];
	FILE* outfile;
	int32_t extrabytes;
	int32_t paddedsize;
	int32_t x; int32_t y; int32_t n;


	extrabytes = 4 - ((WIDTH * 3) % 4);                 // How many bytes of padding to add to each
														// horizontal line - the size of which must
														// be a multiple of 4 bytes.
	if (extrabytes == 4)
		extrabytes = 0;

	paddedsize = ((WIDTH * 3) + extrabytes) * HEIGHT;

	// Headers...
	// Note that the "BM" identifier in bytes 0 and 1 is NOT included in these "headers".

	headers[0] = paddedsize + 54;      // bfSize (whole file size)
	headers[1] = 0;                    // bfReserved (both)
	headers[2] = 54;                   // bfOffbits
	headers[3] = 40;                   // biSize
	headers[4] = WIDTH;  // biWidth
	headers[5] = HEIGHT; // biHeight

						 // Would have biPlanes and biBitCount in position 6, but they're shorts.
						 // It's easier to write them out separately (see below) than pretend
						 // they're a single int, especially with endian issues...

	headers[7] = 0;                    // biCompression
	headers[8] = paddedsize;           // biSizeImage
	headers[9] = 0;                    // biXPelsPerMeter
	headers[10] = 0;                    // biYPelsPerMeter
	headers[11] = 0;                    // biClrUsed
	headers[12] = 0;                    // biClrImportant

	char name[100];
	strcpy(name, BMP_NAME);
	strcat(name, "_");
	strcat(name, type);
	strcat(name, ".bmp");
	outfile = fopen(name, "wb");

	//
	// Headers begin...
	// When printing ints and shorts, we write out 1 character at a time to avoid endian issues.
	//

	fprintf(outfile, "BM");

	for (n = 0; n <= 5; n++)
	{
		fprintf(outfile, "%c", headers[n] & 0x000000FF);
		fprintf(outfile, "%c", (headers[n] & 0x0000FF00) >> 8);
		fprintf(outfile, "%c", (headers[n] & 0x00FF0000) >> 16);
		fprintf(outfile, "%c", (headers[n] & (unsigned int)0xFF000000) >> 24);
	}

	// These next 4 characters are for the biPlanes and biBitCount fields.

	fprintf(outfile, "%c", 1);
	fprintf(outfile, "%c", 0);
	fprintf(outfile, "%c", 24);
	fprintf(outfile, "%c", 0);

	for (n = 7; n <= 12; n++)
	{
		fprintf(outfile, "%c", headers[n] & 0x000000FF);
		fprintf(outfile, "%c", (headers[n] & 0x0000FF00) >> 8);
		fprintf(outfile, "%c", (headers[n] & 0x00FF0000) >> 16);
		fprintf(outfile, "%c", (headers[n] & (unsigned int)0xFF000000) >> 24);
	}

	//
	// Headers done, now write the data...
	//

	for (y = (int)WIDTH - 1; y >= 0; y--)     // BMP image format is written from bottom to top...
	{

		for (x = 0; x <= HEIGHT - 1; x++)
		{
			unsigned char t[] = { 0,0,0 };
			t[0] = dataR[y*WIDTH + x];
			t[1] = dataG[y*WIDTH + x];
			t[2] = dataB[y*WIDTH + x];

			fwrite(t, 1, 3, outfile);
			//fprintf(outfile, "%c", t);
		}
		if (extrabytes)      // See above - BMP lines must be of lengths divisible by 4.
		{
			for (n = 1; n <= extrabytes; n++)
			{
				fprintf(outfile, "%c", 0);
			}
		}

	}

	fclose(outfile);
	std::cout << "Finish Creating File" << std::endl;
	return;
}
int createMandle(char* type);
cl_device_id* getDev(char* type, cl_int& status, cl_platform_id& platform)
{
	cl_uint numDevices = 0;
	cl_device_id *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);

	if (numDevices == 0 || type == "CPU")
	{
		std::cout << "No GPU device available." << std::endl;
		std::cout << "Choose CPU as default device." << std::endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);

	}
	else
	{
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}
	return devices;

}
int main()
{
	//getTime(createMandle, "CPU");
	getTime(createMandle, "GPU");
	return SUCCESS;

}
int createMandle(char* type)
{
	cl_int error; cl_build_status status_s;
	cl_uint numPlatforms;
	cl_platform_id platform = 0;
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		std::cout << "Error: Getting platforms!" << std::endl;
		return FAILURE;
	}
	if (numPlatforms > 0)
	{
		cl_platform_id* platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
		platform = platforms[0];
		free(platforms);
	}
	cl_device_id *devices;

	devices = getDev(type, status, platform);
	cl_device_id selected = devices[1];
	char buffer[10000];
	cl_uint compute;
	clGetDeviceInfo(selected, CL_DEVICE_TYPE, sizeof(buffer), buffer, NULL);
	std::cout << buffer << std::endl;
	clGetDeviceInfo(selected, CL_DEVICE_NAME, sizeof(buffer), buffer, NULL);
	std::cout << buffer << std::endl;
	clGetDeviceInfo(selected, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute), &compute, NULL);
	std::cout << compute << std::endl;

	cl_context context = clCreateContext(NULL, 1, &selected, NULL, NULL, &error);
	std::cout << error<< std::endl;
	cl_command_queue commandQueue = clCreateCommandQueue(context, selected, 0, &error);
	std::cout << error<< std::endl;
	const char *filename = "Mandle.cl";
	std::string sourceStr;
	status = convertToString(filename, sourceStr);
	if (status == FAILURE)
		return FAILURE;
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = { strlen(source) };
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);
	error = clBuildProgram(program, 1, &selected, NULL, NULL, NULL);
	if (error != CL_SUCCESS) {

		// check build error and build status first
		clGetProgramBuildInfo(program, selected, CL_PROGRAM_BUILD_STATUS,
			sizeof(cl_build_status), &status_s, NULL);
		size_t logSize;
		char*programLog;
		// check build log
		clGetProgramBuildInfo(program, selected, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
		programLog = (char*)calloc(logSize + 1, sizeof(char));
		clGetProgramBuildInfo(program, selected, CL_PROGRAM_BUILD_LOG, logSize + 1, programLog, NULL);
		printf("Build failed; error=%d, status=%d, programLog:nn%s",
			error, status_s, programLog);
		std::cout << programLog <<std::endl;
		free(programLog);

	}
	cl_char *outR = (cl_char*)malloc((WIDTH*HEIGHT));
	cl_char *outG = (cl_char*)malloc((WIDTH*HEIGHT));
	cl_char *outB = (cl_char*)malloc((WIDTH*HEIGHT));

	cl_mem outR_Buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, (WIDTH*HEIGHT) * sizeof(char), NULL, &error);
	std::cout << error << std::endl;
	cl_mem outG_Buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, (WIDTH*HEIGHT) * sizeof(char), NULL, &error);
	std::cout << error << std::endl;
	cl_mem outB_Buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, (WIDTH*HEIGHT) * sizeof(char), NULL, &error);
	std::cout << error << std::endl;

	cl_kernel kernel = clCreateKernel(program, "mandle", &error);
	std::cout << status << std::endl;

	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&outR_Buffer);
	std::cout << status << std::endl;
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&outG_Buffer);
	std::cout << status << std::endl;
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&outB_Buffer);
	std::cout << status << std::endl;
	
	size_t const globalWorkSize[2] = { WIDTH,HEIGHT };
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	std::cout << status << std::endl;
	status = clFinish(commandQueue);
	std::cout << status << std::endl;


	status = clEnqueueReadBuffer(commandQueue, outR_Buffer, CL_TRUE, 0,(WIDTH*HEIGHT) * sizeof(char), outR, 0, NULL, NULL);
	std::cout << status << std::endl;

	status = clEnqueueReadBuffer(commandQueue, outG_Buffer, CL_TRUE, 0,(WIDTH*HEIGHT) * sizeof(char), outG, 0, NULL, NULL);
	std::cout << status << std::endl;

	status = clEnqueueReadBuffer(commandQueue, outB_Buffer, CL_TRUE, 0,(WIDTH*HEIGHT) * sizeof(char), outB, 0, NULL, NULL);
	std::cout << status << std::endl;

	writeBMP(outR, outG, outB, type);
	status = clReleaseKernel(kernel);				//Release kernel.
	std::cout << status << std::endl;

	status = clReleaseProgram(program);				//Release the program object.
	std::cout << status << std::endl;

	status = clReleaseMemObject(outR_Buffer);		//Release mem object.
	std::cout << status << std::endl;

	status = clReleaseMemObject(outG_Buffer);
	std::cout << status << std::endl;

	status = clReleaseMemObject(outB_Buffer);
	std::cout << status << std::endl;

	status = clReleaseCommandQueue(commandQueue);	//Release  Command queue.
	std::cout << status << std::endl;

	status = clReleaseContext(context);				//Release context.
	std::cout << status << std::endl;

	if (outR != NULL)
	{
		free(outR);
		outR = NULL;
	}
	if (outG != NULL)
	{
		free(outG);
		outG = NULL;
	}
	if (outB != NULL)
	{
		free(outB);
		outB = NULL;
	}

	if (devices != NULL)
	{
		free(devices);
		devices = NULL;
	}

	std::cout << "Passed!\n";
	return SUCCESS;
}
