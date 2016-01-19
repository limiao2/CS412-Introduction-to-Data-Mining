# Author: Li Miao
# coding: utf-8

# In[1]:

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import random
import math
import sys
from __future__ import division


# In[2]:

class bcubed(object):
    def __init__(self,output,truth):
        self.output = output
        self.truth = truth
    
    def out(self):
        o = pd.read_csv(self.output,skiprows = 3,header = None)
        t = pd.read_csv(self.truth,skiprows = 2,header = None)
        o = o.as_matrix()
        t = t.as_matrix()
        o = o.tolist()
        t = t.tolist()
        
        precision = 0.0
        recall = 0.0
        
        w = 0.0
        w = 1/len(o)
        
        for x in o:
            xtruth = [i for i in t if i[0] ==x[0] and i[1] == x[1]][0]
            prebelow = len([i for i in o if i[2] == x[2]])
            
            if xtruth[2] == 0 and x[2] !=0:
                above = 1
                recallbelow = 1
            elif xtruth[2] != 0 and x[2] ==0:
                above = 1
                prebelow = 1
                recallbelow = len([i for i in t if i[2] == xtruth[2]]) 
            elif xtruth[2] == 0 and x[2] ==0:
                above = 1
                prebelow = 1
                recallbelow = 1
            else:
                above = 0
                recallbelow = len([i for i in t if i[2] == xtruth[2]])
                outputchain = [i for i in o if i[2] == x[2]]
                truthchain = [i for i in t if i[2] == xtruth[2]]
                for y in outputchain:
                    ytruth = [i for i in t if i[0] ==y[0]][0]
                    if ytruth[2] ==xtruth[2]:
                        above +=1       
            precision += above/prebelow
            recall += above/recallbelow
           
        precision = precision / len(o)
        recall = recall/len(o)
        
        return precision,recall
        #sys.stdout.write(str(precision) + "\n")
        #sys.stdout.write(str(recall))

        
        


# In[ ]:

len(o)


# In[4]:

b = bcubed(output,truth)


# In[5]:

b.out()

