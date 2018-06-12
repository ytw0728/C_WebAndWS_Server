#-*- coding: utf-8 -*-

import pymysql
import io
import codecs
from operator import eq

'''
ProblemSet.dat 중복 주의!!


[format]
name1
name2
name3
...


#define DBPORT 8890
#define DBHOST "sccc.kr"
#define DBUSER "root"
#define DBPASSWD "root"
#define DBNAME "networkproject"
'''

print "PROBLEMSET_MAKER::Need ProblemSet.dat"
print "\n[format(don't care about new-line character)]\nname1\nname2\nname3\n...\n"
print "Please care about DUPLICATION\n"

conn = pymysql.connect(host='sccc.kr', 
		user='root', 
		password='root',
		db='networkproject', 
		charset='utf8',
		port=8890)

curs = conn.cursor()

f = codecs.open("ProblemSet.dat", "r", encoding='utf-8')
sql = 'select count(questions.id) from questions where true;'
curs.execute(sql)
idx = int(curs.fetchone()[0])

'''
idx+=1;
sql = 'insert into questions values (%d, \'롤케이크\');' % (idx+1)
print sql
curs.execute(sql)
conn.commit()
'''

for i in f:
	if(eq(i, "\n") == True) :
		continue
	newline = (i.rstrip('\n')).encode('utf-8')
#print newline
	idx+=1
	print idx
	sql = 'insert into questions values (%d, \'%s\');' %(idx, newline)
	print sql
	curs.execute(sql)
	conn.commit()

conn.close()
f.close()
