import os
#os.environ["THEANO_FLAGS"] = "device=gpu0, floatX=float32"
#os.environ["CUDA_VISIBLE_DEVICES"]="0,1,2,3"
import collections
import numpy as np
import pandas as pd
from itertools import chain
from keras.models import load_model
from keras.preprocessing.sequence import pad_sequences
from sklearn.metrics import precision_recall_fscore_support

np.random.seed(1234)

def load_data(traces_path, model_option, npz_path, use_npz = 0, save_npz = 0, one_trace=0, inst_len=0):
    if use_npz == 1:
        data = np.load(npz_path)
        inst = data['inst']
        label = data['label']
        inst_len_all = data['inst_len_all']
        if model_option == 3:
            inst_len = len(inst[0])
        else:
            inst_len = 0
    else:
        inst_orin = []
        label_orin = []
        if one_trace:
            inst_file = os.path.join(traces_path, 'binary')
            label_file = os.path.join(traces_path, 'region')
            label_df = pd.read_csv(label_file, sep='\n', header=None, names=['label'])
            label_df['label'] = label_df['label'].astype('str')
            label_unique = list(label_df['label'].unique())
            for i in label_unique:
                label_tmp = 10 * (int(i[0]) + 1)
                label_df = label_df.replace(i, label_tmp)
            inst_df = pd.read_csv(inst_file, sep='\n', header=None, names=['binary'])
            inst_df = inst_df.binary.apply(lambda x: [int(a, 16) for a in x.split(' ')])
            inst_orin.extend(inst_df.tolist())
            label_orin.extend(label_df.values.tolist())
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
                    label_tmp = 10 * (int(i[0]) + 1)
                    label_df = label_df.replace(i, label_tmp)
                inst_df = pd.read_csv(inst_file, sep='\n', header=None, names=['binary'])
                inst_df = inst_df.binary.apply(lambda x: [int(a, 16) for a in x.split(' ')])
                inst_orin.extend(inst_df.tolist())
                label_orin.extend(label_df.values.tolist())

        inst_len_all = [len(aa) for aa in inst_orin]
        if inst_len == 0:
            inst_len = collections.Counter(inst_len_all).keys()[-3]

        if model_option == 3:
            for i in xrange(len(inst_orin)):
                inst_orin[i] = map(lambda x: x + 1, inst_orin[i])
            inst = pad_sequences(inst_orin, maxlen=inst_len, truncating='post')
            label = np.array(label_orin).flatten()
        else:
            inst = np.array(list(chain.from_iterable(inst_orin)))
            label = [label_orin[i] * inst_len_all[i] for i in xrange(len(label_orin))]
            label = np.hstack(label)
        if save_npz == 1:
            np.savez(npz_path, inst=inst, label=label, inst_len_all = inst_len_all)
    return inst_len_all, inst, label, inst_len

