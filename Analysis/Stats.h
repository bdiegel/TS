/* Copyright (C) 2010 Ion Torrent Systems, Inc. All Rights Reserved */
#ifndef STATS_H
#define STATS_H

#include <algorithm>
#include <vector>
#include <limits>
#include <assert.h>
#include <math.h>

using namespace std;

namespace ionStats
{

  float  median(std::vector<float> &v);
  double median(std::vector<double> &v);
double median(double *x, unsigned int n);
float  median(float  *x, unsigned int n);
double sd(double *x, unsigned int n);

float  geman_mcclure(float  x);

double truncated_mean(double *x, unsigned int n, double keep_percent);

template <class T>
double quantile_sorted(T *x, size_t size, float quantile) {
  assert(size > 0 && quantile >= 0 && quantile <= 1.0);
  double idx = quantile * (size - 1);
  size_t start = (size_t) floor(idx);
  size_t end = min(start + 1, size-1);
  double q = x[start] + ((idx - start) * (x[end] - x[start]));
  return q;
}

template <class T>
  double quantile_sorted(std::vector<T> &x, float quantile) {
  return quantile_sorted(&x[0], x.size(), quantile);
 }

template <class T>
double quantile_in_place(T *x, size_t size, float quantile) {
  assert(size > 0);
  assert(quantile >= 0);
  assert(quantile <= 1.0);
  if (size == 1) {
    return x[0];
  }
  std::sort(&x[0], &x[0] + size);
  return quantile_sorted(x, size, quantile);
}

/** median of part of a numeric sequence, returns a double.
 * Example:
 * vector<int> myvector;
 *  // set some values:
 * for (int i=1; i<10; i++) myvector.push_back(i);   // 1 2 3 4 5 6 7 8 9 10
 * cout << "==> " << median (myvector.begin(), myvector.end()) << endl;
 *
 * ==> 5.5
 *
 * Side effect: data sequence myvector will be changed!
 */
template <class T>
  double median(T first, T last)
  {
    size_t cnt = std::distance(first,last);
    if (cnt > 0)
    {
      T mid = first;
      std::advance(mid, (cnt/2));
      std::nth_element(first, mid, last);

      double median = double(*mid);

      if (cnt % 2 == 0){  // get the average of the 2 middle elements
	std::advance(mid,1);
	T nextVal = std::min_element(mid, last);
	median = (median + double(*nextVal))/2;
      }
      return (median);
    }
    else
      return(numeric_limits<double>::quiet_NaN());
  }

}
#endif // STATS_H
