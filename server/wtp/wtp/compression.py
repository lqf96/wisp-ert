from __future__ import absolute_import, unicode_literals
from recordclass import recordclass
from six.moves import range

from wtp.util import DataStream, BitStream

# Maximum amount of symbols
FGK_H_MAX = 256

# Internal node
FGK_NODE_INTERNAL = -1
# Zero node
FGK_NODE_ZERO = -2

# FGK tree node type
FGKNode = recordclass("FGKNode", [
    "number", # Used by FGK algorithm
    "frequency", # Symbol frequency
    "ch", # For handling symbols <= -1
    "parent", # Parent node
    "left", # Left child
    "right" # Right child
])

class FGKEncoder(object):
    """ FGK adaptive Huffman encoder. """
    def __init__(self):
        # Symbol information list
        self._nodes = [None]*FGK_H_MAX
        # Node number to node mapping
        self._numbers = [None]*(FGK_H_MAX*2+1)
        # Next number to assign
        self._next_number = FGK_H_MAX*2
        # Zero node and top node
        self._zero_node = None
        self._top_node = None
    @classmethod
    def _create_node(cls):
        """
        Create a new node.

        :returns: A new FGK tree node
        """
        return FGKNode(
            number=0,
            frequency=0,
            ch=FGK_NODE_INTERNAL,
            parent=None,
            left=None,
            right=None
        )
    @classmethod
    def _swap_nodes(cls, a, b):
        """
        Swap two FGK nodes.

        :param a, b: FGK tree nodes
        """
        # Same parent
        if a.parent==b.parent:
            parent = a.parent
            # Swap children pointers
            tmp = parent.left
            parent.left = parent.right
            parent.right = tmp
        # Different parents
        else:
            if a.parent.left==a:
                a.parent.left = b
            else:
                a.parent.right = b
            if b.parent.left==b:
                b.parent.left = a
            else:
                b.parent.right = a
            # Swap parents
            parent = a.parent
            a.parent = b.parent
            b.parent = parent
        # Swap nodes in numbers array
        tmp = self._numbers[a.number]
        self._numbers[a.number] = self._numbers[b.number]
        self._numbers[b.number] = tmp
        # Swap numbers
        tmp = a.number
        a.number = b.number
        b.number = tmp
    def _create_zero_node(self, symbol):
        """
        Create new zero node with given symbol.

        :param symbol: Symbol
        """
        zero_node = self._zero_node
        # Create node
        zero_left = zero_node.left = self._create_node()
        zero_right = zero_node.right = self._create_node()
        # Set parents of children
        zero_left.parent = zero_right = zero_node
        # Now an internal node; reset
        zero_node.ch = FGK_NODE_INTERNAL
        # New symbol's node is right child
        self._nodes[symbol] = zero_right
        zero_right.ch = symbol
        # Number the nodes
        zero_node.number = self._next_number
        self._next_number -= 1
        zero_right.number = self._next_number
        self._next_number -= 1
        # Update numbers array
        self._numbers[zero_node.number] = zero_node
        self._numbers[zero_right.number] = zero_right
        # New zero node is the left child
        self._zero_node = zero_left
        zero_left.ch = FGK_NODE_ZERO
    def _highest_equal_leaf_fgk(self, node):
        highest = None
        # Start from next numbered node
        for i in range(node.number+1, FGK_H_MAX*2):
            # Equal count?
            if node.frequency==self._numbers[i].frequency:
                # "Leaf" nodes only
                if self._numbers[i].ch!=FGK_NODE_INTERNAL:
                    highest = self._numbers[i]
            else:
                break
        return highest
    def _highest_equal_node_fgk(self, node):
        """
        Find highest node of equal count with the current node.

        :param node: FGK tree node
        """
        highest = None
        # Start from next numbered node
        for i in range(node.number+1, FGK_H_MAX*2):
            # Equal count?
            if node.frequency==self._numbers[i].frequency:
                highest = self._numbers[i]
            else:
                break
        return highest
    def _update_tree(self, symbol):
        """
        Update the FGK Huffman tree.

        :param symbol: Next symbol
        """
        # Create new zero node for new symbol
        node = self._nodes[symbol]
        if not node:
            self._create_zero_node(symbol)
            node = self._nodes[symbol]
        # Slbling of zero node
        if node.parent==self._zero_node.parent:
            highest = self._highest_equal_leaf_fgk(node)
            if highest:
                self._swap_nodes(node, highest)
            # Increase node count
            node.frequency += 1
            # Go to upper level
            node = node.parent
        # Traverse through the whole tree
        while node!=self._top_node:
            highest = self._highest_equal_node_fgk(node)
            if highest:
                self._swap_nodes(node, highest)
            # Increase node count
            node.frequency += 1
            # Go to upper level
            node = node.parent
    def _compress_symbol(self, node, out):
        """
        Compress a single symbol.

        :param node: Symbol information node
        :param out: Output stream
        """
        # Empty node
        if not node:
            return
        # Build bit stack
        bit_stack = []
        while node.parent:
            # Left child
            if node==node.parent.left:
                bit_stack.append(1)
            # Right child
            elif node==node.parent.right:
                bit_stack.append(0)
            node = node.parent
        # Output bits in reverse for easy decompression
        while bit_stack:
            out.write_bits(bit_stack.pop(), 1)
    def compress(self, data):
        """
        Compress data using FGK algorithm.

        :param data: Data to compress.
        :returns: Compressed data with original message size prepended
        """
        # Output bit stream
        out = BitStream()
        # Write message length to stream
        data_size = len(data)
        stream.write_data("H", data_size)
        # No data to compress
        if data_size==0:
            return stream.getvalue()
        # Input data stream
        _in = DataStream(data)
        # Initialize top node if no compression happened before
        if not self._top_node:
            self._top_node = self._zero_node = self._create_node()
            self._zero_node.ch = FGK_NODE_ZERO
        while True:
            # Read next byte
            symbol = _in.read_data("H")
            if symbol==None:
                break
            # Previously seen symbol
            node = self._nodes[symbol]
            if node:
                self._compress_symbol(node, out)
            # New symbol
            else:
                self._compress_symbol(self._zero_node, out)
                # Write new byte
                out.write_bits(symbol, 8)
            # Update FGK tree
            self._update_tree(symbol)
        # Return compressed data
        return stream.getvalue()
