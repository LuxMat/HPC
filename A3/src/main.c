#define _GNU_SOURCE
#include <pthread.h>

#include <stdbool.h>
#include <assert.h>
#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define cpow(a, b) \
(do { \
    fprintf(stderr, "forbidden :)\n"); \
    exit(EXIT_FAILURE); \
} while(0))

struct cfg {
  int num_threads;
  int lines;
  int degree;
};
struct arg {
  int thread_index;
  mtx_t *mutexes;
  int line_start;
  int line_end; // exclusive
  uint8_t *attractors_buf;
  uint8_t *convergence_buf;
  const complex double *roots;
  const double *root_reals;
  const double *root_imags;
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
  fprintf(f, "P3\n%d %d\n%d\n", cfg->lines, cfg->lines, max_color_val);
  return f;
}

static inline complex double our_cpow(complex double x, int degree) {
  complex double x2, x4;
  switch (degree) {
    case 0:
      return 1.0;
    case 1:
      return x;
    case 2:
      return x * x;
    case 3:
      return x * x * x;
    case 4:
      x2 = x * x;
      return x2 * x2;
    case 5:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x;
    case 6:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x2;
    case 7:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x2 * x;
    case 8:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x4;
    // know statically that d <= 9, and we always pass d-1 to this function
    default:
      abort();
  }
}
static inline double norm_squared(complex double x) {
  return creal(x)*creal(x)+cimag(x)*cimag(x);
}

void generate_roots(complex double *roots, int degree) {
  complex double r = cexp(2.*M_PI*I/((double)degree));
  complex double cur = 1.0;
  for (int i = 0; i < degree; i++) {
    roots[i] = cur;
    cur *= r;
  }
}

void find_root_predefined(complex double x0, int degree, uint8_t *restrict iter_greyscale, uint8_t *restrict attractor, const double *restrict root_reals, const double *restrict root_imags) {
  double d = (double)degree;
  double dm1_over_d = (d-1.)/d;
  complex double xn = x0;

  uint8_t n;
  *attractor = 0xff;
  for (n = 0; n < 50; n++) {
    double xn_nsq = norm_squared(xn);
    // too close to the origin or appears to be diverging
    if (xn_nsq < 1e-6 || xn_nsq > 1e10) {
      break;
    }

    // actual root!

    uint8_t root_idx = 0xff;
    for (int ridx = 0; ridx < degree; ridx++) {
      double dist = xn_nsq + 1. - 2.*(creal(xn)*root_reals[ridx]+cimag(xn)*root_imags[ridx]);
      //printf("root: %lf + %lf i\n", creal(root), cimag(root));
      if (dist < 1e-6) {
        root_idx = ridx;
        break;
      }
    }
    if (root_idx != 0xff) {
      *attractor = root_idx;
      break;
    }

    // FIXME

    complex double xn_pow_dm1 = our_cpow(xn, degree-1);
    xn = xn*dm1_over_d + 1/(d*xn_pow_dm1);
  }
  *iter_greyscale = 5*n;
}

void rgb_from_complex(complex double num, uint8_t *rgb) {
  // https://en.wikipedia.org/wiki/HSL_and_HSV#Color_conversion_formulae
  double hprime = carg(num) * 6.0 / M_2_PI;
  double lightness = 255. * (log(cabs(num))-log(1e-3))/log(50.);
  //double saturation = 1.0;
  double chroma = (1.-fabs(2.*lightness-1.));//*saturation;
  double m = lightness - 0.5*chroma;
  double x = chroma*(1.-fabs(remainder(hprime, 2.) - 1.));

  double r = m, g = m, b = m;

  if (hprime < 1.) {
    r = chroma;
    g = x;
  } else if (hprime < 2.) {
    r = x;
    g = chroma;
  } else if (hprime < 3.) {
    g = chroma;
    b = x;
  } else if (hprime < 4.) {
    g = x;
    b = chroma;
  } else if (hprime < 5.) {
    r = x;
    b = chroma;
  } else {
    r = chroma;
    b = x;
  }
  rgb[0] = (uint8_t)r;
  rgb[1] = (uint8_t)g;
  rgb[2] = (uint8_t)b;
}

