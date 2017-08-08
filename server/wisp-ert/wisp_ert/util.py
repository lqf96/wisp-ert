from __future__ import absolute_import, unicode_literals

# Not implemented prompt
NOT_IMPL_PROMPT = "This function is not implemented."
# Not implemented error prompt
NOT_IMPL_ERROR_PROMPT = "This function is required to be implemented."

def not_implemented(is_error=False, prompt=None):
    """
    Shorthand method to throw not implemented (error).

    :param is_error: Throw NotImplemented or NotImplementedError
    :raises: NotImplemented or NotImplementedError
    """
    if is_error:
        raise NotImplementedError(NOT_IMPL_ERROR_PROMPT)
    else:
        raise NotImplemented(NOT_IMPL_PROMPT)

def setitem_keypath(d, keypath, value):
    """
    Set dictionary item by keypath.

    :param d: Dictinoary
    :param keypath: Key path
    :param value: Value to set
    """
    split_keypath = keypath.split(".")
    # Find the leaf level dictionary
    for key in split_keypath[:-1]:
        d = d.setdefault(key, {})
    # Set item
    d[keypath[-1]] = value
