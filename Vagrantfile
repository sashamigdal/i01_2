require 'socket'

hostname_suffix = ""

if `uname -s` == 'Linux'
    boxes_path = File.exist?('/srv/data/src/vagrant/') ? '/srv/data/src/vagrant/' : ''
elsif `uname -s` == 'Darwin'
    boxes_path = File.exist?(ENV['HOME'] + '/Downloads/') ? ENV['HOME'] + '/Downloads/' : ''
else
    boxes_path = ''
end
boxes_path = boxes_path.empty?() ? '' : 'file:///' + boxes_path

nodes = [
    { :hostname => 'arch-build', :box => 'packer_arch_virtualbox',
      :box_checksum => 'd7f14a80c238787045aa236b05161a6a52b11fe9dd5d3ba28166e4ab4dd07413',
      :ram => 4096,
      :active => true,
    },
    { :hostname => 'centos5-build',
      :box => 'centos-510-x64-virtualbox-puppet',
      :box_checksum => '233d5b6b79c79b80dc4c3216478542632d7910ba42ab5a0cddd14dcc3d163c1e',
      :ram => 4096,
      :active => Socket.gethostname == "primus",
    },
    { :hostname => 'centos6-build',
      :box => 'centos-65-x64-virtualbox-puppet',
      :box_checksum => '0beb827f5e586fe2886ae4f5852d34ccdb0ee5ac9405350059324143066169f8',
      :ram => 4096,
      :active => Socket.gethostname == "primus",
    },
    { :hostname => 'centos7-build',
      :box => 'centos-70-x64-virtualbox-puppet',
      :box_checksum => 'e972e17c342649a23f67c3330a6e93b6f6a3966aece4dd02aecfaf6ba835e794',
      :ram => 4096,
      :active => Socket.gethostname == "primus",
    },
]

Vagrant.configure("2") do |config|
    nodes.each do |node|
        if node[:active] == true
            config.vm.define node[:hostname] do |c|
                c.vm.box = node[:box]
                c.vm.box_url = boxes_path + node[:box] + '.box'
                c.vm.box_download_checksum = node[:box_checksum]
                c.vm.box_download_checksum_type = 'sha256'
                c.vm.box_download_insecure = true
                c.vm.hostname = node[:hostname] + hostname_suffix
                c.vm.provider "virtualbox" do |vb|
                    memory = node[:ram] ? node[:ram] : 1024
                    vb.name = node[:hostname]
                    vb.customize [
                        'modifyvm', :id,
                        '--name', node[:hostname],
                        '--memory', memory.to_s,
                        '--cpus', node[:cpus] ? node[:cpus] : 2,
                        '--usb', 'off',
                        '--usbehci', 'off', # removes need for oracle ext pack...
                        '--accelerate3d', 'off'
                    ]
                end
                c.vm.provision "shell", run: "once",
                    inline: "pacman-db-upgrade"
                c.vm.provision "shell", run: "once",
                    inline: "pacman -Syu --noconfirm"
                c.vm.provision "shell", run: "once",
                    inline: "pacman -Syu --noconfirm --needed               \
                        base-devel gdb cgdb strace clang cmake valgrind     \
                        catdoc doxygen python ipython python2 ipython2      \
                        python2-matplotlib qt4 python2-pyqt4 python2-pyqt5  \
                        qwt python2-virtualenv boost boost-libs blas lapack \
                        eigen intel-tbb gtest google-glog libaio jemalloc   \
                        postgresql postgresql-libs wt fcgi lz4 lua luajit   \
                        qt5 gperftools git libnl libpcap ccache"
                c.vm.provision "shell", privileged: false, run: "once",
                    inline: "sudo dkms autoinstall -k `pacman -Q linux-lts | cut -d' ' -f2`-lts"
                c.vm.provision "shell", privileged: false, run: "once",
                    inline: "curl --silent -S --remote-name-all \
                        https://aur.archlinux.org/packages/pa/package-query/package-query.tar.gz \
                        https://aur.archlinux.org/packages/ya/yaourt/yaourt.tar.gz \
                        && tar xvzf package-query.tar.gz \
                        && cd package-query \
                        && makepkg -si --noconfirm --needed \
                        && cd .. \
                        && tar xvzf yaourt.tar.gz \
                        && cd yaourt \
                        && makepkg -si --noconfirm --needed \
                        && cd ..
                        && echo 'Install fix8 and openonload manually.'"
                c.vm.provision "shell", privileged: false, run: "always",
                    inline: "yaourt -Syua --noconfirm"
                #c.vm.provision "puppet", run: "always" do |puppet|
                #    puppet.manifests_path = 'dist/puppet/manifests'
                #    puppet.manifest_file = 'site.pp'
                #    puppet.module_path = 'dist/puppet/modules'
                #    puppet.facter = {
                #        'vagrant' => '1',
                #        'vagrant_box' => node[:box],
                #        'vagrant_host' => Socket.gethostname,
                #        "hack=hack LANG=en_US.UTF-8 hack" => "hack",
                #    }
                #end
                #c.vm.provision "puppet_server", run: "always" do |puppet|
                #    puppet.puppet_server = '192.168.85.198'
                #    puppet.puppet_node = node[:hostname]
                #    puppet.options = "--test --waitforcert 300"
                #    puppet.facter = {
                #        'vagrant' => '1',
                #        'vagrant_box' => node[:box],
                #        'vagrant_host' => Socket.gethostname,
                #        "hack=hack LANG=en_US.UTF-8 hack" => "hack",
                #    }
                #end
            end
        end
    end

    #config.ssh.private_key_path = "~/.ssh/id_rsa_fdo"
    config.ssh.forward_agent = true
    config.ssh.forward_x11 = true
end
