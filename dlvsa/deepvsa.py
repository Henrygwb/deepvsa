import os
os.environ["THEANO_FLAGS"] = "device=gpu0, floatX=float32"
#os.environ["CUDA_VISIBLE_DEVICES"]="3"
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
from keras.models import Sequential, Model, load_model
from keras.layers.embeddings import Embedding
from keras.preprocessing.sequence import pad_sequences
from sklearn.metrics import precision_recall_fscore_support
from keras.layers import Dense, TimeDistributed, Bidirectional, LSTM, Input, GRU, SimpleRNN, concatenate

np.random.seed(1234)

class AttLayer(Layer):
    def __init__(self, attention_dim):
        self.init = initializers.get('normal')
        self.supports_masking = True
        self.attention_dim = attention_dim
        super(AttLayer, self).__init__()

    def build(self, input_shape):
        assert len(input_shape) == 3
        self.W = K.variable(self.init((input_shape[-1], self.attention_dim)))
        self.b = K.variable(self.init((self.attention_dim, )))
        self.u = K.variable(self.init((self.attention_dim, 1)))
        self.trainable_weights = [self.W, self.b, self.u]
        super(AttLayer, self).build(input_shape)

    def compute_mask(self, inputs, mask=None):
        return mask

    def call(self, x, mask=None):
        # size of x :[batch_size, sel_len, attention_dim]
        # size of u :[batch_size, attention_dim]
        # uit = tanh(xW+b)
        uit = K.tanh(K.bias_add(K.dot(x, self.W), self.b))
        ait = K.dot(uit, self.u)
        ait = K.squeeze(ait, -1)

        ait = K.exp(ait)

        if mask is not None:
            # Cast the mask to floatX to avoid float64 upcasting in theano
            ait *= K.cast(mask, K.floatx())
        ait /= K.cast(K.sum(ait, axis=1, keepdims=True) + K.epsilon(), K.floatx())
        ait = K.expand_dims(ait)
        weighted_input = x * ait
        output = K.sum(weighted_input, axis=1)

        return output

    def compute_output_shape(self, input_shape):
        return (input_shape[0], input_shape[-1])

def load_data(traces_path, model_option, npz_path, save_npz = 0, use_npz = 0, one_trace=0, inst_len=0):
    if use_npz == 1:
        data = np.load(npz_path)
        inst = data['inst']
        label = data['label']
        if model_option == 3:
            inst_len = len(inst[0])
        else:
            model_option = 0
    else:
        inst = []
        label = []
        if one_trace:
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
    return inst, label, inst_len

