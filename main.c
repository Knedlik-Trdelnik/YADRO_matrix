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

void read_matrix(int8_t (*aMat)[height][width], int8_t (*bMat)[height][width],
                 int8_t (*cMat)[height][width], FILE *file) {

  size_t i = 0;
  size_t j = 0;
  for (size_t c = sizeof(uint32_t); c < 7 + height + width * 3; c++) {
    fread(&(*aMat)[i][j], sizeof(int8_t), 1, file);
    fread(&(*bMat)[i][j], sizeof(int8_t), 1, file);
    fread(&(*cMat)[i][j], sizeof(int8_t), 1, file);

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

void read_kernel(int8_t (*d)[Dheight][Dwidth], FILE *file) {
  size_t i = 0;
  size_t j = 0;
  for (size_t k = 0; k < Dwidth * Dheight; k++) {
    fread(&(*d)[i][j], sizeof(int8_t), 1, file);
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
    s = (-s) % 241;
  }
  return s;
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
  read_matrix(&a, &b, &c, inputFile);

  init_kernel(inputFile);
  int8_t d[Dheight][Dwidth];
  read_kernel(&d, inputFile);

  for (size_t i = 0; i < Dheight; i++) {
    for (size_t j = 0; j < Dwidth; j++) {
      printf("%d ", d[i][j]);
    }
  }
  fclose(inputFile);

  int8_t aOut[height][width];
  int8_t bOut[height][width];
  int8_t cOut[height][width];

  int8_t yOffset = (height - 1) / 2;
  int8_t xOffset = (width - 1) / 2;
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      int32_t s = 0;

      for (size_t i_d = 0; i_d < Dheight; i_d++) {
        for (size_t j_d = 0; j_d < Dwidth; j_d++) {
          int8_t matrix_el;
          // если выходим за пределы массива, то 0
          if ((i - yOffset + i_d < 0) || (j - xOffset + j_d < 0) ||
              (i - yOffset + i_d > height - 1) ||
              (j - xOffset + j_d > width - 1)) {
            matrix_el = 0;
          } else {
            matrix_el = a[i - yOffset + i_d][j - xOffset + j_d] * d[i_d][j_d];
          }
          s += matrix_el;
        }
      }

      aOut[i][j] = normilize(s) ;
    }
  }
  printf("\n");
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      printf("%d ", (uint8_t)aOut[i][j]);
    }
  }
  printf("\n");
  return 0;
}
