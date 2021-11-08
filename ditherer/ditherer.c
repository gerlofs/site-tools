#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <unistd.h> // Getopt.

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

/*    DITHERER:
*	Quantize image to black and white then dither. Recolour dependent
*	on input argument (center colour, a gradient is generated between 
*	a top and lower bound with this in the center).
*/

// TODO:
// 1. Alter input sequence to stop invalid filetypes from being entered and
// 	causing segfaults, check info before opening and check x, y, n.
// 2. Clean up code.
// 3. Add two-tone option.

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define AVGPX(a, b, c, n) (((a)+(b)+(c))/(n))
#define CONVA 0.0039215
#define CONVB 0.0000175
#define CONVR(v) (CONVA * (v) + CONVB)
#define CONVU(v) ((v) * 255)
#define SRGBLOWER 0.04045
#define LINEARLOWER 0.0031308

typedef struct img {
	unsigned char *data;
	float *fdata;
	uint64_t length;
	uint8_t channels;
} image;

uint16_t bayer_matrix_l4[8][8] = {
	{0, 32, 8, 40, 2, 34, 10, 42},
	{48, 16, 56, 24, 50, 18, 58, 26},
	{12, 44, 4, 36, 14, 46, 6, 38},
	{60, 28, 52,20, 62, 30, 54, 22}, 
	{3, 35, 11, 43, 1, 33, 9, 41},
	{51, 19, 59, 27, 49, 17, 57, 25}, 
	{15, 47, 7, 39, 13, 45, 5, 37},
	{63, 31, 55, 23, 61, 29, 53, 21}
};

float normalised_matrix_l4[8][8] = {
	{ 0.000000, 0.500000, 0.125000, 0.625000, 
	  0.031250, 0.531250, 0.156250, 0.656250 },
	{ 0.750000, 0.250000, 0.875000, 0.375000, 
	  0.781250, 0.281250, 0.906250, 0.406250 },
	{ 0.187500, 0.687500, 0.062500, 0.562500, 
	  0.218750, 0.718750, 0.093750, 0.593750 },
	{ 0.937500, 0.437500, 0.812500, 0.312500, 
	  0.968750, 0.468750, 0.843750, 0.343750 },
	{ 0.046875, 0.546875, 0.171875, 0.671875,
	  0.015625, 0.515625, 0.140625, 0.640625 },
	{ 0.796875, 0.296875, 0.921875, 0.421875,
	  0.765625, 0.265625, 0.890625, 0.390625 },
	{ 0.234375, 0.734375, 0.109375, 0.609375,
	  0.203125, 0.703125, 0.078125, 0.578125 },
	{ 0.984375, 0.484375, 0.859375, 0.359375,
	  0.953125, 0.453125, 0.828125, 0.328125 }
};

uint16_t bayer_matrix_l2[4][4] = {
	{ 0,	8,	2,	10 },
	{ 12,	4,	14,	6  },
	{ 3,	11,	1,	9  },
	{ 15,	7,	13,	5  }
};

float normalised_matrix_l2[4][4] = {
	{ 0.000000, 0.500000, 0.125000, 0.625000 },
	{ 0.750000, 0.250000, 0.875000, 0.375000 },
	{ 0.187500, 0.687500, 0.062500, 0.562500 },
	{ 0.937500, 0.437500, 0.812500, 0.312500 }
};

void normalise_matrix()
{
	uint16_t m = 4;
	for ( uint16_t i = 0; i < m; i++ )
	{
		for ( uint16_t j = 0; j < m; j++ )
		{
			normalised_matrix_l2[i][j] 
				= bayer_matrix_l2[i][j] / pow(m, 2);
			printf("%f ", normalised_matrix_l4[i][j]);
		}
		printf("\n");
	}
}

image convert_to_greyscale(unsigned char *data, 
				unsigned channels,
				unsigned imgsize)
{

	image grey;
	
	if ( channels == 1 ) 
	{
		grey.data = NULL;
		return grey;
	}
	
	uint8_t greychannels = (channels > 3) ? 2 : 1;
	unsigned char *greydata = malloc((imgsize/channels)*greychannels);
	if ( greydata == NULL ) exit(1);
	unsigned char *rp = data;
	unsigned char *wp = greydata;
	uint64_t greysize = 0;

	for (; rp < (data + imgsize); 
		rp+=channels, wp+=greychannels, greysize+=greychannels)
	{
		*wp = AVGPX(*rp, *(rp+1), *(rp+2), 3.0);
		if ( greychannels > 1 ) 
		{
			*(wp+(greychannels-1)) = *(rp+(channels-1));
			
		}
	}
	
	grey.data = greydata;
	grey.length = greysize;
	grey.channels = greychannels;
	printf("Returning greyscale with %u channels\n", grey.channels);
	return grey;
}

