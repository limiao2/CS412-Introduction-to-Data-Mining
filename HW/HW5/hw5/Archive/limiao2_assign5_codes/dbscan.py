#Author : Li Miao
# In[2]:

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import random
import math

def distance(a,b):
    dim = 2
    dis = 0.0
    sumation = 0.0
        
    for i in xrange(dim):
        sumation += (a[i] - b[i])**2

    dis = math.sqrt(sumation)
    return dis

def neighbors(data,p,epsilon):
    N = []
    for i in data:
        if distance(i,p) <= epsilon:
            N.append(i)

    numberofneig = len(N)
        
    return N,numberofneig



# In[3]:

class dbscan(object):
    def __init__(self,dataset,output,minpts = 25,epsilon = 0.065):
        self.dataset = dataset
        self.output = output
        self.minpts = minpts
        self.epsilon = epsilon
        
    def out(self):
        mydata = pd.read_csv(self.dataset,skiprows = 1,header = None)
        mydata['visited'] = pd.Series(np.zeros(mydata.shape[0]))
        mydata['cluster'] = pd.Series(np.repeat(-1,mydata.shape[0]))
        md = mydata.as_matrix()
        n = mydata.shape[0]
        
        visit = []
        m = md.tolist()
        
        clusternumber = 0
    
    # 0 : unvisited
    # 1 : visited
    
        for p in m:
            if p[2] == 1:
                continue
    
            p[2] = 1
            N,numberofneig = neighbors(m,p,self.epsilon)

            if numberofneig < self.minpts:
                p[3] =0

            if numberofneig >= self.minpts:
                clusternumber += 1
                p[3] = clusternumber
                for pprime in N:
                    if pprime[2] == 0:
                        pprime[2] = 1
                        Npprime,numberpprime = neighbors(m,pprime,self.epsilon)
                        if numberpprime >= self.minpts:
                            for j in Npprime:
                                N.append(j)
        
                    if pprime[3] == -1:
                        pprime[3] = clusternumber
                    

        file = open(self.output,"w")
        file.write(str(self.minpts) + "\n")
        file.write(str(self.epsilon) + "\n")
        file.write(str(clusternumber) + "\n")
        
        for x in m:
            file.write(str(x[0]) + "," + str(x[1]) + "," + str(x[3]) + "\n")
        
        file.close()

    
   
# Running code

dataset = ('/Users/Li/Desktop/hw5/data_normalized.txt')
output = ('/Users/Li/Desktop/hw5/step1.txt')
d = dbscan(dataset,output)
d.out()



