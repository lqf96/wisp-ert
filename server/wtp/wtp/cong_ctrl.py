from __future__ import absolute_import, unicode_literals

from wtp.constants import WISP_OPSPEC_MIN, WISP_OPSPEC_MAX

class SlidingWindowOpSpecControl(object):
    """ Sliding windoe-based OpSpec control. """
    def __init__(self, read_words, write_words):
        # Desired number of words of Read and Write/BlockWrite OpSpec
        self._read_words = read_words
        self._write_words = write_words
        # Number of words of pending Read and Write/BlockWrite OpSpec
        self._pending_reads = []
        self._pending_writes = []
    def add_read_opspec(self, words):
        """
        Add number of words to Read.

        :param words: Number of words to Read
        """
        self._pending_reads.append(words)
    def add_write_opspec(self, words):
        """
        Add number of words to Read.

        :param words: Number of words to Read
        """
        self._pending_writes.append(words)
    def report_read_result(self, words):
        """
        Report Read result to OpSpec control.

        :param words: Number of words actually Read
        :returns: Desired number of words to Read
        """
        recv_words = self._pending_reads.pop(0)
        # All data transfered; increase Read words by 1
        if recv_words==words:
            if self._read_words<WISP_OPSPEC_MAX:
                self._read_words += 1
        # Some data are lost; throttle to number of words reported
        else:
            self._read_words = max(words, WISP_OPSPEC_MIN)
        return self._read_words
    def report_write_result(self, words):
        """
        Report Write/BlockWrite result to OpSpec control.

        :param words: Number of words actually written
        :returns: Desired number of words to Write/BlockWrite
        """
        send_words = self._pending_reads.pop(0)
        # All data transfered; increase Read words by 1
        if send_words==words:
            if self._write_words<WISP_OPSPEC_MAX:
                self._write_words += 1
        # Some data are lost; throttle to number of words reported
        else:
            self._write_words = max(words, WISP_OPSPEC_MAX)
        return self._write_words

class SlidingWindowCongControl(object):
    """ Sliding window-based congestion control. """
    def __init__(self):
        pass
