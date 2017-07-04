=== Windows
===================================
Para compilar e executar este projeto no Windows, baixe a IDE Code::Blocks em
http://codeblocks.org/ e abra o arquivo "Laboratorio_X.cbp".


=== Linux
===================================
Para compilar e executar este projeto no Linux, primeiro você precisa instalar
as biblioteca necessárias. Para tanto, execute o comando abaixo em um terminal.
Esse é normalmente suficiente em uma instalação de Linux Ubuntu:

    sudo apt-get install build-essential make libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxcb1-dev libxext-dev libxrender-dev libxfixes-dev libxau-dev libxdmcp-dev

Após a instalação das bibliotecas acima, você possui duas opções para compilação:
utilizar Code::Blocks ou Makefile.

--- Linux com Code::Blocks
-----------------------------------
Instale a IDE Code::Blocks (versão Linux em http://codeblocks.org/), abra o
arquivo "Laboratorio_X.cbp", e modifique o "Build target" de "Debug" para "Linux".

--- Linux com Makefile
-----------------------------------
Abra um terminal, navegue até a pasta "Laboratorio_0X_Codigo_Fonte", e execute
o comando "make" para compilar. Para executar o código compilado, execute o
comando "make run".


=== Soluções de Problemas
===================================

Caso você tenha problemas em executar o código deste projeto, tente atualizar o
driver da sua placa de vídeo.
