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
from keras.optimizers import Adam
from keras.models import Sequential, Model, load_model
from keras.layers.embeddings import Embedding
from keras.preprocessing.sequence import pad_sequences
from sklearn.metrics import precision_recall_fscore_support
from keras.layers import Dense, TimeDistributed, Bidirectional, LSTM, Input, GRU, SimpleRNN, concatenate, Dropout

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

def load_data(traces_path,
              model_option,
              npz_path,
              save_npz = 0,
              use_npz = 0,
              one_trace=0,
              inst_len=0,
              truncate = 0,
              testing = 0
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
            if truncate and testing == 0:
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
                if truncate and testing == 0:
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
            print len(inst)
            inst_len_all = [len(aa) for aa in inst]
            if inst_len == 0:
                #inst_len = collections.Counter(inst_len_all).keys()[-1]
                inst_len = max(inst_len_all)
            if model_option == 3:
                for i in xrange(len(inst)):
                    inst[i] = map(lambda x: x + 1, inst[i])
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
                 train_label,
                 seq_len,
                 inst_len,
                 model_option,
                 use_attention):
        self.train_label = train_label
        self.seq_len = seq_len
        self.model_option = model_option
        self.inst_len = inst_len
        self.X, self.Y, self.Y_one_hot, self.n_class = self.list2np(inst, label, seq_len, model_option)
        self.build_model(use_attention)
        self.use_attention = use_attention

    def predict_classes(self, proba):
        if proba.shape[-1] > 1:
            return proba.argmax(axis=-1)
        else:
            return (proba > 0.5).astype('int32')

    def list2np(self, inst, label, seq_len, model_option):
        label_all = [10,20,30,40]
        label_right = int((self.train_label+1)*10)
        label[label == label_right] = 1
        label_all.remove(label_right)
        for ii in label_all:
            label[label == ii] = 0
        n_class = 2

        num_sample = inst.shape[0] / seq_len
        if model_option == 3:
            X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len, inst.shape[1])
        else:
            X = inst[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)

        Y = label[0:(num_sample * seq_len), ].reshape(num_sample, seq_len)

        Y_one_hot = to_categorical(Y).astype('int32')
        return X, Y, Y_one_hot, n_class

    def build_model(self, use_attention):
        if self.model_option == 0:
            print "Using Bi-SimpleRNN >>>>>>>>>>>>>>>>>>"
            self.model = Sequential()
            self.model.add(Embedding(input_dim=256, output_dim=64, input_length=self.seq_len))
            self.model.add(Bidirectional(SimpleRNN(units=32, activation='tanh', return_sequences=True)))
            self.model.add(Dropout(0.5))
            self.model.add(Bidirectional(SimpleRNN(units=16, activation='tanh', return_sequences=True)))
            self.model.add(Dropout(0.5))
            self.model.add(Bidirectional(SimpleRNN(units=8, activation='tanh', return_sequences=True)))
            self.model.add(TimeDistributed(Dense(self.n_class, activation='softmax'), input_shape=(self.seq_len, 16)))
            self.model.summary()

        elif self.model_option == 1:
            print "Using Bi-GRU >>>>>>>>>>>>>>>>>>>>>>>>"
            self.model = Sequential()
            self.model.add(Embedding(input_dim=256, output_dim=64, input_length=self.seq_len))
            self.model.add(Bidirectional(GRU(units=32, activation='tanh', return_sequences=True)))
            self.model.add(Dropout(0.5))
            self.model.add(Bidirectional(GRU(units=16, activation='tanh', return_sequences=True)))
            self.model.add(Dropout(0.5))
            self.model.add(Bidirectional(GRU(units=8, activation='tanh', return_sequences=True)))
            self.model.add(TimeDistributed(Dense(self.n_class, activation='softmax'), input_shape=(self.seq_len, 16)))
            self.model.summary()

        elif self.model_option == 2:
            print "Using Bi-LSTM >>>>>>>>>>>>>>>>>>>>>>>"
            self.model = Sequential()
            self.model.add(Embedding(input_dim=256, output_dim=64, input_length=self.seq_len))
            self.model.add(Bidirectional(LSTM(units=32, activation='tanh', return_sequences=True)))
            self.model.add(Dropout(0.5))
            self.model.add(Bidirectional(LSTM(units=16, activation='tanh', return_sequences=True)))
            self.model.add(Dropout(0.5))
            self.model.add(Bidirectional(LSTM(units=8, activation='tanh', return_sequences=True)))
            self.model.add(TimeDistributed(Dense(self.n_class, activation='softmax'), input_shape=(self.seq_len, 16)))
            self.model.summary()

        elif self.model_option == 3:
            print "Using hierarchical attention networks >>>>>>>>>>"
            X_input = Input(shape = (self.seq_len, self.inst_len), name = 'X_input')
            inst_input = Input(shape = (self.inst_len, ), name = 'inst_input')

            bin_embedded = Embedding(input_dim=257, output_dim=64, input_length=self.inst_len)(inst_input)
            inst_embedded = Bidirectional(LSTM(units=32, dropout=0.5, return_sequences=True))(bin_embedded)
            if use_attention:
                inst_embedded = Bidirectional(LSTM(units=16, dropout=0.5, return_sequences=True))(inst_embedded)
                inst_embedded = AttLayer(16)(inst_embedded)
            else:
                inst_embedded = Bidirectional(LSTM(units=16, dropout=0.5))(inst_embedded)

            inst_model = Model(inst_input, inst_embedded)

            seq_embedded = TimeDistributed(inst_model)(X_input)
            if use_attention and False:
                lstm_out_f = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=seq_embedded, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True))(seq_embedded)
                lstm_out_b = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=seq_embedded, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True,
                                            go_backwards=True))(seq_embedded)
                lstm_out = concatenate([lstm_out_f, lstm_out_b])

                lstm_out_f = (AttentionLSTM(units=32, seq_len=self.seq_len, seq_input=lstm_out, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True))(lstm_out)
                lstm_out_b = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=lstm_out, dropout=0.25,
                                            recurrent_dropout=0.25, return_sequences=True,
                                            go_backwards=True))(lstm_out)
                lstm_out = concatenate([lstm_out_f, lstm_out_b])

            else:
                lstm_out = Bidirectional(LSTM(units=32, dropout=0.5, return_sequences=True))(seq_embedded)
                lstm_out = Bidirectional(LSTM(units=16, dropout=0.5, return_sequences=True))(lstm_out)

            model_out = TimeDistributed(Dense(self.n_class, activation='softmax'), name='model_out')(lstm_out)

            self.model = Model([X_input], model_out)
            self.model.summary()

            # inst_embedded = Bidirectional(GRU(units=32, dropout=0.5, return_sequences=True))(bin_embedded)
            # if use_attention:
            #     inst_embedded = Bidirectional(GRU(units=16, dropout=0.5, return_sequences=True))(inst_embedded)
            #     inst_embedded = AttLayer(16)(inst_embedded)
            # else:
            #     inst_embedded = Bidirectional(GRU(units=16, dropout=0.5))(inst_embedded)
            #
            # inst_model = Model(inst_input, inst_embedded)
            #
            # seq_embedded = TimeDistributed(inst_model)(X_input)
            # if use_attention and False:
            #     lstm_out_f = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=seq_embedded, dropout=0.25,
            #                                 recurrent_dropout=0.25, return_sequences=True))(seq_embedded)
            #     lstm_out_b = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=seq_embedded, dropout=0.25,
            #                                 recurrent_dropout=0.25, return_sequences=True,
            #                                 go_backwards=True))(seq_embedded)
            #     lstm_out = concatenate([lstm_out_f, lstm_out_b])
            #
            #     lstm_out_f = (AttentionLSTM(units=32, seq_len=self.seq_len, seq_input=lstm_out, dropout=0.25,
            #                                 recurrent_dropout=0.25, return_sequences=True))(lstm_out)
            #     lstm_out_b = (AttentionLSTM(units=8, seq_len=self.seq_len, seq_input=lstm_out, dropout=0.25,
            #                                 recurrent_dropout=0.25, return_sequences=True,
            #                                 go_backwards=True))(lstm_out)
            #     lstm_out = concatenate([lstm_out_f, lstm_out_b])
            #
            # else:
            #     lstm_out = Bidirectional(GRU(units=32, dropout=0.5, return_sequences=True))(seq_embedded)
            #     lstm_out = Bidirectional(GRU(units=16, dropout=0.5, return_sequences=True))(lstm_out)
            #
            # model_out = TimeDistributed(Dense(self.n_class, activation='softmax'), name='model_out')(lstm_out)
            #
            # self.model = Model([X_input], model_out)
            # self.model.summary()

    def fit(self, batch_size, epoch_1, epoch_2, save_model, save_dir, truncate):
        # self.X = self.X[0:1000, ]
        # self.Y = self.Y[0:1000, ]
        # self.Y_one_hot = self.Y_one_hot[0:1000, ]
        print '================================================'
        print "Data shape..."
        print self.X.shape
        print self.Y_one_hot.shape
        print "Counting the number of data in each category..."
        print collections.Counter(self.Y.flatten())
        print '================================================'

        print 'Starting training...'
        if self.train_label == 0 or self.train_label == 1:
            sample_weights = class_weight.compute_sample_weight('balanced', self.Y.flatten()).reshape(self.Y.shape)
            self.model.compile(optimizer=Adam(lr=0.005), loss='categorical_crossentropy', metrics=['accuracy'], sample_weight_mode="temporal")
            self.model.fit(self.X, self.Y_one_hot, batch_size=batch_size, epochs=epoch_1, verbose=1, sample_weight=sample_weights)

        # if save_model:
        #     if truncate:
        #         if self.model_option == 0:
        #             name = str(self.train_label) + '_bi_rnn_truncate_1.h5'
        #         elif self.model_option == 1:
        #             name = str(self.train_label) + '_bigru_truncate_1.h5'
        #         elif self.model_option == 2:
        #             name = str(self.train_label) +  '_bilstm_truncate_1.h5'
        #         else:
        #             name = str(self.train_label) + '_han_truncate_1.h5'
        #     else:
        #         if self.model_option == 0:
        #             name = str(self.train_label) + 'bi_rnn_1.h5'
        #         elif self.model_option == 1:
        #             name = str(self.train_label) + '_bigru_1.h5'
        #         elif self.model_option == 2:
        #             name = str(self.train_label) +  '_bilstm_1.h5'
        #         else:
        #             name = str(self.train_label) + '_han_1.h5'
        #     save_dir_1 = os.path.join(save_dir, name)
        #     if model_option==3 and self.use_attention:
        #         weights = self.model.get_weights()
        #         io.savemat(save_dir_1, {'weights':weights})
        #     else:
        #         self.model.save(save_dir_1)

        if self.train_label == 3 or self.train_label == 2:
            self.model.compile(optimizer=Adam(lr=0.005), loss='categorical_crossentropy', metrics=['accuracy'], sample_weight_mode="temporal")
            self.model.fit(self.X, self.Y_one_hot, batch_size=batch_size, epochs=epoch_2, verbose=1)

        if save_model:
            if truncate:
                if self.model_option == 0:
                    name = str(self.train_label) + '_birnn_truncate.h5'
                elif self.model_option == 1:
                    name = str(self.train_label) + '_bigru_truncate.h5'
                elif self.model_option == 2:
                    name = str(self.train_label) +  '_bilstm_truncate.h5'
                else:
                    name = str(self.train_label) + '_han_truncate.h5'
            else:
                if self.model_option == 0:
                    name = str(self.train_label) + '_birnn.h5'
                elif self.model_option == 1:
                    name = str(self.train_label) + '_bigru.h5'
                elif self.model_option == 2:
                    name = str(self.train_label) + '_bilstm.h5'
                else:
                    name = str(self.train_label) + '_han.h5'
            save_dir = os.path.join(save_dir, name)
            if model_option==3 and self.use_attention:
                weights = self.model.get_weights()
                io.savemat(save_dir, {'weights':weights})
            else:
                self.model.save(save_dir)
        return 0

    def evaluate(self):
        y_pred = self.predict_classes(self.model.predict(self.X, batch_size=batch_size))

        print 'Evaluating training results'
        precision, recall, f1, _ = precision_recall_fscore_support(self.Y.flatten(), y_pred.flatten(),
                                                                   labels=[0,1], average='weighted')
        print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
        print '================================================'

        for i in xrange(2):
            print 'Evaluating training results of positive labels at region ' + str(i)
            precision, recall, f1, _ = precision_recall_fscore_support(self.Y.flatten(), y_pred.flatten(),
                                                                       labels=[i], average='weighted')
            print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
            print '================================================'
        return 0

    def predict(self, inst_test, label_test):
        X_test, Y_test, _, _, = self.list2np(inst_test, label_test, self.seq_len, self.model_option)

        y_pred = self.predict_classes(self.model.predict(X_test, batch_size=batch_size))

        print 'Evaluating testing results'
        precision, recall, f1, _ = precision_recall_fscore_support(Y_test.flatten(), y_pred.flatten(),
                                                                   labels=[0,1], average='weighted')
        print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
        print '================================================'

        for i in xrange(2):
            print 'Evaluating testing results of positive labels at region ' + str(i)
            precision, recall, f1, _ = precision_recall_fscore_support(Y_test.flatten(), y_pred.flatten(),
                                                                       labels=[i], average='weighted')
            print("Precision: %s Recall: %s F1: %s" % (precision, recall, f1))
            print '================================================'

        return y_pred


