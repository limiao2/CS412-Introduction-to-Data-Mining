library(faraway)

mydata <- read.csv("/Users/Li/Desktop/sd.csv",head = TRUE, sep = ",")
head(mydata)
summary(mydata)
mydata$X3

mydata[0,1]

names(mydata)[1] = "x"
names(mydata)[2] = "y"
names(mydata)[4] = "cluster"

keeps = c("x","y","cluster")
d = mydata[keeps]

head(d)
d$cluster = factor(d$cluster)
summary(d)

library(ggplot2)

p = ggplot(d,aes(x,y))
p + aes(shape = factor(cluster)) + geom_point(aes(colour = factor(cluster)))

ddd<- read.csv("/Users/Li/Desktop/dd.csv",head = TRUE, sep = ",")
head(ddd)

pp = ggplot(ddd,aes(x,y))
pp + aes(shape = factor(cluster)) + geom_point(aes(colour = factor(cluster)))


a = data.frame(read.csv("/Users/Li/Desktop/step2a.txt",head = FALSE, sep = ","))
head(a)
pa = ggplot(a,aes(V1,V2))
pa + aes(shape = factor(V3)) + geom_point(aes(colour = factor(V3)))

b= data.frame(read.csv("/Users/Li/Desktop/step2b.txt",head = FALSE, sep = ","))
head(b)
pa = ggplot(b,aes(V1,V2))
pa + aes(shape = factor(V3)) + geom_point(aes(colour = factor(V3)))

