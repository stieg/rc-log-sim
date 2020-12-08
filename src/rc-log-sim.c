#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dateTime.h>
#include <gps.h>
#include <predictive_timer_2.h>

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
  COLUMN_DEF("\"GPSQual\""),
  COLUMN_DEF("\"GPSDOP\""),
  COLUMN_DEF("\"PredTime\""),
  COLUMN_DEF("\"ElapsedTime\""),
  COLUMN_DEF("\"LapCount\""),
  COLUMN_DEF("\"CurrentLap\""),
  COLUMN_DEF("\"LapTime\""),
  COLUMN_DEF("\"Distance\""),
  COLUMN_DEF("\"Speed\""),
  // Start our faked ones.
  COLUMN_DEF("\"PredTimeFixed\"|\"Min\"|0.0|5.0|5"),
};

#define ASL_CSV_COLS (ARR_SIZE(csv_columns))
#define ASL_CSV_COLS_FAKE (1)
#define ASL_CSV_COLS_READ ((ASL_CSV_COLS) - (ASL_CSV_COLS_FAKE))

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

struct {
  int lap;
} asl_state_data = {
  .lap = 0,
};

int main(int argc, char const *argv[]) {
  struct ASL_CSV csv;

  if (argc != 2) error(1, 0, "Missing file in value");

  asl_csv_open_read(&csv, argv[1], csv_columns, ASL_CSV_COLS_READ);

  // Print all headers we have
  for (size_t i = 0; i < ASL_CSV_COLS; ++i) {
    const struct ASL_CSV_Column *col = csv_columns + i;
    //printf("Found header \"%s\" at column index %d\n", col->header, col->index);
    printf("%s%s", i == 0 ? "" : ",", col->header);
  }
  printf("\n");

  char *csv_data[ASL_CSV_COLS];
  int first_interval = 0;
  resetPredictiveTimer();
  while(true) {
    const size_t cols_read = asl_csv_read_next_row(&csv, csv_data);

    // Done reading?
    if (cols_read == 0) break;

    // If read more than expected, die
    if (cols_read > ASL_CSV_COLS_READ) error(1, 0, "Read Overflow");

    // If we didn't read enough, then keep reading
    if (cols_read < ASL_CSV_COLS_READ) continue;

    // Parse our data from input
    const int interval = atoi(csv_data[0]);
    //const long long utc = atol(csv_data[1]);
    const float latitude = atof(csv_data[2]);
    const float longitude = atof(csv_data[3]);
    const int gps_qual = atoi(csv_data[4]);
    const float gps_dop = atof(csv_data[5]);
    //const float pred_time = atof(csv_data[6]);
    const float elapsed_time = atof(csv_data[7]);
    const int current_lap = atoi(csv_data[8]);

    if (first_interval == 0) {
      first_interval = interval;
    }

    // Build our gps snapshot and sample.
    GpsSnapshot gpsss = {
      .deltaFirstFix = interval - first_interval,
      .sample = {
        .point = {
          .latitude = latitude,
          .longitude = longitude,
        },
        .time = interval,
        .quality = gps_qual,
        .DOP = gps_dop,
      },
    };

    if (current_lap > asl_state_data.lap) {
      // In here we are starting a lap (and possibly ending the last)
      if (asl_state_data.lap > 0) {
        // Finishing the last lap
        finishLap(&gpsss);
        dprintf(2, "Finishing lap %d\n", asl_state_data.lap);
      }

      // Starting next lap
      asl_state_data.lap = current_lap;
      startLap(&gpsss.sample.point, gpsss.deltaFirstFix);
      dprintf(2, "Starting lap %d\n", asl_state_data.lap);
    } else {
      // In here we are just another sample.
      addGpsSample(&gpsss);
    }

    char buff[32];
    snprintf(buff, ARR_SIZE(buff), "%f", tinyMillisToMinutes(getPredictedTime(&gpsss)));
    csv_data[ASL_CSV_COLS_READ] = buff;
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
 *
 * finishLap
 * - gpsSnapshot->deltaFirstFix
 * - gpsSnapshot->sample.point (lat, lon)
 * startLap
 * - geoPoint
 * - time (millis)
 * addGpsSample
 * - gpsSnapshot->deltaFirstFix
 * - gpsSnapshot->sample.point (lat, lon)
 * getPredicttedTime
 * - gpsSnapshot->sample.point (lat, lon)
 * - gpsSnapshot->deltaFirstFix
 *
 * To Do:
 * - Add in header files from firmware project.
 * - sample.quality is used
 */
