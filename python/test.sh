#!/bin/bash

( cd .. && make -j4 ) && make -j4 && python3 test.py
