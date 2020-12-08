#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* void read_csv(int row, int col, char *filename, double **data){ */

/*   int i = 0; */

/*   while (fgets(line, 4098, file) && (i < row)) */
/*     { */
/*     	// double row[ssParams->nreal + 1]; */
/*       char* tmp = strdup(line); */

/*       int j = 0; */
/*       const char* tok; */
/*       for (tok = strtok(line, "\t"); tok && *tok; j++, tok = strtok(NULL, "\t\n")) */
/*         { */
/*           data[i][j] = atof(tok); */
/*           printf("%f\t", data[i][j]); */
/*         } */
/*       printf("\n"); */

/*       free(tmp); */
/*       i++; */
/*     } */
/* } */

int get_col_count(const char *str) {
  if (!str || '\n' == str[0]) return 0;

  int cols = 1;
  for (; *str; ++str) {
    if (',' == *str) ++cols;
  }

  return cols;
}

int main(int argc, char const *argv[]) {
  if (argc != 2) error(1, 0, "Missing file input");

  FILE *file;
  file = fopen(argv[1], "r");
  if (NULL == file) error(1, errno, "Unable to open %s", argv[1]);

  int rows = 0;
  int cols = 0;
  char line_buff[4096];
  while (fgets(line_buff, sizeof line_buff, file)) {
    ++rows;
    const int col_count = get_col_count(line_buff);
    if (1 == rows) {
      // Then count and set expected num of columns
      cols = col_count;
    } else {
      // Compare cols vs what is found.
      if (cols != col_count) {
        error(2, 0, "Error at row %d. Expected %d columns, got %d", rows, cols, col_count);
      }
    }
  }

  printf("All consistent! Cols: %d, Rows: %d\n", cols, rows);
  fclose(file);
  return 0;
}
