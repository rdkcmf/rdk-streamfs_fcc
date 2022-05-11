set -x
sudo ifconfig eth0 multicast
sudo ifconfig lo multicast

ip maddr add 239.0.0.1 dev eth0
sudo ip route add 239.0.0.0/8 dev eth0

#sudo ufw allow in proto udp to 239.100.0.2 from 192.168.2.5
#sudo ufw allow in proto udp to 239.100.0.3 from 192.168.2.5
#sudo ufw allow in proto udp to 239.100.0.1 from 192.168.2.5
#sudo ufw allow in proto udp to 0.0.0.0 from 192.168.2.5