float frand()
{
	int divisor = RAND_MAX/(1000001);
	int stored;
	
	do { 
		stored = rand() / divisor;
	} while ( stored > 1000000 );

	return (float)(stored) / 1000000;
}

void add_random_noise(image *img, uint8_t is_ordered)
{
	if ( img->fdata == NULL )
	{
		fprintf(stderr, "Unable to add white noise to the image as"
				" it has not been converted to floating p"
				"oint values...\n");
		exit(1);
	}
	
	float *p = img->fdata;
	float noisified;
	for (; p != (img->fdata + img->length); p+=img->channels)
	{
		noisified = (((*p + frand()) - 0.5) > 0.5) ? 
		 		1.0 : 0.0;
		if ( is_ordered == 1 )
		{
			noisified = (noisified > (1.0 - frand())) ?
				1.0 : 0.0;
		}
		*p = noisified;
	}
}

void apply_bayer_dithering(image *img, int width, uint8_t bayer_level)
{
	if ( img->fdata == NULL )
	{
		fprintf(stderr, "Unable to apply bayer dithering to the "
				"image as it has not been converted to flo"
				"ating point values...\n");
		exit(1);
	}


	if ( bayer_level != 2 && bayer_level != 4 )
	{
		fprintf(stderr, "Only bayer matrix sizes of 2x2 and 4x4"
				" are currently available! Rerun with "
				"a bayer matrix value of 2 OR 4...\n");
		exit(1);
	}

	float *p = img->fdata;
	float *matrix = (bayer_level == 2) ? 
		&normalised_matrix_l2[0][0] 
		: &normalised_matrix_l4[0][0];
	uint8_t rows = (2*bayer_level);
	uint64_t x = 0;
	uint64_t y = 0;

	for ( uint64_t i = 0; p != (img->fdata + img->length); 
			      p+=img->channels, i++)
	{
		y = (i / width) % rows;
		x = i % rows;
		*p = ( *p > 1 - *(matrix+(x * bayer_level) + y)) ?
				1.0 : 0.0;
	}
}

void srbg_to_linear(image *img)
{
	// Iterate over image data and convert intensity based on on a
	//	generic gamma correction algorithm.
	if ( img->fdata == NULL )
	{
		fprintf(stderr, "Unable to convert brightness range as the"
				"image has not been converted to floating p"
				"oint values...\n");
		exit(1);
	}
	
	float *p = img->fdata;
	float temp;
	
	for (; p != (img->fdata + img->length); p+=img->channels)
	{
		temp = *p;
		*p =  ( temp <= SRGBLOWER ) ?
				temp / 12.92 :
				pow(((temp + 0.055)/1.055), 2.4); 
	}	
}

void convert_data_to_fp(image *img)
{
	img->fdata = malloc(sizeof(float) * img->length);
	if ( img->fdata == NULL ) exit(0);

	unsigned char *rp = img->data;
	float *wp = img->fdata;

	for (; rp != (img->data + img->length); rp+=img->channels,
						wp+=img->channels)
	{
		*wp = CONVR(*rp);
	}

}

void convert_data_to_int(image *img)
{
	unsigned char *wp = img->data;
	float *rp = img->fdata;

	for (; wp != (img->data + img->length); rp+=img->channels,
						wp+=img->channels)
	{
		*wp = CONVU(*rp);
	}
}

image tile_texture(image *img, int inpwidth, 
				int mapwidth, int maplength)
{
	image newimg;	
	newimg.length = img->length;
	newimg.channels = img->channels;
	uint64_t len = newimg.length = mapwidth * maplength;
	newimg.data = malloc(sizeof(unsigned char) * len * newimg.channels);
	if ( newimg.data == NULL ) exit(0);

	unsigned char *wp = newimg.data;
	unsigned row, remainder;
	for (unsigned i = 0; wp != (newimg.data + len); wp++, i++)
	{
		remainder = i % mapwidth;
		row = i / mapwidth;
		*wp=img->data[((row % inpwidth) * inpwidth) 
					+ (remainder % inpwidth)];	
		
	}

	return newimg;
}

