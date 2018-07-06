#define XMAX 1.5f
#define XMIN -2.5f
#define YMAX 2.0f
#define YMIN -2.0f
#define MAXITERATIONS 2000
#define WIDTH 8192
#define HEIGHT 8192

__kernel void mandle(__global char* outR,__global char* outG,__global char* outB)
{

	int idx = get_global_id (0);
	int idy = get_global_id (1);

	float cx = (XMIN + idx * ((XMAX - XMIN) / WIDTH));
	float cy = (YMIN + idy * ((YMAX - YMIN) /  HEIGHT));

	int iter = 0;
	if (cy < 0)
		cy *= -1;
	if (cy < ((YMAX - YMIN) / HEIGHT) / 2)
		cy = 0;

	float zx = 0.0;
	float zy = 0.0;
	float z2x = zx*zx;
	float z2y = zy*zy;
	while (iter < MAXITERATIONS && ((z2x + z2y) < 4)) {

		zy = 2 * zx*zy + cy;
		zx = z2x - z2y + cx;
		z2x = zx*zx;
		z2y = zy*zy;
		iter++;
	}
	outR[idy* WIDTH + idx] =1;
	outG[idy* WIDTH + idx] =1;
	outB[idy* WIDTH + idx] =1;

	if (iter < MAXITERATIONS)
	{
		outR[idy* WIDTH + idx] =255;
		outG[idy* WIDTH + idx] =255;
		outB[idy* WIDTH + idx] =255;
	}

}
