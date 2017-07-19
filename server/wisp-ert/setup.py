#! /usr/bin/env python
from __future__ import unicode_literals
from setuptools import setup

setup(
    name="wisp-ert",
    version="0.1.0",
    author="lqf96",
    author_email="lqf.1996121@gmail.com",
    description="WISP Extended Runtime (Server-side)",
    license="GPL",
    packages=["wtp"],
    install_requires=["six", "u-rpc", "wtp-server"]
)
