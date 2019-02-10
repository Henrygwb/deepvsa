import os
#os.environ["THEANO_FLAGS"] = "device=gpu0, floatX=float32"
os.environ["CUDA_VISIBLE_DEVICES"]="0,1,2,3"
import re
import argparse
import collections
import numpy as np
import pandas as pd
from itertools import chain
from scipy import io
from keras import backend as K
from keras import initializers
from AttentionLSTM import AttentionLSTM
from keras.utils import to_categorical
from keras.engine.topology import Layer
from sklearn.utils import class_weight
from keras.models import Sequential, Model, load_model
from keras.layers.embeddings import Embedding
from keras.preprocessing.sequence import pad_sequences
from sklearn.metrics import precision_recall_fscore_support
from keras.layers import Dense, TimeDistributed, Bidirectional, LSTM, Input, GRU, SimpleRNN, concatenate

np.random.seed(1234)

def load_data(traces_path,
              model_option,
              npz_path,
              save_npz = 0,
              use_npz = 0,
              one_trace=0,
              inst_len=0,
              truncate = 0,
              ):
    if use_npz == 1:
        data = np.load(npz_path)
        inst = data['inst']
        label = data['label']
        if model_option == 3:
            inst_len = len(inst[0])
        else:
            inst_len = 0
    else:
        inst = []
        label = []
        if one_trace:
            if truncate:
                inst_file = os.path.join(traces_path, 'binary_without_lib')
                label_file = os.path.join(traces_path, 'region_without_lib')
            else:
                inst_file = os.path.join(traces_path, 'binary')
                label_file = os.path.join(traces_path, 'region')
            label_df = pd.read_csv(label_file, sep='\n', header=None, names=['label'])
            label_df['label'] = label_df['label'].astype('str')
            label_unique = list(label_df['label'].unique())
            for i in label_unique:
                if len(i) > 2:
                    label_tmp = 10 * (int(i[0]) + 1)
                else:
                    label_tmp = 10 * (int(i[0]) + 1)
                label_df = label_df.replace(i, label_tmp)
            inst_df = pd.read_csv(inst_file, sep='\n', header=None, names=['binary'])
            inst_df = inst_df.binary.apply(lambda x: [int(a, 16) for a in x.split(' ')])
            inst.extend(inst_df.tolist())
            label.extend(label_df.values.tolist())
        else:
            data_set = os.listdir(traces_path)
            for folder in data_set:
                print folder
                data_file = os.path.join(traces_path, folder)
                if truncate:
                    inst_file = os.path.join(data_file, 'binary_without_lib')
                    label_file = os.path.join(data_file, 'region_without_lib')
                else:
                    inst_file = os.path.join(data_file, 'binary')
                    label_file = os.path.join(data_file, 'region')
                label_df = pd.read_csv(label_file, sep='\n', header=None, names=['label'])
                label_df['label'] = label_df['label'].astype('str')
                label_unique = list(label_df['label'].unique())
                for i in label_unique:
                    if len(i) > 2:
                        label_tmp = 10 * (int(i[0]) + 1)
                    else:
                        label_tmp = 10 * (int(i[0]) + 1)
                    label_df = label_df.replace(i, label_tmp)
                inst_df = pd.read_csv(inst_file, sep='\n', header=None, names=['binary'])
                inst_df = inst_df.binary.apply(lambda x: [int(a, 16) for a in x.split(' ')])
                inst.extend(inst_df.tolist())
                label.extend(label_df.values.tolist())

        inst_len_all = [len(aa) for aa in inst]
        if inst_len == 0:
            inst_len = collections.Counter(inst_len_all).keys()[-3]

        if model_option == 3:
            inst = pad_sequences(inst, maxlen=inst_len, truncating='post')
            label = np.array(label).flatten()
        else:
            inst = np.array(list(chain.from_iterable(inst)))
            label = [label[i] * inst_len_all[i] for i in xrange(len(label))]
            label = np.hstack(label)
        if save_npz == 1:
            np.savez(npz_path, inst=inst, label=label)
        print inst.shape
        print label.shape

        label[label == 10] = 0
        label[label == 20] = 1
        label[label == 30] = 2
        label[label == 40] = 3

        num_sample = inst.shape[0] / seq_len
        Y = label[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)

        print '================================================'
        print "Data shape..."
        print "Counting the number of data in each category..."
        a = np.bincount(Y.flatten()) / float(sum(np.bincount(Y.flatten())))
        print a
        print '================================================'
    return a, inst.shape[0]

if __name__ == "__main__":
    print '********************************'
    print '0: global...'
    print '1: heap...'
    print '2: stack...'
    print '3: other...'
    print '********************************'


    train_traces_path = "../../original_data/train_traces_truncated"
    #test_traces_path = "../../original_data/vulnerable"

    seq_len = 200
    model_option = 0
    epochs = 1
    truncate = 1
    npz_path_train = '../data/train_npz_bin.npz'
    save_npz_train = 0
    use_npz_train = 0

    # all cases
    load_data(train_traces_path, model_option = model_option, npz_path=npz_path_train,
              save_npz=save_npz_train, use_npz=use_npz_train, truncate = truncate, one_trace=0)

    # every case
    # name = []
    # wei = []
    # lenn = []
    # for i in os.listdir(train_traces_path):
    #     print i
    #     a, l = load_data(os.path.join(train_traces_path,i), model_option = model_option, npz_path=npz_path_train,
    #                      save_npz=save_npz_train, use_npz=use_npz_train, truncate = truncate, one_trace=1)
    #     name.append(i)
    #     wei.append(a)
    #     lenn.append(l)
    # print 'aaaa'
    # wei = np.array(wei)
    # wei_global = wei[:, 0]
    # wei_heap = wei[:, 1]
    # for ii in np.argsort(wei_global)[0:5]:
    #     if ii in np.argsort(wei_heap)[-10:]:
    #         continue
    #     else:
    #         name_tmp = name[ii]
    #     print name_tmp