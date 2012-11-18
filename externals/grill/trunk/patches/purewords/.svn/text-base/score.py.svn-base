# semantic network score generator
# (c) Thomas Grill, 2005 (gr@grrrr.org)

import pickle

filewords = "words.txt"
filescore = "score.txt"

def load():
    '''load dictionary of words from file'''
    global words,filewords
    f = open(filewords,"r")
    try:
        words = pickle.load(f)
        print "List of words:",words.values()
    except:
        words = {}
        print "Empty word list"
    f.close()

def save():
    '''store dictionary of words to file'''
    global words,filewords
    print "List of words:",words.values()
    f = open(filewords,"w")
    pickle.dump(words,f)
    f.close()

def setword(w,index):
    '''set one single indexed word'''
    global words
    # convert symbol to string and add to score
    words[index] = str(w) 

def getword(index):
    '''get one single indexed word'''
    global words
    return words[index]

def reset():
    '''reset score'''
    global score
    score = []

def output():
    '''output score to file'''
    global score,filescore
    f = open(filescore,"w")
    # write space delimited words to file
    for w in score:
        f.write(w+" ")
    f.close()

def next(index):
    '''append indexed word to score'''
    global score,words
    score.append(words[index])    

load()
reset()
