#define _GNU_SOURCE
#include <pthread.h>

#include <assert.h>
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdint.h>

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
  int line_start;
  int line_end; // exclusive
  uint8_t *rgb_attractors_buf;
  uint8_t *convergence_buf;
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

complex double our_cpow(complex double x, int degree) {
  complex double ret, x2, x4;
  switch (degree) {
    case 0:
      ret = 1.0;
      break;
    case 1:
      return x;
      break;
    case 2:
      return x * x;
      break;
    case 3:
      return x * x * x;
      break;
    case 4:
      x2 = x * x;
      return x2 * x2;
      break;
    case 5:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x;
      break;
    case 6:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x2;
      break;
    case 7:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x2 * x;
      break;
    case 8:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x4;
      break;
    case 9:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x4 * x;
      break;
    case 10:
      x2 = x * x;
      x4 = x2 * x2;
      return x4 * x4 * x2;
      break;
    default:
      abort();
  }
}
complex double find_root(complex double x0, int degree, uint8_t *iter_greyscale) {
  double d = (double)degree;
  complex double xn = x0;
  uint8_t n;
  for (n = 0; n < 50; n++) {
    // too close to the origin or appears to be diverging
    if (cabs(xn) < 1e-3 || cabs(xn) > 1e5) {
      break;
    }
    complex double xn_pow_d = our_cpow(xn, degree);

    // actual root!
    if (cabs(xn_pow_d - 1.) < 1e-3) {
      //printf("root: %lf+%lfi\n", creal(xn), cimag(xn));
      break;
    }

    // f:   x^d - 1
    // f': dx^{d-1}
    //xn = xn - (our_cpow(xn, degree) - 1.0) / ((double)degree * our_cpow(xn, degree-1));
    //xn = xn - function_value / ((double)degree * our_cpow(xn, degree-1));
    xn = (xn/d)*(d-1.+1./xn_pow_d);
    n++;
  }
  *iter_greyscale = 5*n;
  return xn;
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

  for (size_t row = arg.line_start; row < arg.line_end; row++) {
    double imag = -2.0 + 4.0 * (double)row/(double)(cfg.lines);

    for (size_t col = 0; col < cfg.lines; col++) {
      size_t buf_idx = row * cfg.lines + col;
      double real = -2.0 + 4.0 * (double)col/(double)(cfg.lines);

      complex double x0 = CMPLX(real, imag);
      complex double xn = find_root(x0, cfg.degree, &arg.convergence_buf[buf_idx]);
      rgb_from_complex(xn, &arg.rgb_attractors_buf[3*buf_idx]);
    }
  }

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
  assert(cfg.degree > 0); assert(cfg.degree < 10);
  assert(cfg.lines < 10000);

  uint8_t *rgb_attractors_buf = malloc(cfg.lines * cfg.lines * sizeof(uint8_t) * 3);
  uint8_t *convergences_buf = malloc(cfg.lines * cfg.lines * sizeof(uint8_t));
  if (!rgb_attractors_buf || !convergences_buf) {
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
      .rgb_attractors_buf = rgb_attractors_buf,
      .convergence_buf = convergences_buf,
      .thread_index = i,
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
  free(threads);
  free(args);

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
      uint8_t *rgb = &rgb_attractors_buf[3*buf_idx];
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

  fclose(f_conv);
  fclose(f_attr);
}
