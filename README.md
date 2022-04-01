
# Trabalho Prático 1

Este tutorial contém o processo básico usado para desenvolver o trabalho prático 1 da disciplina de Laboratório de Sistemas Operacionais (CC). Este trabalho consiste na geração de uma distribuição Linux que possua um servidor WEB implementado em Python ou C/C++. Este servidor WEB deverá servir uma página HTML contendo informações básicas sobre o sistema (maquina target, onde o servidor estará sendo executado). Segundo o enunciado, as informações que devem ser apresentadas pela página HTML seriam:

* Data e hora do sistema;
* Uptime (tempo de funcionamento sem reinicialização do sistema) em segundos;
* Modelo do processador e velocidade;
* Capacidade ocupada do processador (%);
* Quantidade de memória RAM total e usada (MB);
* Versão do sistema;
* Lista de processos em execução (pid e nome).

Para este trabalho, a implementação do servidor WEB será em Python.

Para mais informações, consulte o enunciado original presente neste anexo (nomeado como 'tp1.pdf').


# Tutorial

## 1. Configurando o Buildroot

Para este trabalho, foi utilizada uma distribuição linux para plataforma x86 e emulada com o QEMU, seguindo o mesmo padrão das atividades realizadas em aula. Assim como nos exemplos em aula também, foi utilizada a versão 2022.02 do Buildroot.

### 1.1 Obtendo o Buildroot
Como já mencionado, iremos fazer uso da versão 2022.02 do Buildroot, portanto podemos seguir os mesmos passos utilizados nos exemplos em aula para obter e preparar os arquivos necessários da ferramenta:

1. Fazer download da versão 2022.02 diretamente do site oficial (buildroot.org)

```
$ wget --no-check-certificate https://buildroot.org/downloads/buildroot-2022.02.tar.gz
```

2. Descompactar o arquivo baixado para o diretório `linuxdistro` e renomear o diretório extraído para `buildroot`

```
$ tar -zxvf buildroot-2022.02.tar.gz
$ mv buildroot-2022.02/ buildroot/
```

### 1.2 Configuração arquitetura

Assim como também mencionado anteriormente, iremos gerar uma distribuição para plataformas x86 para emulação com o QEMU, portanto iremos configurar o Buildroot para tal:

```
$ make qemu_x86_defconfig
```

### 1.3 Configurações dos scripts customizados

No diretório buildroot/, criaremos um diretório chamado `custom-scripts`, com o objetivo de manter scripts de configuração personalizados:

```
$ mkdir custom-scripts
```

Iniciaremos criando um arquivo denominado `qemu-ifup` com o conteúdo abaixo no diretório custom-scripts: 

```
#!/bin/sh
set -x

switch=br0

if [ -n "$1" ];then
        ip tuntap add $1 mode tap user `whoami`		#create tap network 
	
        ip link set $1 up				#bring interface tap up
        sleep 0.5s					#wait the interface come up.
        sysctl -w net.ipv4.ip_forward=1                 # allow forwarding of IPv4
	route add -host 192.168.1.10 dev $1 		# add route to the client
        exit 0
else
        echo "Error: no interface specified"
        exit 1
fi
```

Esse script irá habilitar uma interface de rede tap (virtual) para o sistema guest.

Em seguida, precisaremos configurar a interface de rede do sistema guest para se comunicar com o sistema host, este script precisará ser chamado toda a vez que os sistema da máquina guest for iniciado. Para isso, crie também um script chamado `S41network-config` com o conteúdo abaixo, substituindo `<IP-DO-HOST>` pelo ipv4 da máquina host:

```
#!/bin/sh
#
# Configuring host communication.
#

case "$1" in
  start)
	printf "Configuring host communication."
	
	/sbin/ifconfig eth0 192.168.1.10 up
	/sbin/route add -host <IP-DO-HOST> dev eth0
	/sbin/route add default gw <IP-DO-HOST>
	[ $? = 0 ] && echo "OK" || echo "FAIL"
	;;
  stop)
	printf "Shutdown host communication. "
	/sbin/route del default
	/sbin/ifdown -a
	[ $? = 0 ] && echo "OK" || echo "FAIL"
	;;
  restart|reload)
	"$0" stop
	"$0" start
	;;
  *)
	echo "Usage: $0 {start|stop|restart}"
	exit 1
esac

exit $?
```

