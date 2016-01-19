def run():
	voc = {} # index -> word
	for line in open('vocab.txt','r'):
		st = line.strip().split('\t')
		voc[int(st[0])] = st[1]

	for i in xrange(5):
		inputf = 'topic-'+str(i)+'.txt'
		voc_t = cutVocab(inputf, voc)
		outf = 'topic-'+str(i)+'.arff'
		generateARFF(inputf, voc, voc_t, outf)

def cutVocab(inputf, voc):
	voc_t = {}
	voc_count = {}
	for line in open(inputf):
		st = line.strip().split(' ')
		for s in st:
			if s not in voc_count:
				voc_count[s]=1
			else:
				voc_count[s]+=1
	
	count = 0
	for ind in sorted(voc.keys()):
		if str(ind) not in voc_count:
			continue
		if voc_count[str(ind)] >= 50:
			voc_t[voc[ind]] = count
			count += 1
	return voc_t

def generateARFF(inputf, voc, voc_t, outf):
	arff = open(outf,'w')

	nl='@relation '+inputf+'\n\n'
	arff.write(nl)

	vocsize=len(voc_t)
	for w in sorted(voc_t, key=voc_t.get):
		nl = '@attribute '+w+' {0,1}'+'\n'
		arff.write(nl)

	arff.write('\n@data\n')

	for line in open(inputf,'r'):
		st = line.strip().split(' ')
		nl = ['0' for i in xrange(vocsize)]
		for s in st:
			term = voc[int(s)]
			if term not in voc_t:
				continue
			nl[voc_t[term]] = '1'

		newl=','.join(nl)+'\n'
		arff.write(newl)

	arff.close()

run()