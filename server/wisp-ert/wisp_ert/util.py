from __future__ import absolute_import, unicode_literals

## Not implemented prompt
NOT_IMPL_PROMPT = "This function is not implemented."
## Not implemented error prompt
NOT_IMPL_ERROR_PROMPT = "This function is required to be implemented."

def not_implemented(is_error=False, prompt=None):
    """!
    @brief Shorthand method to throw not implemented (error).

    @param is_error Throw NotImplemented or NotImplementedError.
    @param prompt Custom not implemented prompt.
    @throws NotImplemented or NotImplementedError.
    """
    if is_error:
        prompt = NOT_IMPL_ERROR_PROMPT if prompt==None else prompt
        raise NotImplementedError(prompt)
    else:
        prompt = NOT_IMPL_PROMPT if prompt==None else prompt
        raise NotImplemented(prompt)

def setitem_keypath(d, keypath, value):
    """!
    @brief Set dictionary item by keypath.

    @param d Dictinoary.
    @param keypath Key path.
    @param value Value to set.
    """
    split_keypath = keypath.split(".")
    # Find the leaf level dictionary
    for key in split_keypath[:-1]:
        d = d.setdefault(key, {})
    # Set item
    d[keypath[-1]] = value
