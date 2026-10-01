#include <assert.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdint.h>

struct cfg {
  int num_threads;
  int lines;
  int degree;
};
struct arg {
  int thread_index;
  int line_start;
  int num_lines;
  uint8_t *buf;
  const struct cfg *cfg;
};

int parse_int_or_fail(const char *s, const char *ty) {
    char *end = NULL;
    int val = strtol(s, &end, 10);
    if (val == 0 && s == end) {
      fprintf(stderr, "kind %s: expected int but got `%s`\n", ty, s);
      exit(EXIT_FAILURE);
    }
    return val;
}

FILE *create_ppm(const char *type, const struct cfg *cfg) {
  char filename[64];
  size_t wrote;

  wrote = snprintf(filename, 64, "newton_%s_x%d.ppm", type, cfg->degree);
  assert(wrote < 64);
  FILE *f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "failed to open `%s`\n", filename);
    exit(EXIT_FAILURE);
  }
  int max_color_val = 255;
  fprintf(f, "P3\n%d %d\n%d", cfg->lines, cfg->lines, max_color_val);
  return f;
}

complex double our_cpow(complex double x, int degree) {
  complex double ret = 1.0;
  while (degree > 0) {
    if (degree % 2 == 1) {
      ret *= x;
      degree -= 1;
    } else {
      x *= x;
      degree /= 2;
    }
  }
  return ret;
}
int thread_start(void *arg_raw) {
  struct arg arg = *(struct arg *)arg_raw;
  struct cfg cfg = *arg.cfg;
  printf("thread %d: line %d..%d\n", arg.thread_index, arg.line_start, arg.line_start+arg.num_lines);

  for (size_t row = 0; row < arg.num_lines; row++) {
    double imag = -2.0 + 4.0 * (double)(arg.line_start + row)/(double)(cfg.lines);

    for (size_t col = 0; col < cfg.lines; col++) {
      double real = -2.0 + 4.0 * (double)col/(double)(cfg.lines);

      complex double
    }
  }

  return 0;
}

int main(int argc, const char *argv[]) {
  struct cfg cfg = (struct cfg){
    .num_threads = 1,
    .lines = 1000,
    .degree = 0
  };

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    if (arg[0] == '-') {
      // option
      if (arg[1] == 't') {
        cfg.num_threads = parse_int_or_fail(&arg[2], "num_threads");
      } else if (arg[2] == 'l') {
        cfg.lines = parse_int_or_fail(&arg[2], "lines");
      }
    } else {
      // only non-option arg is the polynomial degree
      cfg.degree = parse_int_or_fail(arg, "degree");
    }
  }

  printf("%d threads, %dx%d image, degree %d\n", cfg.num_threads, cfg.lines, cfg.lines, cfg.degree);
  assert(cfg.degree >= 0); assert(cfg.degree < 10);
  assert(cfg.lines < 10000);

  uint8_t *buf = malloc(cfg.lines * cfg.lines * sizeof(uint8_t));
  if (!buf) {
    return EXIT_FAILURE;
  }

  FILE *f_attr = create_ppm("attractors", &cfg);
  FILE *f_conv = create_ppm("convergence", &cfg);

  thrd_t *threads = malloc(cfg.num_threads * sizeof(thrd_t));
  assert(threads);
  struct arg *args = malloc(cfg.num_threads * sizeof(struct arg));

  int avg_lines_per_thread = cfg.lines / cfg.num_threads;
  int cur_line = 0;

  for (int i = 0; i < cfg.num_threads; i++) {
    int num_lines = i == cfg.num_threads - 1 ? cfg.lines - cur_line : avg_lines_per_thread;
    assert(cur_line + num_lines <= cfg.lines);

    args[i] = (struct arg) {
      .buf = buf,
      .thread_index = i,
      .line_start = cur_line,
      .num_lines = num_lines
    };
    cur_line += num_lines;
    thrd_create(&threads[i], thread_start, &args[i]);
  }

  for (int i = 0; i < cfg.num_threads; i++) {
    int res;
    int sts = thrd_join(threads[i], &res);
    assert(sts == thrd_success);
  }
  free(threads);
  free(args);
  fclose(f_attr);
  fclose(f_conv);
}
