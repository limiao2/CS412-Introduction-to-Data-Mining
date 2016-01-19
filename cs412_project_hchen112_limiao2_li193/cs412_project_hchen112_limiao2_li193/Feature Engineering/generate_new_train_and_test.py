# Libraries
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from sklearn.ensemble import RandomForestClassifier

# Read in Data
features = pd.DataFrame(pd.read_csv('features.csv'))
train = pd.DataFrame(pd.read_csv('train.csv'))
test = pd.DataFrame(pd.read_csv('test.csv'))

# Look at our data

features.head(5)
train.head(5)

# Create new training and testing dataset, merge features and train and test
# Because the original train and test do not have enough features

newtrain = pd.merge(train,features, on = ['bidder_id','outcome'])
newtest = pd.merge(test,features, on = 'bidder_id')

# Lookd at new train
newtrain.head(5)

# Drop useless features
newtrain = newtrain.drop('payment_account',1)
newtrain = newtrain.drop('address',1)

newtest = newtest.drop(['payment_account','address'],1)
newtest.shape
newtest = newtest.drop('outcome',1)

newtrain = newtrain.drop('Unnamed: 0',1)
newtest = newtest.drop('Unnamed: 0',1)


# We tried to do random forest in python firstly, but finally we decided to use R to implement this
# algorithm. But in final, we are still considering to use Python
#clf = RandomForestClassifier(n_estimators=10, max_depth=None, random_state=0)
#clf = clf.fit(newtrain)

# Write out our new train and test files
newtest.to_csv('newtest.csv', sep=',')
newtrain.to_csv('newtrain.csv', sep=',')

# Look at new data set
newtrain.head(5)

# Our try to do classification. We did it in R and the code are in another R file.
'''
X = newtrain.drop('outcome',1)
Y = newtrain['outcome']
X.head(5)
Y.head(5)


X.columns[1:]
XX = X.as_matrix(columns=X.columns[0:])
YY = Y.as_matrix(columns=Y.columns[0:])
Y = pd.DataFrame(Y)
clf = clf.fit(XX,YY)

'''
