# -*- mode: ruby -*-
# vi: set ft=ruby :

require 'socket'

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "ubuntu/trusty64"

  config.vm.hostname = "mapserver-vagrant"

  config.vm.network :forwarded_port, guest: 80, host: 8080

  config.vm.provider "virtualbox" do |v|
     v.customize ["modifyvm", :id, "--memory", 1024, "--cpus", 2]
     v.customize ["modifyvm", :id, "--ioapic", "on", "--largepages", "off", "--vtxvpid", "off"]
     v.name = "mapserver-vagrant"
   end

  # Unless explicitly declined, use the VM host's file system to cache
  # .deb files to avoid repeated downloads on each vagrant up
  unless File.exists?("../.no_apt_cache")
    cache_dir = "../apt-cache/#{config.vm.box}"
    FileUtils.mkdir_p(cache_dir) unless Dir.exists?(cache_dir)
    puts "Using local apt cache, #{cache_dir}"
    config.vm.synced_folder cache_dir, "/var/cache/apt/archives"
  end

  config.vm.provision "shell", path: "scripts/vagrant/virtualbox-fix.sh"
  config.vm.provision "shell", path: "scripts/vagrant/packages.sh"
  config.vm.provision "shell", path: "scripts/vagrant/postgis.sh"
  config.vm.provision "shell", path: "scripts/vagrant/mapserver.sh"

end
