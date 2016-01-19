LOAD DATA LOCAL INFILE '/Users/Li/Desktop/Data_Q2.txt'
INTO TABLE dbPerson.hw2 
FIELDS TERMINATED BY '\t'
LINES TERMINATED BY '\n';

SELECT distinct City, Category,Price, Year
from dbPerson.hw2
group by City, Category,Price, Year;

SELECT distinct City, Category,Price, Year
from dbPerson.hw2
group by City, Category,Price, Year;

SELECT distinct State, Category,Price, Year
from dbPerson.hw2
group by State, Category,Price, Year;

SELECT distinct Category,Price, Quarter
from dbPerson.hw2
group by Category,Price, Quarter;


SELECT State,Category,Quarter,count(*)
FROM dbPerson.hw2
WHERE State = "Illinois" and Category = "food" and Quarter = "Q1";

SELECT City,Price,Year,count(*)
FROM dbPerson.hw2
WHERE City = "Chicago" and Price= "cheap" and Year = 2013;