int thread_start(void *arg_raw) {
  struct arg arg = *(struct arg *)arg_raw;
  struct cfg cfg = *arg.cfg;
  printf("thread %d: line %d..%d\n", arg.thread_index, arg.line_start, arg.line_end);

  struct timespec t1, t2;
  clock_gettime(CLOCK_MONOTONIC, &t1);

  bool first = true;
  for (size_t row = arg.line_start; (row != arg.line_start || first); row = (row == (cfg.lines-1) ? 0 : (row + 1))) {
    first = false;
    if (mtx_trylock(&arg.mutexes[row]) != thrd_success) {
      continue;
    }
    double imag = -2.0 + 4.0 * (double)row/(double)(cfg.lines);

    for (size_t col = 0; col < cfg.lines; col++) {
      size_t buf_idx = row * cfg.lines + col;
      double real = -2.0 + 4.0 * (double)col/(double)(cfg.lines);

      complex double x0 = CMPLX(real, imag);
      find_root_predefined(x0, cfg.degree, &arg.convergence_buf[buf_idx], &arg.attractors_buf[buf_idx], arg.root_reals, arg.root_imags);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &t2);

  uint64_t millis1 = t1.tv_sec * 1000 + t1.tv_nsec / 1000000;
  uint64_t millis2 = t2.tv_sec * 1000 + t2.tv_nsec / 1000000;
  uint64_t delta = millis2 - millis1;
  printf("thread %d took %lld ms\n", arg.thread_index, (unsigned long long)delta);

  return 0;
}

void faster_snprintf(char *dst, uint8_t r, uint8_t g, uint8_t b) {
  dst[0] = '0'+r/100;
  dst[1] = '0'+(r/10)%10;
  dst[2] = '0'+r%10;
  dst[3] = ' ';

  dst[4] = '0'+g/100;
  dst[5] = '0'+(g/10)%10;
  dst[6] = '0'+g%10;
  dst[7] = ' ';

  dst[8] = '0'+b/100;
  dst[9] = '0'+(b/10)%10;
  dst[10] = '0'+b%10;
  dst[11] = '\n';
}

#define OUTBUF_SIZE 12*4096

int main(int argc, const char *argv[]) {
  /*
  cpu_set_t *s = CPU_ALLOC(CPU_SETSIZE);
  for (int i = 0; i <= 112; i++) {
    if (i % 2 == 0) {
      CPU_SET(i, s);
    } else {
      CPU_CLR(i, s);
    }
  }
  pthread_setaffinity_np(pthread_self(), CPU_SETSIZE, s);
  CPU_FREE(s);
  */

  struct cfg cfg = (struct cfg){
    .num_threads = 1,
    .lines = 1000,
    .degree = 3
  };

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    if (arg[0] == '-') {
      // option
      if (arg[1] == 't') {
        cfg.num_threads = parse_int_or_fail(&arg[2], "num_threads");
      } else if (arg[1] == 'l') {
        cfg.lines = parse_int_or_fail(&arg[2], "lines");
      }
    } else {
      // only non-option arg is the polynomial degree
      cfg.degree = parse_int_or_fail(arg, "degree");
    }
  }

  printf("%d threads, %dx%d image, degree %d\n", cfg.num_threads, cfg.lines, cfg.lines, cfg.degree);
  assert(cfg.degree > 0); assert(cfg.degree <= 9);
  assert(cfg.lines < 10000);

  complex double roots[10];
  double root_reals[10];
  double root_imags[10];
  generate_roots(roots, cfg.degree);

  uint8_t roots_rgb[30];
  for (int i = 0; i < 10; i++) {
    root_reals[i] = creal(roots[i]);
    root_imags[i] = cimag(roots[i]);
    rgb_from_complex(roots[i], &roots_rgb[3*i]);
  }

  uint8_t *attractors_buf = malloc(cfg.lines * cfg.lines * sizeof(uint8_t));
  uint8_t *convergences_buf = malloc(cfg.lines * cfg.lines * sizeof(uint8_t));
  if (!attractors_buf || !convergences_buf) {
    return EXIT_FAILURE;
  }

  FILE *f_attr = create_ppm("attractors", &cfg);
  FILE *f_conv = create_ppm("convergence", &cfg);

  thrd_t *threads = malloc(cfg.num_threads * sizeof(thrd_t));
  mtx_t *mutexes = malloc(cfg.lines * sizeof(mtx_t));
  assert(threads);
  assert(mutexes);
  struct arg *args = malloc(cfg.num_threads * sizeof(struct arg));

  int avg_lines_per_thread = cfg.lines / cfg.num_threads;
  int cur_line = 0;

  for (int i = 0; i < cfg.lines; i++) {
    mtx_init(&mutexes[i], mtx_plain);
  }

  for (int i = 0; i < cfg.num_threads; i++) {
    int num_lines = i == cfg.num_threads - 1 ? cfg.lines - cur_line : avg_lines_per_thread;
    assert(cur_line + num_lines <= cfg.lines);

    args[i] = (struct arg) {
      .attractors_buf = attractors_buf,
      .convergence_buf = convergences_buf,
      .thread_index = i,
      .roots = roots,
      .root_reals = root_reals,
      .root_imags = root_imags,
      .mutexes = mutexes,
      .line_start = cur_line,
      .line_end = cur_line+num_lines,
      .cfg = &cfg
    };
    cur_line += num_lines;
    thrd_create(&threads[i], thread_start, &args[i]);
  }
  char *conv_buf = malloc(OUTBUF_SIZE);
  char *attr_buf = malloc(OUTBUF_SIZE);
  if (!conv_buf || !attr_buf) {
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < cfg.num_threads; i++) {
    int res;
    int sts = thrd_join(threads[i], &res);
    assert(sts == thrd_success);
  }
  for (int i = 0; i < cfg.lines; i++) {
    assert(mtx_trylock(&mutexes[i]) != thrd_success);
    mtx_destroy(&mutexes[i]);
  }

  free(threads);
  free(args);
  free(mutexes);

  size_t conv_len = 0;
  size_t attr_len = 0;

  for (size_t r = 0; r < cfg.lines; r++) {
    for (size_t c = 0; c < cfg.lines; c++) {
      size_t buf_idx = cfg.lines*r+c;
      uint8_t grey = convergences_buf[buf_idx];
      if (conv_len + 12 >= OUTBUF_SIZE) {
        fwrite(conv_buf, 1, conv_len, f_conv);
        conv_len = 0;
      }
      faster_snprintf(&conv_buf[conv_len], grey, grey, grey);
      //snprintf(&conv_buf[conv_len], 12, "%03d %03d %03d\n", grey, grey, grey);
      conv_len += 12;
      uint8_t attractor = attractors_buf[buf_idx];
      if (attractor == 0xFF) {
        attractor = 0;
      }
      uint8_t *rgb = &roots_rgb[attractor];
      if (attr_len + 12 >= OUTBUF_SIZE) {
        fwrite(attr_buf, 1, attr_len, f_attr);
        attr_len = 0;
      }
      //snprintf(&attr_buf[attr_len], 12, "%03d %03d %03d\n", rgb[0], rgb[1], rgb[2]);
      faster_snprintf(&attr_buf[attr_len], rgb[0], rgb[1], rgb[2]);
      attr_len += 12;
    }
  }
  if (conv_len > 0) {
    fwrite(conv_buf, 1, conv_len, f_conv);
  }
  if (attr_len > 0) {
    fwrite(attr_buf, 1, attr_len, f_attr);
  }
  free(conv_buf);
  free(attr_buf);

  fclose(f_conv);
  fclose(f_attr);
}
