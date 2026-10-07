#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

uint32_t height;
uint32_t width;

uint16_t Dheight;
uint16_t Dwidth;

char *inputFileName;
char *outputFileName;

void init(FILE *file) {
  uint32_t buf[2];
  size_t wasReaded = fread(buf, sizeof(uint32_t), 2, file);

  height = buf[0];
  width = buf[1];

  printf("height = %d\n", height);
  printf("width = %d\n", width);
}

void read_matrix(int8_t aMat[height][width], int8_t bMat[height][width],
                 int8_t cMat[height][width], FILE *file) {

  size_t i = 0;
  size_t j = 0;
  for (size_t c = sizeof(uint32_t); c < 7 + height + width * 3; c++) {
    fread(&aMat[i][j], sizeof(int8_t), 1, file);
    fread(&bMat[i][j], sizeof(int8_t), 1, file);
    fread(&cMat[i][j], sizeof(int8_t), 1, file);

    if (j == width - 1) {
      j = 0;
      i++;
    } else {
      j++;
    }
    if (i == height) {
      break;
    }
  }
}

void init_kernel(FILE *file) {
  uint16_t buf[2];
  size_t wasReaded = fread(buf, sizeof(uint16_t), 2, file);

  Dheight = buf[0];
  Dwidth = buf[1];

  printf("Dheight = %d\n", Dheight);
  printf("Dwidth = %d\n", Dwidth);
}

void read_kernel(int8_t d[Dheight][Dwidth], FILE *file) {
  size_t i = 0;
  size_t j = 0;
  for (size_t k = 0; k < Dwidth * Dheight; k++) {
    fread(&d[i][j], sizeof(int8_t), 1, file);
    if (j == Dwidth - 1) {
      j = 0;
      i++;
    } else {
      j++;
    }
    if (i == Dheight) {
      break;
    }
  }
}

int8_t normilize(int32_t s) {
  if (s > 255) {
    s = s % 251;
  }
  if (s < 0) {
    s = -s;
    s = (s) % 241;
  }
  return s;
}

void doMagic(int8_t out[height][width], int8_t input[height][width],
             int8_t kernel[Dheight][Dwidth]) {

  int yOffset = (Dheight - 1) / 2;
  int xOffset = (Dwidth - 1) / 2;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int s = 0;

      for (int i_d = 0; i_d < Dheight; i_d++) {
        for (int j_d = 0; j_d < Dwidth; j_d++) {
          int32_t matrix_el;
          // если выходим за пределы массива, то 0

          if (((int)i - (int)yOffset + (int)i_d < 0) ||
              ((int)j - (int)xOffset + (int)j_d < 0) ||
              ((int)i - (int)yOffset + (int)i_d > (int)height - 1) ||
              ((int)j - (int)xOffset + (int)j_d > (int)width - 1)) {
            matrix_el = 0;
          } else {
            matrix_el = (int)input[i - yOffset + i_d][j - xOffset + j_d] *
                        (int)kernel[i_d][j_d];
            //int a = i - yOffset + i_d;
            //int b = j - xOffset + j_d;
            //printf("\n%d %d %d\n", matrix_el, a, b);
            //printf("%x\n", (int8_t)input[i - yOffset + i_d][j - xOffset + j_d]);

          }
          s += matrix_el;
        }
      }

      out[i][j] = normilize(s);
    }
  }
}

int main(int argc, char **argv) {

  if (argc != 5) {
    return 10;
  }

  int arg;
  while ((arg = getopt(argc, argv, "i:o:")) != -1) {
    switch (arg) {
    case 'i':
      printf("arg[i]: %s\n", optarg);
      inputFileName = optarg;
      break;
    case 'o':
      printf("arg[o]: %s\n", optarg);
      outputFileName = optarg;
      break;
    default:
      return 10;
    }
  }

  FILE *inputFile = fopen(inputFileName, "r");
  if (inputFile == NULL) {
    return 10;
  }
  init(inputFile);

  int8_t a[height][width];
  int8_t b[height][width];
  int8_t c[height][width];
  read_matrix(a, b, c, inputFile);

  init_kernel(inputFile);
  int8_t d[Dheight][Dwidth];
  read_kernel(d, inputFile);

  for (size_t i = 0; i < Dheight; i++) {
    for (size_t j = 0; j < Dwidth; j++) {
      printf("%d ", d[i][j]);
    }
  }
  fclose(inputFile);

  int8_t aOut[height][width];
  int8_t bOut[height][width];
  int8_t cOut[height][width];

   doMagic(aOut, a, d);
   doMagic(bOut, b, d);
   doMagic(cOut, c, d);

  printf("\n");
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      printf("%x ", (uint8_t)a[i][j]);
      printf("%x ", (uint8_t)b[i][j]);
      printf("%x ", (uint8_t)c[i][j]);
    }
  }
  printf("\n");

  printf("\n");
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      printf("%x ", (uint8_t)aOut[i][j]);
      printf("%x ", (uint8_t)bOut[i][j]);
      printf("%x ", (uint8_t)cOut[i][j]);
    }
  }
  printf("\n");
  return 0;
}
