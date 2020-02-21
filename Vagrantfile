# -*- mode: ruby -*-
# vi: set ft=ruby :

require 'socket'

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

$set_environment_variables = <<SCRIPT
tee "/etc/profile.d/myvars.sh" > "/dev/null" <<EOF
export LD_LIBRARY_PATH=/vagrant/install-vagrant-proj-6/lib
EOF
SCRIPT

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  vm_ram = ENV['VAGRANT_VM_RAM'] || 2048
  vm_cpu = ENV['VAGRANT_VM_CPU'] || 2

  config.vm.box = "ubuntu/trusty64"

  config.vm.hostname = "mapserver-vagrant"

  config.vm.network :forwarded_port, guest: 80, host: 8080

  config.vm.provider "virtualbox" do |v|
     v.customize ["modifyvm", :id, "--memory", vm_ram, "--cpus", vm_cpu]
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
  config.vm.provision "shell", path: "scripts/vagrant/proj6.sh"
  config.vm.provision "shell", path: "scripts/vagrant/mapserver.sh"

  config.vm.provision "shell", inline: $set_environment_variables, run: "always"

end