Por fim, criaremos mais um arquivo denominado `pre-build.sh`. No próximo passo, configuraremos o buildroot para executar este script específico antes da criação da imagem do sistema de arquivos. Este script será responsável por duas tarefas importantes:

* Copiar o script de configuração do adaptador de rede recém criado `S41network-config` para a pasta `/etc/init.d` da máquina guest para habilitar que este script seja sempre executado no boot do sistema.
* Copiar os arquivos necessários para o servidor WEB para dentro do diretório `/root` do volume da maquina guest


Copie o conteúdo abaixo:
```
#!/bin/sh
cp $BASE_DIR/../custom-scripts/S41network-config $BASE_DIR/target/etc/init.d
chmod +x $BASE_DIR/target/etc/init.d/S41network-config
	
# Copy HTTP server files
cp $BASE_DIR/../custom-scripts/server/* $BASE_DIR/target/root
```
	
Por fim, de permissão de execução para o script pre-build.sh. 
	
```
$ chmod +x custom-scripts/pre-build.sh
```

Agora vamos adicionar o diretório `server` que conterá os arquivos necessários para a execução do servidor WEB, você pode copia-los do repositório do Github ou copiar os snippets diretamente deste tutorial na seção [Servidor HTTP](https://github.com/sarah-lacerda/linuxdistro/edit/tp1/README.md#servidor-http) e colocalos dentro de uma nova pasta `server` dentro de `custom-scripts`

```
$ mkdir custom-scripts/server
```

### 1.4 Customizações do Kernel e da distribuição

Aqui faremos as seguintes customizações através da interface de configuração do Buildroot:

```
$ make menuconfig
```

* Desativar a configuração DHCP da interface de rede (como feito em aula)
* Configurar a porta TTY para ttyS0 (como feito em aula)

```
    System configuration  ---> 
  	 ()  Network interface to configure through DHCP
  	 [*] Run a getty (login prompt) after boot  --->
  		  (ttyS0) TTY port
```
 
* Configurar o Buildroot para executar o script (`pre-build.sh`, criado no passo anterior) antes da geração da imagem do rootfs.

```
    System configuration  --->
    	(custom-scripts/pre-build.sh) Custom scripts to run before creating filesystem images
```

* Habilitar suporte WCHAR. Como estamos utilizando Python, será necessário habilitar o suporte WCHAR para tornar possível a codificação de strings UTF-16

```
    Toolchain  ---> 
  		  [*] Enable WCHAR support
```

* Incluir Python3 à distribuição: Como mencionado anteriormente, iremos fazer a implementação do servidor WEB utilizando a linguagem de programação Python. Para isso, precisaremos incluir o interpretador da Linguagem em nossa distribuição do Linux.
```
    Target packages  ---> 
  	     Interpreter languages and scripting  --->
  		  [*] python3
```

Saia do menu de configurações salvando essas opções.

```
$ make linux-menuconfig
```

Aqui faremos as seguintes customizações:

* Habilitar o driver Ethernet e1000 (como feito em aula)

```
  Device Drivers  ---> 
  	[*] Network device support  --->    
  		[*]   Ethernet driver support  ---> 
  		<*>     Intel(R) PRO/1000 Gigabit Ethernet support
```
  		
Saia do menu de configurações salvando essas opções.

Após isso, podemos iniciar a compilação do sistema

```
$ make
```

O tempo desta operação dependerá da máquina host utilizada e poderá demorar um tempo significante, portanto pegue um café :coffee:

## 2. Iniciando a emulação

Após a finalização da compilação, podemos seguir para a emulação da distribuição recém compilada:

1. Monte a imagem do sistema de arquivos em sua máquina alvo:

```
$ mount -o loop output/images/rootfs.ext2 ../rootfs/
```

2. Execute o comando abaixo para iniciar a emulação:

```
$ sudo qemu-system-i386 --device e1000,netdev=eth0,mac=aa:bb:cc:dd:ee:ff \
--netdev tap,id=eth0,script=custom-scripts/qemu-ifup \
--kernel output/images/bzImage \
--hda output/images/rootfs.ext2 --nographic \
--append "console=ttyS0 root=/dev/sda"
```

*O login padrão é `root`

Caso o comando qemu-system-i386 não seja encontrado, será necessário instalar o QEMU no sistema host:

```
$ sudo apt-get install qemu-system
```

**Para encerrar o QUEMU, abra outro terminal e execute:**

```
$ killall qemu-system-i386
```

## 2. Iniciando o servidor WEB

Apos ter iniciado a emulação do sistema e logado com sucesso, liste os arquivos presentes no diretório do root:

```
# ls
cpustat.py   cpustat.pyc  index.html   server.py
```

Se tudo ocorreu bem, voce deve estar visualizando os arquivos do servidor HTTP que foram copiados nos passos anteriores.

Para iniciar o servidor e começar a escutar por requisições, execute o script `server.py`:

*O servidor estará escutando na porta 8080

```
# python server.py

```

Agora experimente acessar o servidor no navegador ou através do comando curl:

```
$ curl <IP_MAQUINA_GUEST>:8080
```

Nota: Você pode verificar o IP da maquina guest através so comando `ifconfig`

## Servidor HTTP

O servidor HTTP utiliza 3 arquivos para a execução dos requerimentos deste trabalho:

* `index.html`: É a template da pagina HTML que será mostrada ao chamar o servidor, essa template será utilizada e preenchida pela aplicacao para mostrar os dados dinâmicos solicitados.
```
<!DOCTYPE html>
<html>
<head>
<title>
Target system info:</title>
<meta  name="viewport"  content="width=device-width, initial-scale=1">
<style>
body {background-color:#ffffff;background-repeat:no-repeat;background-position:top  left;background-attachment:fixed;}
h1{font-family:Arial, sans-serif;color:#000000;background-color:#ffffff;}
p {font-family:Georgia, serif;font-size:14px;font-style:normal;font-weight:normal;color:#000000;background-color:#ffffff;}
</style>
</head>
<body>
<h1></h1>
<p>Date: %s
<p>Time: </p>
<p>Uptime (seconds): </p>
<p>CPU: %s</p>
<p>CPU Usage: %s</p>
<p>RAM(MB): </p>
<p>OS Version: </p>
<p>Process list: </p>
</body>
</html>
```
* `server.py`: Implementa o servidor http e serve na porta designada em código. Tambem carrega a template `index.html` e preenche com as informações obtidas do sistema. :
```
import http.server
import os.path
import re
import cpustat as cpustat

htmlTemplate = ""
with open ("index.html", "r") as htmlFile:
    htmlTemplate = htmlFile.read().replace("\n", "")

def getSystemInfo() :
    dateTime = os.popen("date").read().split()
    date = dateTime[0] + " " + dateTime[1] + " " + dateTime[2] + " " + dateTime[5]
    time = dateTime[3]

    with open('/proc/uptime') as f: uptime = f.read()
    uptime = uptime.split()[0]

    cpuinf_dict = {}
    with open('/proc/cpuinfo', mode='r') as cpuinfo:
        for line in cpuinfo:
            name, value = line.partition(":")[::2]
            cpuinf_dict[name.strip()] = value.strip()
    cpu = cpuinf_dict["model name"]

    cpuUsage = str(format((cpustat.GetCpuLoad().getcpuload()['cpu'] * 100), ".2f")) + " %"

    meminfo_dict = {}
    with open('/proc/meminfo', mode='r') as cpuinfo:
        for line in cpuinfo:
            name, value = line.partition(":")[::2]
            meminfo_dict[name.strip()] = re.sub("[^0-9]", "", value.strip())
    ram = str((int(meminfo_dict["MemTotal"])-int(meminfo_dict["MemFree"]))/1000) + "/" + str(int(meminfo_dict["MemTotal"])/1000) + " MB"

    with open('/proc/version') as f: version = f.read()

    return date, time, uptime, cpu, cpuUsage, ram, version, getProcessList()

def getProcessList():
    processes = "<p>"
    ps = os.popen("ps -Ao pid,cmd").read()
    for i in range (len(ps)):
        if (ps[i] == "\n"):
            processes += "</p>\n"
        else:
            processes +=  ps[i]
    return processes

class MyHandler(http.server.BaseHTTPRequestHandler):
    getSystemInfo = getSystemInfo

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

        message = htmlTemplate % (getSystemInfo())
        self.wfile.write(message.encode())
#        self.wfile.close()
```
* `cpustat`: Script fornecido no enunciado do trabalho para calcular a porcentagem de consumo da CPU a partir das informações presentes no arquivo `/proc/stat`
```
#!/usr/bin/python 
# -*- coding: utf-8 -*-

'''
Created on 04.12.2014

@author: plagtag
'''
from time import sleep
import sys

class GetCpuLoad(object):
    '''
    classdocs
    '''


    def __init__(self, percentage=True, sleeptime = 1):
        '''
        @parent class: GetCpuLoad
        @date: 04.12.2014
        @author: plagtag
        @info: 
        @param:
        @return: CPU load in percentage
        '''
        self.percentage = percentage
        self.cpustat = '/proc/stat'
        self.sep = ' ' 
        self.sleeptime = sleeptime

    def getcputime(self):
        '''
        http://stackoverflow.com/questions/23367857/accurate-calculation-of-cpu-usage-given-in-percentage-in-linux
        read in cpu information from file
        The meanings of the columns are as follows, from left to right:
            0cpuid: number of cpu
            1user: normal processes executing in user mode
            2nice: niced processes executing in user mode
            3system: processes executing in kernel mode
            4idle: twiddling thumbs
            5iowait: waiting for I/O to complete
            6irq: servicing interrupts
            7softirq: servicing softirqs

        #the formulas from htop 
             user    nice   system  idle      iowait irq   softirq  steal  guest  guest_nice
        cpu  74608   2520   24433   1117073   6176   4054  0        0      0      0


        Idle=idle+iowait
        NonIdle=user+nice+system+irq+softirq+steal
        Total=Idle+NonIdle # first line of file for all cpus

        CPU_Percentage=((Total-PrevTotal)-(Idle-PrevIdle))/(Total-PrevTotal)
        '''
        cpu_infos = {} #collect here the information
        with open(self.cpustat,'r') as f_stat:
            lines = [line.split(self.sep) for content in f_stat.readlines() for line in content.split('\n') if line.startswith('cpu')]

            #compute for every cpu
            for cpu_line in lines:
                if '' in cpu_line: cpu_line.remove('')#remove empty elements
                cpu_line = [cpu_line[0]]+[float(i) for i in cpu_line[1:]]#type casting
                cpu_id,user,nice,system,idle,iowait,irq,softrig,steal,guest,guest_nice = cpu_line

                Idle=idle+iowait
                NonIdle=user+nice+system+irq+softrig+steal

                Total=Idle+NonIdle
                #update dictionionary
                cpu_infos.update({cpu_id:{'total':Total,'idle':Idle}})
            return cpu_infos

    def getcpuload(self):
        '''
        CPU_Percentage=((Total-PrevTotal)-(Idle-PrevIdle))/(Total-PrevTotal)

        '''
        start = self.getcputime()
        #wait a second
        sleep(self.sleeptime)
        stop = self.getcputime()

        cpu_load = {}

        for cpu in start:
            Total = stop[cpu]['total']
            PrevTotal = start[cpu]['total']

            Idle = stop[cpu]['idle']
            PrevIdle = start[cpu]['idle']
            CPU_Percentage=((Total-PrevTotal)-(Idle-PrevIdle))/(Total-PrevTotal)*100
            cpu_load.update({cpu: CPU_Percentage})
        return cpu_load


if __name__=='__main__':
    x = GetCpuLoad()
    while True:
        try:
            data = x.getcpuload()
            print data
        except KeyboardInterrupt:

            sys.exit("Finished")    

```


## Autora

**Sarah Lacerda / 2022.2**
Código disponibilizado em: https://github.com/sarah-lacerda/linuxdistro/tree/tp1
