/* medianfilter.c, to be applied to a PPM format image with a desired size for the median filter */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

typedef struct { unsigned char r, g, b; } rgb_t; //represent RBG values of an image
typedef struct {
    int width, height;
    rgb_t **pix;
} image_t, *image;    //pointer to dynamically allocate memory

typedef struct {
    int r[256], g[256], b[256];
    int n;
} color_histo_t; //histogram

int write_ppm(image im, const char *fn)
{
    FILE *fp;
    fp = fopen(fn, "w");
    if (!fp) return 0;

    fprintf(fp, "P6\n%d %d\n%d\n", im->width, im->height,255);
    fwrite(im->pix[0], 1, sizeof(rgb_t) * im->width * im->height, fp);

    fclose(fp);
    return 1;
}

image img_new(int w, int h)
{
    int i;
    image im = malloc(sizeof(image_t) + (h  * w * sizeof(rgb_t*))
	    + (sizeof(rgb_t) * w * h));
    im->width = w; im->height = h;
    im->pix = (rgb_t**)(im + 1);
    for (im->pix[0] = (rgb_t*)(im->pix + h), i = 1; i < h; i++)
	im->pix[i] = im->pix[i - 1] + w;
    return im;
}

int read_num(FILE *f)
{
    int n;
    while (!fscanf(f, "%d ", &n)) { 
	if ((n = fgetc(f)) == '#') {
	    while ((n = fgetc(f)) != '\n')
		if (n == EOF) break;
	    if (n == '\n') continue;
	} else return 0;
    }
    return n;
}

image read_ppm(char *fn)
{
    FILE *fp = fopen(fn, "r");
    int width, height, maxval;
    image im = 0;
    if (!fp) {
	return 0;
    } 
    if (fgetc(fp) != 'P' || fgetc(fp) != '6' || !isspace(fgetc(fp))){
	goto bail;
    }
    width = read_num(fp);
    height = read_num(fp);
    maxval = read_num(fp);
    if (!width || !height || !maxval) goto bail;

    im = img_new(width, height);
    fread(im->pix[0], 1, sizeof(rgb_t) * width * height, fp);
bail:
    if (fp) fclose(fp);
    return im;
}

void del_pixels(image im, int row, int col, int size, color_histo_t *h)
{
    int i;
    rgb_t *pix;

    if (col < 0 || col >= im->height) return;
    for (i = row - size; i <= row + size && i < im->height; i++) {
	if (i < 0){ 
	    continue;
	}
	pix = im->pix[i] + col;
	h->r[pix->r]--;
	h->g[pix->g]--;
	h->b[pix->b]--;
	h->n--;
    }
}

void add_pixels(image im, int row, int col, int size, color_histo_t *h)
{
    int i;
    rgb_t *pix;

    if (col < 0 || col >= im->height) 
    {

	return;
    }
    for (i = row - size; i <= row + size && i < im->height; i++) {
	if (i < 0) continue;
	pix = im->pix[i] + col;
	h->r[pix->r]++;
	h->g[pix->g]++;
	h->b[pix->b]++;
	h->n++;
    }
}

void init_histo(image im, int row, int size, color_histo_t*h)
{
    int j;

    memset(h, 0, sizeof(color_histo_t));

    for (j = 0; j < size && j < im->width; j++)
	add_pixels(im, row, j, size, h);
}

void printArray(const int * arr, int n) {
    int x;
    printf("Array: [");
    for (x = 0 ; x<n ; x++) {
	printf("%d |", arr[x]);
    }
    printf("]\n");
}

int median(const int *x, int n)
{
    int i;
    for (n /= 2, i = 0; i < 256 && (n -= x[i]) > 0; i++);

    //printf("Median = %d\n",i);

    return i;
}

void median_color(rgb_t *pix, const color_histo_t *h)
{
    pix->r = median(h->r, h->n);
    pix->g = median(h->g, h->n);
    pix->b = median(h->b, h->n);
}

image median_filter(image in, int size) //median filter main function, returns the new image to be written in current directory (using write_ppm())
{
    int row, col;
    image out = img_new(in->width, in->height);
    color_histo_t h;

    for (row = 0; row < in->height; row ++) {
	for (col = 0; col < in->width;col++) {
	    if (!col) {
		init_histo(in, row, size, &h);
	    }	
	    else {
		del_pixels(in, row, col - size, size, &h);
		add_pixels(in, row, col + size, size, &h);
	    }
	    median_color(out->pix[row] + col, &h);
	}
    }

    return out;
}

int main(int c, char **v)
{
    int size;
    image input, output;
    if (c <= 3) {
	printf("Usage: %s size ppm_in ppm_out\n", v[0]);
	return 0;
    }
    size = atoi(v[1]);   //get size of medium filtering
    printf("Medium filtering size of %d\n", size);
    input = read_ppm(v[2]);
    output = median_filter(input, size);
    write_ppm(output, v[3]);
    free(input);
    free(output);

    return 0;
}
