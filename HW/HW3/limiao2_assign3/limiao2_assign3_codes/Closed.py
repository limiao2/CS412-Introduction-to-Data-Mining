
# coding: utf-8

# In[1]:

import numpy as np
import pandas as pd
from scipy import stats
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
from scipy.spatial import distance
from sklearn import preprocessing
from itertools import chain, combinations
from collections import defaultdict
from collections import Counter
from operator import itemgetter


# In[7]:

# Read in file, return transaction,itemset and generalitemset
def readinfile(name):
    f = open(name,"r")
    temp = []
    c1 = []
    transaction = []
    tt= [i.strip().split(' ') for i in f]  
    
    for i in tt:
        transaction.append([int(j) for j in i])
    
    for i in transaction:
        for k in i:
            temp.append(k)
            if not [k] in c1:
                c1.append([k])
    itemset = frozenset(temp)
    c1.sort()
    f.close()
    return transaction,map(frozenset,c1),temp

# Find frequent Itemset satisfy minsupport
def frequentItemset(transaction,itemset,minsupport):
    d = defaultdict(int)
    freqitemset = []
    support = {}
    for item in itemset:
        for record in transaction:
            if item.issubset(record):
                d[item] +=1
    
    for i, count in d.items():
        if count >= minsupport:
            freqitemset.insert(0,i)
            support[i] = count
    
    return freqitemset ,support

# Join candidates
def join(freqitemset, k):
    joint = []
    length = len(freqitemset)
    for i in range(length):
        for j in range(i + 1, length):
            L1 = list(freqitemset[i])[:k - 2]
            L2 = list(freqitemset[j])[:k - 2]
            L1.sort()
            L2.sort()
            if L1 == L2:
                joint.append(freqitemset[i] | freqitemset[j])
    return joint

# closed
def apriori(filename,minsupport):
    (transaction,itemset,generalitemset) = readinfile(filename)
    C1 = itemset
    
    (L1,support) = frequentItemset(transaction,itemset,minsupport)
    L = [L1]
    k = 2
    
    while(len(L[k-2])>0):
        ck = join(L[k-2],k)
        (Lk,s) = frequentItemset(transaction,ck,minsupport)
        support.update(s)
        L.append(Lk)
        k +=1
        
    return L,support

# Read in Vocabulary
def readinvoc(name):
    f = open(name,"r")
    t= []  
    for line in f:
        parts = line.split() 
        if len(parts) > 1:   
            t.append(parts[1]) 
    return t
    
# Wirte out 
def writeout(fname,n,voc):
    kkk = map(list,sorted(n.items(), key=itemgetter(1),reverse = True))
    freq = []
    item = []

    for i in kkk:
        item.append(map(int,i[0]))
        freq.append(i[1])
        
    f = open(fname, "wb")
    
    for i in range(len(freq)):
        f.write(str(freq[i]) + "\t")
        for k in item[i]:
            f.write(voc[k] + " ")
        f.write("\n")

    # Close opend file
    f.close()
    
# close
def closed(L,support):
    close = []
    s = support
    for i in range(len(L)-1):
        L1 = L[i]
        L2 = L[i+1]
    
        for m in L1:
            close.append(m)
            for n in L2:
                if m.issubset(n) and support[m] <= support[n]:
                    close.remove(m)
                    del s[m]
                    break
    return close,s




# In[8]:

voc = readinvoc("/Users/Li/Desktop/data/vocab.txt")


# In[24]:

L,support =apriori("/Users/Li/Desktop/data/topic-4.txt",100)


# In[25]:

close,s = closed(L,support)


# In[26]:

writeout("/Users/Li/Desktop/data/closed-4.txt",s,voc)


# In[ ]:



