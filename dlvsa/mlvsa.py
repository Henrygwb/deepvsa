import os
import numpy as np
import collections
import pandas as pd
from itertools import chain
from keras.utils import to_categorical
from keras.preprocessing.sequence import pad_sequences
from sklearn import svm
from sklearn.ensemble import RandomForestClassifier as rf
from seqlearn.perceptron import StructuredPerceptron
from sklearn.preprocessing import OneHotEncoder as onehot
from sklearn.metrics import precision_recall_fscore_support
"""
0: global
1: heap
2: stack
3: other
"""
np.random.seed(1234)

def load_data(traces_path, npz_path, save_npz = 0, use_npz = 0, one_trace=0):
    if use_npz == 1:
        data = np.load(npz_path)
        inst = data['inst']
        label = data['label']
    else:
        inst = []
        label = []
        if one_trace:
            inst_file = os.path.join(traces_path, 'binary')
            label_file = os.path.join(traces_path, 'region')
            label_df = pd.read_csv(label_file, sep='\n', header=None, names=['label'])
            label_unique = list(label_df['label'].unique())
            for i in label_unique:
                if len(i) > 2:
                    label_tmp = 10 * (int(i[0]) + 1) + int(i[2]) + 1
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
                inst_file = os.path.join(data_file, 'binary')
                label_file = os.path.join(data_file, 'region')
                label_df = pd.read_csv(label_file, sep='\n', header=None, names=['label'])
                label_df['label'] = label_df['label'].astype('str')
                label_unique = list(label_df['label'].unique())
                for i in label_unique:
                    if len(i) > 2:
                        label_tmp = 10 * (int(i[0]) + 1) + int(i[2]) + 1
                    else:
                        label_tmp = 10 * (int(i[0]) + 1)
                    label_df = label_df.replace(i, label_tmp)
                inst_df = pd.read_csv(inst_file, sep='\n', header=None, names=['binary'])
                inst_df = inst_df.binary.apply(lambda x: [int(a, 16) for a in x.split(' ')])
                inst.extend(inst_df.tolist())
                label.extend(label_df.values.tolist())
        if save_npz == 1:
            np.savez(npz_path, inst=inst, label=label)
    return inst, label

