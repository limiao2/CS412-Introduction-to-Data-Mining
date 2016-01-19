
# coding: utf-8

# In[5]:

import numpy as np
import pandas as pd
from scipy import stats
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from scipy.spatial import distance
from sklearn import preprocessing


# In[6]:

df=pd.read_table('/Users/Li/Google Drive/UIUC_Study/2015Fall/CS412/HW/HW1/data/data.online.scores',names=['ID','Midterm','Final'])
df


# In[7]:

d = np.array(df['Midterm'])


# In[8]:

np.mean(d)


# In[9]:

np.var(d,ddof=1)


# In[10]:

std_scale = preprocessing.StandardScaler().fit(d)
df_std = std_scale.transform(d)


# In[12]:

df_std.mean()


# In[13]:

df_std.var()


# In[14]:

(90-np.mean(d))/np.std(d,ddof=1)

