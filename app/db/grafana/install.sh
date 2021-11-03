sudo apt-get install -y adduser libfontconfig1
wget https://dl.grafana.com/oss/release/grafana_7.3.7_amd64.deb
sudo dpkg -i grafana_7.3.7_amd64.deb

rm grafana_7.3.7_amd64.deb

sudo cp grafana.ini /etc/grafana/grafana.ini