class DEEPVSATEST(object):
    def __init__(self,
                 inst,
                 label,
                 truncate,
                 seq_len,
                 model_option):

        self.seq_len = seq_len
        self.model_option = model_option
        self.X_test, self.Y_test= self.list2np(inst, label, self.seq_len, self.model_option)
        self.build_model(truncate)

    def predict_classes(self, proba):
        if proba.shape[-1] > 1:
            return proba.argmax(axis=-1)
        else:
            return (proba > 0.5).astype('int32')

    def list2np(self, inst, label, seq_len, model_option):
        label[label == 10] = 0
        label[label == 20] = 1
        label[label == 30] = 2
        label[label == 40] = 3
        num_sample = inst.shape[0] / seq_len
        if model_option == 3:
            X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len, inst.shape[1])
        else:
            X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)

        Y = label[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)
        return X, Y

    def build_model(self, truncate):
        if self.model_option == 0:
            print "Using Bi-SimpleRNN >>>>>>>>>>>>>>>>>>"
            self.models = []
            if truncate == 1:
                for i in range(4):
                    self.models.append(load_model('../model/'+str(i)+'_birnn_truncate.h5'))
            else:
                for i in range(4):
                    self.models.append(load_model('../model/' + str(i) + '_birnn.h5'))

        elif self.model_option == 1:
            print "Using Bi-GRU >>>>>>>>>>>>>>>>>>>>>>>>"
            self.models = []
            if truncate == 1:
                for i in range(4):
                    self.models.append(load_model('../model/'+str(i)+'_bigru_truncate.h5'))
            else:
                for i in range(4):
                    self.models.append(load_model('../model/' + str(i) + '_bigru.h5'))

        elif self.model_option == 2:
            print "Using Bi-LSTM >>>>>>>>>>>>>>>>>>>>>>>"
            self.models = []
            if truncate == 1:
                for i in range(4):
                    self.models.append(load_model('../model/'+str(i)+'_bilstm_truncate.h5'))
            else:
                for i in range(4):
                    self.models.append(load_model('../model/' + str(i) + '_bilstm.h5'))

        elif self.model_option == 3:
            print "Using hierarchical attention networks >>>>>>>>>>"
            self.models = []
            if truncate == 1:
                for i in range(4):
                    self.models.append(load_model('../model/' + str(i) + '_han_truncate.h5'))
            else:
                for i in range(4):
                    self.models.append(load_model('../model/' + str(i) + '_han.h5'))

    def predict(self, inst_len_all):
        y_pred_all = []
        for i in xrange(4):
            y_pred_all.append(self.predict_classes(self.models[i].predict(self.X_test, batch_size = batch_size, verbose = 1)))

        y_pred = np.zeros_like(self.Y_test)-1
        y_pred[y_pred_all[0] == 1] = 0
        y_pred[y_pred_all[1] == 1] = 1
        y_pred[y_pred_all[2] == 1] = 2
        y_pred[y_pred_all[3] == 1] = 3
        y_pred[y_pred == -1] = self.Y_test[y_pred == -1]

        for i in xrange(4):
            print 'Evaluating testing results of positive labels at region ' + str(i)
            precision, recall, f1, _ = precision_recall_fscore_support(self.Y_test.flatten(), y_pred.flatten(), labels=[i], average='weighted')
            print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
            print '================================================'

        # del y_pred_all
        # del inst
        # del label
        # del self.models

        if self.model_option !=3:
            inst_pred = []
            inst_label = []
            self.Y_test = self.Y_test.flatten()
            y_pred = y_pred.flatten()

            lenn = 0

            for i in xrange(len(inst_len_all)):
                if lenn < y_pred.shape[0]:
                    len_tmp = inst_len_all[i]
                    pred_tmp = y_pred[lenn:lenn+len_tmp]
                    pred_byte = collections.Counter(pred_tmp).most_common()[0][0]
                    # if 1 in pred_tmp:
                    #     pred_byte = 1
                    inst_pred.append(pred_byte)
                    label_tmp = self.Y_test[lenn:lenn+len_tmp]
                    label_byte = collections.Counter(label_tmp).most_common()[0][0]
                    inst_label.append(label_byte)
                    lenn = len_tmp+lenn
            inst_pred = np.array(inst_pred)
            inst_label = np.array(inst_label)

            for i in xrange(4):
                print 'Evaluating testing results of positive labels at region ' + str(i)
                precision, recall, f1, _ = precision_recall_fscore_support(inst_label, inst_pred, labels=[i], average='weighted')
                print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
                print '================================================'
        return y_pred_all