if __name__ == "__main__":
    print '********************************'
    print '0: global...'
    print '1: heap...'
    print '2: stack...'
    print '3: other...'
    print '********************************'

    train_traces_path = "../../original_data/finetune_traces"
    test_traces_path = "../../original_data/vulnerable"

    truncate = 0
    seq_len = 200

    model_option = 3
    label_train = [0, 1, 2, 3]
    epochs_1 = 8
    epochs_2 = 8

    if model_option == 3:
        batch_size = 500
        if truncate == 0:
            npz_path_train = '../data/train_npz_inst.npz'
            save_npz_train = 0
            use_npz_train = 0
            npz_path_test = '../data/test_npz_inst.npz'
            save_npz_test = 0
            use_npz_test = 1
        else:
            npz_path_train = '../data/train_npz_inst_truncated.npz'
            save_npz_train = 0
            use_npz_train = 1
            npz_path_test = '../data/test_npz_inst.npz'
            save_npz_test = 0
            use_npz_test = 1
    else:
        batch_size = 10000
        if truncate == 0:
            npz_path_train = '../data/train_npz_bin.npz'
            save_npz_train = 1
            use_npz_train = 0
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

    for train_label in label_train:
        print "====================================="
        print "training region " + str(train_label)
        print "====================================="

        inst, label, inst_len = load_data(train_traces_path, model_option=model_option, npz_path=npz_path_train,
                                          save_npz=save_npz_train, use_npz=use_npz_train, truncate=truncate, inst_len=8)
        print inst.shape
        print label.shape

        vsa = DEEPVSA(inst, label, train_label, seq_len = seq_len, inst_len = inst_len, model_option = model_option, use_attention = 0)
        vsa.fit(batch_size, epochs_1, epochs_2, save_model = 1, save_dir='../model', truncate = truncate)
        vsa.evaluate()

        inst_test, label_test, inst_len = load_data(test_traces_path, model_option = model_option, npz_path=npz_path_test,
                                                    save_npz=save_npz_test, use_npz=use_npz_test, inst_len = inst_len, truncate = truncate, testing=1)
        print inst_test.shape
        print label_test.shape
        y_pred = vsa.predict(inst_test, label_test)
