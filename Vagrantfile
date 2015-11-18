# -*- mode: ruby -*-
# vi: set ft=ruby :

require 'socket'

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "precise64"
  config.vm.box_url = "http://files.vagrantup.com/precise64.box"

  config.vm.hostname = "mapserver-vagrant"

  config.vm.network :forwarded_port, guest: 80, host: 8080

  config.vm.provider "virtualbox" do |v|
     v.customize ["modifyvm", :id, "--memory", 1024, "--cpus", 2]
     v.customize ["modifyvm", :id, "--ioapic", "on", "--largepages", "off", "--vtxvpid", "off"]
     v.name = "mapserver-vagrant"
   end

  config.vm.provision "shell", path: "scripts/vagrant/virtualbox-fix.sh"
  config.vm.provision "shell", path: "scripts/vagrant/packages.sh"
  config.vm.provision "shell", path: "scripts/vagrant/postgis.sh"
  config.vm.provision "shell", path: "scripts/vagrant/mapserver.sh"

end