void convolve_with_texture(image *map, image *img)
{
	float *rp = map->fdata;
	float *wp = img->fdata;

	for (; rp != (map->fdata + map->length); 
			rp+=img->channels,wp+=map->channels)
	{
		*wp = (*wp > *rp) ? 1.0 : 0.0;
	}
}

void build_blur_boxes(unsigned char *boxes, double sigma, int n)
{
	int wl = (int)(floor((sqrt((12*sigma*sigma/n)+1))));
	if ( wl % 2 == 0 ) wl--;
	int wu = wl+2;

	int m = round((12.0*sigma*sigma -n*wl*wl - 4.0*n*wl -3.0*n)/
			(-4.0*wl -4.0));
	
	if ( m>n || m<0 )
	{
		exit(0);
	}
	
	for (int i = 0; i < n; i++ )
	{
		boxes[i] = (i<m) ? wl : wu;
	}
}

void box_blurH(unsigned char *srcdata, unsigned char *dstdata,
		int w, int h, int r)
{
	double ia = 1.0 / (r+r+1);
	for ( int i = 0; i < h; i++ )
	{
		int ti = i*w, li = ti, ri = ti+r;
		int fv = (int)(srcdata[ti]), lv = (int)(srcdata[ti+w-1]),
			val = (r+1)*fv;
		for ( int j = 0; j < r; j++) val += (int)(srcdata[ti+j]);
		for ( int j = 0; j <= r; j++ ) 
		{
			val += (int)(srcdata[ri++]) - fv;
			dstdata[ti++] = 
				(unsigned char)(round((double)(val) * ia));
		}
		for ( int j = r+1; j < w-r; j++ )
		{
			val += (int)(srcdata[ri++] - srcdata[li++]);
			dstdata[ti++] = 
				(unsigned char)(round((double)(val) * ia));
		}
		for ( int j = w-r; j < w; j++ ) 
		{
			val += lv - (int)(srcdata[li++]);
			dstdata[ti++] = 
				(unsigned char)(round((double)(val) * ia));
		}
	}

}

void box_blurT(unsigned char *srcdata, unsigned char *dstdata,
		int w, int h, int r)
{
	double ia = 1.0 / (r+r+1);
	
	for ( int i = 0; i < w; i++ )
	{
		int ti = i, li = ti, ri = ti+r*w;
		int fv = (int)(srcdata[ti]), lv = (int)(srcdata[ti+w*(h-1)]),
			val = (r+1)*fv;
		for ( int j = 0; j < r; j++) val += (int)(srcdata[ti+j*w]);
		for ( int j = 0; j <= r; j++ ) 
		{
			val += (int)(srcdata[ri]) - fv;
			dstdata[ti] = 
				(unsigned char) round((double)(val) * ia);
			ri += w;
			ti += w;
		}
		for ( int j = r+1; j < h-r; j++ )
		{
			val += srcdata[ri] - srcdata[li];
			dstdata[ti] = 
				(unsigned char) round((double)(val) * ia);
			li += w;
			ri += w;
			ti += w;
		}
		for ( int j = h-r; j < h; j++ ) 
		{
			val += lv - srcdata[li];
			dstdata[ti++] = 
				(unsigned char) round((double)(val) * ia);
			li += w;
			ti += w;
		}
	}
}

void box_blur(unsigned char *srcdata, unsigned char *dstdata, 
		int w, int h, int r)
{
	uint64_t length = w * h;
	for(uint64_t i = 0;i<length;i++) 
	{
		dstdata[i] = srcdata[i];	
	}
	box_blurH(srcdata, dstdata, w, h, r);
	box_blurT(srcdata, dstdata, w, h, r);
}