class DEEPVSA(object):
    def __init__(self,
                 inst,
                 label,
                 seq_len,
                 inst_len,
                 train_region,
                 model_option,
                 use_attention):

        self.seq_len = seq_len
        self.model_option = model_option
        self.train_region = train_region
        self.inst_len = inst_len
        self.X, self.Y, self.Y_one_hot, self.true_label, self.n_class = \
            self.list2np(inst, label, seq_len, train_region, model_option)
        self.build_model(use_attention)

    def predict_classes(self, proba):
        if proba.shape[-1] > 1:
            return proba.argmax(axis=-1)
        else:
            return (proba > 0.5).astype('int32')

    def list2np(self, inst, label, seq_len, train_region, model_option, testing=False):

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
            assert len(self.true_label) >= len(true_label), "Number of testing classes is larger than training classes."

            true_label = self.true_label
            n_class = len(true_label)
        else:
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

        Y_one_hot = to_categorical(Y).astype('int32')
        return X, Y, Y_one_hot, true_label, n_class

    def build_model(self, use_attention):
        if self.model_option == 0:
            print "Using Bi-SimpleRNN >>>>>>>>>>>>>>>>>>"
            self.model = Sequential()
            self.model.add(Embedding(input_dim=256, output_dim=16, input_length=self.seq_len))
            self.model.add(Bidirectional(SimpleRNN(units=8, activation='relu', dropout=0.5, return_sequences=True)))
            self.model.add(Bidirectional(SimpleRNN(units=8, activation='relu', dropout=0.5, return_sequences=True)))
            self.model.add(TimeDistributed(Dense(self.n_class, activation='softmax'), input_shape=(self.seq_len, 16)))
            self.model.summary()

        elif self.model_option == 1:
            print "Using Bi-GRU >>>>>>>>>>>>>>>>>>>>>>>>"
            self.model = Sequential()
            self.model.add(Embedding(input_dim=256, output_dim=16, input_length=self.seq_len))
            self.model.add(Bidirectional(GRU(units=8, activation='relu', dropout=0.5, return_sequences=True)))
            self.model.add(Bidirectional(GRU(units=8, activation='relu', dropout=0.5, return_sequences=True)))
            self.model.add(TimeDistributed(Dense(self.n_class, activation='softmax'), input_shape=(self.seq_len, 16)))
            self.model.summary()

        elif self.model_option == 2:
            print "Using Bi-LSTM >>>>>>>>>>>>>>>>>>>>>>>"
            self.model = Sequential()
            self.model.add(Embedding(input_dim=256, output_dim=16, input_length=self.seq_len))
            self.model.add(Bidirectional(LSTM(units=8, dropout=0.25, recurrent_dropout=0.25, return_sequences=True)))
            self.model.add(Bidirectional(LSTM(units=8, dropout=0.25, recurrent_dropout=0.25, return_sequences=True)))
            self.model.add(TimeDistributed(Dense(self.n_class, activation='softmax'), input_shape=(self.seq_len, 16)))
            self.model.summary()

        elif self.model_option == 3:
            print "Using hierarchical attention networks >>>>>>>>>>"
            X_input = Input(shape = (self.seq_len, self.inst_len), name = 'X_input')
            inst_input = Input(shape = (self.inst_len, ), name = 'inst_input')

            bin_embedded = Embedding(input_dim=257, output_dim=16, input_length=self.inst_len)(inst_input)
            inst_embedded = Bidirectional(LSTM(units=8, dropout=0.25, recurrent_dropout=0.25,
                                               return_sequences=True))(bin_embedded)
            if True:
                inst_embedded = Bidirectional(LSTM(units=8, dropout=0.25, recurrent_dropout=0.25,
                                                   return_sequences=True))(inst_embedded)
                inst_embedded = AttLayer(8)(inst_embedded)
            else:
                inst_embedded = Bidirectional(LSTM(units=8, dropout=0.25, recurrent_dropout=0.25))(inst_embedded)

            inst_model = Model(inst_input, inst_embedded)

            seq_embedded = TimeDistributed(inst_model)(X_input)
            if use_attention:
                lstm_out_f = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=seq_embedded, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True))(seq_embedded)
                lstm_out_b = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=seq_embedded, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True,
                                            go_backwards=True))(seq_embedded)
                lstm_out = concatenate([lstm_out_f, lstm_out_b])

                lstm_out_f = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=lstm_out, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True))(lstm_out)
                lstm_out_b = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=lstm_out, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True,
                                            go_backwards=True))(lstm_out)
                lstm_out = concatenate([lstm_out_f, lstm_out_b])

            else:
                lstm_out = Bidirectional(LSTM(units=8, dropout=0.25, recurrent_dropout=0.25, return_sequences=True))(seq_embedded)
                lstm_out = Bidirectional(LSTM(units=8, dropout=0.25, recurrent_dropout=0.25, return_sequences=True))(lstm_out)

            model_out = TimeDistributed(Dense(self.n_class, activation='softmax'), name='model_out')(lstm_out)

            self.model = Model([X_input], model_out)
            self.model.summary()

    def fit(self, batch_size, epoch, save_model, save_dir):
        print '================================================'
        print "Data shape..."
        print self.X.shape
        print self.Y_one_hot.shape
        print "Counting the number of data in each category..."
        print "Existing labels: "
        print self.true_label
        print collections.Counter(self.Y.flatten())
        print '================================================'

        print 'Starting training...'
        self.model.compile('adam', 'categorical_crossentropy', metrics=['accuracy'])
        self.model.fit(self.X, self.Y_one_hot, batch_size=batch_size, epochs=epoch, verbose=1)
        if save_model:
            if self.model_option == 0:
                name = str(self.train_region) + '_birnn.h5'
            elif self.model_option == 1:
                name = str(self.train_region) + '_bigru.h5'
            elif self.model_option == 2:
                name = str(self.train_region) + '_bilstm.h5'
            else:
                name = str(self.train_region) + '_han.h5'
            save_dir = os.path.join(save_dir, name)
            if model_option==3:
                weights = self.model.get_weights()
                io.savemat(save_dir, {'weights':weights})
            else:
                self.model.save(save_dir)
        return 0

    def evaluate(self):
        y_pred = self.predict_classes(self.model.predict(self.X))

        print 'Evaluating training results at region ' + str(self.train_region)
        precision, recall, f1, _ = precision_recall_fscore_support(self.Y.flatten(), y_pred.flatten(),
                                                                   labels=range(self.n_class), average='weighted')
        print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
        print '================================================'

        print 'Evaluating training results of positive labels at region ' + str(self.train_region)
        precision, recall, f1, _ = precision_recall_fscore_support(self.Y.flatten(), y_pred.flatten(),
                                                                   labels=range(self.n_class-1), average='weighted')
        print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
        print '================================================'

        for label in xrange(self.n_class):
            if label < len(self.true_label):
                print 'Evaluating ' + str(self.true_label[label]) + ' ...'
                precision, recall, f1, _ = precision_recall_fscore_support(self.Y.flatten(), y_pred.flatten(),
                                                                           labels=[label], average ='macro')
                print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
                print '================================================'
        return 0

    def predict(self, inst_test, label_test):
        X_test, Y_test, _, _, _ = self.list2np(inst_test, label_test, self.seq_len,
                                               self.train_region, self.model_option, testing=True)

        y_pred = self.predict_classes(self.model.predict(X_test))

        print 'Evaluating testing results at region ' + str(self.train_region)
        precision, recall, f1, _ = precision_recall_fscore_support(Y_test.flatten(), y_pred.flatten(),
                                                                   labels=range(self.n_class), average='weighted')
        print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
        print '================================================'

        print 'Evaluating training results of positive labels at region ' + str(self.train_region)
        precision, recall, f1, _ = precision_recall_fscore_support(Y_test.flatten(), y_pred.flatten(),
                                                                   labels=range(self.n_class-1), average='weighted')
        print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
        print '================================================'

        for label in xrange(self.n_class):
            if label < len(self.true_label):
                print 'Evaluating ' + str(self.true_label[label]) + ' ...'
                precision, recall, f1, _ = precision_recall_fscore_support(Y_test.flatten(), y_pred.flatten(),
                                                                           labels=[label], average ='macro')
                print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
                print '================================================'

        return y_pred, self.true_label

