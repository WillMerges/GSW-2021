"""
Run as: ./graph_logs [log CSV file directory] [X measurement] [Y measurement]
"""
import os
import sys
import matplotlib.pyplot as plt
import math
import numpy as np


def get_pts(file_name, x_col_name, y_col_name):
    global l
    
    f = open(file_name, "r")

    line = f.readline()

    tokens = line.split(',')

    xi = -1
    yi = -1
    i = 0
    for token in tokens:
        if token == x_col_name:
            xi = i
        elif token == y_col_name:
            yi = i

        i = i + 1

    if xi == -1:
        print("no X measurement: " + x_col_name)
        return ([], [])

    if yi == -1:
        print("no Y measurement: " + y_col_name)
        return ([], [])

    xs = []
    ys = []
    last_x = None
    for line in f:
        tokens = line.split(',')

        #if tokens[xi] != "":
        #    last_x = int(tokens[xi])

        if tokens[xi] != "" and tokens[yi] != "":
        #if tokens[yi] != "" and last_x != None:
            #t = tokens[xi].split('.')
            #millis = int(t[0])
            #micros = int(t[1]) + (millis * 1000)
            millis = int(tokens[xi])
            micros = int(tokens[xi + 1]) + (millis * 1000)

            if micros < 9.35 * 1e8 or micros > 9.375 * 1e8:
                continue

            #xs.append(last_x)
            #ys.append(float(tokens[yi]))
            
            y = float(tokens[yi]) * 2.442 / (1 << 23)
            ff = (((y / 0.01)) / 352)
            ff = ff * 2.06579

            a = -9.417589e-11
            b = -8.261031e-4
            c = 1.294404e-4 - ff
            xx = (-b - math.sqrt((b*b) - (4 * a * c))) / (2 * a)
   
            #if xx <= 0:
            #    continue

            xs.append(micros)
            ys.append(xx)

            """
            l = l[1::]
            l.append(xx)
            s = 0.0
            for i in l:
                s = s + i
            
            ys.append(s / len(l))
            """


    f.close()
    return (xs, ys)


def rolling_avg(ys, window_size):
    l = [0] * window_size

    zs = []
    for y in ys:
        l = l[1::]
        l.append(y)

        s = 0.0
        for ll in l:
            s = s + ll

        zs.append(s / len(l))

    return zs


def windowed_avg(xs, ys, window_size):
    x2 = []
    y2 = []

    sx = 0.0
    sy = 0.0
    i = 0
    c = 0
    for x in xs:
        sx = sx + x
        sy = sy + ys[i]

        i = i + 1
        c = c + 1

        if c == window_size:
            x2.append(sx / window_size)
            y2.append(sy / window_size)

            sx = 0.0
            sy = 0.0

            c = 0

    return (x2, y2)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: ./graph_logs [log CSV file directory] [X measurement] [Y measurement]")
        exit(-1)

    xs = []
    ys = []

    for f in os.listdir(sys.argv[1]):
        if f.find(".csv") == -1:
            # not a CSV
            continue
       
        print("parsing file: " + f)

        (x_new, y_new) = get_pts(sys.argv[1] + "/" + f, sys.argv[2], sys.argv[3])
        #print(x_new)
        #print(y_new)

        xs.extend(x_new)
        ys.extend(y_new)

    xs, ys = zip(*sorted(zip(xs, ys)))
    
    print(xs[0:10])
    print(ys[0:10])
    print(xs[-10::])
    print(ys[-10::])

    a = plt.subplot("111")
    #a.plot(xs, ys, label="raw")

    zs = rolling_avg(ys, 200)

    b = plt.subplot("111")
    b.plot(xs, zs, label="FIR")

    (wx, wy) = windowed_avg(xs, ys, 175)

    c = plt.subplot("111")
    #c.plot(wx, wy, label="windowed")

    plt.show()
