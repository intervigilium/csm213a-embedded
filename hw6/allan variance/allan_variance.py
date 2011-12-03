#!/usr/bin/env python

# Allan Variance code originally written by Ian Crossfield of UCLA Astronomy
# Dept as part of astroph.py library available here:
# www.astro.ucla.edu/~ianc/computing.shtml

from numpy import array, mean, zeros
from scipy import optimize

import numpy


def allan_variance(data, dt=1):
    """Compute the Allan variance on a set of regularly-sampled data (1D).

       If the time between samples is dt and there are N total
       samples, the returned variance spectrum will have frequency
       indices from 1/dt to (N-1)/dt."""
    # 2008-07-30 10:20 IJC: Created
    # 2011-04-08 11:48 IJC: Moved to analysis.py
    # 2011-10-27 09:25 IJMC: Corrected formula; thanks to Xunchen Liu
    #                        of U. Alberta for catching this.

    newdata = array(data, subok=True, copy=True)
    dsh = newdata.shape

    newdata = newdata.ravel()

    nsh = newdata.shape

    alvar = zeros(nsh[0]-1, float)

    for lag in range(1, nsh[0]):
        # Old, wrong formula:
        #alvar[lag-1]  = mean( (newdata[0:-lag] - newdata[lag:])**2 )
        alvar[lag-1] = (mean(newdata[0:(lag+1)])-mean(newdata[0:lag]))**2

    return (alvar*0.5)


def main():
  in_file = open('log.out')
  data_point = 1
  useful_data = []
  av = allan_variance([float(offset) for offset in in_file])
  n = len(av)
  b = 61920
  for i in range(len(av)):
    if i == data_point:
      print av[i]
      useful_data.append(av[i])
      data_point *= 2

  print 'Mean:',
  print mean(useful_data)
  print 'Min:',
  print min(useful_data)
  print 'Max:',
  print max(useful_data)
  print 'Efficiency:',
  print mean(useful_data) * b / n


if __name__ == '__main__':
  main()
