#!/usr/bin/python
# Python imports
import numpy as np
import sys, re, os

def loadFile(filename):
    f = open(filename, "r")
    weights = []
    for line in f:
        tokens = line.split("\t")
        weights.append(tokens[-1])
    print(weights)
    return np.asarray(weights)

def compareWeight(pre, cur):
    pre_zeros = np.where(pre == 0)[0]
    cur_zeros = np.where(cur == 0)[0]
    mySet = set(cur_zeros.flatten())
    for p in pre_zeros:
        if p not in mySet:
            return False
    return True

def checkAllWeights():
    unchanged = True
    for i in range(50):
        f = "o_weights_info_trained_intern_" + str(i) + ".txt"
        filename = os.path.join("./" + f)
        if not os.path.isfile(filename):
            print("File: {} does not exist".format(filename))
        if i == 0:
            pre_weight = loadFile(filename)
        else:
            cur_weight = loadFile(filename)
            print("Result: {}".format(unchanged))
            unchanged = unchanged and compareWeight(pre_weight, cur_weight)
            pre_weight = cur_weight

def main():
    checkAllWeights()


# call the main
if __name__ == "__main__":
    main()