class MLVSA(object):
    def __init__(self,
                 inst_train,
                 label_train,
                 inst_test,
                 label_test,
                 seq_len,
                 train_region,
                 model_option):
        self.seq_len = seq_len
        self.model_option = model_option
        self.train_region = train_region
        self.X_train, self.Y_train, self.true_label, self.n_class = self.list2np(inst_train, label_train,
                                                                     seq_len, train_region, model_option)
        self.X_test, self.Y_test, _, _ = self.list2np(inst_train,label_train,
                                                      seq_len, train_region, model_option, testing=True)

    def predict_classes(self, proba):
        if proba.shape[-1] > 1:
            return proba.argmax(axis=-1)
        else:
            return (proba > 0.5).astype('int32')

    def list2np(self, inst, label, seq_len, train_region, model_option, testing=False):
        #inst_len_all = [len(aa) for aa in inst]
        #inst = np.array(list(chain.from_iterable(inst)))
        #label = [label[i] * inst_len_all[i] for i in xrange(len(label))]
        #label = np.hstack(label)
        label[label==11] = 13
        #label[label == 41] = 43
        #label[label == 42] = 43
        #label[label == 44] = 43

        label_list = [(train_region + 1) * 10 + id for id in xrange(5)]

        true_label = []
        for i in label_list:
            if np.where(label == i)[0].shape[0] != 0:
                true_label.append(i)
        if testing == True:
            print 'Testing true labels: '
            print true_label

            print 'Training true labels: '
            print self.true_label
            assert len(self.true_label) >= len(
                true_label), "Number of testing classes is larger than training classes."

            true_label = self.true_label
            n_class = self.n_class

        true_label = true_label
        n_class = len(true_label)

        for idx in xrange(n_class):
            label[label == true_label[idx]] = idx
        label[label > n_class] = n_class
        n_class = n_class + 1
        if testing == False:
            truncate_start = max([np.where(label == i)[0][0] for i in xrange(n_class - 1)])
            truncate_end = max([np.where(label == i)[0][-1] for i in xrange(n_class - 1)])
            inst = inst[truncate_start:truncate_end]
            label = label[truncate_start:truncate_end]

        num_sample = inst.shape[0] / seq_len
        if model_option == 3:
            X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len, inst.shape[1])
        else:
            X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)

        Y = label[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)

        return X, Y, true_label, n_class

    def fit(self):
        if self.model_option == 0:
            print "Using SVM >>>>>>>>>>>>>>>>>>>>>>>"
            self.model = svm.SVC(kernel = 'rbf', decision_function_shape = 'ovo')
            self.y_pred = np.zeros_like(self.Y_test)
            for i in xrange(self.seq_len):
                self.model.fit(self.X_train[:,i].reshape(-1,1), self.Y_train[:,i].reshape(-1, 1))
                self.y_pred[:,i] = self.model.predict(self.X_test[:,i].reshape(-1, 1))

        elif self.model_option == 1:
            print "Using RF >>>>>>>>>>>>>>>>>>>>>>>>"
            self.model = rf(n_estimators = 100)
            self.y_pred = np.zeros_like(self.Y_test)
            for i in xrange(self.seq_len):
                self.model.fit(self.X_train[:,i].reshape(-1, 1), self.Y_train[:,i].reshape(-1, 1))
                self.y_pred[:,i] = self.model.predict(self.X_test[:,i].reshape(-1, 1))

        elif self.model_option == 2:
            print "Using HMM >>>>>>>>>>>>>>>>>>>>>>>"
            self.X_train = self.X_train.flatten()
            self.Y_train = self.Y_train.flatten()
            self.X_train = self.X_train.reshape(self.X_train.shape[0], 1)

            self.X_test = self.X_test.flatten()
            self.Y_test = self.Y_test.flatten()
            self.X_test = self.X_test.reshape(self.X_test.shape[0], 1)

            self.model = StructuredPerceptron()
            oh = onehot()
            self.X_train =oh.fit_transform(self.X_train)
            self.X_test =  oh.transform(self.X_test)
            self.model.fit(self.X_train, self.Y_train, [self.Y_train.shape[0]])
            self.y_pred = self.model.predict(self.X_test, [self.Y_test.shape[0]])

            print 'Evaluating training results at region ' + str(self.train_region)
            precision, recall, f1, _ = precision_recall_fscore_support(self.Y_test, self.y_pred,
                                                                       labels=range(self.n_class), average='weighted')
            print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
            print '================================================'

            print 'Evaluating training results of positive labels at region ' + str(self.train_region)
            precision, recall, f1, _ = precision_recall_fscore_support(self.Y_test, self.y_pred,
                                                                       labels=range(self.n_class-1), average='weighted')
            print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
            print '================================================'

            for label in xrange(self.n_class):
                if label < len(self.true_label):
                    print 'Evaluating ' + str(self.true_label[label]) + ' ...'
                    precision, recall, f1, _ = precision_recall_fscore_support(self.Y_test, self.y_pred,
                                                                               labels=[label], average ='macro')
                    print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
                    print '================================================'
        return 0


if __name__ == "__main__":
    print '********************************'
    print '0: global...'
    print '1: heap...'
    print '2: stack...'
    print '3: other...'
    print '********************************'

    #traces_path_train = "../data/binutils/"
    traces_path_train = "/home/wzg13/Data/Traces/train_traces"
    traces_path_test = "../data/vulnerable/"

    seq_len = 200
    label = [0, 1, 2, 3]
    model_option = 1
    y_preds = []
    true_labels = []
    inst_train, label_train = load_data(traces_path_train, npz_path='../data/train_npz_bin.npz', save_npz=0, use_npz=1)
    inst_test, label_test = load_data(traces_path_test, npz_path='../data/test_npz_bin.npz', save_npz=0, use_npz=1) 
    for label in label:
        vsa = MLVSA(inst_train, label_train, inst_test, label_test, seq_len = seq_len,
                    train_region = label, model_option = model_option)
        vsa.fit()
