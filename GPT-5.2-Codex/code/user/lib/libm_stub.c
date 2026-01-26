#include <stdint.h>

double sqrt(double x) {
  (void)x;
  return 0.0;
}

double pow(double x, double y) {
  (void)x;
  (void)y;
  return 0.0;
}

double sin(double x) {
  (void)x;
  return 0.0;
}

double cos(double x) {
  (void)x;
  return 0.0;
}

double tan(double x) {
  (void)x;
  return 0.0;
}

double log(double x) {
  (void)x;
  return 0.0;
}

double exp(double x) {
  (void)x;
  return 0.0;
}

double floor(double x) {
  return (double)(int64_t)x;
}

double ceil(double x) {
  int64_t v = (int64_t)x;
  return (x > (double)v) ? (double)(v + 1) : (double)v;
}

double fabs(double x) {
  return (x < 0.0) ? -x : x;
}