class DEEPVSA4VSA(object):
    def __init__(self, inst, label, inst_len, seq_len, model_option, model0, model1, model2, model3,
                 true_label0, true_label1, true_label2, true_label3):
        self.model_option = model_option
        self.list2np(inst, label, seq_len, inst_len)
        self.models = [model0, model1, model2, model3]
        self.true_labels = [true_label0, true_label1, true_label2, true_label3]

    def predict_classes(self, proba):
        if proba.shape[-1] > 1:
            return proba.argmax(axis=-1)
        else:
            return (proba > 0.5).astype('int32')

    def list2np(self, inst, label, seq_len, inst_len):
        inst_len_all = [len(aa) for aa in inst]
        if self.model_option == 3:
            inst = pad_sequences(inst, maxlen=inst_len, truncating='post')
            self.label = np.array(label).flatten()
        else:
            inst = np.array(list(chain.from_iterable(inst)))
            label = [label[i] * inst_len_all[i] for i in xrange(len(label))]
            self.label = np.hstack(label)
        num_sample = inst.shape[0] / seq_len
        if self.model_option == 3:
            self.X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len, inst.shape[1])
        else:
            self.X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)
        return 0

    def predict(self):
        self.y_preds = []
        for model in self.models:
            model.summary()
            y_pred = self.predict_classes(model.predict(self.X))
            self.y_preds.append(y_pred.flatten())
        return 0

    def write_label(self, data_file, use_other = 0):
        self.predict()
        label_pred = np.copy(self.label)
        if use_other == 0:
            self.y_preds.pop()
            self.true_labels.pop()

        for y_pred_tmp, true_label_tmp in zip(self.y_preds, self.true_labels):
            for i in xrange(len(true_label_tmp)):
                label_pred[np.where(y_pred_tmp == i)] = true_label_tmp[i]

        name = 'region_DL'
        inst_file = os.path.join(data_file, 'binary')
        region_file = os.path.join(data_file, name)

        f_1 = open(region_file, 'w')
        if self.model_option != 3:
            with open(inst_file) as f:
                data_txt = f.readlines()
            ll = 0
            for i in xrange(len(data_txt)):
                data_tmp = re.sub('\s', '', data_txt[i])
                len_inst = (len(data_tmp)) / 2
                label_tmp = label_pred[ll: ll + len_inst]
                ll = ll + len_inst
                label_byte = collections.Counter(label_tmp).most_common()[0][0]

                if label_byte % 10 == 0:
                    string = str((label_byte / 10 - 1)) + '\n'
                else:
                    string = str((label_byte / 10 - 1)) + ':' + str((label_byte % 10 - 1)) + '\n'
                f_1.writelines(string)
        elif self.model_option == 4:
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

    # parser = argparse.ArgumentParser()
    # parser.add_argument("-t", "--trace", help="trace folder")
    # parser.add_argument("-l", "--label_train", nargs='+', type=int)
    # parser.add_argument("-m", "--model_option", type=int)
    # arg = parser.parse_args()
    # 
    # traces_path = arg.trace
    # seq_len = arg.seq_len
    # label_train = arg.label_train
    # model_option = arg.model_option

    train_traces_path = '/home/wzg13/Data/Traces/train_traces'
    #train_traces_path = "../data/binutils/"
    test_traces_path = "../data/vulnerable"
    seq_len = 400
    label_trains = [0, 1, 2, 3]
    model_option = 0
    batch_size = 500
    epochs = 100

    for label_train in label_trains:
        print '****************************************'
        print '****************************************'
        print label_train
        print '****************************************'
        print '****************************************'

        if model_option == 0 and label_train == 0:
            npz_path_train = '../data/train_npz_bin.npz'
            save_npz_train = 1
            use_npz_train = 0
            npz_path_test = '../data/test_npz_bin.npz'
            save_npz_test = 1
            use_npz_test = 0

        elif model_option == 3 and label_train == 0:
            npz_path_train = '../data/train_npz_inst.npz'
            save_npz_train = 1
            use_npz_train = 0
            npz_path_test = '../data/test_npz_inst.npz'
            save_npz_test = 1
            use_npz_test = 0

        elif model_option == 3:
            npz_path_train = '../data/train_npz_inst.npz'
            save_npz_train = 0
            use_npz_train = 1
            npz_path_test = '../data/test_npz_inst.npz'
            save_npz_test = 0
            use_npz_test = 1

        else:
            npz_path_train = '../data/train_npz_bin.npz'
            save_npz_train = 0
            use_npz_train = 1
            npz_path_test = '../data/test_npz_bin.npz'
            save_npz_test = 0
            use_npz_test = 1

        inst, label, inst_len = load_data(train_traces_path, model_option = model_option, npz_path=npz_path_train, save_npz=save_npz_train, use_npz=use_npz_train)
        print inst.shape
        print label.shape
        vsa = DEEPVSA(inst, label, seq_len = seq_len, inst_len = inst_len, train_region = label_train, model_option = model_option, use_attention = 0)
        vsa.fit(batch_size, epochs, save_model = 1, save_dir='../model')
        vsa.evaluate()

        inst, label, inst_len = load_data(test_traces_path, model_option = model_option, npz_path=npz_path_test, save_npz=save_npz_test, use_npz=use_npz_test, inst_len = inst_len)
        print inst.shape
        print label.shape
        y_pred, true_label = vsa.predict(inst, label)

    """    
    y_preds = []
    true_labels = []
    ####
        y_preds.append(y_preds)
        true_labels.append(true_label)
    io.savemat('../model'+'result_'+str(model_option)+'.mat', {'y_pred':y_preds})

    traces_path = "../data/vulnerable/abc2mtex"
    inst, label = load_data(traces_path, npz_path='', save_npz=0, use_npz=0, one_trace=1)
    test = DEEPVSA4VSA(inst, label, inst_len = 0, seq_len = 200, model_option = 0,
                       model0 = load_model('../model/0_birnn.h5'),
                       model1 = load_model('../model/1_birnn.h5'),
                       model2 = load_model('../model/2_birnn.h5'),
                       model3 = load_model('../model/3_birnn.h5'),
                       true_label0 = [10,13],
                       true_label1 = [20, 22, 23, 24],
                       true_label2 = [30, 32, 33, 34],
                       true_label3 = [40,43],)

    test.write_label(traces_path)
    """
