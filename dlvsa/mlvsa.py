import os
import re
import argparse
import pycrfsuite
import collections
import numpy as np
import pandas as pd
from itertools import chain
from sklearn import svm
from sklearn.ensemble import RandomForestClassifier as rf
from seqlearn.perceptron import StructuredPerceptron
from sklearn.preprocessing import OneHotEncoder as onehot
from sklearn.metrics import precision_recall_fscore_support

np.random.seed(1234)

def load_data(traces_path,
              npz_path,
              save_npz = 0,
              use_npz = 0,
              one_trace=0,
              truncate = 0,
              testing = 0
              ):
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
            if truncate and testing == 0:
                inst_file = os.path.join(traces_path, 'binary_without_lib')
                label_file = os.path.join(traces_path, 'region_without_lib')
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
                inst_file = os.path.join(data_file, 'binary')
                label_file = os.path.join(data_file, 'region')
                if truncate and testing == 0:
                    inst_file = os.path.join(data_file, 'binary_without_lib')
                    label_file = os.path.join(data_file, 'region_without_lib')
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

            inst = np.array(list(chain.from_iterable(inst)))
            label = [label[i] * inst_len_all[i] for i in xrange(len(label))]
            label = np.hstack(label)
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
                 model_option):

        self.seq_len = seq_len
        self.model_option = model_option
        self.X_train, self.Y_train, self.n_class = self.list2np(inst_train, label_train, seq_len)
        self.X_test, self.Y_test, _, = self.list2np(inst_test,label_test, seq_len)

    def predict_classes(self, proba):
        if proba.shape[-1] > 1:
            return proba.argmax(axis=-1)
        else:
            return (proba > 0.5).astype('int32')

    def list2np(self, inst, label, seq_len):
        label[label == 10] = 0
        label[label == 20] = 1
        label[label == 30] = 2
        label[label == 40] = 3
        n_class = 4

        num_sample = inst.shape[0] / seq_len
        X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)
        Y = label[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)

        return X, Y, n_class

    def fit(self):
        self.X_train = self.X_train
        self.Y_train = self.Y_train
        self.X_test = self.X_test
        self.Y_test = self.Y_test

        print '================================================'
        print "Data shape..."
        print self.X_train.shape
        print self.Y_train.shape
        print self.X_test.shape
        print self.Y_test.shape
        print "Counting the number of data in each category..."
        print collections.Counter(self.Y_train.flatten())
        print collections.Counter(self.Y_test.flatten())
        print '================================================'
        if self.model_option == 0:
            print "Using SVM >>>>>>>>>>>>>>>>>>>>>>>"
            self.model = svm.SVC(kernel='rbf', decision_function_shape='ovo')
            self.y_pred = np.zeros_like(self.Y_test)
            for i in xrange(self.seq_len):
                self.model.fit(self.X_train[:, i].reshape(-1, 1), self.Y_train[:, i].reshape(-1, 1))
                self.y_pred[:, i] = self.model.predict(self.X_test[:, i].reshape(-1, 1))

        elif self.model_option == 1:
            print "Using RF >>>>>>>>>>>>>>>>>>>>>>>>"
            self.model = rf(n_estimators=100)
            self.y_pred = np.zeros_like(self.Y_test)
            for i in xrange(self.seq_len):
                self.model.fit(self.X_train[:, i].reshape(-1, 1), self.Y_train[:, i].reshape(-1, 1))
                self.y_pred[:, i] = self.model.predict(self.X_test[:, i].reshape(-1, 1))

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
            self.X_train = oh.fit_transform(self.X_train)
            self.X_test = oh.transform(self.X_test)
            num_seq = self.X_train.shape[0]/self.seq_len
            length_train = [self.seq_len]*num_seq
            self.model.fit(self.X_train, self.Y_train, length_train)

            num_seq = self.X_test.shape[0]/self.seq_len
            length_test = [self.seq_len]*num_seq
            self.y_pred = self.model.predict(self.X_test, length_test)

        elif self.model_option == 3:
            print "Using CRF >>>>>>>>>>>>>>>>>>>>>>>"

            trainer = pycrfsuite.Trainer(verbose=True)
            for i in xrange(self.X_train.shape[0]):
                trainer.append(self.X_train[i,].astype('string'), self.Y_train[i,].astype('string'))

            trainer.set_params({
                'c1': 0.1,
                'c2': 0.01,
                'max_iterations': 2000,
                'feature.possible_transitions': True
            })

            trainer.train('crf.model')

            tagger = pycrfsuite.Tagger()
            tagger.open('crf.model')
            self.y_pred = []
            for i in xrange(self.X_test.shape[0]):
                self.y_pred.append(np.array(tagger.tag(self.X_test[i,].astype('string'))).astype('int'))
            self.y_pred = np.array(self.y_pred)

        print 'Evaluating testing results'
        precision, recall, f1, _ = precision_recall_fscore_support(self.Y_test.flatten(), self.y_pred.flatten(),
                                                                   labels=[0, 1, 2, 3], average='weighted')
        print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
        print '================================================'

        for i in xrange(4):
            print 'Evaluating testing results of positive labels at region ' + str(i)
            precision, recall, f1, _ = precision_recall_fscore_support(self.Y_test.flatten(), self.y_pred.flatten(),
                                                                       labels=[i], average='weighted')
            print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
            print '================================================'

        return self.y_pred

if __name__ == "__main__":
    print '********************************'
    print '0: global...'
    print '1: heap...'
    print '2: stack...'
    print '3: other...'
    print '********************************'

    train_traces_path = " "
    test_traces_path = " "

    truncate = 1
    seq_len = 200
    model_option = 3

    if truncate == 0:
        npz_path_train = '../data/train_npz_bin.npz'
        save_npz_train = 0
        use_npz_train = 1
        npz_path_test = '../data/test_npz_bin.npz'
        save_npz_test = 0
        use_npz_test = 1
    else:
        npz_path_train = '../data/train_npz_bin_truncated.npz'
        save_npz_train = 0
        use_npz_train = 1
        npz_path_test = '../data/test_npz_bin.npz'
        save_npz_test = 0
        use_npz_test = 1

    inst_train, label_train = load_data(train_traces_path, npz_path=npz_path_train,
                                        save_npz=save_npz_train, use_npz=use_npz_train, truncate = truncate)
    inst_test, label_test = load_data(test_traces_path, npz_path=npz_path_test,
                                      save_npz=save_npz_test, use_npz=use_npz_test, truncate = truncate)
    print inst_train.shape
    print label_train.shape
    print inst_test.shape
    print label_test.shape

    vsa = MLVSA(inst_train, label_train, inst_test, label_test, seq_len = seq_len, model_option = model_option)
    vsa.fit()
