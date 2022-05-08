"""
Run as: ./graph_logs [log CSV file directory] [X measurement] [Y measurement]
"""
import os
import sys
import matplotlib.pyplot as plt


def get_pts(file_name, x_col_name, y_col_name):
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

        if tokens[xi] != "":
            last_x = int(tokens[xi])

        #if tokens[xi] != "" and tokens[yi] != "":
        if tokens[yi] != "" and last_x != None:
            #t = tokens[xi].split('.')
            #millis = int(t[0])
            #micros = int(t[1]) + (millis * 1000)
            
            #xs.append(micros)
            xs.append(last_x)
            ys.append(float(tokens[yi]))

    f.close()
    return (xs, ys)


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

    plt.plot(xs, ys)
    plt.show()
