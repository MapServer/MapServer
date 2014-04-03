# -*- mode: ruby -*-
# vi: set ft=ruby :

require 'socket'

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = "precise64"

  config.vm.hostname = "mapserver-vagrant"
  config.vm.box_url = "http://files.vagrantup.com/precise64.box"
  config.vm.host_name = "mapserver-vagrant"
  
  config.vm.network :forwarded_port, guest: 80, host: 8080

  config.vm.provider :virtualbox do |vb|
     vb.customize ["modifyvm", :id, "--memory", "4096"]
     vb.customize ["modifyvm", :id, "--cpus", "2"]
     vb.customize ["modifyvm", :id, "--ioapic", "on"]
     vb.name = "mapserver-vagrant"
   end  

  ppaRepos = [
    "ppa:ubuntugis/ppa",
  ]

  packageList = [
    "git",
    "build-essential",
    "pkg-config",
    "cmake",
    "libgeos-dev",
    "libpq-dev",
    "python-all-dev",
    "libproj-dev",
    "libxml2-dev",
    "postgis",
    "postgresql-server-dev-9.1",
    "postgresql-9.1-postgis",
    "vim",
    "bison",
    "flex",
    "swig",
    "librsvg2-dev",
    "libpng12-dev",
    "libjpeg-dev",
    "libgif-dev",
    "libgd2-xpm-dev",
    "libfreetype6-dev",
    "libfcgi-dev",
    "libcurl4-gnutls-dev",
    "libcairo2-dev",
    "libgdal1-dev",
    "php5-dev",
    "libexempi-dev"
  ];

  if Dir.glob("#{File.dirname(__FILE__)}/.vagrant/machines/default/*/id").empty?
	  pkg_cmd = "sed -i 's#deb http://us.archive.ubuntu.com/ubuntu/#deb mirror://mirrors.ubuntu.com/mirrors.txt#' /etc/apt/sources.list; "

	  pkg_cmd << "apt-get update -qq; apt-get install -q -y python-software-properties; "

	  if ppaRepos.length > 0
		  ppaRepos.each { |repo| pkg_cmd << "add-apt-repository -y " << repo << " ; " }
		  pkg_cmd << "apt-get update -qq; "
	  end

	  # install packages we need we need
	  pkg_cmd << "apt-get install -q -y " + packageList.join(" ") << " ; "
	  config.vm.provision :shell, :inline => pkg_cmd
    scripts = [
      "mapserver.sh",
      "postgis.sh"
    ];
    scripts.each { |script| config.vm.provision :shell, :path => "scripts/vagrant/" << script }
  end
end
