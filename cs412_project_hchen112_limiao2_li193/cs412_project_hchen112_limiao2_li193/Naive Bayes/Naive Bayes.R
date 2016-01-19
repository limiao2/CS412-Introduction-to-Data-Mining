###Naive Bayes Classifier###
revised_train = read.csv("revised_train1.csv", header = T)
revised_test = read.csv("revised_test1.csv", header = T)

##select binary feature index from train and test data set##
train_idx=c(2,3:17,24,106,300:306)
test_idx=c(1,2:16,23,105,299:305)


##Max-Min Normalization Data##
for (j in 1:dim(revised_train)[2]){
     revised_train[,j]=as.numeric(revised_train[,j])
}

for (j in 1:dim(revised_test)[2]){
     revised_test[,j]=as.numeric(revised_test[,j])
}

for (i in 2:dim(revised_train)[2]){
    if ((i %in% train_idx)==FALSE){
       max=max(revised_train[,i])
       min=min(revised_train[,i])
       for (j in 1:dim(revised_train)[1]){
          revised_train[j,i]=(revised_train[j,i]-min)/(max-min)
       }
     }
  }
for (i in 1:dim(revised_test)[2]){
    if ((i %in% test_idx)==FALSE){
       max=max(revised_test[,i])
       min=min(revised_test[,i])
       for (j in 1:dim(revised_test)[1]){
          revised_test[j,i]=(revised_test[j,i]-min)/(max-min)
       }
     }
  }
for (i in 1:dim(revised_train)[1]){
    if(revised_train[i,2]==2){
        revised_train[i,2]=1
     }
 else{
        revised_train[i,2]=0
  }
}
for (i in 1:dim(revised_train)[1]){
    if(revised_train[i,106]==2){
        revised_train[i,106]=1
     }
 else{
        revised_train[i,106]=0
  }
}
for (i in 1:dim(revised_test)[1]){
    if(revised_test[i,1]==2){
        revised_test[i,1]=1
     }
 else{
        revised_test[i,1]=0
  }
}
for (i in 1:dim(revised_test)[1]){
    if(revised_test[i,105]==2){
        revised_test[i,105]=1
     }
 else{
        revised_test[i,105]=0
  }
}

##Compute the sum of count for binary and numeric based on robot or human##
cnt=matrix(0,2,dim(revised_train)[2])

for (i in 1:dim(revised_train)[1]){
  for (j in 2:dim(revised_train)[2]){
      if (revised_train[i,1]==1){
           if((j %in% train_idx)==TRUE && revised_train[i,j]==1){
             cnt[1,j]=cnt[1,j]+1
            }
      if ((j %in% train_idx)==FALSE){
             cnt[1,j]=cnt[1,j]+revised_train[i,j]
            }
      }
      if (revised_train[i,1]==0){
            if((j %in% train_idx)==TRUE && revised_train[i,j]==1){
             cnt[2,j]=cnt[2,j]+1
             }
      if((j %in% train_idx)==FALSE){
             cnt[2,j]=cnt[2,j]+revised_train[i,j]
            }
     }
   }
}

##Laplace Correction for Naive Bayes##
for (i in 1:dim(cnt)[1]){
  for(j in 2:dim(cnt)[2]){
     if(cnt[i,j]==0){
         cnt[i,j]=cnt[i,j]+1
      }
  }
}

total=0
for(i in 1:dim(revised_train)[1]){
 if(revised_train[i,1]==0){
   total=total+1
 }
}
prior=total/dim(revised_train)[1]
remain=dim(revised_train)[1]-total

##To estimate the parameter for binary features and numeric feature##
parameter=matrix(0,4,dim(revised_train)[2])

