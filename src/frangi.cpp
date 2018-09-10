#include "frangi.h"
#include <iostream>
#include <fstream>
using namespace std;
using namespace cv;

void frangi2d_hessian(const Mat &src, Mat &Dxx, Mat &Dxy, Mat &Dyy, float sigma){
	//construct Hessian kernels
	float M_PI = 3.14;
	int n_kern_x = 2*round(3*sigma) + 1;
	int n_kern_y = n_kern_x;
	float *kern_xx_f = new float[n_kern_x*n_kern_y]();
	float *kern_xy_f = new float[n_kern_x*n_kern_y]();
	float *kern_yy_f = new float[n_kern_x*n_kern_y]();
	int i=0, j=0;
	for (int x = -round(3*sigma); x <= round(3*sigma); x++){
		j=0;
		for (int y = -round(3*sigma); y <= round(3*sigma); y++){
			kern_xx_f[i*n_kern_y + j] = 1.0f/(2.0f*M_PI*sigma*sigma*sigma*sigma) * (x*x/(sigma*sigma) - 1) * exp(-(x*x + y*y)/(2.0f*sigma*sigma));
			kern_xy_f[i*n_kern_y + j] = 1.0f/(2.0f*M_PI*sigma*sigma*sigma*sigma*sigma*sigma)*(x*y)*exp(-(x*x + y*y)/(2.0f*sigma*sigma));
			j++;
		}
		i++;
	}
	for (int j=0; j < n_kern_y; j++){
		for (int i=0; i < n_kern_x; i++){
			kern_yy_f[j*n_kern_x + i] = kern_xx_f[i*n_kern_x + j];
		}
	}

	//flip kernels since kernels aren't symmetric and opencv's filter2D operation performs a correlation, not a convolution
	Mat kern_xx;
	flip(Mat(n_kern_y, n_kern_x, CV_32FC1, kern_xx_f), kern_xx, -1);
	
	Mat kern_xy;
	flip(Mat(n_kern_y, n_kern_x, CV_32FC1, kern_xy_f), kern_xy, -1);

	Mat kern_yy;
	flip(Mat(n_kern_y, n_kern_x, CV_32FC1, kern_yy_f), kern_yy, -1);

	//specify anchor since we are to perform a convolution, not a correlation
	Point anchor(n_kern_x - n_kern_x/2 - 1, n_kern_y - n_kern_y/2 - 1);

	//run image filter
	filter2D(src, Dxx, -1, kern_xx, anchor);
	filter2D(src, Dxy, -1, kern_xy, anchor);
	filter2D(src, Dyy, -1, kern_yy, anchor);


	delete [] kern_xx_f;
	delete [] kern_xy_f;
	delete [] kern_yy_f;
}

void frangi2d_createopts(frangi2d_opts_t *opts){
	//these parameters depend on the scale of the vessel, depending ultimately on the image size...
	opts->sigma_start = DEFAULT_SIGMA_START;
	opts->sigma_end = DEFAULT_SIGMA_END;
	opts->sigma_step = DEFAULT_SIGMA_STEP;

	opts->BetaOne = DEFAULT_BETA_ONE; //ignore blob-like structures?
	opts->BetaTwo = DEFAULT_BETA_TWO; //appropriate background suppression for this specific image, but can change. 

	opts->BlackWhite = DEFAULT_BLACKWHITE; 
}
		
void frangi2_eig2image(const Mat &Dxx, const Mat &Dxy, const Mat &Dyy, Mat &lambda1, Mat &lambda2, Mat &Ix, Mat &Iy){
	//calculate eigenvectors of J, v1 and v2
	Mat tmp, tmp2;
	tmp2 = Dxx - Dyy;
	sqrt(tmp2.mul(tmp2) + 4*Dxy.mul(Dxy), tmp);
	Mat v2x = 2*Dxy;
	Mat v2y = Dyy - Dxx + tmp;

	//normalize
	Mat mag;
	sqrt((v2x.mul(v2x) + v2y.mul(v2y)), mag);
	Mat v2xtmp = v2x.mul(1.0f/mag);
	v2xtmp.copyTo(v2x, mag != 0);
	Mat v2ytmp = v2y.mul(1.0f/mag);
	v2ytmp.copyTo(v2y, mag != 0);

	//eigenvectors are orthogonal
	Mat v1x, v1y;
	v2y.copyTo(v1x);
	v1x = -1*v1x;
	v2x.copyTo(v1y);

	//compute eigenvalues
	Mat mu1 = 0.5*(Dxx + Dyy + tmp);
	Mat mu2 = 0.5*(Dxx + Dyy - tmp);

	//sort eigenvalues by absolute value abs(Lambda1) < abs(Lamda2)
	Mat check = abs(mu1) > abs(mu2);
	mu1.copyTo(lambda1); mu2.copyTo(lambda1, check);
	mu2.copyTo(lambda2); mu1.copyTo(lambda2, check);

	v1x.copyTo(Ix); v2x.copyTo(Ix, check);
	v1y.copyTo(Iy); v2y.copyTo(Iy, check);
	
}


void frangi2d(const Mat &src, Mat &maxVals, Mat &whatScale, Mat &outAngles, frangi2d_opts_t opts){
	vector<Mat> ALLfiltered;
	vector<Mat> ALLangles;
	float beta = 2*opts.BetaOne*opts.BetaOne;
	float c = 2*opts.BetaTwo*opts.BetaTwo;

	for (float sigma = opts.sigma_start; sigma <= opts.sigma_end; sigma += opts.sigma_step){
		//create 2D hessians
		Mat Dxx, Dyy, Dxy;
		frangi2d_hessian(src, Dxx, Dxy, Dyy, sigma);

		//correct for scale
		Dxx = Dxx*sigma*sigma;
		Dyy = Dyy*sigma*sigma;
		Dxy = Dxy*sigma*sigma;
	
		//calculate (abs sorted) eigenvalues and vectors
		Mat lambda1, lambda2, Ix, Iy;
		frangi2_eig2image(Dxx, Dxy, Dyy, lambda1, lambda2, Ix, Iy);
		
		//compute direction of the minor eigenvector
		Mat angles;
		phase(Ix, Iy, angles);
		ALLangles.push_back(angles);
		
		//compute some similarity measures
		lambda2.setTo(nextafterf(0, 1), lambda2 == 0);
		Mat Rb = lambda1.mul(1.0/lambda2);
		Rb = Rb.mul(Rb);
		Mat S2 = lambda1.mul(lambda1) + lambda2.mul(lambda2);

		//compute output image
		Mat tmp1, tmp2;
		exp(-Rb/beta, tmp1);
		exp(-S2/c, tmp2);
	
		Mat Ifiltered = tmp1.mul(Mat::ones(src.rows, src.cols, src.type()) - tmp2);
		if (opts.BlackWhite){
			Ifiltered.setTo(0, lambda2 < 0);
		} else {
			Ifiltered.setTo(0, lambda2 > 0);
		}

		//store results
		ALLfiltered.push_back(Ifiltered);
	}

	float sigma = opts.sigma_start;
	ALLfiltered[0].copyTo(maxVals);
	ALLfiltered[0].copyTo(whatScale);
	ALLfiltered[0].copyTo(outAngles);
	whatScale.setTo(sigma);

	//find element-wise maximum across all accumulated filter results
	for (int i=1; i < ALLfiltered.size(); i++){
		maxVals = max(maxVals, ALLfiltered[i]);
		whatScale.setTo(sigma, ALLfiltered[i] == maxVals);
		ALLangles[i].copyTo(outAngles, ALLfiltered[i] == maxVals);
		sigma += opts.sigma_step;
	}
}
