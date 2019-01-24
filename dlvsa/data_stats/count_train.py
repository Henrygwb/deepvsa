import os
#os.environ["THEANO_FLAGS"] = "device=gpu0, floatX=float32"
os.environ["CUDA_VISIBLE_DEVICES"]="0"
import numpy as np

from keras.models import Sequential, Model
from keras.layers import Dense, TimeDistributed, Bidirectional, LSTM, concatenate, Input, GRU, SimpleRNN
from keras.layers.embeddings import Embedding
from keras.preprocessing.sequence import pad_sequences
import re
import argparse
from keras.utils import to_categorical
from keras import backend as K
from keras.engine.topology import Layer
from keras import initializers

"""
0: global
1: heap
2: stack
3: other
"""
np.random.seed(1234)

class count_categories(object):
    def __init__(self, trace_folder, seq_len, label_train, model_option):
        self.seq_len = seq_len
        self.label_train = label_train
        self.model_option = model_option
	self.load_data(trace_folder)

    def process_data(self, data_file, label_file):
        with open(label_file) as f:
            label_txt = f.readlines()
        with open(data_file) as f:
            data_txt = f.readlines()
        data = []
        label = []
        for i in xrange(len(data_txt)):
            data_tmp = re.sub('\s', '', data_txt[i])
            label_tmp = label_txt[i]
            if len(label_tmp) > 2:
                label_tmp = 10 * (int(label_tmp[0]) + 1) + int(label_tmp[2]) + 1
            else:
                label_tmp = 10 * (int(label_tmp[0]) + 1)
            label_tmp = int(label_tmp)
            inst_tmp = []
            for j in xrange((len(data_tmp)) / 2):
                if self.model_option == 4:
                    dec = int(data_tmp[j * 2:(j + 1) * 2], 16)+1
                    inst_tmp.append(dec)
                else:
                    dec = int(data_tmp[j * 2:(j + 1) * 2], 16)
                    data.append(dec)
                    label.append(label_tmp)
            if self.model_option == 4:
                data.append(inst_tmp)
                label.append(label_tmp)
        return data, label

    def load_data(self, trace_folder):
        check_path= os.path.dirname(os.path.normpath(trace_folder))
        if self.model_option == 0 or self.model_option == 1 or self.model_option == 2 or self.model_option == 3:
            check_file = os.path.join(check_path, 'train_seq.npz')
        else:
            check_file = os.path.join(check_path, 'train_seq_inst.npz')

        if os.path.isfile(check_file):
            data = np.load(check_file)
            self.X = data['X']
            self.Y = data['Y']
            self.Y_one_hot = data['Y_one_hot']
            self.n_class = data['n_class'].tolist()
            self.true_label = data['true_label']
            if self.model_option == 4:
                self.inst_len = self.X.shape[-1]
        else:
            data = []
            label = []
            data_set = os.listdir(trace_folder)
            if self.model_option == 4:
                for folder in data_set:
                    data_file = os.path.join(trace_folder, folder)
                    inst_file = os.path.join(data_file, 'binary')
                    label_file = os.path.join(data_file, 'region')
                    data_temp, label_temp = self.process_data(inst_file, label_file)
                    data.extend(data_temp)
                    label.extend(label_temp)
                data = pad_sequences(data)

            else:
                for folder in data_set:
                    data_file = os.path.join(trace_folder, folder)
                    inst_file = os.path.join(data_file, 'binary')
                    label_file = os.path.join(data_file, 'region')
                    data_temp, label_temp = self.process_data(inst_file, label_file)
                    data.extend(data_temp)
                    label.extend(label_temp)
                data = np.array(data)
            label = np.asarray(label)
            label_1 = label.copy()

            label_list = []
            for label_t in self.label_train:
                for id in xrange(5):
                    label_list.append((label_t+1)*10+id)

            true_label = []
            for i in xrange(len(label_list)):
                if np.where(label == label_list[i])[0].shape[0] != 0:
                    true_label.append(label_list[i])

            self.true_label = true_label
            self.n_class = len(true_label)

            ii = 0
            for label_id in true_label:
                label_1[np.where(label == label_id)] = ii
                ii = ii+1

            all_list = []
            for label_t in [0,1,2,3]:
                for id in xrange(5):
                    all_list.append((label_t+1)*10+id)

            for label_id in all_list:
                if label_id in true_label:
                    continue
                else:
                    label_1[np.where(label == label_id)] = ii

            if len(all_list) != len(true_label) and len(self.label_train)!=4:
                self.n_class = self.n_class + 1

            num_sample = label.shape[0] / self.seq_len
            if self.model_option == 4:
                self.X = data[0:(num_sample * self.seq_len), ].reshape(num_sample, self.seq_len, data.shape[1])
                self.inst_len = data.shape[1]
            else:
                self.X = data[0:(num_sample * self.seq_len), ].reshape(num_sample, self.seq_len)

            self.Y = label_1[0:(num_sample*self.seq_len),].reshape(num_sample, self.seq_len)

            self.Y_one_hot = np.zeros((self.Y.shape[0], self.seq_len, self.n_class))
            for id in xrange(self.Y.shape[0]):
                self.Y_one_hot[id, np.arange(self.seq_len), self.Y[id]] = 1

            np.savez(check_file, X = self.X, Y = self.Y, Y_one_hot = self.Y_one_hot, n_class = self.n_class,
                     true_label = self.true_label)

    def categories_stats(self):
        print "Counting the number of data in each category..."
        print "Existing labels: "
        print self.true_label
        ii = 0
        for label in self.true_label:
            print 'Current label: ' + str(label)
            print np.where(self.Y == ii)[0].shape[0]
            ii = ii + 1
        print 'Done...'


if __name__ == "__main__":
    print '********************************'
    print '0: global...'
    print '1: heap...'
    print '2: stack...'
    print '3: other...'
    print '********************************'

    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--trace", help="trace folder")
    parser.add_argument("-q", "--seq_len", help="sequence lenght", type=int)
    parser.add_argument("-l", "--label_train", nargs='+', type=int)
    parser.add_argument("-m", "--model_option", type=int)
    arg = parser.parse_args()

    #trace_folder = '/Users/wenbo/Desktop/deepvsa/trace'
    trace_folder = '/home/wzg13/work/deepvsa/Traces/core_inet_utils'  
    seq_len = 400
    label_train = [0,1,2,3]
    model_option = 4
    print trace_folder
    print seq_len
    print label_train
    print model_option

    """
    trace_folder = arg.trace
    seq_len = arg.seq_len
    label_train = arg.label_train
    model_option = arg.model_option
    """  
    vsa_1 = count_categories(trace_folder=trace_folder, seq_len=seq_len, model_option=model_option, label_train=label_train)
    print vsa_1.X.shape
    print vsa_1.Y.shape
    print vsa_1.Y_one_hot.shape
    vsa_1.categories_stats()
