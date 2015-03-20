#include <iostream>
#include <string>
#include <getopt.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include "frangi.h"
using namespace std;
using namespace cv;

void showHelp(){
	cerr << "Usage: frangi-bin [OPTION]... [FILE]" << endl
		<< "Frangi vesselness filter. Only image files supported by OpenCV are supported." << endl
		<< "--startSigma=START_SIGMA\t\t\t Start vessel scale. Default: " << DEFAULT_SIGMA_START << endl
		<< "--stepSigma=STEP_SIGMA\t\t\t\t Vessel scale increment. Default: " << DEFAULT_SIGMA_END << endl
		<< "--endSigma=END_SIGMA\t\t\t\t End vessel scale. Default: " << DEFAULT_SIGMA_END << endl
		<< "--blob-suppression=SUPP\t\t\t\t Blob suppression (BetaOne). Default: " << DEFAULT_BETA_ONE << endl
		<< "--background-suppression=SUPP\t\t\t Background suppression (BetaTwo). Default: " << DEFAULT_BETA_TWO << endl
		<< "--enhance-white\t\t\t\t\t Enhance white structures (default: enhance black structures)." <<  endl
		<< "--output=FILE\t\t\t\t\t Specify output file. Defaults to [input file]_frangi.png." << endl
		<< "--help\t\t\t\t\t\t Show help." << endl;
}
void createOptions(option **options){
	int numOptions = 9;
	*options = new option[numOptions];

	(*options)[0].name = "startSigma";
	(*options)[0].has_arg = required_argument;
	(*options)[0].flag = NULL;
	(*options)[0].val = 0;

	(*options)[1].name = "endSigma";
	(*options)[1].has_arg = required_argument;
	(*options)[1].flag = NULL;
	(*options)[1].val = 1;
	
	(*options)[2].name = "stepSigma";
	(*options)[2].has_arg = required_argument;
	(*options)[2].flag = NULL;
	(*options)[2].val = 2;
	
	(*options)[3].name = "blob-suppression";
	(*options)[3].has_arg = required_argument;
	(*options)[3].flag = NULL;
	(*options)[3].val = 3;
	
	(*options)[4].name = "background-suppression";
	(*options)[4].has_arg = required_argument;
	(*options)[4].flag = NULL;
	(*options)[4].val = 4;
	
	(*options)[5].name = "help";
	(*options)[5].has_arg = no_argument;
	(*options)[5].flag = NULL;
	(*options)[5].val = 5;
	
	(*options)[6].name = "output";
	(*options)[6].has_arg = required_argument;
	(*options)[6].flag = NULL;
	(*options)[6].val = 6;
	
	(*options)[7].name = "enhance-white";
	(*options)[7].has_arg = no_argument;
	(*options)[7].flag = NULL;
	(*options)[7].val = 7;
	
	(*options)[8].name = 0;
	(*options)[8].has_arg = 0;
	(*options)[8].flag = 0;
	(*options)[8].val = 0;

}

int main(int argc, char *argv[]){
	//set default frangi opts
	frangi2d_opts_t opts;
	frangi2d_createopts(&opts);

	//process command line options
	int index;
	char shortopts[] = "";
	option *longopts;
	createOptions(&longopts);

	string outFilename;
	bool outFilenameSet = false;

	while (true){
		int flag = getopt_long(argc, argv, shortopts, longopts, &index);	
		switch (flag){
			case 0: //sigma_start 
				opts.sigma_start = strtod(optarg, NULL);
			break;

			case 1: //sigma_end
				opts.sigma_end = strtod(optarg, NULL);
			break;
			
			case 2: //sigma_step
				opts.sigma_step = strtod(optarg, NULL);
			break;

			case 3: //BetaOne (blob suppression)
				opts.BetaOne = strtod(optarg, NULL);
			break;

			case 4: //BetaTwo (background suppression)
				opts.BetaTwo = strtod(optarg, NULL);
			break;
			case 5:
				showHelp();
				exit(0);
			break;
			case 6:
				outFilename = string(optarg);
				outFilenameSet = true;
			break;
			case 7:
				opts.BlackWhite = false;
			break;
		}
		if (flag == -1){
			break;
		}
	}

	//filenames
	char *filename = argv[optind];
	if (filename == NULL){
		cerr << "Missing filename!" << endl;
		exit(1);
	}

	if (!outFilenameSet){
		outFilename = string(filename) + "_frangi";
	}
	delete [] longopts;
	
	//read image file, run frangi, output to output file
	Mat input_img = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
	Mat input_img_fl;
	input_img.convertTo(input_img_fl, CV_32FC1);
	Mat vesselness, scale, angles;
	frangi2d(input_img_fl, vesselness, scale, angles, opts);
	imwrite(outFilename + ".png", vesselness*255);
}