class DEEPVSA4VSA(object):
    def __init__(self, traces_path, models, inst_len, seq_len):
        inst = self.load_data(traces_path, inst_len)
        self.list2np(inst, seq_len)
        self.true_label = [10, 20, 30, 40]
        self.models = models

    def load_data(self, traces_path, inst_len):
        inst_file = os.path.join(traces_path, 'binary')
        label_file = os.path.join(traces_path, 'region')
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
        inst = inst_df.tolist()
        for i in xrange(len(inst)):
            inst[i] = map(lambda x: x + 1, inst[i])
        inst = pad_sequences(inst, maxlen=inst_len, truncating='post')
        self.label = np.array(label_df.values.tolist()).flatten()
        return inst

    def list2np(self, inst, seq_len):
        num_sample = inst.shape[0] / seq_len
        self.X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len, inst.shape[1])
        self.Y = self.label[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)
        return 0

    def predict_classes(self, proba):
        if proba.shape[-1] > 1:
            return proba.argmax(axis=-1)
        else:
            return (proba > 0.5).astype('int32')

    def write_label(self, data_file):
        y_pred_all = []
        for i in xrange(4):
            y_pred_all.append(self.predict_classes(self.models[i].predict(self.X, batch_size = batch_size, verbose = 1)))

        y_pred = np.zeros_like(self.Y)-1
        y_pred[y_pred_all[0] == 1] = 0
        y_pred[y_pred_all[1] == 1] = 1
        y_pred[y_pred_all[2] == 1] = 2
        y_pred[y_pred_all[3] == 1] = 3
        y_pred[y_pred == -1] = self.Y[y_pred == -1]
        y_pred = y_pred.flatten()
        y_pred[y_pred == 0] = 10
        y_pred[y_pred == 1] = 20
        y_pred[y_pred == 2] = 30
        y_pred[y_pred == 3] = 40
        print y_pred.shape

        # label_pred = np.copy(self.label)
        # print label_pred.shape
        # for i in self.true_label:
        #     label_pred[np.where(label_pred == i)] = y_pred[np.where(label_pred == i)]

        label_pred = np.copy(y_pred)
        print label_pred.shape
        for i in np.unique(self.label):
            if i % 10 !=0:
                label_pred[np.where(self.label == i)] = self.label[np.where(self.label == i)]


        print label_pred
        print self.label
        if self.label.shape[0] == label_pred.shape[0]:
            print np.count_nonzero((self.label-label_pred))

        name = 'region_DL'
        region_file = os.path.join(data_file, name)

        f_1 = open(region_file, 'w')
        for i in xrange(label_pred.shape[0]):
            label_byte = label_pred[i]
            if label_byte % 10 == 0:
                string = str((label_byte / 10 - 1)) + '\n'
            else:
                string = str((label_byte / 10 - 1)) + ':' + str((label_byte % 10 - 1)) + '\n'
            f_1.writelines(string)
        f_1.close()
        return (0)

if __name__ == "__main__":
    print '********************************'
    print '0: global...'
    print '1: heap...'
    print '2: stack...'
    print '3: other...'
    print '********************************'
    overall_test = 1

    test_traces_path = "../vulnerable"
    seq_len = 200
    model_option = 0
    truncate = 0
    if model_option == 3:
        batch_size = 500
        inst_len = 8
        npz_path = '../data/test_npz_inst.npz'
        use_npz = 0
        save_npz = 0
    else:
        batch_size = 5000
        inst_len = 0
        npz_path = '../data/test_npz_bin.npz'
        use_npz = 1
        save_npz = 0

    if overall_test == 1:
        inst_len_all, inst, label, inst_len = load_data(test_traces_path, model_option, npz_path,
                                                        use_npz=use_npz, save_npz=save_npz, inst_len = inst_len)
        vsa = DEEPVSATEST(inst, label, truncate, seq_len, model_option)
        print inst.shape
        print label.shape

        y_pred_all =  vsa.predict(inst_len_all)

        if model_option == 0:
            name_result = 'birnn_pred.npz'
        elif model_option == 1:
            name_result = 'bigru_pred.npz'
        elif model_option == 2:
            name_result = 'bilstm_pred.npz'
        else:
            name_result = 'bihan_pred.npz'

        np.savez(name_result, y_pred_all = y_pred_all)
    else:
        models = []
        for i in xrange(4):
            print i
            models.append(load_model('../model/' + str(i) + '_han_truncate.h5'))

        for i in os.listdir(test_traces_path):
            print i
            traces_path = os.path.join(test_traces_path, i)
            test = DEEPVSA4VSA(traces_path, models, inst_len = inst_len, seq_len = seq_len)
            test.write_label(traces_path)