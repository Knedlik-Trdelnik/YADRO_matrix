#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

uint32_t height;
uint32_t width;

uint16_t Dheight;
uint16_t Dwidth;

char *inputFileName;
char *outputFileName;


uint8_t **alloc_u8_2d(size_t rows, size_t cols) {
    uint8_t **m = malloc(rows * sizeof(uint8_t *));
    if (!m) return NULL;

    for (size_t i = 0; i < rows; i++) {
        m[i] = malloc(cols * sizeof(uint8_t));
        if (!m[i]) {
            for (size_t k = 0; k < i; k++) free(m[k]);
            free(m);
            return NULL;
        }
    }
    return m;
}

void free_u8_2d(uint8_t **m, size_t rows) {
    if (!m) return;
    for (size_t i = 0; i < rows; i++) free(m[i]);
    free(m);
}

int8_t **alloc_i8_2d(size_t rows, size_t cols) {
    int8_t **m = malloc(rows * sizeof(int8_t *));
    if (!m) return NULL;

    for (size_t i = 0; i < rows; i++) {
        m[i] = malloc(cols * sizeof(int8_t));
        if (!m[i]) {
            for (size_t k = 0; k < i; k++) free(m[k]);
            free(m);
            return NULL;
        }
    }
    return m;
}

void free_i8_2d(int8_t **m, size_t rows) {
    if (!m) return;
    for (size_t i = 0; i < rows; i++) free(m[i]);
    free(m);
}

void init(FILE *file) {
  uint32_t buf[2];
  size_t wasReaded = fread(buf, sizeof(uint32_t), 2, file);

  height = buf[0];
  width = buf[1];
  /*
  printf("height = %d\n", height);
  printf("width = %d\n", width);
  */
}

void read_matrix(uint8_t **aMat, uint8_t **bMat, uint8_t **cMat, FILE *file) {
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      fread(&aMat[i][j], sizeof(uint8_t), 1, file);
      fread(&bMat[i][j], sizeof(uint8_t), 1, file);
      fread(&cMat[i][j], sizeof(uint8_t), 1, file);
    }
  }
}

void init_kernel(FILE *file) {
  uint16_t buf[2];
  size_t wasReaded = fread(buf, sizeof(uint16_t), 2, file);

  Dheight = buf[0];
  Dwidth = buf[1];

  //printf("Dheight = %d\n", Dheight);
  //printf("Dwidth = %d\n", Dwidth);
}

void read_kernel(int8_t **d, FILE *file) {
  for (size_t i = 0; i < Dheight; i++) {
    for (size_t j = 0; j < Dwidth; j++) {
      fread(&d[i][j], sizeof(int8_t), 1, file);
    }
  }
}

uint8_t normilize(int32_t s) {
  if (s > 255) {
    s = s % 251;
  }
  if (s < 0) {
    s = -s;
    s = s % 241;
  }
  return (uint8_t)s;
}

void doMagic(uint8_t **out, uint8_t **input, int8_t **kernel) {

  int yOffset = (Dheight - 1) / 2;
  int xOffset = (Dwidth - 1) / 2;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      int s = 0;

      for (int i_d = 0; i_d < Dheight; i_d++) {
        for (int j_d = 0; j_d < Dwidth; j_d++) {
          int matrix_el;

          if (((int)i - (int)yOffset + (int)i_d < 0) ||
              ((int)j - (int)xOffset + (int)j_d < 0) ||
              ((int)i - (int)yOffset + (int)i_d > (int)height - 1) ||
              ((int)j - (int)xOffset + (int)j_d > (int)width - 1)) {
            matrix_el = 0;
          } else {
            matrix_el = (int)input[i - yOffset + i_d][j - xOffset + j_d] *
                        (int)kernel[i_d][j_d];
          }
          s += matrix_el;
        }
      }

      out[i][j] = normilize(s);
    }
  }
}

void wrire_answer(uint8_t **a, uint8_t **b, uint8_t **c, FILE *out) {
  uint32_t buf[2] = {height, width};
  fwrite(&buf, sizeof(uint32_t), 2, out);
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      int8_t anotherBuf[3] = {a[i][j], b[i][j], c[i][j]};

      fwrite(&anotherBuf, sizeof(int8_t), 3, out);
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
      // printf("arg[i]: %s\n", optarg);
      inputFileName = optarg;
      break;
    case 'o':
      // printf("arg[o]: %s\n", optarg);
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

  uint8_t **a = alloc_u8_2d(height, width);
  uint8_t **b = alloc_u8_2d(height, width);
  uint8_t **c = alloc_u8_2d(height, width);
  read_matrix(a, b, c, inputFile);

  init_kernel(inputFile);
  int8_t **d = alloc_i8_2d(Dheight, Dwidth);
  read_kernel(d, inputFile);

  fclose(inputFile);

  uint8_t **aOut = alloc_u8_2d(height, width);
  uint8_t **bOut = alloc_u8_2d(height, width);
  uint8_t **cOut = alloc_u8_2d(height, width);

  doMagic(aOut, a, d);
  doMagic(bOut, b, d);
  doMagic(cOut, c, d);

  free_i8_2d(d, Dheight);
  free_u8_2d(a, height);
  free_u8_2d(b, height);
  free_u8_2d(c, height);

  FILE *outputFile = fopen(outputFileName, "w");
  if (NULL == outputFile) {
    return 10;
  }

  wrire_answer(aOut, bOut, cOut, outputFile);
  fclose(outputFile);
  free_u8_2d(aOut, height);
  free_u8_2d(bOut, height);
  free_u8_2d(cOut, height);
  return 0;
}