for (i in 1:dim(revised_train)[1]){
  for (j in 2:dim(revised_train)[2]){
      if (revised_train[i,1]==1){
      if ((j %in% train_idx)==FALSE){
             parameter[2,j]=parameter[2,j]+(revised_train[i,j]-cnt[1,j]/remain)^2
            }
      }
      if (revised_train[i,1]==0){
      if((j %in% train_idx)==FALSE){
             parameter[4,j]=parameter[4,j]+(revised_train[i,j]-cnt[2,j]/total)^2
            }
     }
   }
}

 

 for (j in 2:dim(parameter)[2]){
     if((j %in% train_idx)==TRUE){
         parameter[1,j]=cnt[1,j]/remain
         parameter[2,j]=1-parameter[1,j]
         parameter[3,j]=cnt[2,j]/total
         parameter[4,j]=1-parameter[3,j]
}
     if((j %in% train_idx)==FALSE){
         parameter[1,j]=cnt[1,j]/remain
         parameter[2,j]=parameter[2,j]/remain
         parameter[3,j]=cnt[2,j]/total
         parameter[4,j]=parameter[4,j]/total
 }
}
         

parameter=parameter[,-1]

##To caculate conditional probability based on robot and human##
p_robot=matrix(0,dim(revised_test)[1],dim(revised_test)[2])
p_human=matrix(0,dim(revised_test)[1],dim(revised_test)[2])

myfactor=function(a,b,c){
result=((a)^c)*((b)^(1-c))
return(result)
}

mynumeric=function(d,e,f){
result=(1/sqrt(2*pi*e))*exp(-(f-d)^2/(2*e))
return(result)
}



for (i in 1:dim(revised_test)[1]){
  for (j in 1:dim(revised_test)[2]){
   if((j %in% test_idx)==TRUE){
     p_robot[i,j]=myfactor(parameter[1,j],parameter[2,j],revised_test[i,j])
     p_human[i,j]=myfactor(parameter[3,j],parameter[4,j],revised_test[i,j])
 }
   else{
     p_robot[i,j]=mynumeric(parameter[1,j],parameter[2,j],revised_test[i,j])
     p_human[i,j]=mynumeric(parameter[3,j],parameter[4,j],revised_test[i,j])
 }
}
}

for (i in 1:dim(p_robot)[1]){
  for(j in 1:dim(p_robot)[2]){
     if (p_robot[i,j]==0){
       p_robot[i,j]=0.0001
     }
   }
 }

for (i in 1:dim(p_human)[1]){
  for(j in 1:dim(p_human)[2]){
     if (p_human[i,j]==0){
       p_human[i,j]=0.0001
     }
   }
 }

##To select most important features(Top-10) from randomForest Classifier##
points=importance(rf.robots)
points=data.frame(points)
idx={1:305}
points=cbind(idx,points)
colnames(points)=c("idx","MSE","Purity")
attach(points)
points=points[order(MSE),]

top_idx=points[296:305,1]

robot=matrix(0,dim(p_robot)[1],1)
human=matrix(0,dim(p_human)[1],1)

 for(j in 1:dim(p_robot)[2]){
     if ((j %in% top_idx)==TRUE){
           robot=cbind(robot,p_robot[,j])
}
}

 for(j in 1:dim(p_human)[2]){
     if ((j %in% top_idx)==TRUE){
           human=cbind(human,p_human[,j])
}
}
   
robot=robot[,-1]
human=human[,-1]

##To compute posterior##
robot_posterior=rep(1,dim(robot)[1])
human_posterior=rep(1,dim(human)[1])

for(i in 1:dim(robot)[1]){
 for(j in 1:dim(robot)[2]){
   robot_posterior[i]=robot_posterior[i]*robot[i,j]
}
}

for(i in 1:dim(human)[1]){
 for(j in 1:dim(human)[2]){
   human_posterior[i]=human_posterior[i]*human[i,j]
}
}

for (i in 1:length(robot_posterior)){
     if(robot_posterior[i]==0){
       robot_posterior[i]=0.0001
   }
}

for (i in 1:length(human_posterior)){
      if(human_posterior[i]==0){
          human_posterior[i]=0.0001
     }
}


output=rep(0,dim(revised_test)[1])

for (i in 1:dim(revised_test)[1]){
      output[i]=robot_posterior[i]*(1-prior)/(robot_posterior[i]*(1-prior)+human_posterior[i]*prior)
}

 
##Final Output##
sample = read.csv("sampleSubmission.csv", header = T)
filepath = "E:/Stat/cs 412/final project/Mid_Submission_damn.csv"
output1=sample$bidder_id
myout = data.frame(bidder_id=output1, prediction = output)
write.csv(myout, file=filepath,  row.names=FALSE, quote = FALSE)
