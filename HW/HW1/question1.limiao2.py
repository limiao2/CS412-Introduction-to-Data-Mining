
# coding: utf-8

import numpy as np
import pandas as pd
from scipy import stats
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab

# Question 1
df=pd.read_table('/Users/Li/Google Drive/UIUC_Study/2015Fall/CS412/HW/HW1/data/data.online.scores',names=['ID','Midterm','Final'])
a = df['Final']
b = np.array(a)


# # a

np.median(b)
np.percentile(b,25)
np.percentile(b,50)
np.percentile(b,75)


# # b
fmean=np.mean(b)
print np.around(fmean,decimals=3)


# # c
stats.mode(b)
a.describe()


# # (d)
n, bins, patches =plt.hist(b,20)
mu = np.mean(b)
sig = np.std(b)
y = mlab.normpdf(20,mu,sig)
plt.plot(20, y, 'r--')
plt.show()

