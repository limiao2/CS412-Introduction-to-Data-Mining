
# coding: utf-8

# In[4]:

import numpy as np
import pandas as pd
from scipy import stats
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from scipy.spatial import distance


# In[9]:

df=pd.read_table('/Users/Li/Google Drive/UIUC_Study/2015Fall/CS412/HW/HW1/data/vectors.txt')


# In[15]:

x = np.array(df.ix[0,1:])
y = np.array(df.ix[1,1:])


# In[19]:

distance.minkowski(x,y,2)


# In[21]:

distance.minkowski(x,y,3)

