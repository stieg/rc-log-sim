#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COLUMNS 7
#define COLUMN_DEF(PREFIX) { .index = -1, .header = PREFIX }
#define HEADER_MAX_SIZE 64
#define LINE_BUFF_SIZE 4096
#define ARR_SIZE(ARR) (sizeof(ARR) / sizeof(*ARR))

static char line_buff[LINE_BUFF_SIZE];

struct ASL_CSV_Column {
  int index;
  char header[HEADER_MAX_SIZE];
};

struct ASL_CSV_Column csv_columns[] = {
  COLUMN_DEF("\"Interval\""),
  COLUMN_DEF("\"Utc\""),
  COLUMN_DEF("\"Latitude\""),
  COLUMN_DEF("\"Longitude\""),
  COLUMN_DEF("\"PredTime\""),
  COLUMN_DEF("\"ElapsedTime\""),
  COLUMN_DEF("\"CurrentLap\""),
  //  COLUMN_DEF("\"PredTimeFixed\"|\"Min\"|0.0|0.0|50"),
};

char* trim_whitespace(char *str) {
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

bool match_populate_columns(struct ASL_CSV_Column cols[], const size_t cols_len, char *data) {
  char *saveptr = NULL, *header;

  int index = 0;
  header = strtok_r(data, ",", &saveptr);
  while (header) {
    header = trim_whitespace(header);

    for (size_t i = 0; i < cols_len; ++i) {
      struct ASL_CSV_Column *col = cols + i;

      // If matched already, then ignore it
      if (col->index >= 0) continue;

      // Check for match by matching substring
      if (0 == strncmp(col->header, header, strlen(col->header))) {
        strncpy(col->header, header, ARR_SIZE(col->header));
        col->index = index;
        break;
      }
    }

    header = strtok_r(NULL, ",", &saveptr);
    ++index;
  }

  // Ensure all headers populated
  for (size_t i = 0; i < cols_len; ++i) {
    if (cols[i].index < 0) return false;
  }
  return true;
}

struct ASL_CSV {
  FILE *file;
  struct ASL_CSV_Column *cols;
  size_t cols_len;
};

void asl_csv_open_read(struct ASL_CSV *data, const char *file_path, struct ASL_CSV_Column cols[], const size_t cols_len) {
  data->file = fopen(file_path, "r");
  data->cols = cols;
  data->cols_len = cols_len;

  if (!data->file) error(1, errno, "Unable to open %s", file_path);

  char *line = fgets(line_buff, ARR_SIZE(line_buff), data->file);
  if (!line) error(1, errno, "Error reading CSV header");

  const size_t line_len = strlen(line);
  if (line[line_len - 1] != '\n') error(1, errno, "CSV line larger than buffer");

  const bool all_found = match_populate_columns(data->cols, data->cols_len, line);
  if (!all_found) {
    for (size_t i = 0; i < cols_len; ++i) {
      if (cols[i].index < 0) error(0, 0, "Missing header %s", data->cols[i].header);
    }
    error(1, errno, "Some headers were not matched");
  }
}

size_t asl_csv_read_next_row(struct ASL_CSV *data, char *cols_data[]) {
  struct ASL_CSV_Column *cols = data->cols;
  const size_t cols_len = data->cols_len;
  size_t cols_read = 0;

  // Get a line and validate it
  char *line = fgets(line_buff, ARR_SIZE(line_buff), data->file);
  if (!line) return 0;

  for (int index = 0; line; ++index) {
    char *tok = strsep(&line, ",");
    tok = trim_whitespace(tok);

    // Does one of our colums match? If so, put its tok into col_data
    for (size_t i = 0; i < cols_len; ++i) {
      struct ASL_CSV_Column *col = cols + i;
      if (col->index == index) {
        cols_data[i] = tok;
        // Increment read count if not empty
        if (*tok != '\0') ++cols_read;
        break;
      }
    }
  }

  return cols_read;
}

int main(int argc, char const *argv[]) {
  struct ASL_CSV csv;
  char *csv_data[ARR_SIZE(csv_columns)];

  if (argc != 2) error(1, 0, "Missing file in value");

  asl_csv_open_read(&csv, argv[1], csv_columns, ARR_SIZE(csv_columns));

  // Print all headers we have
  for (size_t i = 0; i < ARR_SIZE(csv_columns); ++i) {
    const struct ASL_CSV_Column *col = csv_columns + i;
    //printf("Found header \"%s\" at column index %d\n", col->header, col->index);
    printf("%s%s", i == 0 ? "" : ",", col->header);
  }
  printf("\n");


  while(true) {
    size_t cols_read;
    do {
      cols_read = asl_csv_read_next_row(&csv, csv_data);
    } while (cols_read != 0 && cols_read != 7);
    if (cols_read == 0) break;

    for (size_t j = 0; j < ARR_SIZE(csv_data); ++j) {
      printf("%s%s", j == 0 ? "" : ",", csv_data[j]);
    }
    printf("\n");
  }

  // For each line, validate all fields are defined
  // If not, get next line
  // Plug the data into predictive timer.  Get Expected value
  // Print it to console.

  return 0;
}

/*
 * finishLap
 * - gpsSnapshot->deltaFirstFix
 * - gpsSnapshot->sample.point (lat, lon)
 * addGpsSample
 * - gpsSnapshot->deltaFirstFix
 * - gpsSnapshot->sample.point (lat, lon)
 *
 */
