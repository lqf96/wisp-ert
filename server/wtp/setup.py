#! /usr/bin/env python
from __future__ import unicode_literals
from setuptools import setup

setup(
    name="wtp-server",
    version="0.1.0",
    author="lqf96",
    author_email="lqf.1996121@gmail.com",
    description="WISP Transmission Protocol (Server-side)",
    license="MIT",
    packages=["wtp"],
    install_requires=["six", "twisted", "sllurp"]
)