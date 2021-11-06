#!/bin/bash

sudo service grafana-server start
sudo python3 -m http.server 8080