unsigned char *gaussian_blur(unsigned char *inpdata, int w, int h, int r, 
			int blur_level)
{
	/*
	*	Blur algorithm taken (and adapted to C) from Ivan 
	*	Kutskir: http://blog.ivank.net/fastest-gaussian-blur.html.
	*/
	unsigned char *dstdata = malloc(sizeof(unsigned char) * w * h);
	if ( dstdata == NULL ) exit(0);
	unsigned char *boxes = malloc(sizeof(int) * blur_level);
	build_blur_boxes(boxes, r, blur_level);	
	box_blur(inpdata, dstdata, w, h, (boxes[0]-1)/2);
	box_blur(inpdata, dstdata, w, h, (boxes[1]-1)/2);	
	box_blur(inpdata, dstdata, w, h, (boxes[2]-1)/2);
	free(boxes);	
	return dstdata;
}

int main(int argc, char **argv)
{
	// -h, --help: Print help.
	// -b, --blur: Level of blur (radius value).
	// -m, --matrix: Bayer matrix size available options are 2 and 4.
	// -t, --type: Type of dithering to apply. Accepts an integer to 
	// 		switch dithering types: 0 equals bayer, 1 equals
	// 		blue-noise, 2 equals random noise.
	// -i, --image: Image filepath, always an abs. path.
	/* Prototype variables for blurring and dithering */
	int opt, dithering_type, matrix_select = 4, blur_level = 2;
	char *img_filepath = NULL;

	/* Check arguments... */
	char *options_available = "hb:m:t:i:";
	opterr = 0;
	while ( (opt = getopt(argc, argv, options_available)) != -1)
	{
		switch(opt)
		{
			case 'h':
				fprintf(stdout, 
				"Ditherer\nby Gerlofs\n\nSummary:\n\tDither"
				" images using either the bayer algorithm "
				"or through convolution with white or blue"
				" noise.\n\nOptions:\n\th: Print this help"
				"file.\n\tb: Blur level, radius used in ga"
				"uss bluring of blue noise.\n\tm: Bayer mat"
				"rix size to use, either 2 or 4\n\tt: Dithe"
				"ring type to use:\n\t\t0 equals bayer,"
				"\n\t\t1 equals blue noise convolution,"
				"\n\t\t2 equals white noise convolution."
				"\n\t-i: Image-to-dither absolute filepath"
				"\n");
				exit(0);
			case 'b':
				blur_level = atoi(optarg);
				break;
			case 't':
				dithering_type = atoi(optarg);
				break;
			case 'm':
				matrix_select = atoi(optarg);
				break;
			case 'i':
				img_filepath = optarg;
				break;
		}
	}

	printf("Optargs: -b %d -t %d -m %d -i %s\n", blur_level,
			dithering_type, matrix_select,
			img_filepath);
		
	/* Tile the blue noise texture */
	int x, y, w, h, c, n;
	int blur_boxes = 3;
	unsigned char *data = stbi_load(img_filepath, &x, &y, &n, 0);
	printf("Converting to greyscale...\n");

	image greyscale = convert_to_greyscale(data, n, x*y*n);
	
	if ( greyscale.data == NULL )
	{
		greyscale.data = data;
		greyscale.length = x * y * n;
		greyscale.channels = 1;
	}

	printf("Converting to floating point...\n");
	convert_data_to_fp(&greyscale);	
	
	switch(dithering_type)
	{	// Bayer algorithm
		case 0:
			printf("Applying bayer dithering...\n");
			apply_bayer_dithering(&greyscale, x, matrix_select);
			break;

		case 1: // Blue-noise.
			image texture;
			texture.data = stbi_load("bluenoise.png", &w,
						&h, &c, 0);
			texture.length = w * h * c;
			texture.channels = c;
			printf("Tiling texture...\n");
			image tiled = tile_texture(&texture, w, x, y);
			printf("Blurring data... \n");
			tiled.data = gaussian_blur(tiled.data, x, y, 
						blur_level, blur_boxes);
			convert_data_to_fp(&tiled);
			printf("Convolving... \n");
			convolve_with_texture(&tiled, &greyscale);
			free(tiled.data);
			free(tiled.fdata);
			stbi_image_free(texture.data);
			break;
		case 2: // White-noise.
			printf("Generating random noise pattern...\n");
			srand((unsigned int)time(NULL));
			add_random_noise(&greyscale, 1);

	}
	
	srbg_to_linear(&greyscale);
	convert_data_to_int(&greyscale);
	stbi_write_png("dithered.png", x, y, greyscale.channels, 
			greyscale.data, sizeof(unsigned char) * 
			greyscale.channels * x);

	free(greyscale.fdata);
	stbi_image_free(greyscale.data);
	stbi_image_free(data);
}
